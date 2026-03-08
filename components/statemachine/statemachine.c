#include "esp_log.h"
#include "statemachine.h"
#include "io.h"
#include "config.h"
#include "BMS.h"
#include "CAN.h"
#include "Inverter.h"

state_t moboState;
static const char* TAG = "statemachine";
bool useInverterVoltage = true;
float HV_Voltage = 0;

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
      // check if precharge timed out
      if((pdTICKS_TO_MS(xTaskGetTickCount()) - moboState.prechargeStartTime) > PRECHARGE_TIMEOUT){
        ESP_LOGE(TAG, "Precharge Timed out!");
        moboState.currentState = FAULT;
        moboState.lastState = PRECHARGE;
        moboState.error = PRECHARGE_FAIL;
        break;
      }
      // check if precharge success


      HV_Voltage = Vsense_VtoV(analogVoltages[ANALOG_VSENSE]);
      if(useInverterVoltage){
        HV_Voltage = getInverterVoltage();
      }
      if( HV_Voltage > (PRECHARGE_RATIO * getPackVoltage()) && (pdTICKS_TO_MS(xTaskGetTickCount()) - moboState.prechargeStartTime) > PRECHARGE_MINDELAY){
        moboState.currentState = HV_ACTIVE;
        moboState.lastState = PRECHARGE;
        outputStates[OUTPUTS_PRECH_OK] = 1;
        ESP_LOGI(TAG, "PRECHARGE -> HV_ACTIVE");
      }
      break;
    case HV_ACTIVE:
      // if charge switch toggled, switch to charging mode
      if(inputStates[CHARGE_EN] == 1){
        moboState.currentState = CHARGING;
        moboState.lastState = HV_ACTIVE;
        ESP_LOGI(TAG, "HV_ACTIVE -> CHARGING");
      }
      break;
    case CHARGING:
      if(inputStates[CHARGE_EN] == 0){
        moboState.currentState = HV_ACTIVE;
        moboState.lastState = CHARGING;
        ESP_LOGI(TAG, "CHARGING -> HV_ACTIVE");
      }
      if(getPackVoltage() > CHARGE_TARGET){
        moboState.currentState = IDLE;
        moboState.lastState = CHARGING;
        ESP_LOGI(TAG, "CHARGING -> IDLE");
      }
      break;
    case CHARGE_COMPLETE:
      break;
    case FAULT:
      ESP_LOGE(TAG,"FAULT: %d", moboState.error);
      outputStates[OUTPUTS_RED_LED] = 1;
      outputStates[OUTPUTS_BMS_OK] = 0;
      break;
  }
}

void errorCheck(){
  // 2 error check levels: mission mode for only critical faults, and test mode for all faults
  if(getMaxTemp() > OVERTEMP_THRESHOLD){
    moboState.error = OVERTEMP;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: OVERTEMP");
  } else if(getMaxVoltage() > OVERVOLTAGE_THRESHOLD){
      moboState.error = OVERVOLTAGE;
      moboState.lastState = moboState.currentState;
      moboState.currentState = FAULT;
      ESP_LOGE(TAG, "Fault: OVERVOLTAGE");
  } else if(getMinVoltage() < UNDERVOLTAGE_THRESHOLD && getMinVoltage() > 0){ // if voltage is 0, it's likely an open circuit which is a different fault
    moboState.error = UNDERVOLTAGE;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: UNDERVOLTAGE");
  } else if (Cursense_VtoA(analogVoltages[ANALOG_CURSENSE]) > CURRENT_LIMIT){
    moboState.error = OVERCURRENT;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: OVERCURRENT");
  } else if(getMaxModuleTimeout() > MAX_MODULE_TIMEOUT){
    moboState.error = CANTIMEOUT;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: CAN Timeout");
  }else if(bus_recovery_attempts >= MAX_RECOVERY_ATTEMPTS){
    moboState.error = CANERROR;
    moboState.lastState = moboState.currentState;
    moboState.currentState = FAULT;
    ESP_LOGE(TAG, "Fault: CAN Failed to recover");
  }
}

void stateMachinePeriodic(){
  errorCheck();
  stateTransition();
}