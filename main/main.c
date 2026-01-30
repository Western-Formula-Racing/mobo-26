#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "io.h"
#include "periodic.h"

// Code entry point
void app_main() {
  static const char* TAG = "Main";

  //set log levels for each component
  esp_log_level_set("CAN", CONFIG_LOG_MAXIMUM_LEVEL);
  esp_log_level_set("io", CONFIG_LOG_MAXIMUM_LEVEL);
  esp_log_level_set("periodic", CONFIG_LOG_MAXIMUM_LEVEL);
  esp_log_level_set("statemachine", CONFIG_LOG_MAXIMUM_LEVEL);
  
  //init functions go here
  initGPIO();

  //create periodic function timer
  TimerHandle_t periodicTimer = xTimerCreate("periodic",pdMS_TO_TICKS(1),pdTRUE,(void *) 1,periodicCallback);

  //start periodic function timer
  if( xTimerStart(periodicTimer,0) != pdPASS){
    ESP_LOGE(TAG,"Error: Periodic timer failed to start");
  }

  vTaskStartScheduler();

  while(1){
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "heartbeat.");
  }

}