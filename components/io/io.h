#ifndef IO_H
#define IO_H

#include "freertos/FreeRTOS.h"
#include "hw_define.h"

#define INPUT_BUFFER_SIZE 5
#define Cursense_VtoA(v) (v-2.5)*(1/0.0057)

//60v = 0.355v on ADC 
//^ Not sure how this was calculated... ratio is VADC = VBUS*0.0109
// #define Vsense_VtoV(v) (v)*169.014
#define Vsense_VtoV(v) (v)*0.0109




enum digitalInputs_e{
  IMD_RELAY,
  BSPD_RELAY,
  LATCH_RELAY,
  AIRN_RELAY,
  AIRP_RELAY,
  CHARGE_EN,
  INPUTS_COUNT
};

enum analog_e{
  ANALOG_CURSENSE,
  ANALOG_3V3,
  ANALOG_VSENSE,
  ANALOG_COUNT,
};

enum digitalOutputs_e{
  OUTPUTS_BMS_OK,
  OUTPUTS_PRECH_OK,
  OUTPUTS_RED_LED,
  OUTPUTS_GREEN_LED,
  OUTPUTS_COUNT,
};

extern uint8_t inputStates[INPUTS_COUNT];
extern uint8_t outputStates[OUTPUTS_COUNT];
extern float   analogVoltages[ANALOG_COUNT];

void initIO();
void ioPeriodic();
esp_err_t enablePrecharge();

#endif // IO_H