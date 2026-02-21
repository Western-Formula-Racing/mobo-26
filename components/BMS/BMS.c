#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "BMS.h"

const char* TAG = "BMS";

Module modules[5];

void setModuleVoltage(int module, int cell, float newVoltage){
  if(module < 6 && cell <21){
    modules[module].voltages[cell] = newVoltage;
  } 
  else{ESP_LOGE(TAG, "Cell index out of range!");}
}

void setModuleTemp(int module, int thermistor, float newTemp){
  if(module < 6 && thermistor <19){
    modules[module].temps[thermistor] = newTemp;
  } 
  else{ESP_LOGE(TAG, "Temp index out of range!");}
}

float getPackVoltage(){
  float total = 0;
  for(int i=0; i<5; i++){
    for(int j=0; j<20; j++){
      total += modules[i].voltages[j];
    }
  }
  return total;
}

float getMaxTemp(){
  float max = 0;
  for(int i=0;i<5;i++){
    for(int j=0;j<18;j++){
      if(max<modules[i].temps[j]){
        max = modules[i].temps[j];
      }
    }
  }
  return max;
}

float getMaxTempIndex(uint8_t* module, uint8_t* index){
  float max = 0;
  for(int i=0;i<5;i++){
    for(int j=0;j<18;j++){
      if(max<modules[i].temps[j]){
        max = modules[i].temps[j];
        *module = i;
        *index = j;
      }
    }
  }
  return max;
}

float getMinTemp(){
  float min = 100;
  for(int i=0;i<5;i++){
    for(int j=0;j<18;j++){
      if(min>modules[i].temps[j]) min = modules[i].temps[j];
    }
  }
  return min;
}

float getMaxVoltage(){
  float max = 0;
  for(int i=0;i<5;i++){
    for(int j=0;j<20;j++){
      if(max<modules[i].voltages[j]){ 
        max = modules[i].voltages[j];
      }
    }
  }
  return max;
}

float getMaxVoltageIndex(uint8_t* module, uint8_t* index){
  float max = 0;
  for(int i=0;i<5;i++){
    for(int j=0;j<20;j++){
      if(max<modules[i].voltages[j]){ 
        max = modules[i].voltages[j];
        *module = i;
        *index = j;
      }
    }
  }
  return max;
}

float getMinVoltage(){
  float min = 100;
  for(int i=0;i<5;i++){
    for(int j=0;j<20;j++){
      if(min>modules[i].voltages[j]) min = modules[i].voltages[j];
    }
  }
  return min;
}

float getMinVoltageIndex(uint8_t* module, uint8_t* index){
  float min = 100;
  for(int i=0;i<5;i++){
    for(int j=0;j<20;j++){
      if(min>modules[i].voltages[j]){
        min = modules[i].voltages[j];
        *module = i;
        *index = j;
      }
    }
  }
  return min;
}

int getMaxModuleTimeout(uint8_t *module){
  int max = 0;
  for(int i = 0; i < 5; i++){
    if(modules[i].timeout > max){
      max = modules[i].timeout;
      *module = i;
    }
  }
  return max;
}

//serial debugging
void printModules(){
  printf("===Module Info===\n");
  printf("   Module 1 --- Cells        Module 2 --- Cells        Module 3 --- Cells        Module 4 --- Cells        Module 5 --- Cells     \n");
  for(int k=0;k<5;k++){
    for(int j=0; j<5;j++){
      for(int i =0;i<4;i++){
        printf("|%.3f",modules[j].voltages[i+k*4]);
      }
      printf("| ");
    }
    printf("\n");
  }
  printf("   Module 1 --- Temp        Module 2 --- Temp        Module 3 --- Temp        Module 4 --- Temp        Module 5 --- Temp     \n");
  for(int k=0;k<5;k++){
    for(int j=0; j<5;j++){
      for(int i =0;i<4;i++){
        if(i+k*4<18){
          printf("|%.3f",modules[j].temps[i+k*4]);
        } else{printf("|x.xxx");}
      }
      printf("| ");
    }
    printf("\n");
  }
}