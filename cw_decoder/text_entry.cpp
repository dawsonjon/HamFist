#include "text_entry.h"
#include <cstdio>
#include <cstring>
#include <string>

#include "colours.h"
#include "font_16x12.h"
#include "font_8x5.h"
#include "dictionary.h"

//#define LOGGING

#ifdef LOGGING
#ifdef ARDUINO
#include <Arduino.h>
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#include <cstdio>
#define DEBUG_PRINTF(...) std::printf(__VA_ARGS__)
#endif
#else
#define DEBUG_PRINTF(...)
#endif

static const uint16_t DISPLAY_WIDTH = 320;
static const uint16_t DISPLAY_HEIGHT = 240;
static const char char_select[4][4][4] = {
{{'A', 'B', 'C', 'D'},
 {'E', 'F', 'G', 'H'},
 {AUTOCOMPLETE, AUTOCOMPLETE, AUTOCOMPLETE, '_'},
 {'.', ',', '?', '/'},},

{{'I', 'J', 'K', 'L'},
 {'M', 'N', 'O', 'P'},
 {'Q', ' ', ' ', '_'},
 {'-', '=', '(', ')'},},

{{'R', 'S', 'T', 'U'},
 {'V', 'W', 'X', 'Y'},
 {'Z', ' ', ' ', '_'},
 {':', ' ', ' ', ' '},},

{{'0', '1', '2', '3'},
 {'4', '5', '6', '7'},
 {'8', '9', CLEAR, '_'},
 {LEFT, RIGHT, ENTER, BACKSPACE},}};

bool c_text_entry :: is_active()
{
  return m_state != INACTIVE;
}

void c_text_entry :: raise(char* string, uint16_t n)
{
  m_n = n;
  m_string = string;
  m_state = INITIALISE;
}

c_text_entry :: c_text_entry(ILI934X *display, button &button_left, button &button_right, button &button_down, button &button_up) :
  m_display(display)
{
  m_buttons[0] = &button_left;
  m_buttons[1] = &button_right;
  m_buttons[2] = &button_down;
  m_buttons[3] = &button_up;
}

static uint16_t colours[] = {COLOUR_RED, COLOUR_GREEN, COLOUR_YELLOW, COLOUR_BLUE};
static std::string last_word(const std::string& text)
{
    if (text.empty())
        return "";

    // If the string ends with whitespace, return empty
    if (std::isspace(static_cast<unsigned char>(text.back())))
        return "";

    // Find the last non-space character
    size_t end = text.find_last_not_of(" \t\n");
    if (end == std::string::npos)
        return "";

    // Find the space before the last word
    size_t start = text.find_last_of(" \t\n", end);
    if (start == std::string::npos)
        return text.substr(0, end + 1);

    return text.substr(start + 1, end - start);
}

void c_text_entry :: run()
{

  switch(m_state)
  {

    case INACTIVE:
      break;

    case INITIALISE:
    {
      m_cursor = 0;
      uint16_t chars_per_row = DISPLAY_WIDTH/6 - 2;
      uint16_t rows = (m_n + chars_per_row/2) / chars_per_row;
      uint16_t string_width = chars_per_row*6;
      uint16_t y = 20;
      uint16_t x = (DISPLAY_WIDTH - string_width)/2;
      uint16_t cursor_y = m_cursor/chars_per_row;
      uint16_t cursor_x = m_cursor%chars_per_row;
      
      m_display->clear(COLOUR_BLACK);
      m_display->drawRect(x, y, rows*10, chars_per_row*6, COLOUR_NAVY);
      uint16_t chars_left = m_n;
      for(uint8_t row=0; row<rows; row++)
      {
        char buffer[chars_per_row+1];
        uint16_t chars_to_copy = std::min(chars_left, chars_per_row);
        chars_left -= chars_per_row;
        strncpy(buffer, m_string+(row*chars_per_row), chars_to_copy);
        buffer[chars_per_row] = 0;

        m_display->drawString(x, y, font_8x5, buffer, COLOUR_WHITE, COLOUR_NAVY);
        if(row == cursor_y) m_display->drawRect(x + cursor_x*6, y, 8, 5, COLOUR_RED);
        y+=10;
      }
      m_display->drawString(x, y+5, font_8x5, m_autocomplete_suggestion.c_str(), colours[0], COLOUR_BLACK);
      m_display->drawString(x, y+15, font_8x5, m_second.c_str(), colours[1], COLOUR_BLACK);
      m_display->drawString(x, y+25, font_8x5, m_third.c_str(), colours[2], COLOUR_BLACK);
      m_state = GETCHAR1_DRAW;
      break;
    }

    case GETCHAR1_DRAW:
      m_display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
      for(uint8_t i=0; i<4; i++) {
        for(uint8_t j=0; j<4; j++) {
          char disp[20];
          snprintf(disp, 20, " %c%c%c%c", char_select[i][j][0], char_select[i][j][1],char_select[i][j][2],char_select[i][j][3]);
          m_display->drawString(10+i*75, 120+j*20, font_16x12, disp, colours[i], COLOUR_BLACK);
        }
      }
      m_state = GETCHAR1_GET;
      break;

    case GETCHAR1_GET:
      for(uint8_t i=0; i<4; i++) {
        if(m_buttons[i]->is_pressed())
        {
          m_sel1 = i;
          m_state = GETCHAR2_DRAW;
        }
      }
      break;

    case GETCHAR2_DRAW:
      m_display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
      for(uint8_t i=0; i<4; i++) {
          DEBUG_PRINTF("%i\n", i);
          char disp[20];
          snprintf(disp, 20, " %c%c%c%c", char_select[m_sel1][i][0], char_select[m_sel1][i][1],char_select[m_sel1][i][2],char_select[m_sel1][i][3]);
          DEBUG_PRINTF("%c\n", char_select[m_sel1][i][3]);
          m_display->drawString(10+i*75, 120, font_16x12, disp, colours[i], COLOUR_BLACK);
      }
      m_state = GETCHAR2_GET;
      break;

    case GETCHAR2_GET:
      for(uint8_t i=0; i<4; i++) {
        if(m_buttons[i]->is_pressed())
        {
          m_sel2 = i;
          m_state = GETCHAR3_DRAW;
        }
      }
      break;

    case GETCHAR3_DRAW:
      m_display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
      if(m_sel2==3 && m_sel1==3) {
        m_display->drawString(16, 120, font_16x12, " LEFT RIGHT ENTER BKSPC", COLOUR_WHITE, COLOUR_BLACK); 
      } else {
        for(uint8_t i=0; i<4; i++) {
          char disp[20];
          snprintf(disp, 20, "%c", char_select[m_sel1][m_sel2][i]);
          DEBUG_PRINTF("%c\n", char_select[m_sel1][m_sel2][i]);
          m_display->drawString(10+i*75, 120, font_16x12, disp, colours[i], COLOUR_BLACK);
        }
      }
      if(m_sel2==2 && m_sel1==0) {
        m_display->drawString(10+0*75, 140, font_8x5, m_autocomplete_suggestion.c_str(), colours[0], COLOUR_BLACK);
        m_display->drawString(10+1*75, 140, font_8x5, m_second.c_str(), colours[1], COLOUR_BLACK);
        m_display->drawString(10+2*75, 140, font_8x5, m_third.c_str(), colours[2], COLOUR_BLACK);
      }
      m_state = GETCHAR3_GET;
      break;

    case GETCHAR3_GET:
      for(uint8_t i=0; i<4; i++) {
        if(m_buttons[i]->is_pressed())
        {
          m_sel3 = i;
          m_state = NEXT_CHAR;
        }
      }
      break;

    case NEXT_CHAR:
    {
      char entry = char_select[m_sel1][m_sel2][m_sel3];
      if(entry == LEFT){
        m_cursor--;
      } else if (entry == RIGHT){
        m_cursor++;
      } else if (entry == BACKSPACE){
        size_t len = strlen(m_string);
        memmove(&m_string[m_cursor], &m_string[m_cursor + 1], len - m_cursor);
        if(len - m_cursor == 0) {
          m_string[m_cursor-1] = 0;
        }
        DEBUG_PRINTF("len_to_copy %u\n", len-m_cursor);
        m_cursor--;
      } else if (entry == ENTER){
        m_display->clear(COLOUR_BLACK);
        m_state = INACTIVE;
        break;
      } else if (entry == AUTOCOMPLETE){
        //add autocomplete suggestion
        std::string string_word = std::string(m_string);
        std::string last_word_of_string = last_word(string_word);
        std::string completion;

        if(m_sel3==0 && m_autocomplete_suggestion.size())
          completion = m_autocomplete_suggestion.substr(last_word_of_string.size(), m_autocomplete_suggestion.size()-1);
        else if(m_sel3==1 && m_second.size())
          completion = m_second.substr(last_word_of_string.size(), m_second.size()-1);
        else if(m_sel3==2 && m_third.size())
          completion = m_third.substr(last_word_of_string.size(), m_third.size()-1);

        strncat(m_string, completion.c_str(), m_n-strnlen(m_string, m_n)-1);
        //istrncat(m_string, " ", m_n-strnlen(m_string, m_n)-1);
        m_cursor = strnlen(m_string, m_n);
      } else if (entry == CLEAR){
        for(uint8_t i=0; i<m_n; i++) m_string[i] = 0;
        m_cursor=0;
      } else if (entry == '_'){
        m_string[m_cursor++] = ' ';
      } else {
        m_string[m_cursor++] = entry;
      }
      if(m_cursor > strnlen(m_string, m_n)) {
        m_cursor = strnlen(m_string, m_n);
      }
      

      //autocomplete
      std::string string_word = std::string(m_string);
      std::string last_word_of_string = last_word(string_word);
      DEBUG_PRINTF("%s\n", last_word_of_string.c_str());
      m_autocomplete_suggestion = suggestion(last_word_of_string, m_second, m_third);

      //redraw
        
      uint16_t chars_per_row = DISPLAY_WIDTH/6 - 2;
      uint16_t rows = (m_n + chars_per_row/2) / chars_per_row;
      uint16_t string_width = chars_per_row*6;
      uint16_t y = 20;
      uint16_t x = (DISPLAY_WIDTH - string_width)/2;
      uint16_t cursor_y = m_cursor/chars_per_row;
      uint16_t cursor_x = m_cursor%chars_per_row;
      
      m_display->clear(COLOUR_BLACK);
      m_display->drawRect(x, y, rows*10, chars_per_row*6, COLOUR_NAVY);
      uint16_t chars_left = m_n;
      for(uint8_t row=0; row<rows; row++)
      {
        char buffer[chars_per_row+1];
        uint16_t chars_to_copy = std::min(chars_left, chars_per_row);
        chars_left -= chars_per_row;
        strncpy(buffer, m_string+(row*chars_per_row), chars_to_copy);
        buffer[chars_per_row] = 0;

        m_display->drawString(x, y, font_8x5, buffer, COLOUR_WHITE, COLOUR_NAVY);
        if(row == cursor_y) m_display->drawRect(x + cursor_x*6, y, 8, 5, COLOUR_RED);
        y+=10;
      }
      m_display->drawString(x, y+5, font_8x5, m_autocomplete_suggestion.c_str(), colours[0], COLOUR_BLACK);
      m_display->drawString(x, y+15, font_8x5, m_second.c_str(), colours[1], COLOUR_BLACK);
      m_display->drawString(x, y+25, font_8x5, m_third.c_str(), colours[2], COLOUR_BLACK);

      m_state = GETCHAR1_DRAW;
      break;
    }
  }
}
