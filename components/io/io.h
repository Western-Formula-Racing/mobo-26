#include "freertos/FreeRTOS.h"
#define INPUT_BUFFER_SIZE 5

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
  ANALOG_PRECHARGE,
  ANALOG_COUNT,
};

enum digitalOutputs_e{
  GPIO_BMS_OK,
  GPIO_PRECH_OK,
  GPIO_RED_LED,
  GPIO_GREEN_LED,
};

extern uint8_t inputStates[INPUTS_COUNT];
extern float   analogVoltages[ANALOG_COUNT];

void initGPIO();
void ioPeriodic(); 
esp_err_t enablePrecharge();