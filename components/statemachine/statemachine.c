#include "esp_log.h"
#include "statemachine.h"
#include "io.h"
#include "config.h"


state_t MoboState;
static const char* TAG = "statemachine";

void stateTransition(){
  
  switch(MoboState.currentState){
    case IDLE:
      // check for precharge start
      if(inputStates[AIRN_RELAY] = 1){
        MoboState.currentState = PRECHARGE;
        MoboState.lastState = IDLE;
        MoboState.prechargeStartTime = pdTICKS_TO_MS(xTaskGetTickCount());
        ESP_LOGI(TAG, "IDLE -> PRECHARGE");
      }
      break;
    case PRECHARGE:
      // check if precharge timed out
      if((pdTICKS_TO_MS(xTaskGetTickCount()) - MoboState.prechargeStartTime) > PRECHARGE_TIMEOUT){
        ESP_LOGE(TAG, "Precharge Timed out!");
        MoboState.currentState = FAULT;
        MoboState.lastState = PRECHARGE;
        MoboState.error = PRECHARGE_FAIL;
        break;
      }
      // check if precharge success
      if( getPrechargeVoltage() > (PRECHARGE_RATIO * getPackVoltage())){
        MoboState.currentState = HV_ACTIVE;
        MoboState.lastState = PRECHARGE;
        ESP_LOGI(TAG, "PRECHARGE -> HV_ACTIVE");
      }
      break;
    case HV_ACTIVE:
      // if charge switch toggled, switch to charging mode
      if(inputStates[CHARGE_EN] == 1){
        MoboState.currentState = CHARGING;
        MoboState.lastState = HV_ACTIVE;
        ESP_LOGI(TAG, "HV_ACTIVE -> CHARGING");
      }
      break;
    case CHARGING:
      if(inputStates[CHARGE_EN] == 0){
        MoboState.currentState = HV_ACTIVE;
        MoboState.lastState = CHARGING;
        ESP_LOGI(TAG, "CHARGING -> HV_ACTIVE");
      }
      if(getPackVoltage() > CHARGE_TARGET){
        MoboState.currentState = IDLE;
        MoboState.lastState = CHARGING;
        ESP_LOGI(TAG, "CHARGING -> IDLE");
      }
      break;
    case CHARGE_COMPLETE:
      break;
  }
}

void errorCheck(){
  
}

void stateMachinePeriodic(){
  errorCheck();
  stateTransition();
}