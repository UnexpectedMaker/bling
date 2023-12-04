#pragma once

#include <Preferences.h>
Preferences prefs;

bool settings_clock_24hour = true;
bool settings_dim_night = true;
bool settings_speak_time = true;
bool settings_show_secs = false;
uint8_t settings_volume = 0;
uint8_t settings_brightness = 0;
uint8_t settings_clock_col = 0;

void Save_Settings()
{
  prefs.begin("settings");
  prefs.putUInt("clock_24hour", settings_clock_24hour ? 1 : 0);
  prefs.putUInt("dim_night", settings_dim_night ? 1 : 0);
  prefs.putUInt("speak_time", settings_speak_time ? 1 : 0);
  prefs.putUInt("show_secs", settings_show_secs ? 1 : 0);
  prefs.putUInt("volume", settings_volume);
  prefs.putUInt("brightness", settings_brightness);
  prefs.putUInt("clock_col", settings_clock_col);
  prefs.end();
}

void Load_Settings()
{
  prefs.begin("settings");
  settings_clock_24hour = (prefs.getUInt("clock_24hour", 1) == 1);
  settings_dim_night = (prefs.getUInt("dim_night", 1) == 1);
  settings_speak_time = (prefs.getUInt("speak_time", 1) == 1);
  settings_show_secs = (prefs.getUInt("show_secs", 1) == 1);
  settings_volume = (uint8_t)prefs.getUInt("volume", 5);
  settings_brightness = (uint8_t)prefs.getUInt("brightness", 20);
  settings_clock_col = (uint8_t)prefs.getUInt("clock_col", 0);
  prefs.end();
}
