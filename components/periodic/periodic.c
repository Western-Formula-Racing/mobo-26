#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "io.h"
#include "CAN.h"

// main periodic callback function
void periodicCallback(TimerHandle_t xTimer){
  relayPeriodic(); // get relay states
  canTxPeriodic(); // send CAN messages
}