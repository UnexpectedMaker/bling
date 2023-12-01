//===========================================================================
//
// BLING FFT Example
//
// November 2023
//
// Written by Seon Rozenblum
// Unexpected Maker
//
//===========================================================================

#include <driver/i2s.h>
#include <soc/i2s_reg.h>
#include <arduinoFFT.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 18

// you shouldn't need to change these settings
#define SAMPLE_BUFFER_SIZE 128
#define SAMPLE_RATE 16000
// most microphones will probably default to left channel but you may need to tie the L/R pin low
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_42 // blc
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_40 // ws
#define I2S_MIC_SERIAL_DATA GPIO_NUM_41

int bands[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int bands_wide[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int BLOCK_SIZE = 512;

const double signalFrequency = 1000;
const double samplingFrequency = 10000;
const uint8_t amplitude = 150;

double vReal[BLOCK_SIZE];
double vImag[BLOCK_SIZE];

#define BUT_1 11
#define BUT_2 10
#define BUT_3 33
#define BUT_4 34

uint8_t visual_state = 0;
unsigned long nextButton = 0;


// don't mess around with this
i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = BLOCK_SIZE,
  .use_apll = true,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

// and don't mess around with this
i2s_pin_config_t i2s_mic_pins = {
  .bck_io_num = I2S_MIC_SERIAL_CLOCK,
  .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_MIC_SERIAL_DATA
};

const i2s_port_t I2S_PORT = I2S_NUM_0;



int32_t max_peak = -1e8;
int32_t min_peak = 1e8;

int wait_count = 100;

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(40, 8, LED_PIN,
                            NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0),
  matrix.Color(255, 128, 0),
  matrix.Color(255, 255, 0),
  matrix.Color(128, 255, 0),
  matrix.Color(0, 255, 0),
  matrix.Color(0, 255, 128),
  matrix.Color(0, 255, 255),
  matrix.Color(0, 128, 255),
  matrix.Color(0, 0, 255),
};

const uint16_t colors_dark[] = {
  matrix.Color(128, 0, 0),
  matrix.Color(128, 64, 0),
  matrix.Color(128, 128, 0),
  matrix.Color(64, 128, 0),
  matrix.Color(0, 128, 0),
  matrix.Color(0, 128, 64),
  matrix.Color(0, 128, 128),
  matrix.Color(0, 64, 128),
  matrix.Color(0, 0, 128),
};

void setup()
{
  // we need serial output for the plotter
  Serial.begin(115200);

  // Power
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);

  pinMode(39, OUTPUT);
  digitalWrite(39, LOW);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(5);

  pinMode(BUT_1, INPUT);
  pinMode(BUT_2, INPUT);
  pinMode(BUT_3, INPUT);
  pinMode(BUT_4, INPUT);

  // Start up the I2S peripheral
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);

  matrix.setRotation(0);
  matrix.setTextColor(matrix.Color(255, 84, 255));
  matrix.fillScreen(0);
  matrix.setCursor(6, 0);
  matrix.print("BLING");
  matrix.show();

  delay(1500);
  matrix.setRotation(2);

  swipeClear();
}

int32_t raw_samples[BLOCK_SIZE];
void loop()
{
  matrix.fillScreen(0);

  do_buttons();

  if ( visual_state == 5 || visual_state == 6)
  {
    do_fft_calcs_wide();
    do_display_wide();
  }
  else
  {
    do_fft_calcs();
    do_display();
  }

  delay(5);
}

void do_fft_calcs()
{
  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * BLOCK_SIZE, &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int32_t);

  for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
    vReal[i] = raw_samples[i] << 8;
    vImag[i] = 0.0; //Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows

    if (min_peak > raw_samples[i])
      min_peak = raw_samples[i];

    if (max_peak < raw_samples[i])
      max_peak = raw_samples[i];
  }

  FFT.Windowing(vReal, BLOCK_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, BLOCK_SIZE, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, BLOCK_SIZE);

  // Clear bands
  for (int i = 0; i < 8; i++)
    bands[i] = 0;

  // Calculate bands
  for (int i = 2; i < (BLOCK_SIZE / 2); i++) { // Don't use sample 0 and only first SAMPLES/2 are usable. Each array eleement represents a frequency and its value the amplitude.
    if (vReal[i] > 2000) { // Add a crude noise filter, 10 x amplitude or more
      if (i <= 3 )             bands[0] = max(bands[0], (int)(vReal[i] / amplitude)); // 125Hz
      if (i > 4   && i <= 6 )   bands[1] = max(bands[1], (int)(vReal[i] / amplitude)); // 250Hz
      if (i > 6   && i <= 8 )   bands[2] = max(bands[2], (int)(vReal[i] / amplitude)); // 500Hz
      if (i > 8   && i <= 15 )  bands[3] = max(bands[3], (int)(vReal[i] / amplitude)); // 1000Hz
      if (i > 15  && i <= 30 )  bands[4] = max(bands[4], (int)(vReal[i] / amplitude)); // 2000Hz
      if (i > 30  && i <= 53 )  bands[5] = max(bands[5], (int)(vReal[i] / amplitude)); // 4000Hz
      if (i > 53  && i <= 200 ) bands[6] = max(bands[6], (int)(vReal[i] / amplitude)); // 8000Hz
      if (i > 200           ) bands[7] = max(bands[7], (int)(vReal[i] / amplitude)); // 16000Hz
    }
  }
}

void do_fft_calcs_wide()
{
  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * BLOCK_SIZE, &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int32_t);

  for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
    vReal[i] = raw_samples[i] << 8;
    vImag[i] = 0.0; //Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows

    if (min_peak > raw_samples[i])
      min_peak = raw_samples[i];

    if (max_peak < raw_samples[i])
      max_peak = raw_samples[i];
  }

  FFT.Windowing(vReal, BLOCK_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, BLOCK_SIZE, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, BLOCK_SIZE);

  // Clear bands
  for (int i = 0; i < 16; i++)
    bands_wide[i] = 0;

  // Calculate bands
  for (int i = 2; i < (BLOCK_SIZE / 2); i++) { // Don't use sample 0 and only first SAMPLES/2 are usable. Each array eleement represents a frequency and its value the amplitude.
    if (vReal[i] > 2000) { // Add a crude noise filter, 10 x amplitude or more
      if (i <= 3 )             bands_wide[0] = max(bands_wide[0], (int)(vReal[i] / amplitude)); // 125Hz
      if (i > 4   && i <= 6 )   bands_wide[1] = max(bands_wide[1], (int)(vReal[i] / amplitude)); // 250Hz
      if (i > 6   && i <= 8 )   bands_wide[2] = max(bands_wide[2], (int)(vReal[i] / amplitude)); // 500Hz
      if (i > 8   && i <= 10 )  bands_wide[3] = max(bands_wide[3], (int)(vReal[i] / amplitude)); // 1000Hz
      if (i > 10  && i <= 20 )  bands_wide[4] = max(bands_wide[4], (int)(vReal[i] / amplitude)); // 2000Hz
      if (i > 20  && i <= 30 )  bands_wide[5] = max(bands_wide[5], (int)(vReal[i] / amplitude)); // 4000Hz
      if (i > 30  && i <= 42 )  bands_wide[6] = max(bands_wide[6], (int)(vReal[i] / amplitude)); // 8000Hz
      if (i > 42  && i < 57 )  bands_wide[7] = max(bands_wide[7], (int)(vReal[i] / amplitude)); // 125Hz
      if (i > 57 && i <= 75 ) bands_wide[8] = max(bands_wide[8], (int)(vReal[i] / amplitude)); // 250Hz
      if (i > 75 && i <= 94 ) bands_wide[9] = max(bands_wide[9], (int)(vReal[i] / amplitude)); // 500Hz
      if (i > 94 && i <= 115 ) bands_wide[10] = max(bands_wide[10], (int)(vReal[i] / amplitude)); // 1000Hz
      if (i > 115 && i <= 140 ) bands_wide[11] = max(bands_wide[11], (int)(vReal[i] / amplitude)); // 2000Hz
      if (i > 140 && i <= 170 ) bands_wide[12] = max(bands_wide[12], (int)(vReal[i] / amplitude)); // 4000Hz
      if (i > 170 && i <= 205 ) bands_wide[13] = max(bands_wide[13], (int)(vReal[i] / amplitude)); // 8000Hz
      if (i > 205 && i <= 246 ) bands_wide[14] = max(bands_wide[14], (int)(vReal[i] / amplitude)); // 16000Hz
      if (i > 246             ) bands_wide[15] = max(bands_wide[15], (int)(vReal[i] / amplitude)); // 16000Hz
    }
  }
}

void do_buttons()
{
  if (digitalRead(BUT_3))
  {
    if (millis() - nextButton > 500)
    {
      nextButton = millis();

      visual_state++;
      if (visual_state == 7)
        visual_state = 0;

      if (visual_state < 2 || visual_state > 4)
        matrix.setBrightness(10);
      else
        matrix.setBrightness(55);
    }
  }
}

void do_display()
{
  for (int b = 0; b < 8; b++)
  {
    int8_t bar = bands[b] / 26400000 / 2 - 1;

    if ( visual_state == 0)
    {
      matrix.fillRect(b * 5, 0, 4, bar, colors[b]);
    }
    else if ( visual_state == 1)
    {
      bar = max((int)bar, 0);
      matrix.fillRect(b * 5, bar, 4, 1, colors[b]);
    }
    else if ( visual_state == 2)
    {
      matrix.fillRect(b * 5, 0, 4, bar, matrix.Color(64 + (b * 16), 0, 0));
    }
    else if ( visual_state == 3)
    {
      matrix.fillRect(b * 5, 0, 4, bar, matrix.Color(0, 64 + (b * 16), 0));
    }
    else if ( visual_state == 4)
    {
      matrix.fillRect(b * 5, 0, 4, bar, matrix.Color(0, 0, 64 + (b * 16)));
    }
  }

  matrix.show();
}

void do_display_wide()
{
  for (int b = 0; b < 16; b++)
  {

    int8_t bar = bands_wide[b] / 26400000 / 2 - 1;

    if ( visual_state == 5)
    {
      matrix.fillRect(4 + b * 2, 0, 1, bar, colorWheel(255 - (b * 12)));
    }
    else if ( visual_state == 6)
    {
      bar = max((int)bar, 0);
      matrix.fillRect(4 + b * 2, bar, 2, 1, colorWheel(255 - (b * 12)));
    }
  }

  matrix.show();
}

void swipeClear()
{
  for (int x = 0; x < 40; x++)
  {
    matrix.fillRect(x, 0, 1, 8, matrix.Color(255, 255, 255));
    matrix.show();
    delay(1);
    matrix.fillRect(x, 0, 1, 8, 0);
    matrix.show();
  }
}

uint32_t colorWheel(uint8_t pos)
{
  if (pos < 85) {
    return matrix.Color(255 - pos * 3, pos * 3, 0);
  } else if (pos < 170) {
    pos -= 85;
    return matrix.Color(0, 255 - pos * 3, pos * 3);
  } else {
    pos -= 170;
    return matrix.Color(pos * 3, 0, 255 - pos * 3);
  }
}
