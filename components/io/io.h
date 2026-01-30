#include "freertos/FreeRTOS.h"
#define RELAY_BUFFER_SIZE 5

enum relays_e{
  IMD_RELAY,
  BSPD_RELAY,
  LATCH_RELAY,
  AIRN_RELAY,
  AIRP_RELAY,
  RELAYS_COUNT
};

enum analog_e{
  ANALOG_PRECHARGE,
  ANALOG_COUNT,
};

extern uint8_t relayStates[RELAYS_COUNT];
extern float   analogVoltages[ANALOG_COUNT];

void initGPIO();
void relayPeriodic(); 
esp_err_t enablePrecharge();