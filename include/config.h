#define PRECHARGE_TIMEOUT 7000    // milliseconds
#define PRECHARGE_RATIO   0.9    // % of pack voltage to complete precharge at
#define CHARGE_TARGET     415     // voltage to stop charging at
#define CHARGE_CURRENT    5
#define DO_BUS_RECOVERY   true
#define MAX_RECOVERY_ATTEMPTS 10

#define OVERTEMP_THRESHOLD 60
#define OVERVOLTAGE_THRESHOLD 4.20
#define UNDERVOLTAGE_THRESHOLD 2.7
#define MAX_CELL_DELTA  0.2
#define CURRENT_LIMIT 200
#define MAX_CAN_TIMEOUT 60000
#define PRECHARGE_MINDELAY 5000

#define MISSION_MODE // only critical faults, for racing
#define INVERTER_PRECHARGE // precharge voltage from inverter

// --- SOC estimation (hybrid coulomb-counting + ECM/OCV correction, VTC6 cell data) ---
// telemetry only - never feeds back into the state machine / fault logic
#define PACK_NUM_SERIES           100     // cells in series (5 modules x 20s)
#define PACK_NUM_PARALLEL         6       // cells in parallel per series position (100s6p)
#define SOC_OCV_CORRECTION_GAIN  0.02f    // per-second complementary-filter gain pulling the coulomb count toward the ECM-based OCV estimate
#define SOC_IDLE_REST_MS          300000  // IDLE (AIRs open, guaranteed zero current) duration before hard-anchoring to the OCV table
#define SOC_NVS_SAVE_INTERVAL_MS  60000   // minimum time between NVS writes (flash wear)
#define SOC_NVS_SAVE_DELTA        0.5f    // minimum SOC change (%) required to trigger a save once the interval elapses