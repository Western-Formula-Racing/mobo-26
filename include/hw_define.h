// hw_define.h
// for hardware-related definitions

#include "driver/gpio.h"
#include <inttypes.h>
#include <stdlib.h>

//====GPIO======//

//digital inputs
#define GPIO_IMD        GPIO_NUM_38
#define GPIO_BSPD       GPIO_NUM_37
#define GPIO_LATCH      GPIO_NUM_39
#define GPIO_AIRN       GPIO_NUM_11
#define GPIO_AIRP       GPIO_NUM_10
#define GPIO_INPUT_PIN_SELECT ((1ULL<<GPIO_IMD)|(1ULL<<GPIO_BSPD)|(1ULL<<GPIO_LATCH)|(1ULL<<GPIO_AIRN)|(1ULL<<GPIO_AIRP))

#define GPIO_TEMP_DATA  GPIO_PIN_REG_35

//digital outputs
#define GPIO_BMS_OK        GPIO_NUM_9
#define GPIO_CHARGE_EN     GPIO_NUM_42
#define GPIO_PRECH_OK      GPIO_NUM_36
#define GPIO_RED_LED       GPIO_NUM_15
#define GPIO_GREEN_LED     GPIO_NUM_16
#define GPIO_OUTPUT_PIN_SELECT ((1ULL<<GPIO_BMS_OK)|(1ULL<<GPIO_CHARGE_EN)|(1ULL<<GPIO_PRECH_OK)|(1ULL<<GPIO_RED_LED)|(1ULL<<GPIO_GREEN_LED))

#define GPIO_TEMP_PULLUP   GPIO_NUM_21
#define ISOSPI_CS          GPIO_NUM_2
#define ADC_CS             GPIO_NUM_3

//====COMMS====//

#define GPIO_MISO     GPIO_NUM_4
#define GPIO_SCK      GPIO_NUM_5
#define GPIO_MOSI     GPIO_NUM_6
#define GPIO_CAN_TX   GPIO_NUM_48
#define GPIO_CAN_RX   GPIO_NUM_47