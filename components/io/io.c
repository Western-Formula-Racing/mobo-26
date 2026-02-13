#include "hw_define.h"
#include "io.h"
#include "esp_log.h"

uint8_t inputBuffers[INPUTS_COUNT][INPUT_BUFFER_SIZE] = {};
uint8_t relayStates[INPUTS_COUNT] = {};
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


// update digital inputs
void digitalInputs(){
  int doUpdate = 0;
  // shift relay buffers right
  for(int i = 0; i < INPUTS_COUNT; i++){
    for(int j = INPUT_BUFFER_SIZE; j > 0; j--){
      inputBuffers[i][j] = inputBuffers[i][j-1];
    }
  }
  // update first element
  inputBuffers[IMD_RELAY][0]   = gpio_get_level(GPIO_IMD);
  inputBuffers[BSPD_RELAY][0]  = gpio_get_level(GPIO_BSPD);
  inputBuffers[LATCH_RELAY][0] = gpio_get_level(GPIO_LATCH);
  inputBuffers[AIRN_RELAY][0]  = gpio_get_level(GPIO_AIRN);
  inputBuffers[AIRP_RELAY][0]  = gpio_get_level(GPIO_AIRP);
  inputBuffers[CHARGE_EN][0]  = gpio_get_level(GPIO_CHARGE_EN);
  // update relay status based on buffer content
  for(int i = 0; i < INPUTS_COUNT; i++){
    doUpdate = 1;
    // check if all elements are equal
    for(int j = 0; j < INPUT_BUFFER_SIZE; j++){
      if(inputBuffers[i][j] != inputBuffers[i][j+1]){
        doUpdate = 0;
      }
    }
    // if all elements are equal, update 
    if(doUpdate == 1){
      inputStates[i] = inputBuffers[i][0];
      ESP_LOGI(TAG,"input %d updated to %d, array values: [%d, %d, %d, %d, %d]",i, inputStates[i], inputBuffers[i][0], inputBuffers[i][1], inputBuffers[i][2], inputBuffers[i][3], inputBuffers[i][4]);
    }
  }
}

// update analog inputs
// TODO

// update digital outputs
void digitalOutputs(){

}
// IO Periodic function
void ioPeriodic(){
  digitalInputs();
  digitalOutputs();
}
