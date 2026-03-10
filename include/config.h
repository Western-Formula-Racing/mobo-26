#define PRECHARGE_TIMEOUT 5000    // milliseconds
#define PRECHARGE_RATIO   0.9    // % of pack voltage to complete precharge at
#define CHARGE_TARGET     410     // voltage to stop charging at
#define CHARGE_CURRENT    5
#define DO_BUS_RECOVERY   true
#define MAX_RECOVERY_ATTEMPTS 10

#define OVERTEMP_THRESHOLD 60
#define OVERVOLTAGE_THRESHOLD 4.15
#define UNDERVOLTAGE_THRESHOLD 2.7
#define MAX_CELL_DELTA  0.2
#define CURRENT_LIMIT 200
#define MAX_MODULE_TIMEOUT 50000000
#define PRECHARGE_MINDELAY 2000

#define MISSION_MODE // only critical faults, for racing
#define INVERTER_PRECHARGE // precharge voltage from inverter