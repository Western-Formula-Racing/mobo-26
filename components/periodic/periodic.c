#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "io.h"
#include "CAN.h"
#include "statemachine.h"
#include "BMS.h"

static const char* TAG = "periodic";
int periodicCount = 0;

void printInfo(){
  // teleplot io
  //gpios
  if(moboState.currentState == FAULT){
    printFault();
  }else{
    //printModules();
  }
  printf(">IMD:%d|np \n>BSPD:%d|np \n>LATCH:%d|np \n>AIRN:%d|np \n>AIRP:%d|np \n>CHARGE_EN:%d|np \n>BMS_OK:%d|np \n>PRECH_OK:%d|np \n>RED_LED:%d|np \n>GREEN_LED:%d|np \n",inputStates[IMD_RELAY],inputStates[BSPD_RELAY],inputStates[LATCH_RELAY],inputStates[AIRN_RELAY],inputStates[AIRP_RELAY],inputStates[CHARGE_EN],outputStates[OUTPUTS_BMS_OK],outputStates[OUTPUTS_PRECH_OK],outputStates[OUTPUTS_RED_LED],outputStates[OUTPUTS_GREEN_LED]);
  printf(">precharge_voltage: %.2f\n",Vsense_VtoV(analogVoltages[ANALOG_VSENSE]));
  printf(">ADC CURR SENSE: %.2f\n", Vsense_VtoV(analogVoltages[ANALOG_CURSENSE]));
  printf(">state:%d|np \n>error:%d|np \n", moboState.currentState, moboState.error);
  printf(">pack_voltage:%.2f\n", getPackVoltage());
  printf(">max_cell_voltage:%.3f\n", getMaxVoltage());
  printf(">CHARGE_EN: %d\n", gpio_get_level(GPIO_NUM_42));
  //ESP_LOGI(TAG,"> Digital Inputs: IMD: %d | BSPD: %d | LATCH: %d | AIRN: %d | AIRP: %d | CHARGE_EN %d",inputStates[IMD_RELAY],inputStates[BSPD_RELAY],inputStates[LATCH_RELAY],inputStates[AIRN_RELAY],inputStates[AIRP_RELAY],inputStates[CHARGE_EN]);
  //ESP_LOGI(TAG,"> Digital Outputs: BMS_OK: %d | PRECH_OK: %d | RED_LED: %d | GREEN_LED: %d",outputStates[OUTPUTS_BMS_OK],outputStates[OUTPUTS_PRECH_OK],outputStates[OUTPUTS_RED_LED],outputStates[OUTPUTS_GREEN_LED]);
  printf(">m1_timeout,Module Timeouts:%d \n>m2_timeout,Module Timeouts:%d \n>m3_timeout,Module Timeouts:%d \n>m4_timeout,Module Timeouts:%d \n>m5_timeout,Module Timeouts:%d \n",modules[0].timeout,modules[1].timeout,modules[2].timeout,modules[3].timeout,modules[4].timeout);
  printf(">inverter_voltage: %.2f\n", inverterVoltage);
  printf(">inverter_timeout:%ld\n", getMaxCanTimeout());
  printCANInfo();
}

// main periodic callback function
void periodicCallback(TimerHandle_t xTimer){
  ioPeriodic(); // Digital/Analog IO
  canTxPeriodic(); // send CAN messages
  stateMachinePeriodic(); // run state machine
  periodicCount++;
  if(periodicCount>10){
    outputStates[OUTPUTS_GREEN_LED] = !outputStates[OUTPUTS_GREEN_LED];
    printInfo();
    periodicCount = 0;
  }
}