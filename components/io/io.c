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

// ADC filtering config
#define ADC_SAMPLE_COUNT 16 // number of samples for moving-average filter
static uint16_t adcSampleBuffers[ANALOG_COUNT][ADC_SAMPLE_COUNT] = {{0}};
static uint8_t adcSampleIndex[ANALOG_COUNT] = {0};

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
  
  // Configure GPIO_CHARGE_EN (GPIO 42) as input with no internal pull resistors (externally pulled up)
  io_cfg.pin_bit_mask = (1ULL << GPIO_CHARGE_EN);
  io_cfg.pull_down_en = false;
  io_cfg.pull_up_en = false;
  gpio_config(&io_cfg);
  
  io_cfg.mode = GPIO_MODE_OUTPUT;               // set mode to output
  io_cfg.pin_bit_mask = GPIO_OUTPUT_PIN_SELECT; // output pin mask
  
  gpio_config(&io_cfg); // config outputs
  //SPI init
  ESP_LOGI(TAG, "Initializing SPI...");
  spi_bus_config_t buscfg={
    .miso_io_num=GPIO_MISO,
    .mosi_io_num=GPIO_MOSI,
    .sclk_io_num=GPIO_SCK,
    .quadwp_io_num=-1,
    .quadhd_io_num=-1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
  ESP_LOGI(TAG, "SPI initialized");
  spi_device_interface_config_t adcCfg = {
    .clock_speed_hz = 1*1000*1000,        //Clock out at 1 MHz
    .mode = 2,                            //SPI mode 1
    .spics_io_num = ADC_CS,               //CS pin
    .queue_size = 5,
  };
  ESP_LOGI(TAG, "Adding ADC device");
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &adcCfg, &adcHandle));
  uint8_t txWrite[6] = {0b10001110, 0b01000000, 0, 0, 0, 0}; // Write 0x8440 (bit 6 = internal ref)
  spi_transaction_t t_write = {
    .length = 48,
    .tx_buffer = txWrite,
    .rx_buffer = NULL
  };
  spi_device_transmit(adcHandle, &t_write);
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
  printf(">AIRN0,AirBuffer:%d \n>AIRN1,AirBuffer:%d \n>AIRN2,AirBuffer:%d \n>AIRN3,AirBuffer:%d \n>AIRN4,AirBuffer:%d\n",inputBuffers[AIRN_RELAY][0],inputBuffers[AIRN_RELAY][1],inputBuffers[AIRN_RELAY][2],inputBuffers[AIRN_RELAY][3],inputBuffers[AIRN_RELAY][4]);
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
  uint8_t txDummy[6] = {0, 0, 0, 0, 0, 0}; // Dummy data
  uint8_t rxData[6] = {0};
  spi_transaction_t t_receive = {
    .length = 48,           // Send 48 SCLK (6 bytes)
    .tx_buffer = txDummy,
    .rx_buffer = rxData     // Response comes in first 2 bytes
  };
  spi_device_transmit(adcHandle, &t_receive);

  // Parse 14-bit raw ADC values (two channels per transfer)
  uint16_t rawValue1 = ((rxData[0] & 0xFF) << 8) | rxData[1]; // 14-bit value from first 2 bytes
  uint16_t rawValue2 = ((rxData[2] & 0xFF) << 8) | rxData[3]; // 14-bit value from next 2 bytes

  // Push new samples into circular buffers and compute moving average
  const float adcMax = 16383.0f; // 14-bit max (2^14 - 1)
  uint32_t sum0 = 0;
  uint32_t sum1 = 0;

  // channel 0
  adcSampleBuffers[0][adcSampleIndex[0]] = rawValue1;
  adcSampleIndex[0] = (adcSampleIndex[0] + 1) % ADC_SAMPLE_COUNT;
  for(int i=0;i<ADC_SAMPLE_COUNT;i++) sum0 += adcSampleBuffers[0][i];
  uint16_t avgRaw0 = (uint16_t)(sum0 / ADC_SAMPLE_COUNT);

  // channel 1
  adcSampleBuffers[1][adcSampleIndex[1]] = rawValue2;
  adcSampleIndex[1] = (adcSampleIndex[1] + 1) % ADC_SAMPLE_COUNT;
  for(int i=0;i<ADC_SAMPLE_COUNT;i++) sum1 += adcSampleBuffers[1][i];
  uint16_t avgRaw1 = (uint16_t)(sum1 / ADC_SAMPLE_COUNT);

  // Convert averaged ADC counts to voltage assuming 5.0V reference
  analogVoltages[ANALOG_CURSENSE] = ((float)avgRaw0 / adcMax) * 5.0f;
  analogVoltages[ANALOG_VSENSE] = ((float)avgRaw1 / adcMax) * 5.0f;
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
  analogInputs();
  digitalOutputs();
}
