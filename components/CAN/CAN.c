#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_log.h"
#include "hw_define.h"

static const char* TAG = "CAN";

twai_node_handle_t mobo_node_handle = NULL;

// callback for recieving messages
static bool can_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx) {
  uint8_t recv_buff[8];
  twai_frame_t rx_frame = {
    .buffer = recv_buff,
    .buffer_len = sizeof(recv_buff),
  };
  if (ESP_OK == twai_node_receive_from_isr(handle, &rx_frame)) {
    ESP_LOGI(TAG,"Recieved bits: %X,%X,%X,%X,%X,%X,%X,%X",recv_buff[0],recv_buff[1],recv_buff[2],recv_buff[3],recv_buff[4],recv_buff[5],recv_buff[6],recv_buff[7]);
  }
  return false;
}

// init CAN
void initCAN(){
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
}

// CAN message transmission
int txCounter = 0;
uint8_t canTxBuffer[8] = {0};

twai_frame_t txMessage = {
  .header.id = 0x0,            // Message ID
  .header.ide = false,          // Use 29-bit extended ID format
  .buffer = canTxBuffer, // Pointer to data to transmit
  .buffer_len = 8,             // Length of data to transmit
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