#include "driver/spi_master.h"
#include "hw_define.h"
#include "io.h"
#include "esp_log.h"

// TLA2518 specific opcodes and registers
#define TLA_CMD_WRITE 0x08
#define TLA_CHANNEL_SEL 0x11
#define TLA_PIN_CFG 0x01

uint8_t inputBuffers[INPUTS_COUNT][INPUT_BUFFER_SIZE] = {};
uint8_t inputStates[INPUTS_COUNT] = {};
float   analogVoltages[ANALOG_COUNT] = {};
uint8_t outputStates[OUTPUTS_COUNT] = {};
spi_device_handle_t adcHandle;

const uint8_t channels[ANALOG_COUNT] = {0, 1, 3};

static const char* TAG = "io"; 

// ADC filtering config
#define ADC_SAMPLE_COUNT 16 // number of samples for moving-average filter
static uint16_t adcSampleBuffers[ANALOG_COUNT][ADC_SAMPLE_COUNT] = {{0}};
static uint8_t adcSampleIndex[ANALOG_COUNT] = {0};


void tla2518_write_register(uint8_t address, uint8_t value) {
  uint8_t tx_buffer[3] = {TLA_CMD_WRITE, address, value};
  spi_transaction_t t = {};
  t.length = 24; // 8 * 3 bytes
  t.tx_buffer = tx_buffer;
  spi_device_transmit(adcHandle, &t);
}

uint16_t tla2518_read_channel(uint8_t channel) {
  uint8_t tx_buffer[3] = {TLA_CMD_WRITE, TLA_CHANNEL_SEL, channel};
  uint8_t rx_buffer[2] = {0, 0};
  
  spi_transaction_t t = {};
  t.length = 24;      // Transmit 24 bits total
  t.tx_buffer = tx_buffer;
  t.rxlength = 16;    // But only capture the first 16 bits of response
  t.rx_buffer = rx_buffer;  
  
  // Brute-force flush the N+1 pipeline to guarantee current channel data
  spi_device_transmit(adcHandle, &t);  
  spi_device_transmit(adcHandle, &t);
  spi_device_transmit(adcHandle, &t);
  
  // Extract the 12-bit value (left-justified in the 16-bit response)
  uint16_t val = ((uint16_t)rx_buffer[0] << 4) | (rx_buffer[1] >> 4);
  return val;
}

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
    .clock_speed_hz = 10*1000*1000,       // 10 MHz
    .mode = 0,                            // SPI mode 0 for TLA2518
    .spics_io_num = ADC_CS,               // CS pin
    .queue_size = 5,
  };
  ESP_LOGI(TAG, "Adding ADC device");
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &adcCfg, &adcHandle));
  
  // Force all channels to be analog inputs as per the library config
  tla2518_write_register(TLA_PIN_CFG, 4);
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
  // printf(">AIRN0,AirBuffer:%d \n>AIRN1,AirBuffer:%d \n>AIRN2,AirBuffer:%d \n>AIRN3,AirBuffer:%d \n>AIRN4,AirBuffer:%d\n",inputBuffers[AIRN_RELAY][0],inputBuffers[AIRN_RELAY][1],inputBuffers[AIRN_RELAY][2],inputBuffers[AIRN_RELAY][3],inputBuffers[AIRN_RELAY][4]);
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
    }
  }
}

// Moving analog input code into another function
float ADC_read_filter(enum analog_e channel) {

  uint8_t ADC_channel = channels[channel];

  //read channel
  uint16_t rawValue1 = tla2518_read_channel(ADC_channel); 


  adcSampleBuffers[channel][adcSampleIndex[channel]] = rawValue1;
  adcSampleIndex[channel] = (adcSampleIndex[channel] + 1) % ADC_SAMPLE_COUNT;

  uint32_t sum = 0;
  for(int i = 0; i < ADC_SAMPLE_COUNT; i++){
    sum += adcSampleBuffers[ADC_channel][i];
  }

  uint16_t avgRaw = (uint16_t)(sum / ADC_SAMPLE_COUNT);

  return ((float)avgRaw / 4095.0f) * 5.0f;
}



// update analog inputs
void analogInputs(){
  for(int i =0; i<ANALOG_COUNT;i++){
    analogVoltages[i] = ADC_read_filter((enum analog_e)i);
  }
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