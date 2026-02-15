#include "freertos/FreeRTOS.h"
#include "hw_define.h"
#define INPUT_BUFFER_SIZE 5
#define Cursense_VtoA(v) (v)
#define Vsense_VtoV(v) (v)

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
  ANALOG_VSENSE,
  ANALOG_CURSENSE,
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