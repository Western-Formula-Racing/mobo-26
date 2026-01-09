#include "freertos/FreeRTOS.h"

struct relayStates{
  uint8_t imd;
  uint8_t bspd;
  uint8_t latch;
  uint8_t airn;
  uint8_t airp;
};

void initGPIO();
void relayPeriodic(); 