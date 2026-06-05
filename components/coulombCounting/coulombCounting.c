#include "coulombCounting.h"
#include <math.h>

static const char* TAG = "soc"; 

#define RESET_SOC 100.0f
#define PACK_CAPACITY_AH 16.0f //theoretical capacity is 18Ah (added 2 Ah buffer)
#define LOOP_TIME 0.01f
#define SAVE_THRESHOLD 1.0f

CoulombState_t coulomb_counting = {
    .current_soc = 100.0f,
    .last_saved_soc = 100.0f,
    .last_time_us = 100.0f,
    .update_flag = true,
};

static nvs_handle_t my_nvs_handle;

void initNVS(){

    esp_err_t err = nvs_open("soc_storage", NVS_READWRITE, &my_nvs_handle);
    if (err == ESP_OK) {
        uint32_t stored_soc_int = 10000; // buffer for pointer
        err = nvs_get_u32(my_nvs_handle, "saved_soc", &stored_soc_int);

        if (err == ESP_OK) {
            coulomb_counting.current_soc = (float)stored_soc_int / 100.0f;
            coulomb_counting.last_saved_soc = coulomb_counting.current_soc;
            ESP_LOGI(TAG, "Restored SoC from memory: %.2f%%", coulomb_counting.current_soc);
        }
    }
}

void save_soc_to_nvs(float soc){

    uint32_t soc_int = (uint32_t)(soc * 100.0f);
    
    nvs_set_u32(my_nvs_handle, "saved_soc", soc_int);
    nvs_commit(my_nvs_handle); // Actually writes to flash
    ESP_LOGI(TAG, "SoC saved to flash: %.2f%%", soc);
}


void update_soc() {
    int64_t current_time_us = esp_timer_get_time();

    //handle first iteration edge case
    if(coulomb_counting.last_time_us==0){
        coulomb_counting.last_time_us=current_time_us;
        return; //skips integration and unnecessary overhead
    }

    float dt = (float)(current_time_us-coulomb_counting.last_time_us)/1000000.0f; //convert to seconds
    coulomb_counting.last_time_us = current_time_us; //update 'last time'

    // float current_analog = Cursense_VtoA(analogVoltages[ANALOG_CURSENSE]);
    float current_analog = getInverterCurrent(); //get current -> assume equal current for duration

    float amp_hour = current_analog*dt/3600; //converting current to Ah
    coulomb_counting.current_soc = (coulomb_counting.current_soc) - (amp_hour)/(PACK_CAPACITY_AH)*100; //last saved SoC% - current Increment SoC%

    if (fabs(coulomb_counting.current_soc - coulomb_counting.last_saved_soc) >= SAVE_THRESHOLD) { //updating 'last saved SoC
        save_soc_to_nvs(coulomb_counting.current_soc);
        coulomb_counting.last_saved_soc = coulomb_counting.current_soc;

        coulomb_counting.update_flag = true; 
    }
}
void reset_soc() {
    coulomb_counting.current_soc = RESET_SOC;
    coulomb_counting.last_saved_soc = RESET_SOC; 

    save_soc_to_nvs(RESET_SOC);
    ESP_LOGW(TAG, "BENCH COMMAND: SoC forcefully reset to %.2f%%", RESET_SOC);
}