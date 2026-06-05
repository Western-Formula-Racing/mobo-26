#pragma once
#include "io.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
#include "CAN.h"

static float analog_cursense = 0.0f;

void initNVS();
void save_soc_to_nvs(float soc);
void update_soc();
void reset_soc();