#include "driver/spi_master.h"
#include "hw_define.h"
#include "io.h"
#include "esp_log.h"

// ADS7853 Register Commands (bits 15:12)
#define ADS7X53_WRITE_CFR  0x8  // Write config register
#define ADS7X53_READ_CFR   0x3  // Read config register

uint8_t inputBuffers[INPUTS_COUNT][INPUT_BUFFER_SIZE] = {};
uint8_t inputStates[INPUTS_COUNT] = {};
float   analogVoltages[ANALOG_COUNT] = {};
uint8_t outputStates[OUTPUTS_COUNT] = {};
spi_device_handle_t adcHandle;

static const char* TAG = "io"; 

// initilaize digital input/output pins 
void initIO(){
  //GPIO
  ESP_LOGI(TAG, "Initializing GPIO...");
  gpio_config_t io_cfg = {};
  
  io_cfg.intr_type = GPIO_INTR_DISABLE;        // disable interrupt
  io_cfg.mode = GPIO_MODE_INPUT;               // set mode to input
  io_cfg.pin_bit_mask = GPIO_INPUT_PIN_SELECT; // input pin mask
  io_cfg.pull_down_en = true;                  // enable pulldown mode
  
  gpio_config(&io_cfg); // config inputs
  
  io_cfg.mode = GPIO_MODE_OUTPUT;               // set mode to output
  io_cfg.pin_bit_mask = GPIO_OUTPUT_PIN_SELECT; // output pin mask
  
  gpio_config(&io_cfg); // config outputs
  //SPI init
  //ESP_LOGI(TAG, "Initializing SPI...");
  //spi_bus_config_t buscfg={
  //  .miso_io_num=GPIO_MISO,
  //  .mosi_io_num=GPIO_MOSI,
  //  .sclk_io_num=GPIO_SCK,
  //  .quadwp_io_num=-1,
  //  .quadhd_io_num=-1,
  //};
  //ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
  //ESP_LOGI(TAG, "SPI initialized");
  //spi_device_interface_config_t adcCfg = {
  //  .clock_speed_hz = 1*1000*1000,        //Clock out at 1 MHz
  //  .mode = 2,                            //SPI mode 1
  //  .spics_io_num = ADC_CS,               //CS pin
  //  .queue_size = 5,
  //};
  //ESP_LOGI(TAG, "Adding ADC device");
  //ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &adcCfg, &adcHandle));
}

// update digital inputs
void digitalInputs(){
  int doUpdate = 0;
  // shift relay buffers right
  for(int i = 0; i < INPUTS_COUNT; i++){
    for(int j = INPUT_BUFFER_SIZE-1; j > 0; j--){
      inputBuffers[i][j] = inputBuffers[i][j-1];
    }
  }
  // update first element
  inputBuffers[IMD_RELAY][0]   = gpio_get_level(GPIO_IMD);
  inputBuffers[BSPD_RELAY][0]  = gpio_get_level(GPIO_BSPD);
  inputBuffers[LATCH_RELAY][0] = gpio_get_level(GPIO_LATCH);
  inputBuffers[AIRN_RELAY][0]  = gpio_get_level(GPIO_AIRN);
  //printf(">AIRN0:%d \n>AIRN1:%d \n>AIRN2:%d \n>AIRN3:%d \n>AIRN4:%d",inputBuffers[AIRN_RELAY][0],inputBuffers[AIRN_RELAY][1],inputBuffers[AIRN_RELAY][2],inputBuffers[AIRN_RELAY][3],inputBuffers[AIRN_RELAY][4]);
  inputBuffers[AIRP_RELAY][0]  = gpio_get_level(GPIO_AIRP);
  inputBuffers[CHARGE_EN][0]  = gpio_get_level(GPIO_CHARGE_EN);

  // update relay status based on buffer content
  for(int i = 0; i < INPUTS_COUNT; i++){
    doUpdate = 1;
    // check if all elements are equal
    for(int j = 0; j < INPUT_BUFFER_SIZE-1; j++){
      if(inputBuffers[i][j] != inputBuffers[i][j+1]){
        doUpdate = 0;
      }
    }
    // if all elements are equal, update 
    if(doUpdate == 1){
      inputStates[i] = inputBuffers[i][0];
      //ESP_LOGI(TAG,"input %d updated to %d, array values: [%d, %d, %d, %d, %d]",i, inputStates[i], inputBuffers[i][0], inputBuffers[i][1], inputBuffers[i][2], inputBuffers[i][3], inputBuffers[i][4]);
    }
  }
}

// update analog inputs
void analogInputs(){
 // FRAME F: Write config register
uint8_t txWrite[6] = {0x84, 0x40, 0, 0, 0, 0}; // Write 0x8440 (bit 6 = internal ref)
spi_transaction_t t_write = {
  .length = 48,
  .tx_buffer = txWrite,
  .rx_buffer = NULL
};
spi_device_transmit(adcHandle, &t_write);
vTaskDelay(pdMS_TO_TICKS(1)); // Wait 1ms between frames

// FRAME F+1: Send READ command (48 SCLK minimum)
uint8_t txReadCmd[6] = {0x30, 0x00, 0, 0, 0, 0}; // Read command: 0x3000
spi_transaction_t t_readcmd = {
  .length = 48,
  .tx_buffer = txReadCmd,
  .rx_buffer = NULL  // Ignore MISO during this frame
};
spi_device_transmit(adcHandle, &t_readcmd);
vTaskDelay(pdMS_TO_TICKS(1)); // Small delay between frames
//
//// FRAME F+2: Send dummy data and receive response (first 16 SCLK = 2 bytes)
//uint8_t txDummy[6] = {0, 0, 0, 0, 0, 0}; // Dummy data
//uint8_t rxData[6] = {0};
//spi_transaction_t t_receive = {
//  .length = 48,           // Send 48 SCLK (6 bytes)
//  .tx_buffer = txDummy,
//  .rx_buffer = rxData     // Response comes in first 2 bytes
//};
//spi_device_transmit(adcHandle, &t_receive);
//
//// Response is in FIRST 2 bytes (first 16 SCLK falling edges)
//uint16_t config = ((uint16_t)rxData[0] << 8) | rxData[1];
//ESP_LOGI(TAG, "Config register: 0x%03X", config & 0x0FFF);
//ESP_LOGI(TAG, "Raw received: %02X %02X %02X %02X %02X %02X",
//         rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5]);
}

// update digital outputs
void digitalOutputs(){
  gpio_set_level(GPIO_BMS_OK,outputStates[OUTPUTS_BMS_OK]);
  gpio_set_level(GPIO_PRECH_OK,outputStates[OUTPUTS_PRECH_OK]);
  gpio_set_level(GPIO_RED_LED,outputStates[OUTPUTS_RED_LED]);
  gpio_set_level(GPIO_GREEN_LED,outputStates[OUTPUTS_GREEN_LED]);
}
// IO Periodic function
void ioPeriodic(){
  digitalInputs();
  //analogInputs();
  digitalOutputs();
}
