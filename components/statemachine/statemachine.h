#pragma once
#include "freertos/FreeRTOS.h"

enum state_e {
  IDLE,             // LV on 
  PRECHARGE,        // check for precharge complete
  HV_ACTIVE,        // HV Active, precharge complete
  CHARGING,         // On chargecart and charging
  CHARGE_COMPLETE,  // Charging complete, "limp" mode
  FAULT             // AMS Fault raised
};

enum error_e {
  NO_ERROR = 0,
  OVERTEMP = 69,  //Thermistor > 60C
  UNDERVOLTAGE,   //Cell voltage < 3V
  OVERVOLTAGE,    //cell voltage > 4.2 V
  IMBALANCE,      //cell imbalance > 0.2 V
  OPENCELL,       //open cell circuit
  OPENTHERMISTOR, //open thermistor circuit
  DIAGN,          //LTC6813 DIAGN fail
  AXST,           //LTC6813 AXST fail
  CVST,           //LTC6813 CVST fail
  STATST,         //LTC6813 STATST fail
  ADOW,           //LTC6813 ADOW fail
  AXOW,           //LTC6813 AXOW fail
  ADOL,           //LTC6813 ADOL fail
  CRCFAIL,        //LTC6813 repeating CRC fail
  OVERCURRENT,    //Overcurrent fail
  CANTIMEOUT,     //Can Timeout fail
  CANERROR,       //CAN errors > 96
  PRECHARGE_FAIL   //Precharge took longer than PRECHARGE_TIMEOUT
};

typedef struct{
  error_e error;
  state_e currentState;
  state_e lastState;
  uint64_t prechargeStartTime;
} state_t;


void stateMachinePeriodic();