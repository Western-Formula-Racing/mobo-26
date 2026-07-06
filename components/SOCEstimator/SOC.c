#include <math.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "SOC.h"
#include "BMS.h"
#include "io.h"
#include "statemachine.h"
#include "config.h"

static const char* TAG = "SOC";

#define NVS_NAMESPACE  "soc"
#define NVS_KEY_SOC    "soc_cpct" // stored as centi-percent (0-10000) so it fits an int32 without float rounding surprises
#define NVS_KEY_VALID  "soc_ok"

// --- Cell characterization data (Sony/Murata VTC6, cell-level) ---
// Source: vtc6_correction_tables.csv (kept alongside this file for provenance).
// Pack topology: 100s6p (5 modules of 20s6p) -> see PACK_NUM_SERIES/PACK_NUM_PARALLEL in config.h

// OCV(SOC) @ 25C, C/25 charge/discharge average
#define OCV_TABLE_N 21
static const float ocvSOC[OCV_TABLE_N]     = {0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100};
static const float ocvVoltage[OCV_TABLE_N] = {2.6667f,3.1758f,3.2962f,3.3906f,3.4588f,3.5260f,3.5701f,3.6237f,3.6705f,3.7152f,
                                               3.7617f,3.8049f,3.8457f,3.8884f,3.9372f,3.9889f,4.0446f,4.0795f,4.0946f,4.1191f,4.1878f};

// R0/R1 (mOhm per cell) vs SOC, tabulated at 5 temperatures
#define R_TABLE_SOC_N 20
#define R_TABLE_T_N   5
static const float rTableSOC[R_TABLE_SOC_N] = {5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100};
static const float rTableT[R_TABLE_T_N]     = {5,15,25,35,45};

static const float r0Table[R_TABLE_T_N][R_TABLE_SOC_N] = {
  {75.98f,54.27f,40.66f,35.37f,32.10f,30.56f,30.03f,29.74f,29.68f,29.75f,29.81f,29.89f,30.08f,30.44f,30.91f,31.90f,33.06f,34.32f,36.01f,38.38f}, // 5C
  {54.98f,34.97f,27.87f,24.19f,22.94f,22.46f,22.27f,22.04f,22.00f,22.06f,22.13f,22.20f,22.38f,22.80f,23.13f,23.85f,24.93f,26.19f,27.73f,30.07f}, // 15C
  {40.51f,25.93f,20.62f,18.90f,18.23f,17.89f,17.76f,17.63f,17.61f,17.64f,17.69f,17.73f,17.90f,18.24f,18.50f,19.02f,19.83f,20.90f,22.22f,24.43f}, // 25C
  {31.58f,19.68f,16.78f,16.04f,15.58f,15.36f,15.29f,15.21f,15.20f,15.22f,15.24f,15.24f,15.38f,15.68f,15.87f,16.23f,16.78f,17.53f,18.57f,20.52f}, // 35C
  {25.99f,16.44f,15.00f,14.50f,14.18f,14.01f,13.96f,13.91f,13.90f,13.89f,13.89f,13.90f,14.00f,14.27f,14.42f,14.66f,15.03f,15.54f,16.32f,17.85f}, // 45C
};
static const float r1Table[R_TABLE_T_N][R_TABLE_SOC_N] = {
  {1.79f, 5.30f,11.75f,10.30f, 9.92f,8.95f,9.85f,10.21f,10.22f,10.15f,9.80f,9.19f,8.60f,8.65f,9.09f,9.46f,8.86f,7.80f,6.87f,7.10f}, // 5C
  {3.95f,11.71f,15.20f,16.07f,15.85f,9.88f,9.04f, 9.77f, 9.87f, 9.84f,9.43f,8.60f,7.78f,8.21f,8.60f,8.73f,8.01f,6.77f,5.97f,6.14f}, // 15C
  {7.81f,20.84f,16.85f,18.11f,13.54f,8.94f,7.87f, 8.66f, 8.88f, 8.84f,8.35f,7.47f,6.70f,7.31f,7.72f,7.92f,7.24f,6.08f,5.40f,5.60f}, // 25C
  {10.66f,17.04f,17.22f,14.65f,11.08f,7.77f,7.08f,7.53f, 7.85f, 7.84f,7.45f,6.65f,5.88f,6.50f,6.89f,7.15f,6.52f,5.52f,4.99f,5.32f}, // 35C
  {14.16f,15.24f,15.06f,12.52f, 9.61f,7.07f,6.66f,6.69f, 7.23f, 7.34f,6.97f,6.15f,5.34f,5.87f,6.27f,6.54f,5.96f,5.03f,4.61f,5.20f}, // 45C
};
static const float RC_TAU_S = 5.51f; // R1/C1 branch time constant from characterization data

// Capacity vs temperature, Ah per cell at 0.5C
#define CAP_TABLE_N 5
static const float capT[CAP_TABLE_N] = {5,15,25,35,45};
static const float capQ[CAP_TABLE_N] = {2.853f,2.904f,2.927f,2.934f,2.932f};

static float soc = 50.0f; // default until NVS/OCV establishes a real value
static float socAtLastSave = 50.0f;
static float v1 = 0.0f;   // RC-branch (polarization) voltage state, volts
static int64_t lastUpdateUs = 0;
static int64_t lastSaveUs = 0;
static int64_t idleStartUs = 0;
static bool wasIdle = false;
static nvs_handle_t nvsHandle;
static bool nvsOk = false;

// clamped linear interpolation over an ascending x table
static float interp1D(const float* xs, const float* ys, int n, float x){
  if(x <= xs[0]) return ys[0];
  if(x >= xs[n - 1]) return ys[n - 1];
  for(int i = 0; i < n - 1; i++){
    if(x >= xs[i] && x <= xs[i + 1]){
      float frac = (x - xs[i]) / (xs[i + 1] - xs[i]);
      return ys[i] + frac * (ys[i + 1] - ys[i]);
    }
  }
  return ys[n - 1];
}

// bilinear interpolation over the R0/R1 (SOC x temperature) tables
static float interpRTable(const float table[R_TABLE_T_N][R_TABLE_SOC_N], float socPct, float tempC){
  if(tempC <= rTableT[0]) return interp1D(rTableSOC, table[0], R_TABLE_SOC_N, socPct);
  if(tempC >= rTableT[R_TABLE_T_N - 1]) return interp1D(rTableSOC, table[R_TABLE_T_N - 1], R_TABLE_SOC_N, socPct);
  for(int i = 0; i < R_TABLE_T_N - 1; i++){
    if(tempC >= rTableT[i] && tempC <= rTableT[i + 1]){
      float vLo = interp1D(rTableSOC, table[i], R_TABLE_SOC_N, socPct);
      float vHi = interp1D(rTableSOC, table[i + 1], R_TABLE_SOC_N, socPct);
      float frac = (tempC - rTableT[i]) / (rTableT[i + 1] - rTableT[i]);
      return vLo + frac * (vHi - vLo);
    }
  }
  return interp1D(rTableSOC, table[R_TABLE_T_N - 1], R_TABLE_SOC_N, socPct);
}

static void saveToNVS(){
  if(!nvsOk) return;
  int32_t stored = (int32_t)(soc * 100.0f);
  nvs_set_i32(nvsHandle, NVS_KEY_SOC, stored);
  nvs_set_u8(nvsHandle, NVS_KEY_VALID, 1);
  esp_err_t err = nvs_commit(nvsHandle);
  if(err != ESP_OK){
    ESP_LOGE(TAG, "NVS commit failed: %d", err);
  } else {
    socAtLastSave = soc;
  }
}

static void loadFromNVS(){
  esp_err_t err = nvs_flash_init();
  if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
    ESP_LOGW(TAG, "NVS partition needs erase, reinitializing");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandle);
  if(err != ESP_OK){
    ESP_LOGE(TAG, "Failed to open NVS namespace '%s': %d", NVS_NAMESPACE, err);
    nvsOk = false;
    return;
  }
  nvsOk = true;

  uint8_t valid = 0;
  int32_t stored = 0;
  if(nvs_get_u8(nvsHandle, NVS_KEY_VALID, &valid) == ESP_OK && valid &&
     nvs_get_i32(nvsHandle, NVS_KEY_SOC, &stored) == ESP_OK){
    soc = stored / 100.0f;
    ESP_LOGI(TAG, "Restored SOC from NVS: %.2f%%", soc);
  } else {
    ESP_LOGW(TAG, "No persisted SOC found, starting from default %.1f%%", soc);
  }
  socAtLastSave = soc;
}

void initSOC(){
  loadFromNVS();
  lastUpdateUs = esp_timer_get_time();
  lastSaveUs = lastUpdateUs;
}

void socPeriodic(){
  int64_t now = esp_timer_get_time();
  float dtHours = (now - lastUpdateUs) / 3600000000.0f; // us -> hours
  float dtSeconds = dtHours * 3600.0f;
  lastUpdateUs = now;

  // NOTE: assumes positive packCurrent = discharge (out of the pack). Flip the
  // sign here if the current sensor's polarity is wired the other way.
  float packCurrent = Cursense_VtoA(analogVoltages[ANALOG_CURSENSE]);
  float iCell = packCurrent / PACK_NUM_PARALLEL;
  float packTempC = (getMaxTemp() + getMinTemp()) / 2.0f; // proxy for cell temperature

  // --- coulomb counting (primary estimate), temperature-compensated capacity ---
  float capacityAh = interp1D(capT, capQ, CAP_TABLE_N, packTempC) * PACK_NUM_PARALLEL;
  soc -= (packCurrent * dtHours / capacityAh) * 100.0f;
  if(soc > 100.0f) soc = 100.0f;
  if(soc < 0.0f) soc = 0.0f;

  // --- ECM-based OCV correction (continuous, works under load too) ---
  // Estimate the RC-branch polarization voltage, then back out an OCV
  // estimate from the measured terminal voltage: Vt = OCV - I*R0 - V1
  float r0 = interpRTable(r0Table, soc, packTempC) / 1000.0f; // mOhm -> Ohm
  float r1 = interpRTable(r1Table, soc, packTempC) / 1000.0f;
  float alpha = expf(-dtSeconds / RC_TAU_S);
  v1 = v1 * alpha + iCell * r1 * (1.0f - alpha);

  float vCell = getPackVoltage() / PACK_NUM_SERIES;
  float ocvEst = vCell + iCell * r0 + v1;
  float socFromOcv = interp1D(ocvVoltage, ocvSOC, OCV_TABLE_N, ocvEst);

  // Nudge the coulomb count toward the model-based estimate. Heavier load
  // current means a noisier/less-trustworthy ECM estimate (pack imbalance,
  // measurement error amplified by I*R terms), so the correction weakens as
  // |iCell| grows - it's strongest exactly when the pack is lightly loaded.
  float confidence = 1.0f / (1.0f + fabsf(iCell));
  soc += SOC_OCV_CORRECTION_GAIN * confidence * dtSeconds * (socFromOcv - soc);

  // --- hard anchor when the state machine confirms true rest ---
  // IDLE means the AIR contactors are open - pack current is guaranteed zero,
  // not just momentarily low. After the RC branch has had time to relax
  // (several time constants), the terminal voltage IS the OCV; trust it fully.
  bool isIdle = (moboState.currentState == IDLE);
  if(isIdle && !wasIdle){
    idleStartUs = now;
  }
  wasIdle = isIdle;
  if(isIdle && (now - idleStartUs) > (int64_t)SOC_IDLE_REST_MS * 1000){
    soc = interp1D(ocvVoltage, ocvSOC, OCV_TABLE_N, vCell);
    v1 = 0.0f;
  }

  if(soc > 100.0f) soc = 100.0f;
  if(soc < 0.0f) soc = 0.0f;

  // --- persistence (rate-limited to avoid unnecessary flash wear) ---
  bool dueForSave = (now - lastSaveUs) > (int64_t)SOC_NVS_SAVE_INTERVAL_MS * 1000;
  bool changedEnough = fabsf(soc - socAtLastSave) >= SOC_NVS_SAVE_DELTA;
  if(dueForSave && changedEnough){
    saveToNVS();
    lastSaveUs = now;
  }
}

float getSOC(){
  return soc;
}

void setSOC(float newSOC){
  if(newSOC < 0.0f) newSOC = 0.0f;
  if(newSOC > 100.0f) newSOC = 100.0f;
  soc = newSOC;
  v1 = 0.0f;
  saveToNVS();
}
