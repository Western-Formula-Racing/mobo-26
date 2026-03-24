#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "hw_define.h"
#include "CAN.h"
#include "BMS.h"
#include "io.h"
#include "statemachine.h"
#include "config.h"

static const char* TAG = "CAN";

twai_node_handle_t mobo_node_handle = NULL;
QueueHandle_t canRxQueue;
twai_node_status_t canStatus;
twai_node_record_t canRecord;
int bus_recovery_attempts = 0;
uint32_t lastModuleTimestamp[5] = {0,0,0,0,0};


typedef struct {
  uint32_t id;
  uint8_t  dlc;
  uint8_t  data[8];
} CanRxItem;



int f2i_CAN(float input, float factor, int offset){
  //convert float to int
  return (int)((input * factor) + offset);
}

float i2f_CAN(int input, float factor, int offset){
  //convert int to float
  return ((float)(input*factor)+offset);
}

void canTask(void *arg)
{
  CanRxItem item;

  while (1) {
    if (xQueueReceive(canRxQueue, &item, portMAX_DELAY) == pdTRUE) {
      union CANBuffer_u rx_data;
      memcpy(rx_data.array, item.data, 8);

      if (item.id >= 1006 && item.id <= 1030) {
        float s1 = (float)(rx_data.BMSVoltages.v1_lo | (rx_data.BMSVoltages.v1_hi << 8)) / 10000.f;
        float s2 = (float)(rx_data.BMSVoltages.v2_lo | (rx_data.BMSVoltages.v2_hi << 8)) / 10000.f;
        float s3 = (float)(rx_data.BMSVoltages.v3_lo | (rx_data.BMSVoltages.v3_hi << 8)) / 10000.f;
        float s4 = (float)(rx_data.BMSVoltages.v4_lo | (rx_data.BMSVoltages.v4_hi << 8)) / 10000.f;

        int id = (int)item.id - 1006;
        int module = id / 5;
        int cell = id * 4 - module * 20;

        setModuleVoltage(module, cell++, s1);
        setModuleVoltage(module, cell++, s2);
        setModuleVoltage(module, cell++, s3);
        setModuleVoltage(module, cell++, s4);

        lastModuleTimestamp[module] = xTaskGetTickCount();
      }
      else if (item.id >= 1031 && item.id <= 1055) {
        double s1 = (rx_data.BMSTemperatures.t1_lo | (rx_data.BMSTemperatures.t1_hi << 8)) * 0.001;
        double s2 = (rx_data.BMSTemperatures.t2_lo | (rx_data.BMSTemperatures.t2_hi << 8)) * 0.001;
        double s3 = (rx_data.BMSTemperatures.t3_lo | (rx_data.BMSTemperatures.t3_hi << 8)) * 0.001;
        double s4 = (rx_data.BMSTemperatures.t4_lo | (rx_data.BMSTemperatures.t4_hi << 8)) * 0.001;

        int id = (int)item.id - 1031;
        int module = id / 5;
        int cell = id * 4 - module * 20;

        setModuleTemp(module, cell++, s1);
        setModuleTemp(module, cell++, s2);
        if ((id + 1) % 5 != 0) {
          setModuleTemp(module, cell++, s3);
          setModuleTemp(module, cell++, s4);
        }

        lastModuleTimestamp[module] = xTaskGetTickCount();
      }
      else if (item.id == 1000) {
        uint8_t module_id = rx_data.array[0];
        uint8_t err       = rx_data.array[1];
        (void)module_id;
      }
    }
    twai_node_get_info(mobo_node_handle,&canStatus,&canRecord);
    if(canStatus.state == TWAI_ERROR_BUS_OFF && bus_recovery_attempts < MAX_RECOVERY_ATTEMPTS){
      ESP_LOGE(TAG,"CAN Bus Error - off. Recovery Attempts: %d Attempting recovery...", bus_recovery_attempts);
      if(twai_node_recover(mobo_node_handle) == ESP_OK){
        ESP_LOGI(TAG, "CAN Bus Recovery Success!");
      } else{
        ESP_LOGE(TAG, "Could not recover bus");
      }
      bus_recovery_attempts++;
    }
    //update module timeout
    for(int i = 0; i < 5; i++){
      modules[i].timeout = pdTICKS_TO_MS(xTaskGetTickCount() - lastModuleTimestamp[i]);
    }
  }
}
 
static bool can_rx_cb(twai_node_handle_t handle,
                      const twai_rx_done_event_data_t *edata,
                      void *user_ctx)
{
  twai_frame_t f;
  uint8_t buf[8];

  memset(&f, 0, sizeof(f));
  f.buffer = buf;
  f.buffer_len = sizeof(buf);

  if (twai_node_receive_from_isr(handle, &f) != ESP_OK) {
    return false;
  }

  CanRxItem item = {0};
  item.id  = f.header.id;
  item.dlc = f.header.dlc;

  uint8_t n = (item.dlc > 8) ? 8 : item.dlc;
  memcpy(item.data, buf, n);

  BaseType_t hpTaskWoken = pdFALSE;
  xQueueSendFromISR(canRxQueue, &item, &hpTaskWoken);
  return hpTaskWoken == pdTRUE;
}



// init CAN
void initCAN(){
  //create RX queue
  canRxQueue = xQueueCreate(10, sizeof(CanRxItem));
  //configure TWAI node
  twai_onchip_node_config_t node_config = {
    .io_cfg.tx = GPIO_CAN_TX,             // TWAI TX GPIO pin
    .io_cfg.rx = GPIO_CAN_RX,             // TWAI RX GPIO pin
    .bit_timing.bitrate = 500000,  // 500 kbps bitrate
    .tx_queue_depth = 5,           // Transmit queue depth set to 5
    .fail_retry_cnt = 1,            // retry tx 1 time on fail
  };
  ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &mobo_node_handle));
  
  //create RX callback
  twai_event_callbacks_t user_cbs = {
    .on_rx_done = can_rx_cb,
  };
  ESP_ERROR_CHECK(twai_node_register_event_callbacks(mobo_node_handle, &user_cbs, NULL));
  
  // Start the TWAI controller
  ESP_ERROR_CHECK(twai_node_enable(mobo_node_handle));

  //start can rx task
  xTaskCreate(
  canTask,        // task function
  "CAN_RX_TASK",    // name
  4096,             // stack size (important for floats!)
  NULL,             // parameter
  5,                // priority (medium)
  NULL              // task handle (optional)
  );
}

// CAN message transmission
int txCounter = 0;
union CANBuffer_u canTxBuffer = {.data=0};

twai_frame_t txMessage = {
  .header.id = 0x0,            // Message ID
  .header.ide = false,          // Use 29-bit extended ID format
  .buffer_len = 8,             // Length of data to transmit
};

// Periodic function for transmission of CAN messages
void canTxPeriodic(){

    // send every 10ms

    // send every 100ms
  if(txCounter%10 == 0){
      // PackStatus (ID 1056) - only PackStatus + Fault set
    txMessage.header.id  = id_packStatus;   // 1056 (0x420)
    txMessage.header.ide = false;           // standard 11-bit
    txMessage.header.rtr = false;
    txMessage.header.dlc = 8;

    memset(canTxBuffer.array, 0, 8);

    //IMD status in bit 0 of byte 2
    uint8_t imd = inputStates[IMD_RELAY] & 0x1;
    canTxBuffer.array[2] |= (imd << 0);

    //AMS status in bit 1 of byte 2
    uint8_t ams = gpio_get_level(GPIO_BMS_OK) & 0x1;
    canTxBuffer.array[2] |= (ams << 1);

    // Use your enums directly (full bytes in DBC)
    canTxBuffer.array[5] = (uint8_t)moboState.currentState; // PackStatus
    canTxBuffer.array[6] = (uint8_t)moboState.error;        // Fault

    txMessage.buffer = canTxBuffer.array;

    esp_err_t err = twai_node_transmit(mobo_node_handle, &txMessage, pdMS_TO_TICKS(10));
    if (err != ESP_OK) {
      printf("PackStatus TX failed: %d\n", (int)err);
    }
    // //packinfo
    txMessage.header.id = id_packInfo;
    int minTemp = f2i_CAN(getMinTemp(),10,0);
    int maxTemp = f2i_CAN(getMaxTemp(),10,0);
    int minVoltage = f2i_CAN(getMinVoltage(),1000,0);
    int maxVoltage = f2i_CAN(getMaxVoltage(),1000,0);
    canTxBuffer.packInfo.minTemp_lo    = minTemp & 0xFF;
    canTxBuffer.packInfo.minTemp_hi    = (minTemp & 0xFF00)>>8;
    canTxBuffer.packInfo.maxTemp_lo    = maxTemp & 0xFF;
    canTxBuffer.packInfo.maxTemp_hi    = (maxTemp & 0xFF00)>>8;
    canTxBuffer.packInfo.minVoltage_lo = minVoltage & 0xFF;
    canTxBuffer.packInfo.minVoltage_hi = (minVoltage & 0xFF00)>>8;
    canTxBuffer.packInfo.maxVoltage_lo = (maxVoltage & 0xFF);
    canTxBuffer.packInfo.maxVoltage_hi = (maxVoltage & 0xFF00)>>8;
    txMessage.buffer = canTxBuffer.array;
    
    err = twai_node_transmit(mobo_node_handle, &txMessage,0);
    if (err != ESP_OK) {
      printf("PackInfo TX failed: %d\n", (int)err);
    }
    // //packStatus
    // txMessage.header.id = 1056;
    // int packCurrent = Cursense_VtoA(analogVoltages[ANALOG_CURSENSE]);
    // canTxBuffer.packStatus.packCurrent_lo = packCurrent & 0xFF;
    // canTxBuffer.packStatus.packCurrent_hi = (packCurrent & 0xFF00)>>8;
    // canTxBuffer.packStatus.IMD = inputStates[IMD_RELAY] & 0x1;
    // canTxBuffer.packStatus.AMS = gpio_get_level(GPIO_BMS_OK) & 0x1;
    // canTxBuffer.packStatus.BSPD = inputStates[BSPD_RELAY] & 0x1;
    // canTxBuffer.packStatus.Latch = inputStates[LATCH_RELAY] & 0x1;
    // canTxBuffer.packStatus.HVActive = inputStates[HV_ACTIVE] & 0x1;
    // //TODO: rough SOC approx
    // canTxBuffer.packStatus.SOC_lo = 0;
    // canTxBuffer.packStatus.SOC_hi = 0;
    // canTxBuffer.packStatus.packStatus = moboState.currentState;
    // canTxBuffer.packStatus.fault = moboState.error;
    // txMessage.buffer = canTxBuffer.array;
    // twai_node_transmit(mobo_node_handle, &txMessage, 0);


    // //BMS Current limit to Cascadia Inverter
    txMessage.header.id = id_BMSCurrentLimit;
    canTxBuffer.BMSCurrentLimit.BMSChargeCurrent_lo = 10;
    canTxBuffer.BMSCurrentLimit.BMSChargeCurrent_hi = 0;
    canTxBuffer.BMSCurrentLimit.BMSCurrentLimit_lo = 255;
    canTxBuffer.BMSCurrentLimit.BMSCurrentLimit_hi = 0;
    txMessage.buffer = canTxBuffer.array;
    twai_node_transmit(mobo_node_handle, &txMessage,0);
  }

  // send every 1s
  if(txCounter>=100){
    //charging message
    txMessage.header.ide = true;
    txMessage.header.id = id_ElconLimits;
    txMessage.header.rtr = false;
    txMessage.header.dlc = 8;
    canTxBuffer.elconLimits.maxChargeVoltage_lo = ((CHARGE_TARGET * 10) & 0xFF00)>>8;
    canTxBuffer.elconLimits.maxChargeVoltage_hi = (CHARGE_TARGET * 10) & 0xFF;
    canTxBuffer.elconLimits.maxChargeCurrent_lo = ((CHARGE_CURRENT * 10) & 0xFF00)>>8;
    canTxBuffer.elconLimits.maxChargeCurrent_hi = (CHARGE_CURRENT * 10) & 0xFF;
    canTxBuffer.elconLimits.control = moboState.currentState == CHARGING ? 0 : 1;
    txMessage.buffer = canTxBuffer.array;
    esp_err_t err = twai_node_transmit(mobo_node_handle, &txMessage, 0);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "ELCON TX failed: %d", (int)err);
    } else {
      ESP_LOGI(TAG, "ELCON TX success");
    }
    txMessage.header.ide = false;
    txCounter = 0;
  }
  txCounter++;
}

void printCANInfo(){
  if(canStatus.rx_error_count > 0){
    ESP_LOGE(TAG, "RX Error count: %d",canStatus.rx_error_count);
  }
  if(canStatus.tx_error_count > 0){
    ESP_LOGE(TAG, "TX Error count: %d",canStatus.tx_error_count);
  }
  if(canStatus.state == TWAI_ERROR_BUS_OFF){
    ESP_LOGE(TAG, "CAN BUS OFF");
  }
  if(canRecord.bus_err_num > 0){
    ESP_LOGE(TAG, "Lifetime CAN errors: %d", canRecord.bus_err_num);
  }
}