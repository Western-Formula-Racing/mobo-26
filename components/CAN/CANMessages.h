#include "freeRTOS/FreeRTOS.h"

// TX message definitions

typedef struct{
  uint8_t minTemp_lo;
  uint8_t minTemp_hi;
  uint8_t maxTemp_lo;
  uint8_t maxTemp_hi;
  uint8_t minVoltage_lo;
  uint8_t minVoltage_hi;
  uint8_t maxVoltage_lo;
  uint8_t maxVoltage_hi;
} packInfo_m;

typedef struct{
  uint8_t maxChargeVoltage_lo;
  uint8_t maxChargeVoltage_hi;
  uint8_t maxChargeCurrent_lo;
  uint8_t maxChargeCurrent_hi;
  uint8_t control; // 0 = start charging, 1 = stop charging
  uint8_t spare1;
  uint8_t spare2;
  uint8_t spare3;
} elconLimits_m;

typedef struct{
  uint8_t packCurrent_lo;
  uint8_t packCurrent_hi;
  uint8_t IMD : 1;
  uint8_t AMS : 1;
  uint8_t BSPD : 1;
  uint8_t Latch : 1;
  uint8_t AirN: 1;
  uint8_t HVActive : 3;
  uint8_t SOC_lo;
  uint8_t SOC_hi;
  uint8_t packStatus;
  uint8_t fault;
} packStatus_m;

typedef struct{
  uint8_t BMSCurrentLimit_lo;
  uint8_t BMSCurrentLimit_hi;
  uint8_t BMSChargeCurrent_lo;
  uint8_t BMSChargeCurrent_hi;
  uint16_t spare1;
  uint16_t spare2;
} BMSCurrentLimit_m;

//RX Messages
typedef struct{
  uint8_t v1_lo;
  uint8_t v1_hi;
  uint8_t v2_lo;
  uint8_t v2_hi;
  uint8_t v3_lo;
  uint8_t v3_hi;
  uint8_t v4_lo;
  uint8_t v4_hi;
} BMSVoltages_m;

typedef struct{
  uint8_t t1_lo;
  uint8_t t1_hi;
  uint8_t t2_lo;
  uint8_t t2_hi;
  uint8_t t3_lo;
  uint8_t t3_hi;
  uint8_t t4_lo;
  uint8_t t4_hi;
} BMSTemperatures_m;

typedef struct{
  int16_t INV_DC_Bus_Voltage;
  int16_t INV_Output_Voltage;
  int16_t INV_VAB_Vd_Voltage;
  int16_t INV_VBC_Vq_Voltage;
} M167_Voltage_Info_m;