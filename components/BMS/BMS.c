#include "freertos/FreeRTOS.h"
#include "BMS.h"

const char* TAG = "BMS";

Module modules[5];

void setModuleVoltage(int module, int cell, double newVoltage){
  if(module < 6 && cell <21){
    modules[module].voltages[cell] = newVoltage;
  } 
  else{ESP_LOGE(TAG, "Cell index out of range!");}
}

void setModuleTemp(int module, int thermistor, double newTemp){
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

double getMaxTemp(){
  double max = 0;
  for(int i=0;i<5;i++){
    for(int j=0;j<18;j++){
      if(max<modules[i].temps[j]){
        max = modules[i].temps[j];
      }
    }
  }
  return max;
}

double getMaxVoltage(){
  double max = 0;
  for(int i=0;i<5;i++){
    for(int j=0;j<20;j++){
      if(max<modules[i].voltages[j]){ 
        max = modules[i].voltages[j];
      }
    }
  }
  return max;
}

double getMinVoltage(){
  double min = 100;
  for(int i=0;i<5;i++){
    for(int j=0;j<20;j++){
      if(min>modules[i].voltages[j]) min = modules[i].voltages[j];
    }
  }
  return min;
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