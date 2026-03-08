#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "Inverter.h"

float inverterVoltage = 0.0f;

void setInverterVoltage(int voltage_x10){
    inverterVoltage = voltage_x10/10.0f;
}

float getInveterVoltage(){
    return inverterVoltage;
}