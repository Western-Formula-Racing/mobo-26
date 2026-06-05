#pragma once
#include "freertos/FreeRTOS.h"

enum state_e {
  IDLE,             // LV on 0
  PLACEHOLDER,    
  PRECHARGE,
  HV_ACTIVE,        // HV Active, precharge complete 3
  CHARGING,         // On chargecart and charging 4 
  CHARGE_COMPLETE,  // Charging complete, "limp" mode 5
  FAULT             // AMS Fault raised 6
};

enum error_e {
  NO_ERROR = 0,
  // Shared between TORCH and Mobo
  OVERTEMP = 69,  //Thermistor > 60C
  UNDERVOLTAGE = 70,   //Cell voltage < 3V
  OVERVOLTAGE = 71,    //cell voltage > 4.2 V
  OPENCELL = 72,       //open cell circuit
  OPENTHERMISTOR = 73, //open thermistor circuit
  ERROR_DIAGN = 74,          //LTC6813 DIAGN fail
  ERROR_MUTE = 75,
  ERROR_CVST = 76,           //LTC6813 CVST fail
  ERROR_STATST = 77,         //LTC6813 STATST fail
  ERROR_AXST = 78,           //LTC6813 AXST fail
  ERROR_ADOL = 79,           //LTC6813 ADOL fail
  ERROR_OUT_OF_RANGE_VA = 80,
  ERROR_OUT_OF_RANGE_VD = 81,
  ERROR_OUT_OF_RANGE_REF2 = 82,
  ERROR_LTC6813_OVERHEAT = 83,
  ERROR_PWM_SETUP = 84,
  ERROR_BALANCE_INITIATION = 85,
  ERROR_PEC = 86,
  ERROR_CAN_READ = 87, // TORCH CAN ERROR
  // Mobo specific Error codes
  OVERCURRENT = 88 ,    //Overcurrent fail
  CANTIMEOUT_INVERTER = 89, // CAN Inverter Timeout
  CANTIMEOUT_MODULES = 90,   //Can Torch board Timeout 
  CANERROR = 91,       //CAN bus tried restarting >MAX_RECOVERY_ATTEMPTS times
  IMBALANCE = 92,      //cell imbalance > 0.2 V
  PRECHARGE_FAIL = 93   //Precharge took longer than PRECHARGE_TIMEOUT
};

typedef struct{
  enum error_e error;
  enum state_e currentState;
  enum state_e lastState;
  uint64_t prechargeStartTime;
  int errorIndex; // which cell/thermistor raised error (if applicable)
  int errorModule; // which module raised the error
  int timeout_length;
} state_t;

extern state_t moboState;

void stateMachinePeriodic();
void raiseTorchError(enum error_e error, int module);
void printFault();
