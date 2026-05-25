#include "tempsense.h"

void scanDevices(){
    size_t addr_count  = 5;
    

    ESP_ERROR_CHECK(ds18x20_scan_devices(GPIO_ONE_WIRE, tempStatus.address, addr_count, &tempStatus.found));
    // Worthwhile rerunning fqunction if all 5 devices are not found? 
};

void writeScratch() {

    int i;
    uint8_t buffer[3];
    buffer[0] = 0b0101111; //11-bit resolution
    buffer [1] = 0b1000001; //65 deg
    buffer [2] = 0b00001010; //10 deg

    //configuring all 5 devices
    for(i=0;i<5;i++){
        ESP_ERROR_CHECK(ds18x20_write_scratchpad(GPIO_ONE_WIRE, tempStatus.address[i], buffer));
    }
};

void measureTemp() {
    
    bool wait = false;

    ESP_ERROR_CHECK(ds18x20_measure(GPIO_ONE_WIRE, DS18X20_ANY, wait)); //how do I 1) run this function synchronously 2) know when the function is complete
    tempStatus.tempFlag = true;

};




void readTemp() {
    
    int i = 0;
    for(i=0;i<5;i++){

        esp_err_t err = ds18x20_read_temperature(GPIO_ONE_WIRE, tempStatus.address[i], &tempStatus.temp[i]);

        if (err!=ESP_OK){
            tempStatus.temp[i] = -999;
            tempStatus.tempFlag = false;

        }
        

    }
    tempStatus.tempFlag = false;

};

