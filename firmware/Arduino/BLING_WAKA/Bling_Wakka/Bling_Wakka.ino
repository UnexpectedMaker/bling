#include "Arduino.h"
#include "Wire.h"
#include "bitmaps.h"
#include "Audio.h" // Library Manager - ESP32_AudioI2S
#include "SD.h"
#include "SPI.h"
#include "FS.h"

#include <Adafruit_GFX.h> // Library Manager
#include <Adafruit_NeoMatrix.h> // Library Manager
#include <Adafruit_NeoPixel.h> // Library Manager

#include "display.h"

#define LED_PIN   18
#define SD_CS     21
#define I2S_DOUT  3
#define I2S_BCLK  2
#define I2S_LRC 1
#define I2S_SD  4

Audio audio;

unsigned long nextDisplayUpdate = 0;
bool ready = false;
bool has_sd = true;

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(40, 8, LED_PIN,
                            NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB            + NEO_KHZ800);


void setup()
{
  // we need serial output for the plotter
  Serial.begin(115200);

  delay(1000);

  // Matrix Power
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);

  Wire.begin();
  
  if (!SD.begin(SD_CS))
  {
    Serial.println("Card Mount Failed");
    has_sd = false;
  }

  // I2S Amp Enable
  pinMode(I2S_SD, OUTPUT);
  digitalWrite(I2S_SD, HIGH);
  
  // Init I2S Amplifier
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT, -1);
  audio.setVolume(5); // 0...21

  delay(200);

  // Initialiase the RGB Matrix
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(5);

  // Statup WiFi
  matrix.setRotation(0);
  matrix.fillScreen(0);

  AssembleFrames();

  DisplayText(matrix, "BLING", matrix.Color(255, 84, 255), 2000);
  delay(2000);
  audio.connecttoFS(SD, "/pacman_chomp.wav");
}

void loop()
{
  // Scrolling text overrides any other display
  if (ScrollText(matrix))
    return;

  if ((long)(millis() - nextDisplayUpdate > 0))
  {
    nextDisplayUpdate = millis();
    isDisplaying = false;
  }
  else
    return;


  //Process the audio loop
  audio.loop();

  Animate(matrix);
}


const uint16_t colors[9] = {
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

const uint16_t colors_colon[9] = {
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
  matrix.Color(128, 128, 128),
};

// optional
void audio_info(const char *info){
   // Serial.print("info        "); Serial.println(info);
}
