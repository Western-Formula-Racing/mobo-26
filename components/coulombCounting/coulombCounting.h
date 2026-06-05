#pragma once
#include "io.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
#include "CAN.h"

typedef struct {
    float current_soc;
    float last_saved_soc;
    int64_t last_time_us;
    bool update_flag;
}CoulombState_t;

extern CoulombState_t coulomb_counting;   

void initNVS();
void save_soc_to_nvs(float soc);
void update_soc();
void reset_soc();