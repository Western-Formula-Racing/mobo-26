idf_component_register(SRCS "telemetry.c"
                      INCLUDE_DIRS "." "../../include"
                      REQUIRES esp_wifi esp_http_server esp_netif nvs_flash esp_event json esp_timer)
