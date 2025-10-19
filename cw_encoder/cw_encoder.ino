#include <EEPROM.h>
#include "hardware/spi.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "ili934x.h"
#include "button.h"


//CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

#define PIN_MISO 12 //not used by TFT but part of SPI bus
#define PIN_CS   13
#define PIN_SCK  14
#define PIN_MOSI 15 
#define PIN_DC   11
#define SPI_PORT spi1

//#define ROTATION R0DEG
//#define ROTATION R90DEG
//#define ROTATION R180DEG
#define ROTATION R270DEG
//#define ROTATION MIRRORED0DEG
//#define ROTATION MIRRORED90DEG
//#define ROTATION MIRRORED180DEG
//#define ROTATION MIRRORED270DEG

//#define INVERT_COLOURS false
#define INVERT_COLOURS true

#define INVERT_DISPLAY false
//#define INVERT_DISPLAY true

#define DISPLAY_TYPE 0 //ILI934x 320x240 TFT DIsplay
//#define DISPLAY_TYPE 1 //ILI934x (driver 2) 320x240 TFT DIsplay
//#define DISPLAY_TYPE 2 //ST7796 480x320
//#define DISPLAY_TYPE 3 //ILI9488 480x320

#if DISPLAY_TYPE == 0
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  e_display_type display_type = ILI9341;
#elif DISPLAY_TYPE == 1
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  e_display_type display_type = ILI9341_2;
#elif DISPLAY_TYPE == 2
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  e_display_type display_type = ST7796;
#else
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  e_display_type display_type = ILI9488;
#endif



ILI934X *display;


void setup() {
  Serial.begin(115200);
  //while(!Serial){}
  configure_display();
  Serial.println("HamFist (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");
}

void loop()
{
  char buffer[20] = {0};
  while(1)
  {
    text_entry(buffer, 20);
  }
}

void configure_display()
{
  spi_init(SPI_PORT, 75000000);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_init(PIN_DC);
  gpio_set_dir(PIN_DC, GPIO_OUT);
  display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, width, height);
  display->init(ROTATION, INVERT_COLOURS, INVERT_DISPLAY, display_type);
  display->powerOn(true);
  display->clear();
}

c_button m_fire_button(17);
c_button m_pause_button(20);
c_button m_right_button(21);
c_button m_left_button(22);
static char char_select[4][4][4] = {

{
{'A', 'B', 'C', 'D'},
{'E', 'F', 'G', 'H'},
{' ', ' ', ' ', '_'},
{'<', '>', '#', '!'},
},

{
{'I', 'J', 'K', 'L'},
{'M', 'N', 'O', 'P'},
{'Q', ' ', ' ', '_'},
{'<', '>', '#', '!'},
},

{
{'R', 'S', 'T', 'U'},
{'V', 'W', 'X', 'Y'},
{'Z', ' ', ' ', '_'},
{'<', '>', '#', '!'},
},

{
{'0', '1', '2', '3'},
{'4', '5', '6', '7'},
{'8', '9', ' ', '_'},
{'<', '>', '#', '!'},
}

};

uint8_t get_char0(c_button *buttons[])
{

  display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
  for(uint8_t i=0; i<4; i++)
  {
    for(uint8_t j=0; j<4; j++)
    {
      char disp[20];
      snprintf(disp, 20, " %c%c%c%c", char_select[i][j][0], char_select[i][j][1],char_select[i][j][2],char_select[i][j][3]);
      display->drawString(10+i*75, 120+j*20, font_16x12, disp, COLOUR_WHITE, COLOUR_BLACK);
    }
  }
  while(1)
  {
    for(uint8_t i=0; i<4; i++)
    {
      if(buttons[i]->is_pressed()) return i; 
    }
  }

}

uint8_t get_char1(c_button *buttons[], uint8_t sel1)
{
  display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
  for(uint8_t i=0; i<4; i++)
  {
      char disp[20];
      snprintf(disp, 20, " %c%c%c%c", char_select[sel1][i][0], char_select[sel1][i][1],char_select[sel1][i][2],char_select[sel1][i][3]);
      display->drawString(10+i*75, 120, font_16x12, disp, COLOUR_WHITE, COLOUR_BLACK);
  }
  while(1)
  {
    for(uint8_t i=0; i<4; i++)
    {
      if(buttons[i]->is_pressed()) return i; 
    }
  }

}

uint8_t get_char2(c_button *buttons[], uint8_t sel0, uint8_t sel1)
{
  display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
  if(sel1==3){
    display->drawString(16, 120, font_16x12, " LEFT RIGHT ENTER CLEAR", COLOUR_WHITE, COLOUR_BLACK);
  } else {
    for(uint8_t i=0; i<4; i++)
    {
      char disp[20];
      snprintf(disp, 20, "%c", char_select[sel0][sel1][i]);
      display->drawString(10+i*75, 120, font_16x12, disp, COLOUR_WHITE, COLOUR_BLACK);
    }
  }
  while(1)
  {
    for(uint8_t i=0; i<4; i++)
    {
      if(buttons[i]->is_pressed()) return i; 
    }
  }

}


void text_entry(char string[], uint8_t n)
{
  c_button* buttons[] = {&m_left_button, &m_right_button, &m_pause_button, &m_fire_button};
  uint8_t cursor = 0;
  while(1)
  {
    display->clear(COLOUR_BLACK);
    display->drawRect((320-(n*12))/2, 20, 16, n*12,  COLOUR_NAVY);
    display->drawString((320-(n*12))/2, 20, font_16x12, string, COLOUR_WHITE, COLOUR_BLACK);
    display->drawRect((320-(n*12))/2 + cursor*12, 20, 16, 12, COLOUR_RED);

    uint8_t sel0 = get_char0(buttons);
    uint8_t sel1 = get_char1(buttons, sel0);
    uint8_t sel2 = get_char2(buttons, sel0, sel1);

    char entry = char_select[sel0][sel1][sel2];

    if(entry == '<'){
      cursor--;
    } else if (entry == '>'){
      cursor++;
    } else if (entry == '#'){
      return;
    } else if (entry == '!'){
      for(uint8_t i=0; i<n; i++) string[i] = 0;
      cursor=0;
    } else if (entry == '_'){
      string[cursor++] = ' ';
    } else {
      string[cursor++] = entry;
    }

    if(cursor == n) return;
    cursor %= n;
  }
}

