#include "tempsense.h"
#include "esp_log.h"

static const char *TAG = "tempsense";
static int tempCount = 0;
TempStatus_t tempStatus = {0};

void scanDevices(){
    // Scan up to the maximum capacity of our array
    size_t addr_count = MAX_SENSOR_SLOTS;
    
    esp_err_t err = ds18x20_scan_devices(GPIO_ONE_WIRE, tempStatus.address, addr_count, &tempStatus.found);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "1-Wire Scan Failed completely");
        tempStatus.found = 0;
        return;
    }

    ESP_LOGI(TAG, "Found %d temperature sensor(s).", tempStatus.found);
}

void writeScratch() {
    uint8_t buffer[3];
    buffer[2] = 65;
    buffer[1] = 10;
    buffer[0] = 0b01111111; //12-bit resolution

    //Writing to the found sensors
    for (int i = 0; i < tempStatus.found; i++) {
        ESP_ERROR_CHECK(ds18x20_write_scratchpad(GPIO_ONE_WIRE, tempStatus.address[i], buffer));
    }
}

void measureTemp() {

    esp_err_t err = ds18x20_measure(GPIO_ONE_WIRE, DS18X20_ANY, false); //Broadcasting measure temp function
    if (err == ESP_OK) {
        tempStatus.tempFlag = true; 
    }
}

void readTemp() {

    for (int i = 0; i < MAX_SENSOR_SLOTS; i++) {
        if (i < tempStatus.found) {
            esp_err_t err = ds18x20_read_temperature(GPIO_ONE_WIRE, tempStatus.address[i], &tempStatus.temp[i]);
            if (err != ESP_OK) {
                tempStatus.temp[i] = -999.0; // Sensor read fault
            }
        } else {
            tempStatus.temp[i] = -888.0; // Filling emmpty slots
        }
    }

}
void tempSensePeriodic() {
    if (tempStatus.found == 0) {
    //'rescan' the sensors
        if (tempCount >= 500) { // Try to find sensors every 5 seconds (10ms*500 loops)      
            scanDevices(); 
        if (tempStatus.found > 0) {
            writeScratch(); //write to the scratch now, that sensors have been located
        }
        tempCount = 0;
    }
    } else {
    if (tempStatus.tempFlag == false) {
        if (tempCount >= 500) { //5 second bus downtime, to restore IC voltage
            measureTemp(); // Start conversion asynchronously
            tempCount = 0;
        }
    } else {
            if (tempCount >= 50) { // Wait 500ms for hardware conversion
                onewire_depower(GPIO_ONE_WIRE);
                readTemp(); 

            tempStatus.printReady = 1;
            tempStatus.tempFlag = false; //back to measure mode
            tempCount = 0; 

            }
        }
    }

    tempCount++;
}