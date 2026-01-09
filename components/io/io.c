#include "hw_define.h"
#include "io.h"

relayStates = {};

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
  relayStates.imd = gpio_get_level(GPIO_IMD);
  relayStates.bspd = gpio_get_level(GPIO_BSPD);
  relayStates.latch = gpio_get_level(GPIO_LATCH);
  relayStates.airn = gpio_get_level(GPIO_AIRN);
  relayStates.airp = gpio_get_level(GPIO_AIRP);
}
