#pragma once

typedef struct{
  float voltages[20];
  float temps[18];
  int timeout;
} Module;

extern Module modules[5];

// functions used for CAN input (setters):
void setModuleVoltage(int module, int cell, float newVoltage);
void setModuleTemp(int module, int thermistor, float newTemp);

// info for other functions
float getPackVoltage();  // sum of all cell voltages
float getMaxTemp();      // maximum thermistor temperature
float getMinTemp();      // minimum thermistor temperature
float getMaxVoltage();   // maximum cell voltage
float getMinVoltage();   // minimum cell voltage
float getPackVoltage();  // sum of all cell voltages
int getMaxModuleTimeout(); // max time since last update for any module

//serial debugging
void printModules();
