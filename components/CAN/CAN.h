#pragma once
#include "CANMessages.h"
// CAN IDs
enum canID{
  // TX ids
  id_packStatus = 1056,
  id_packInfo = 1057,
  id_BMSCurrentLimit = 514,
  id_ElconLimits = 403105268,
};

extern int bus_recovery_attempts;

void initCAN();
void canTxPeriodic();
void printCANInfo();

union CANBuffer_u{
  uint8_t array[8];
  uint64_t data;
  packInfo_m packInfo;
  elconLimits_m elconLimits;
  packStatus_m packStatus;
  BMSCurrentLimit_m BMSCurrentLimit;
  BMSVoltages_m BMSVoltages;
  BMSTemperatures_m BMSTemperatures;
  M167_Voltage_Info_m M167_Voltage_Info;
};