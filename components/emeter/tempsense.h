#pragma once

#include "freertos/FreeRTOS.h"
#include "hw_define.h"
#include "ds18x20.h"
#include "onewire.h"
#define MAX_SENSOR_SLOTS 5  // Physical maximum your board supports

void scanDevices();
void writeScratch();
void measureTemp();
void readTemp();
void tempSensePeriodic(); 

typedef struct {
    ds18x20_addr_t address[MAX_SENSOR_SLOTS];
    float temp[MAX_SENSOR_SLOTS];
    size_t found;
    bool tempFlag; //temperature 'ready-to-measure' indicator (1 is ready, 0 is measuring)
    bool printReady;
}TempStatus_t;

extern TempStatus_t tempStatus;

