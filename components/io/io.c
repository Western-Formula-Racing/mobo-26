#include "hw_define.h"
#include "io.h"
#include "esp_log.h"

uint8_t relayBuffers[RELAYS_COUNT][RELAY_BUFFER_SIZE] = {};
uint8_t relayStates[RELAYS_COUNT] = {};
float   analogVoltages[ANALOG_COUNT] = {};

static const char* TAG = "io"; 

// initilaize digital input/output pins 
void initGPIO(){
  gpio_config_t io_cfg = {};
  
  io_cfg.intr_type = GPIO_INTR_DISABLE;        // disable interrupt
  io_cfg.mode = GPIO_MODE_INPUT;               // set mode to input
  io_cfg.pin_bit_mask = GPIO_INPUT_PIN_SELECT; // input pin mask
  io_cfg.pull_down_en = true;                  // enable pulldown mode
  
  gpio_config(&io_cfg); // config inputs
  
  io_cfg.mode = GPIO_MODE_OUTPUT;               // set mode to output
  io_cfg.pin_bit_mask = GPIO_OUTPUT_PIN_SELECT; // output pin mask
  
  gpio_config(&io_cfg); // config outputs
}

// Periodic function to update relay inputs
void relayPeriodic(){
  int updateState = 0;
  // shift relay buffers right
  for(int i = 0; i < RELAYS_COUNT; i++){
    for(int j = RELAY_BUFFER_SIZE; j > 0; j--){
      relayBuffers[i][j] = relayBuffers[i][j-1];
    }
  }
  // update first element
  relayBuffers[IMD_RELAY][0]   = gpio_get_level(GPIO_IMD);
  relayBuffers[BSPD_RELAY][0]  = gpio_get_level(GPIO_BSPD);
  relayBuffers[LATCH_RELAY][0] = gpio_get_level(GPIO_LATCH);
  relayBuffers[AIRN_RELAY][0]  = gpio_get_level(GPIO_AIRN);
  relayBuffers[AIRP_RELAY][0]  = gpio_get_level(GPIO_AIRP);
  // update relay status based on buffer content
  for(int i = 0; i < RELAYS_COUNT; i++){
    updateState = 1;
    // check if all elements are equal
    for(int j = 0; j < RELAY_BUFFER_SIZE; j++){
      if(relayBuffers[i][j] != relayBuffers[i][j+1]){
        updateState = 0;
      }
    }
    // if all elements are equal, update 
    if(updateState == 1){
      relayStates[i] = relayBuffers[i][0];
      ESP_LOGI(TAG,"Relay %d updated to %d, array values: [%d, %d, %d, %d, %d]",i, relayStates[i], relayBuffers[i][0], relayBuffers[i][1], relayBuffers[i][2], relayBuffers[i][3], relayBuffers[i][4]);
    }
  }
}
