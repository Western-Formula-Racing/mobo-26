#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "io.h"
#include "CAN.h"
#include "statemachine.h"
#include "BMS.h"
#include "tempsense.h"

static const char* TAG = "periodic";
int periodicCount = 0;

static int tempCount = 0;

void printInfo(){
  // teleplot io
  //printf(">IMD:%d|np \n>BSPD:%d|np \n>LATCH:%d|np \n>AIRN:%d|np \n>AIRP:%d|np \n>CHARGE_EN:%d|np \n>BMS_OK:%d|np \n>PRECH_OK:%d|np \n>RED_LED:%d|np \n>GREEN_LED:%d|np \n",inputStates[IMD_RELAY],inputStates[BSPD_RELAY],inputStates[LATCH_RELAY],inputStates[AIRN_RELAY],inputStates[AIRP_RELAY],inputStates[CHARGE_EN],outputStates[OUTPUTS_BMS_OK],outputStates[OUTPUTS_PRECH_OK],outputStates[OUTPUTS_RED_LED],outputStates[OUTPUTS_GREEN_LED]);
  //ESP_LOGI(TAG,"> Digital Inputs: IMD: %d | BSPD: %d | LATCH: %d | AIRN: %d | AIRP: %d | CHARGE_EN %d",inputStates[IMD_RELAY],inputStates[BSPD_RELAY],inputStates[LATCH_RELAY],inputStates[AIRN_RELAY],inputStates[AIRP_RELAY],inputStates[CHARGE_EN]);
  //ESP_LOGI(TAG,"> Digital Outputs: BMS_OK: %d | PRECH_OK: %d | RED_LED: %d | GREEN_LED: %d",outputStates[OUTPUTS_BMS_OK],outputStates[OUTPUTS_PRECH_OK],outputStates[OUTPUTS_RED_LED],outputStates[OUTPUTS_GREEN_LED]);
  printf(">m1_timeout:%d \n>m2_timeout:%d \n>m3_timeout:%d \n>m4_timeout:%d \n>m5_timeout:%d \n",modules[0].timeout,modules[1].timeout,modules[2].timeout,modules[3].timeout,modules[4].timeout);
  printModules();
}

// main periodic callback function
void periodicCallback(TimerHandle_t xTimer){
  ioPeriodic(); // Digital/Analog IO
  //canTxPeriodic(); // send CAN messages
  stateMachinePeriodic(); // run state machine
  periodicCount++;
  if(periodicCount>100){
    outputStates[OUTPUTS_GREEN_LED] = !outputStates[OUTPUTS_GREEN_LED];
    printInfo();
    periodicCount = 0;
  }



//Temp sense handling
if (tempCount >=500 && tempStatus.tempFlag == false) { //5 sec bus downtime
        measureTemp();
        tempCount = 0;
    }

 if (tempCount>=75 && tempStatus.tempFlag == true)  { //periodic temp read
    onewire_depower(GPIO_ONE_WIRE);

    readTemp(); 
    printf(">temp1:%f \n>temp1:%f \n>temp1:%f \n>temp1:%f \n>temp1:%f \n", tempStatus.temp[0], tempStatus.temp[1], tempStatus.temp[2], tempStatus.temp[3], tempStatus.temp[4]);
    tempStatus.tempFlag = false;
    tempCount = 0; 

  }

  tempCount++;

}