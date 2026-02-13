#pragma once

typedef struct{
  float voltages[20];
  float temps[18];
} Module;

// functions used for CAN input (setters):
void setModuleVoltage(int module, int cell, double newVoltage);
void setModuleTemp(int module, int thermistor, double newTemp);

// info for other functions
double getPackVoltage();  // sum of all cell voltages
double getMaxTemp();      // maximum thermistor temperature
double getMaxVoltage();   // maximum cell voltage
double getMinVoltage();   // minimum cell voltage

//serial debugging
void printModules();