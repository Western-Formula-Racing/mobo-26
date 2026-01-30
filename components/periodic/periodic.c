#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "io.h"
#include "CAN.h"
#include "esp_log.h"

static const char* TAG = "periodic.c";

// main periodic callback function
void periodicCallback(TimerHandle_t xTimer){
  relayPeriodic(); // get relay states
  canTxPeriodic(); // send CAN messages
}