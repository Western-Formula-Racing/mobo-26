#include "esp_log.h"
#include "statemachine.h"
#include "io.h"
#include "config.h"
#include "BMS.h"
#include "CAN.h"

state_t MoboState;
static const char* TAG = "statemachine";

void stateTransition(){
  
  switch(MoboState.currentState){
    case IDLE:
      outputStates[OUTPUTS_BMS_OK] = 0;
      // check for precharge start
      if(inputStates[AIRN_RELAY] == 1){
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
      if( Vsense_VtoV(analogVoltages[ANALOG_VSENSE]) > (PRECHARGE_RATIO * getPackVoltage())){
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
    case FAULT:
      ESP_LOGE(TAG,"FAULT: %d", MoboState.error);
      outputStates[OUTPUTS_RED_LED] = 1;
      outputStates[OUTPUTS_BMS_OK] = 0;
      break;
  }
}

void errorCheck(){
  // 2 error check levels: mission mode for only critical faults, and test mode for all faults
  if(getMaxTemp() > OVERTEMP_THRESHOLD){
    MoboState.error = OVERTEMP;
    MoboState.lastState = MoboState.currentState;
    MoboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: OVERTEMP");
  } else if(getMaxVoltage() > OVERVOLTAGE_THRESHOLD){
    MoboState.error = OVERVOLTAGE;
    MoboState.lastState = MoboState.currentState;
    MoboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: OVERVOLTAGE");
  } else if(getMinVoltage() < UNDERVOLTAGE_THRESHOLD && getMinVoltage() > 0){ // if voltage is 0, it's likely an open circuit which is a different fault
    MoboState.error = UNDERVOLTAGE;
    MoboState.lastState = MoboState.currentState;
    MoboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: UNDERVOLTAGE");
  } else if (Cursense_VtoA(analogVoltages[ANALOG_CURSENSE]) > CURRENT_LIMIT){
    MoboState.error = OVERCURRENT;
    MoboState.lastState = MoboState.currentState;
    MoboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: OVERCURRENT");
  } else if(getMaxModuleTimeout() > MAX_MODULE_TIMEOUT){
    MoboState.error = CANTIMEOUT;
    MoboState.lastState = MoboState.currentState;
    MoboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: CAN Timeout");
  }else if(bus_recovery_attempts >= MAX_RECOVERY_ATTEMPTS){
    MoboState.error = CANERROR;
    MoboState.lastState = MoboState.currentState;
    MoboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: CAN Failed to recover");
  }
}

void stateMachinePeriodic(){
  errorCheck();
  stateTransition();
}