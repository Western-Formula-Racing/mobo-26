#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_log.h"
#include "hw_define.h"

static const char* TAG = "CAN";

// init CAN
void initCAN(){

  uint32_t alerts_to_enable = TWAI_ALERT_ABOVE_ERR_WARN|TWAI_ALERT_ERR_ACTIVE|TWAI_ALERT_RX_QUEUE_FULL;

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_CAN_TX,GPIO_CAN_RX,TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  
  ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
  ESP_LOGI(TAG, "CAN Driver Installed\n");
  ESP_ERROR_CHECK(twai_reconfigure_alerts(alerts_to_enable,NULL));
  // Start TWAI driver
  ESP_ERROR_CHECK(twai_start());
}

// CAN message transmission
int txCounter = 0;
twai_message_t txMessage = {

  // Message type and format settings
    .extd = 0,              // Standard vs extended format
    .rtr = 0,                // Data vs RTR frame
    .ss = 0,                // Whether the message is single shot (i.e., does not repeat on error)
    .self = 0,              // Whether the message is a self reception request (loopback)
    .dlc_non_comp = 0,      // DLC is less than 8
    .reserved = 0,          // Reserved field
  // Message ID and payload
  .identifier = 0,
  .data_length_code = 8,
  .data = {0,0,0,0,0,0,0,0},
};

// Periodic function for transmission of CAN messages
void canTxPeriodic(){

  // send every 10ms
  if(txCounter%10 == 0){

  }

  // send every 100ms
  if(txCounter%100 == 0){

  }

  // send every 1s
  if(txCounter>=1000){
    txCounter = 0;
  }
  txCounter++;
}