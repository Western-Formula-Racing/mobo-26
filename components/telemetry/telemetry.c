#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "telemetry.h"
#include <string.h>

#define WIFI_SSID       "wfr-mobo"
#define MAX_WS_CLIENTS  4
#define FRAME_QUEUE_SIZE 64
#define BATCH_SIZE       32
#define BATCH_INTERVAL_MS 100

static const char *TAG = "telemetry";
static httpd_handle_t server = NULL;
static QueueHandle_t frameQueue;
static SemaphoreHandle_t clientMutex;

typedef struct {
    uint32_t canId;
    uint8_t data[8];
    uint8_t len;
    int64_t timestamp_ms;
} telemetry_frame_t;

// Connected WebSocket client file descriptors
static int ws_fds[MAX_WS_CLIENTS];
static int ws_count = 0;

// ---- WiFi AP ----

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Station connected to AP");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Station disconnected from AP");
    }
}

static void init_wifi_ap(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = sizeof(WIFI_SSID) - 1,
            .channel = 1,
            .max_connection = MAX_WS_CLIENTS,
            .authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started. SSID: %s", WIFI_SSID);
}

// ---- WebSocket Server ----

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        // WebSocket handshake — register this client
        int fd = httpd_req_to_sockfd(req);
        xSemaphoreTake(clientMutex, portMAX_DELAY);
        if (ws_count < MAX_WS_CLIENTS) {
            ws_fds[ws_count++] = fd;
            ESP_LOGI(TAG, "WS client connected fd=%d (total=%d)", fd, ws_count);
        } else {
            ESP_LOGW(TAG, "WS client rejected, max reached");
        }
        xSemaphoreGive(clientMutex);
        return ESP_OK;
    }

    // Downlink only — ignore any incoming WS frames
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    return httpd_ws_recv_frame(req, &ws_pkt, 0);
}

static void remove_ws_client(int idx) {
    // Caller must hold clientMutex
    ws_fds[idx] = ws_fds[--ws_count];
}

static void start_ws_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = ws_handler,
            .is_websocket = true,
        };
        httpd_register_uri_handler(server, &ws_uri);
        ESP_LOGI(TAG, "WebSocket server started on port %d", config.server_port);
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}

// ---- Broadcast Task ----

static void telemetry_task(void *arg) {
    telemetry_frame_t batch[BATCH_SIZE];
    int batch_count;

    while (1) {
        batch_count = 0;
        TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(BATCH_INTERVAL_MS);

        // Collect frames until batch is full or interval expires
        while (batch_count < BATCH_SIZE) {
            TickType_t now = xTaskGetTickCount();
            TickType_t remaining = (deadline > now) ? (deadline - now) : 0;
            if (remaining == 0 && batch_count > 0) break;

            if (xQueueReceive(frameQueue, &batch[batch_count],
                              remaining > 0 ? remaining : pdMS_TO_TICKS(BATCH_INTERVAL_MS)) == pdTRUE) {
                batch_count++;
            } else {
                break;
            }
        }

        if (batch_count == 0) continue;

        xSemaphoreTake(clientMutex, portMAX_DELAY);
        if (ws_count == 0) {
            xSemaphoreGive(clientMutex);
            continue;
        }

        // Build JSON array: [{"time":ms,"canId":id,"data":[b0,b1,...]}]
        cJSON *array = cJSON_CreateArray();
        for (int i = 0; i < batch_count; i++) {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(obj, "time", (double)batch[i].timestamp_ms);
            cJSON_AddNumberToObject(obj, "canId", batch[i].canId);

            cJSON *data_arr = cJSON_CreateArray();
            for (int j = 0; j < batch[i].len; j++) {
                cJSON_AddItemToArray(data_arr, cJSON_CreateNumber(batch[i].data[j]));
            }
            cJSON_AddItemToObject(obj, "data", data_arr);
            cJSON_AddItemToArray(array, obj);
        }

        char *json = cJSON_PrintUnformatted(array);
        cJSON_Delete(array);

        if (json) {
            httpd_ws_frame_t ws_pkt = {
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = (uint8_t *)json,
                .len = strlen(json),
            };

            for (int i = ws_count - 1; i >= 0; i--) {
                esp_err_t ret = httpd_ws_send_frame_async(server, ws_fds[i], &ws_pkt);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "WS client fd=%d send failed, removing", ws_fds[i]);
                    remove_ws_client(i);
                }
            }
            free(json);
        }
        xSemaphoreGive(clientMutex);
    }
}

// ---- Public API ----

void initTelemetry(void) {
    clientMutex = xSemaphoreCreateMutex();
    frameQueue = xQueueCreate(FRAME_QUEUE_SIZE, sizeof(telemetry_frame_t));

    init_wifi_ap();
    start_ws_server();

    xTaskCreate(telemetry_task, "telemetry", 8192, NULL, 3, NULL);
    ESP_LOGI(TAG, "Telemetry initialized");
}

void telemetryQueueFrame(uint32_t canId, uint8_t *data, uint8_t len) {
    telemetry_frame_t frame = {
        .canId = canId,
        .len = len > 8 ? 8 : len,
        .timestamp_ms = esp_timer_get_time() / 1000,
    };
    memcpy(frame.data, data, frame.len);
    xQueueSend(frameQueue, &frame, 0); // non-blocking, drop if full
}
