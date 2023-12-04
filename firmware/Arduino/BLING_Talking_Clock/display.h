#include "settings.h"

// Seriously, don't change this - donrt set fire to your BLING or your house!
#define MAX_BRIGHTNESS 50
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

// Enums
enum modes
{
  RUNNING = 0,
  MENU = 1,
};

void Speak(String text);
void SpeakCurrentTime();

// External refrences
extern modes mode;
extern unsigned long nextDisplayUpdate;
extern const uint16_t colors[9];
extern const uint16_t colors_colon[9];
extern RV3028C7 rtc;

bool isScrolling = false;
bool isDisplaying = false;
bool isInMenu = false;
bool isColon = true;
int current_setting = 0;

// Scrolling text data holders
String scrolling_text[] = {"", "", ""};
int scrolling_text_width[] = {0, 0, 0};
int scrolling_text_cursor[] = {0, 0, 0};
uint32_t scrolling_tex_color[] = {0, 0, 0};


const String setting_names[] = {
  "BRI",
  "DIM",
  "24H",
  "TLK",
  "SEC"
};

const String on_off[] = {
  "OFF",
  "ON",
};

// Returns the setting string for the currrent menu item
String GetMenuString()
{
  switch (current_setting)
  {
    case 0:
      return String(settings_brightness) + "%";
      break;

    case 1:
      return on_off[settings_dim_night ? 1 : 0];
      break;

    case 2:
      return on_off[settings_clock_24hour ? 1 : 0];
      break;

    case 3:
      return on_off[settings_speak_time ? 1 : 0];
      break;

    case 4:
      return on_off[settings_show_secs ? 1 : 0];
      break;



    default:
      return "";
      break;
  }
}

void SetBrightness(Adafruit_NeoMatrix &matrix)
{
  uint8_t hour = rtc.getCurrentDateTimeComponent(DATETIME_HOUR);
  if (settings_dim_night && (hour >= 21 || hour < 7))
    matrix.setBrightness(MAX_BRIGHTNESS * 15 / 100);
  else
    matrix.setBrightness(MAX_BRIGHTNESS * settings_brightness / 100);

}


// Settings scrolling switcher - animates a veertical scroll between each setting
void SwitchSetting(Adafruit_NeoMatrix &matrix, uint32_t color)
{
  matrix.setTextColor(color);

  if (mode == RUNNING)
  {
    matrix.fillScreen(0);
    matrix.show();
    matrix.setRotation(2);
    mode = MENU;
  }
  else
  {
    // scroll last setting off
    for (int y = 0; y < 10; y++)
    {
      matrix.fillScreen(0);
      matrix.setRotation(2);
      matrix.setCursor(0, y);
      matrix.setTextColor(color);
      matrix.print(setting_names[current_setting]);
      matrix.setTextColor(colors[3]);
      matrix.print(GetMenuString());
      matrix.show();
      delay(5);
    }

    current_setting++;
    if (current_setting == ELEMENTS(setting_names))
      current_setting = 0;
  }

  // scroll new saetting on
  for (int y = -10; y <= 0; y++)
  {
    matrix.fillScreen(0);
    matrix.setCursor(0, y);
    matrix.setTextColor(color);
    matrix.print(setting_names[current_setting]);
    matrix.setTextColor(colors[3]);
    matrix.print(GetMenuString());
    matrix.show();
    delay(5);
  }
}

// Get the next available or used slot in the text scrolling array
int GetNextTextSlot(bool used)
{
  for (int index = 0; index < ELEMENTS(scrolling_text); index++)
  {
    if (used)
    {
      if (scrolling_text[index] != "")
        return index;
    }
    else
    {
      if (scrolling_text[index] == "")
        return index;
    }
  }

  return -1;
}

// Updates the current menu setting based on user input and then re-displays the menu item
void UpdateSetting(Adafruit_NeoMatrix &matrix, uint32_t color, int dir)
{
  switch (current_setting)
  {
    case 0:
      settings_brightness += (dir * 10);
      if (settings_brightness > 100)
        settings_brightness = 10;
      else if (settings_brightness < 10)
        settings_brightness = 100;

      SetBrightness(matrix);
      break;

    case 1:
      settings_dim_night = !settings_dim_night;
      SetBrightness(matrix);
      break;

    case 2:
      settings_clock_24hour = !settings_clock_24hour;
      break;

    case 3:
      settings_speak_time = !settings_speak_time;
      break;

    case 4:
      settings_show_secs = !settings_show_secs;
      break;

    default:
      break;
  }

  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.setTextColor(color);
  matrix.print(setting_names[current_setting]);
  matrix.setTextColor(colors[3]);
  matrix.print(GetMenuString());
  matrix.show();
}

// Displays text for a period of time if the text is less then 8 chars
// otherwise it adds the text to a array to pass to the ScrollText function
void DisplayText(Adafruit_NeoMatrix &matrix, String txt, uint32_t color, int display_hold)
{
  if (txt.length() > 8)
  {
    int text_index = GetNextTextSlot(false);
    if (text_index < 0)
      return;

    scrolling_tex_color[text_index] = color;
    scrolling_text[text_index] = txt;
    scrolling_text_width[text_index] = txt.length() * 6;
    scrolling_text_cursor[text_index] = matrix.width();
  }
  else if (!isScrolling)
  {
    isDisplaying = true;
    matrix.setRotation(2);
    matrix.setTextColor(color);
    nextDisplayUpdate = millis() + display_hold;

    matrix.fillScreen(0);
    matrix.setCursor(0, 0);
    matrix.print(txt);
    matrix.show();
    matrix.setRotation(0);
  }
}

// Scrolls any text in the array, one at a time, clearing the out afterwards
bool ScrollText(Adafruit_NeoMatrix &matrix)
{
  int text_index = GetNextTextSlot(true);
  if (text_index >= 0)
  {
    matrix.setRotation(2);
    matrix.setTextColor(scrolling_tex_color[text_index]);
    isScrolling = true;
    if (scrolling_text_cursor[text_index] > -scrolling_text_width[text_index])
    {
      if ((millis() - nextDisplayUpdate) > 20)
      {
        nextDisplayUpdate = millis();

        matrix.fillScreen(0);
        matrix.setCursor(--scrolling_text_cursor[text_index], 0);
        matrix.print(scrolling_text[text_index]);
        matrix.show();
      }
    }
    else
    {
      scrolling_text[text_index] = "";
      matrix.setRotation(0);
      nextDisplayUpdate = millis() + 100;
      isScrolling = false;
    }
    return true;
  }
  return false;
}

// The usual colour wheel first made by Damien George
uint32_t colorWheel(Adafruit_NeoMatrix &matrix, uint8_t pos)
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

String pad_digit(int comp, String pad)
{
  if (comp < 10)
    return pad + String(comp);
  else
    return String(comp);
}

void ShowClock(Adafruit_NeoMatrix &matrix, bool force)
{
  if (force || (millis() - nextDisplayUpdate) > 1000)
  {
    uint8_t hour = rtc.getCurrentDateTimeComponent(DATETIME_HOUR);

    if (!force)
    {
      nextDisplayUpdate = millis();
      if (settings_speak_time && rtc.getCurrentDateTimeComponent(DATETIME_MINUTE) == 00 && rtc.getCurrentDateTimeComponent(DATETIME_SECOND) == 0)
      {
        SpeakCurrentTime();
      }
    }

    matrix.setRotation(2);
    matrix.fillScreen(0);
    SetBrightness(matrix);

    matrix.setTextColor(colors[settings_clock_col]);

    if (!settings_clock_24hour && hour > 12 )
      hour -= 12;

    if (settings_show_secs)
    {
      matrix.setCursor(0, 0);
      matrix.print(pad_digit(hour, " "));
      matrix.setCursor(14, 0);
      matrix.print(pad_digit(rtc.getCurrentDateTimeComponent(DATETIME_MINUTE), "0"));
      matrix.setCursor(28, 0);
      matrix.print(pad_digit(rtc.getCurrentDateTimeComponent(DATETIME_SECOND), "0"));

      if (isColon)
      {
        matrix.drawPixel(12, 2, colors_colon[settings_clock_col]);
        matrix.drawPixel(12, 4, colors_colon[settings_clock_col]);
        matrix.drawPixel(26, 2, colors_colon[settings_clock_col]);
        matrix.drawPixel(26, 4, colors_colon[settings_clock_col]);
      }
    }
    else
    {
      uint8_t start_x = (hour < 10) ? 5 : 7;

      matrix.setCursor(start_x, 0);
      matrix.print(pad_digit(hour, " "));
      matrix.setCursor(start_x+14, 0);
      matrix.print(pad_digit(rtc.getCurrentDateTimeComponent(DATETIME_MINUTE), "0"));

      if (isColon)
      {
        matrix.drawPixel(start_x+12, 2, colors_colon[settings_clock_col]);
        matrix.drawPixel(start_x+12, 4, colors_colon[settings_clock_col]);
      }
    }
  }
  matrix.show();

  if (!force)
    isColon = !isColon;
}