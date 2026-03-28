#include "esp_log.h"
#include "statemachine.h"
#include "io.h"
#include "config.h"
#include "BMS.h"
#include "CAN.h"

state_t moboState;
static const char* TAG = "statemachine";
uint8_t module, errIndex;
uint8_t module, errIndex;

void stateTransition(){
  switch(moboState.currentState){
    case IDLE:
      outputStates[OUTPUTS_BMS_OK] = 1;
      // check for precharge start
      if(inputStates[AIRN_RELAY] == 1 && inputStates[LATCH_RELAY] == 1 && inputStates[BSPD_RELAY] == 1 && inputStates[IMD_RELAY] == 1){
        moboState.currentState = PRECHARGE;
        moboState.lastState = IDLE;
        moboState.prechargeStartTime = pdTICKS_TO_MS(xTaskGetTickCount());
        ESP_LOGI(TAG, "IDLE -> PRECHARGE");
      }
      break;
    case PRECHARGE:
      vTaskDelay(pdMS_TO_TICKS(5000)); 
      moboState.currentState = HV_ACTIVE;
      moboState.lastState = PRECHARGE;
      outputStates[OUTPUTS_PRECH_OK] = 1;
      ESP_LOGI(TAG, "PRECHARGE -> HV_ACTIVE");
      break;
      // // check if precharge timed out
      // if((pdTICKS_TO_MS(xTaskGetTickCount()) - moboState.prechargeStartTime) > PRECHARGE_TIMEOUT){
      //   ESP_LOGE(TAG, "Precharge Timed out!");
      //   moboState.currentState = FAULT;
      //   moboState.lastState = PRECHARGE;
      //   moboState.error = PRECHARGE_FAIL;
      //   break;
      // }
      // // check if precharge success
      // if( Vsense_VtoV(analogVoltages[ANALOG_VSENSE]) > (PRECHARGE_RATIO * getPackVoltage()) && (pdTICKS_TO_MS(xTaskGetTickCount()) - moboState.prechargeStartTime) > PRECHARGE_MINDELAY){
      //   moboState.currentState = HV_ACTIVE;
      //   moboState.lastState = PRECHARGE;
      //   outputStates[OUTPUTS_PRECH_OK] = 1;
      //   ESP_LOGI(TAG, "PRECHARGE -> HV_ACTIVE");
      // }
      break;
    case HV_ACTIVE:
      // if charge switch toggled, switch to charging mode
      if(inputStates[CHARGE_EN] == 0){
        moboState.currentState = CHARGING;
        moboState.lastState = HV_ACTIVE;
        ESP_LOGI(TAG, "HV_ACTIVE -> CHARGING");
      }
      if(inputStates[AIRN_RELAY] == 0 || inputStates[LATCH_RELAY] == 0 || inputStates[BSPD_RELAY] == 0 ){
        moboState.currentState = IDLE;
        moboState.lastState = HV_ACTIVE;
        outputStates[OUTPUTS_PRECH_OK] = 0;
        ESP_LOGI(TAG, "HV_ACTIVE -> IDLE");
      }
      if(inputStates[IMD_RELAY] == 0 || inputStates[GPIO_BMS_OK] == 0){
        moboState.currentState = FAULT;
        moboState.lastState = HV_ACTIVE;
        outputStates[OUTPUTS_PRECH_OK] = 0;
        ESP_LOGI(TAG, "HV_ACTIVE -> FAULT");
      }
      break;
    case CHARGING:
      if(inputStates[CHARGE_EN] == 1){ //not charging 
        moboState.currentState = HV_ACTIVE;
        moboState.lastState = CHARGING;
        ESP_LOGI(TAG, "CHARGING -> HV_ACTIVE");
      }
      //IF ESTOP IS HIT ON CHARGE CART.
      if(inputStates[AIRN_RELAY] == 0 ){
        moboState.currentState = IDLE;
        moboState.lastState = CHARGING;
        outputStates[OUTPUTS_PRECH_OK] = 0;
        ESP_LOGI(TAG, "CHARGING -> IDLE");
      }
      if(getPackVoltage() > CHARGE_TARGET){
        moboState.currentState = IDLE;
        moboState.lastState = CHARGING;
        ESP_LOGI(TAG, "CHARGING -> IDLE");
      }
      break;
    case CHARGE_COMPLETE:
      break;
    case PLACEHOLDER:
      moboState.currentState = FAULT;
      moboState.lastState = PLACEHOLDER;
      ESP_LOGI(TAG, "idk how tf you got here");
      break;
    case FAULT:
      outputStates[OUTPUTS_RED_LED] = 1;
      outputStates[OUTPUTS_BMS_OK] = 0;
      outputStates[OUTPUTS_PRECH_OK] = 0;
      break;
  }
}

void errorCheck(){
  // skip check if already in fault state
  if (moboState.currentState == FAULT) return; 
  // 2 error check levels: mission mode for only critical faults, and test mode for all faults
  if(getMaxTempIndex(&module, &errIndex) > OVERTEMP_THRESHOLD){
    moboState.error = OVERTEMP;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    moboState.errorIndex = errIndex;
    moboState.errorModule = module;
    ESP_LOGE(TAG, "Fault: OVERTEMP");
  } else if(getMaxVoltageIndex(&module, &errIndex) > OVERVOLTAGE_THRESHOLD){
    moboState.error = OVERVOLTAGE;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    moboState.errorIndex = errIndex;
    moboState.errorModule = module;
    ESP_LOGE(TAG, "Fault: OVERVOLTAGE");
  } else if(getMinVoltageIndex(&module, &errIndex) < UNDERVOLTAGE_THRESHOLD && getMinVoltage() > 0){
    moboState.error = UNDERVOLTAGE;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    moboState.errorIndex = errIndex;
    moboState.errorModule = module;
    ESP_LOGE(TAG, "Fault: UNDERVOLTAGE");
  } else if (Cursense_VtoA(analogVoltages[ANALOG_CURSENSE]) > CURRENT_LIMIT){
    moboState.error = OVERCURRENT;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: OVERCURRENT");
  } else if(getMaxModuleTimeout(&module) > MAX_MODULE_TIMEOUT){
    moboState.error = CANTIMEOUT;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    moboState.errorModule = module;
    ESP_LOGE(TAG, "Fault: CAN Timeout");
  }else if(bus_recovery_attempts >= MAX_RECOVERY_ATTEMPTS){
    moboState.error = CANERROR;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: CAN Failed to recover");
  }
}

void raiseTorchError(enum error_e error, int module){
  moboState.error = error;
  moboState.errorModule = module;
  moboState.lastState = moboState.currentState;
  moboState.currentState = FAULT;
}

void stateMachinePeriodic(){
  errorCheck();
  stateTransition();
}

void printFault(){
  if(moboState.currentState == FAULT){
    ESP_LOGE(TAG,"Current State: %d, Last State: %d, Error: %d, Module: %d, Index: %d", moboState.currentState, moboState.lastState, moboState.error, moboState.errorModule, moboState.errorIndex);
  }
}