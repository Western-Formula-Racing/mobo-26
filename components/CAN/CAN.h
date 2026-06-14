#pragma once
#include "CANMessages.h"
#include "config.h"
// CAN IDs
enum canID{
  //rx ids
  id_torchFault = 1000,
  #ifdef INVERTER_PRECHARGE
  id_InverterVoltageInfo = 167,
  #endif
  // TX ids
  id_packStatus = 1056,
  id_packInfo = 1057,
  id_BMSCurrentLimit = 514,
  id_ElconLimits = 403105268,
  id_analogReading = 1063,
  id_oneWireTemp = 1065,
};

typedef struct {
  uint32_t id;
  uint8_t  dlc;
  uint8_t  data[8];
} CanRxItem;

extern int bus_recovery_attempts;

#ifdef INVERTER_PRECHARGE
extern int inCar;
extern float inverterVoltage;
#endif


void initCAN();
void canTxPeriodic();
void printCANInfo();
uint32_t getMaxInverterTimeout();

union CANBuffer_u{
  uint8_t array[8];
  uint64_t data;
  packInfo_m packInfo;
  analogReadings_m analogReadings;
  oneWireTemps_m oneWireTemps;
  elconLimits_m elconLimits;
  packStatus_m packStatus;
  BMSCurrentLimit_m BMSCurrentLimit;
  BMSVoltages_m BMSVoltages;
  BMSTemperatures_m BMSTemperatures;
  TORCHFault_m TORCHFault;
  #ifdef INVERTER_PRECHARGE
  InverterVoltageInfo_m InverterVoltageInfo;
  #endif
};