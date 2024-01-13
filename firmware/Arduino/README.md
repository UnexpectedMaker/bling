# BLING! Arduino Examples

Various Arduino IDE project examples for Unexpected Maker BLING!

You can find out more about BLING! at https://unexpectedmaker.com/bling

## FFT Example
The FFT example uses the onboard ICS-43434 I2S MEMS Microphone to capture sound, and then parse it through a Fast Fourier Transform (FFT) to separate the audio into bands, and then display them in a rainbow color on the RGB LED display.

You can press the [C] user button to switch between different visualisations.

This project is the firmware that ships on new BLING board when they are sent out. 


## WAKKA Example
Displays some simple PAC-MAN animations for fun.
Don't forget to copy the files in the `sfx_sdcard` folder to a micro SD card, and then insert it into your BLING. 


## Talking Clock
The Talking Clock example is a digital clock, that is also able to speak the time every hour, or whatever the current time is when you press the [C] user button.

This example requires WiFi and uses the NTP server and some code from Larry Bank (BitBank Software Inc) to automatically grab your time and GMT offset + DST to set your time on the RV-3028-C7 RTC.

You need to add your WiFi SSID and Password to the secret.h file in the project.

There's a bit of functionality and options built into the clock code. Settings that can be changed are:

- Set time display to 24 or 12 hour
- Set automatic brightness dimming for nighttime (between 9pm and 7am)
- Set RGB LED brightness
- Set time display to show to hide seconds
- Set the clock to automatically speak the time on the hour, every hour. 

All settings, including current volume and clock colors are stored in NVS on the FLASH and persist between flashing firmware updates.

The user buttons perform the following tasks:

#### Button [A]
In clock mode, click this button to adjust the volume lower
In settings mode, click this button to set the current option value lower

#### Button [B]
In clock mode, click this button to adjust the volume higher
In settings mode, click this button to set the current option value higher

#### Button [C]
In clock mode, click this button to speak the current time
In clock mode, long press this button to enter settings mode
In settings mode, click this button to cycle to the next setting
In settings mode, long press this button to save settings and exit settings mode

#### Button [D]
In clock mode, click this button to cycle between different clock colors
In clock mode, double click this button to RESET your BLING
In clock mode, long press this button to out your bling into DOWNLOAD MODE
In settings mode, click this button to exit settings without saving

#### TODO:
Improve the clock & colon color combinations
