#include "settings.h"
#include <vector>

#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

constexpr uint32_t RGB(uint8_t r, uint8_t g, uint8_t b)
{
  return (( r & 0xf8 ) << 8 ) | (( g & 0xfc ) << 3 ) | ( b >> 3 );
}

// External refrences
extern unsigned long nextDisplayUpdate;
extern const uint16_t colors[9];
extern const uint16_t colors_colon[9];
extern Audio audio;

bool isScrolling = false;
bool isDisplaying = false;
int pacman_index = 0;
unsigned long next_pacman = 0;
unsigned long next_move = 0;
int pacman_x = -8;
int ghost_x = -20;
int current_ghost_color = 0;
bool chase = false;
int wakka_count = 0;
int speed_chase = 65;
int speed_chomp = 50;
int current_speed = 50;
int anim_bounds[2] = { -24, 70};

enum AnimMode
{
  EAT = 0,
  EAT_REV = 1,
  CHASE = 2,
  CHASE_REV = 3,
  FRUIT = 9,
};

String mode_names[4] = {"EAT", "EAT_REV", "CHASE", "CHASE_REV"};

AnimMode current_anim = EAT;
AnimMode last_anim = CHASE;

uint32_t ghost_colors[4] = {0xf800, RGB(255, 119, 2), RGB(35, 195, 255), RGB(255, 84, 255)};

// Scrolling text data holders
String scrolling_text[] = {"", "", ""};
int scrolling_text_width[] = {0, 0, 0};
int scrolling_text_cursor[] = {0, 0, 0};
uint32_t scrolling_tex_color[] = {0, 0, 0};

std::vector<const uint16_t*>pacman_frames;

void AssembleFrames()
{
  pacman_frames.push_back(pacman_frame_1_8x8);
  pacman_frames.push_back(pacman_frame_2_8x8);
  pacman_frames.push_back(pacman_frame_3_8x8);
  pacman_frames.push_back(pacman_frame_2_8x8);
}

const uint16_t *GetPacmanFrame()
{
  if (millis() - next_pacman > 100 )
  {
    next_pacman = millis();
    pacman_index++;
    if (pacman_index == 4)
      pacman_index = 0;
  }

  return pacman_frames[pacman_index];
}

void DrawSprite(Adafruit_NeoMatrix &matrix, const uint16_t *Frame, int sprite_x)
{
  uint pixel = 0;
  for (int _y = 0; _y < 8; _y++)
  {
    for (int _x = 0; _x < 8; _x++)
    {
      // Draw the sprite backwards on X to save having to store 2 sets of every sprite,
      // one facing right and one facing left
      if (current_anim == CHASE_REV || current_anim == EAT_REV)
        matrix.drawPixel((8 - _x) + sprite_x, _y, Frame[pixel++]);
      else
        matrix.drawPixel(_x + sprite_x, _y, Frame[pixel++]);
    }
  }
}

void DrawGhostSprite(Adafruit_NeoMatrix &matrix, const uint16_t *Frame, int sprite_x)
{
  uint pixel = 0;
  for (int _y = 0; _y < 8; _y++)
  {
    for (int _x = 0; _x < 8; _x++)
    {
      // Get the pixel colour for the ghost - if it's red, replace it with the selected colour for this
      // animation pass, otherwise, leave it
      uint32_t pixel_color = Frame[pixel++];
      if (pixel_color == 0xf800)
        pixel_color = ghost_colors[current_ghost_color];

      // Draw the sprite backwards on X to save having to store 2 sets of every sprite,
      // one facing right and one facing left
      if (current_anim == CHASE || current_anim == EAT_REV)
        matrix.drawPixel((8 - _x) + sprite_x, _y, pixel_color);
      else
        matrix.drawPixel(_x + sprite_x, _y, pixel_color);
    }
  }
}

void DrawDots(Adafruit_NeoMatrix &matrix, bool slow)
{
  for (int dot_x = 0; dot_x < 40; dot_x += 10)
  {
    if ((current_anim == EAT && dot_x > pacman_x) || (current_anim == EAT_REV && dot_x < pacman_x))
    {
      matrix.drawRGBBitmap(dot_x, 0, dot_8x8, 8, 8);
      if (slow)
      {
        matrix.show();
        delay(150);
      }
    }
  }
}

void Animate(Adafruit_NeoMatrix &matrix)
{
  matrix.fillScreen(0);

  // We draw the dots on the display before the chosts and pacman if it's in EAT mode
  if (current_anim == EAT || current_anim == EAT_REV)
    DrawDots(matrix, false);
    
  DrawSprite(matrix, GetPacmanFrame(), pacman_x);
  DrawGhostSprite(matrix, ((current_anim == EAT || current_anim == EAT_REV) ? ghost_red_8x8 : ghost_blue_8x8), ghost_x);

  matrix.show();

  // Update the frame and if we have hit a bounbds, calculate the next animation
  if (millis() - next_move > current_speed )
  {
    next_move = millis();
    int dir = (current_anim == EAT || current_anim == CHASE) ? 1 : -1;

    pacman_x += dir;
    ghost_x += dir;

    if (pacman_x > anim_bounds[1] || pacman_x < anim_bounds[0] )
    {
      last_anim = current_anim;

      bool chase = (((random(100) < 20) || wakka_count > 4) && (last_anim != CHASE || last_anim != CHASE_REV));

      if (!chase)
      {
        audio.connecttoFS(SD, "/pacman_chomp.wav");
        DrawDots(matrix, true);
        current_speed = speed_chomp;
        current_anim = (AnimMode)random(2);
        wakka_count++;
        current_ghost_color = random(4);
      }
      else
      {
        audio.connecttoFS(SD, "/pacman_intermission.wav");
        current_speed = speed_chase;
        current_anim = (AnimMode)(random(2) + 2);
        wakka_count = 0;
      }

      if (current_anim == EAT ) 
      {
        pacman_x = -8;
        ghost_x = -20;
      }
      else if (current_anim == EAT_REV)
      {
        pacman_x = 58;
        ghost_x = 70;
      }
      else if (current_anim == CHASE)
      {
        pacman_x = -20;
        ghost_x = -8;
      }
      else if (current_anim == CHASE_REV)
      {
        pacman_x = 70;
        ghost_x = 58;
      } 

//      Serial.println("Anim mode: "+mode_names[(int)current_anim]+" - "+String((int)current_anim));
    }
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
    matrix.setRotation(2);
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
