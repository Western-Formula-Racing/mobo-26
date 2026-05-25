#pragma once

#include "freertos/FreeRTOS.h"
#include "hw_define.h"
#include "ds18x20.h"
#include "onewire.h"


void scanDevices();
void writeScratch();
void measureTemp();
void readTemp();

static struct {
    float temp[5];
    bool tempFlag; // 'True' bus measuring-state 'False' bus idle/fault state
    size_t found; 
    onewire_addr_t address[5]; 
}tempStatus;

