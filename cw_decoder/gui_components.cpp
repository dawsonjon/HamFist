#include "gui_components.h"
#include <cstdio>
#include <cstring>
#include <string>

#include "colours.h"
#include "dictionary.h"
#include "font_16x12.h"
#include "font_8x5.h"
#include "utils.h"
#include "buttons.h"
#include "frame_buffer.h"

#define LOGGING

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


// COLOUR SCHEME
static uint16_t BACKGROUND = COLOUR_BLACK;
static uint16_t ACTIVE_BORDER = COLOUR_ORANGE;
static uint16_t TEXT_ACTIVE = COLOUR_BLUE;
static uint16_t TEXT_INACTIVE = COLOUR_GREY;
static uint16_t TEXT_TITLE = COLOUR_WHITE;
static uint16_t BUTTON_TEXT = COLOUR_WHITE;
static uint16_t BUTTON_COLOUR = COLOUR_BLUE;
static uint16_t BUTTON_BORDER = COLOUR_WHITE;
static uint16_t BUTTON_INACTIVE = COLOUR_GREY;
static uint16_t BUTTON_INACTIVE_BORDER = COLOUR_LIGHTGREY;
static uint16_t MENU_ACTIVE = COLOUR_ORANGE;
static uint16_t MENU_INACTIVE = COLOUR_AQUA;

static const uint16_t DISPLAY_WIDTH = 320;
static const uint16_t DISPLAY_HEIGHT = 240;
static const int BUTTON_BAR_HEIGHT = 25;

static const char char_select[4][4][4] = {{
                                              {'A', 'B', 'C', 'D'},
                                              {'E', 'F', 'G', 'H'},
                                              {AUTOCOMPLETE, AUTOCOMPLETE, AUTOCOMPLETE, '_'},
                                              {'.', ',', '?', '/'},
                                          },

                                          {
                                              {'I', 'J', 'K', 'L'},
                                              {'M', 'N', 'O', 'P'},
                                              {'Q', ' ', ' ', '_'},
                                              {'-', '=', '(', ')'},
                                          },

                                          {
                                              {'R', 'S', 'T', 'U'},
                                              {'V', 'W', 'X', 'Y'},
                                              {'Z', ' ', ' ', '_'},
                                              {':', '@', ' ', ' '},
                                          },

                                          {
                                              {'0', '1', '2', '3'},
                                              {'4', '5', '6', '7'},
                                              {'8', '9', CLEAR, '_'},
                                              {LEFT, RIGHT, ENTER, BACKSPACE},
                                          }};


     
void draw_button_bar(ILI934X *display, const char* btn1, const char* btn2, const char* btn3, const char* btn4)
{
  display->fillRect(0, DISPLAY_HEIGHT - BUTTON_BAR_HEIGHT - 1, BUTTON_BAR_HEIGHT, DISPLAY_WIDTH,
                      BACKGROUND);
  const uint16_t button_width = 60;
  const uint16_t button_height = 14;
  const uint16_t padding = (DISPLAY_WIDTH - (4 * button_width)) / 5;
  const char* btn_txt[] = {btn1, btn2, btn3, btn4};
  uint16_t button_x = padding;
  for (uint8_t idx = 0; idx < 4; ++idx) {
    bool active = strlen(btn_txt[idx]);
    static uint16_t button_image[button_width * button_height];
    c_frame_buffer button_frame_buffer(button_image, button_width, button_height);
    button_frame_buffer.draw_image(0, 0, button_width, button_height, blank_button);
    button_frame_buffer.draw_string(((button_width - (6 * strlen(btn_txt[idx]))) / 2), 4, font_8x5, btn_txt[idx], BUTTON_TEXT);
    display->writeImage(button_x,  DISPLAY_HEIGHT - BUTTON_BAR_HEIGHT + 2, button_width, button_height, button_image);
    button_x += button_width + padding;
  }
}

void draw_button_bar(ILI934X *display, const uint16_t* btn1, const uint16_t* btn2, const uint16_t* btn3,
                                   const uint16_t* btn4)
{
  display->fillRect(0, DISPLAY_HEIGHT - BUTTON_BAR_HEIGHT - 1, BUTTON_BAR_HEIGHT, DISPLAY_WIDTH,
                      BACKGROUND);
  const uint16_t button_width = 60;
  const uint16_t button_height = 14;
  const uint16_t padding = (DISPLAY_WIDTH - (4 * button_width)) / 5;
  const uint16_t* btn_img[] = {btn1, btn2, btn3, btn4};
  uint16_t button_x = padding;
  for (uint8_t idx = 0; idx < 4; ++idx) {
    display->writeImage(button_x,  DISPLAY_HEIGHT - BUTTON_BAR_HEIGHT + 2, button_width, button_height, btn_img[idx]);
    button_x += button_width + padding;
  }
}

void draw_heading(ILI934X *display, const char title[])
{
  const uint16_t heading_width = 280;
  const uint16_t heading_height = 20;
  uint16_t width = strlen(title) * 12;

  static uint16_t heading_image[heading_width * heading_height];
  c_frame_buffer heading_frame_buffer(heading_image, heading_width, heading_height);
  heading_frame_buffer.draw_image(0, 0, heading_width, heading_height, blank_heading);
  heading_frame_buffer.draw_string((heading_width - width) / 2, 2, font_16x12, title, BUTTON_TEXT);
  display->writeImage((DISPLAY_WIDTH - heading_width) / 2,  2, heading_width, heading_height, heading_image);
}

bool c_text_entry ::is_active()
{
  return m_state != INACTIVE;
}

void c_text_entry ::raise(char* string, uint16_t n)
{
  m_n = n;
  m_string = string;
  m_state = INITIALISE;
  m_cursor = strlen(string);
}

c_text_entry ::c_text_entry(ILI934X* display, button& button_left, button& button_right,
                            button& button_down, button& button_up)
    : m_display(display)
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

void c_text_entry ::run()
{

  // autocomplete
  std::string string_word = std::string(m_string).substr(0, m_cursor);
  std::string last_word_of_string = last_word(string_word);
  m_autocomplete_suggestion = suggestion(last_word_of_string, m_second, m_third);

  switch (m_state) {

    case INACTIVE:
      break;

    case INITIALISE: {
      m_cursor = strlen(m_string);
      uint16_t chars_per_row = DISPLAY_WIDTH / 6 - 2;
      uint16_t rows = (m_n + chars_per_row - 1) / chars_per_row;
      uint16_t string_width = chars_per_row * 6;
      uint16_t y = 20;
      uint16_t x = (DISPLAY_WIDTH - string_width) / 2;
      uint16_t cursor_y = m_cursor / chars_per_row;
      uint16_t cursor_x = m_cursor % chars_per_row;

      m_display->clear(COLOUR_BLACK);
      m_display->drawRect(x, y, rows * 10, chars_per_row * 6, COLOUR_NAVY);
      uint16_t chars_left = m_n;
      for (uint8_t row = 0; row < rows; row++) {
        char buffer[chars_per_row + 1];
        uint16_t chars_to_copy = std::min(chars_left, chars_per_row);
        chars_left -= chars_per_row;
        strncpy(buffer, m_string + (row * chars_per_row), chars_to_copy);
        buffer[chars_per_row] = 0;

        m_display->drawString(x, y, font_8x5, buffer, COLOUR_WHITE, COLOUR_NAVY);
        if (row == cursor_y)
          m_display->drawRect(x + cursor_x * 6, y, 8, 5, COLOUR_RED);
        y += 10;
      }
      m_display->drawString(x, y + 5, font_8x5, m_autocomplete_suggestion.c_str(), colours[0],
                            COLOUR_BLACK);
      m_display->drawString(x, y + 15, font_8x5, m_second.c_str(), colours[1], COLOUR_BLACK);
      m_display->drawString(x, y + 25, font_8x5, m_third.c_str(), colours[2], COLOUR_BLACK);
      m_state = GETCHAR1_DRAW;
      break;
    }

    case GETCHAR1_DRAW:
      m_display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
      for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
          char disp[20];
          snprintf(disp, 20, " %c%c%c%c", char_select[i][j][0], char_select[i][j][1],
                  char_select[i][j][2], char_select[i][j][3]);
          m_display->drawString(10 + i * 75, 120 + j * 20, font_16x12, disp, colours[i],
                                COLOUR_BLACK);
        }
      }
      m_state = GETCHAR1_GET;
      break;

    case GETCHAR1_GET:
      for (uint8_t i = 0; i < 4; i++) {
        if (m_buttons[i]->is_pressed()) {
          m_sel1 = i;
          m_state = GETCHAR2_DRAW;
        }
      }
      break;

    case GETCHAR2_DRAW:
      m_display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
      for (uint8_t i = 0; i < 4; i++) {
        DEBUG_PRINTF("%i\n", i);
        char disp[20];
        snprintf(disp, 20, " %c%c%c%c", char_select[m_sel1][i][0], char_select[m_sel1][i][1],
                char_select[m_sel1][i][2], char_select[m_sel1][i][3]);
        DEBUG_PRINTF("%c\n", char_select[m_sel1][i][3]);
        m_display->drawString(10 + i * 75, 120, font_16x12, disp, colours[i], COLOUR_BLACK);
      }
      m_state = GETCHAR2_GET;
      break;

    case GETCHAR2_GET:
      for (uint8_t i = 0; i < 4; i++) {
        if (m_buttons[i]->is_pressed()) {
          m_sel2 = i;
          m_state = GETCHAR3_DRAW;
        }
      }
      break;

    case GETCHAR3_DRAW:
      m_display->fillRect(0, 120, 120, 320, COLOUR_BLACK);
      if (m_sel2 == 3 && m_sel1 == 3) {
        m_display->drawString(16, 120, font_16x12, " LEFT RIGHT ENTER BKSPC", COLOUR_WHITE,
                              COLOUR_BLACK);
      } else {
        for (uint8_t i = 0; i < 4; i++) {
          char disp[20];
          snprintf(disp, 20, "%c", char_select[m_sel1][m_sel2][i]);
          DEBUG_PRINTF("%c\n", char_select[m_sel1][m_sel2][i]);
          m_display->drawString(10 + i * 75, 120, font_16x12, disp, colours[i], COLOUR_BLACK);
        }
      }
      if (m_sel2 == 2 && m_sel1 == 0) {
        m_display->drawString(10 + 0 * 75, 140, font_8x5, m_autocomplete_suggestion.c_str(),
                              colours[0], COLOUR_BLACK);
        m_display->drawString(10 + 1 * 75, 140, font_8x5, m_second.c_str(), colours[1], COLOUR_BLACK);
        m_display->drawString(10 + 2 * 75, 140, font_8x5, m_third.c_str(), colours[2], COLOUR_BLACK);
      }
      m_state = GETCHAR3_GET;
      break;

    case GETCHAR3_GET:
      for (uint8_t i = 0; i < 4; i++) {
        if (m_buttons[i]->is_pressed()) {
          m_sel3 = i;
          m_state = NEXT_CHAR;
        }
      }
      break;

    case NEXT_CHAR: {
      char entry = char_select[m_sel1][m_sel2][m_sel3];
      if (entry == LEFT) {
        m_cursor--;
      } else if (entry == RIGHT) {
        m_cursor++;
      } else if (entry == BACKSPACE) {
        size_t len = strlen(m_string);

        if(m_cursor > 0 && m_cursor <= len) {
          memmove(&m_string[m_cursor-1], &m_string[m_cursor], len - m_cursor);
          m_string[len-1]=0;
          m_cursor--;
        }
      } else if (entry == ENTER) {
        m_display->clear(COLOUR_BLACK);
        m_state = INACTIVE;
        break;
      } else if (entry == AUTOCOMPLETE) {
        std::string completion;

        if (m_sel3 == 0 && m_autocomplete_suggestion.size())
          completion = m_autocomplete_suggestion.substr(last_word_of_string.size(), m_autocomplete_suggestion.size() - 1);
        else if (m_sel3 == 1 && m_second.size())
          completion = m_second.substr(last_word_of_string.size(), m_second.size() - 1);
        else if (m_sel3 == 2 && m_third.size())
          completion = m_third.substr(last_word_of_string.size(), m_third.size() - 1);

        str_insert(m_string, m_n, m_cursor, completion.c_str());
        m_cursor += completion.size();

        m_cursor = strnlen(m_string, m_n);
      } else if (entry == CLEAR) {
        for (uint8_t i = 0; i < m_n; i++)
          m_string[i] = 0;
        m_cursor = 0;
      } else if (entry == '_') {
        str_insert(m_string, m_n, m_cursor, " ");
        m_cursor++;
      } else {
        const char new_text[2] = {entry, 0};
        str_insert(m_string, m_n, m_cursor, new_text);
        m_cursor++;
      }
      if (m_cursor > strnlen(m_string, m_n)) {
        m_cursor = strnlen(m_string, m_n);
      }
      m_state = NEXT_CHAR_DRAW;
      break;
    }
    
    case NEXT_CHAR_DRAW:
      DEBUG_PRINTF("m_string %s\n", m_string);

      uint16_t chars_per_row = DISPLAY_WIDTH / 6 - 2;
      uint16_t rows = (m_n + chars_per_row - 1) / chars_per_row;
      uint16_t string_width = chars_per_row * 6;
      uint16_t y = 20;
      uint16_t x = (DISPLAY_WIDTH - string_width) / 2;
      uint16_t cursor_y = m_cursor / chars_per_row;
      uint16_t cursor_x = m_cursor % chars_per_row;

      m_display->clear(COLOUR_BLACK);
      m_display->drawRect(x, y, rows * 10, chars_per_row * 6, COLOUR_NAVY);
      uint16_t chars_left = m_n;
      for (uint8_t row = 0; row < rows; row++) {
        char buffer[chars_per_row + 1];
        uint16_t chars_to_copy = std::min(chars_left, chars_per_row);
        chars_left -= chars_per_row;
        strncpy(buffer, m_string + (row * chars_per_row), chars_to_copy);
        buffer[chars_per_row] = 0;

        m_display->drawString(x, y, font_8x5, buffer, COLOUR_WHITE, COLOUR_NAVY);
        if (row == cursor_y)
          m_display->drawRect(x + cursor_x * 6, y, 8, 5, COLOUR_RED);
        y += 10;
      }
      m_display->drawString(x, y + 5, font_8x5, m_autocomplete_suggestion.c_str(), colours[0],
                            COLOUR_BLACK);
      m_display->drawString(x, y + 15, font_8x5, m_second.c_str(), colours[1], COLOUR_BLACK);
      m_display->drawString(x, y + 25, font_8x5, m_third.c_str(), colours[2], COLOUR_BLACK);

      m_state = GETCHAR1_DRAW;
      break;
  }
}

bool c_multi_text_entry ::is_active()
{
  return m_active;
}

void c_multi_text_entry ::raise()
{
  m_active = true;
  m_needs_redraw = true;
}

c_multi_text_entry ::c_multi_text_entry(ILI934X* display, button& button_left, button& button_right,
                                        button& button_down, button& button_up,
                                        s_messages& messages)
    : m_display(display), text_entry(display, button_left, button_right, button_down, button_up),
      m_messages(messages)
{
  m_buttons[0] = &button_left;
  m_buttons[1] = &button_right;
  m_buttons[2] = &button_down;
  m_buttons[3] = &button_up;
}

// layout
const int NUM_ITEMS_ON_SCREEN = 8;
const int ITEMS_Y = 35;
const int PADDING = 2;
const int ITEM_HEIGHT = 20;
const int ITEMS_INTERVAL = ITEM_HEIGHT + 2;

void c_multi_text_entry::run()
{

  if (!m_active)
    return;

  if (text_entry.is_active()) {
    text_entry.run();
    return;
  }

  bool selection_changed = false;
  bool offset_changed = false;

  if ((m_buttons[2]->is_pressed() || m_buttons[2]->is_held()) && m_message_idx > 0) {
    m_message_idx--;
    selection_changed = true;
  }
  if ((m_buttons[3]->is_pressed() || m_buttons[3]->is_held()) && m_message_idx < NUM_MESSAGES - 1) {
    m_message_idx++;
    selection_changed = true;
  }

  // ok
  if (m_buttons[0]->is_pressed()) {
    m_active = false;
    return;
  }

  // edit
  if (m_buttons[1]->is_pressed()) {
    text_entry.raise(m_messages.text[m_message_idx], 250);
    m_needs_redraw = true;
    return;
  }
  if (m_message_idx < m_message_offset){
    m_message_offset--;
    offset_changed = true;
  }
  if (m_message_idx > m_message_offset + NUM_ITEMS_ON_SCREEN - 1){
    m_message_offset++;
    offset_changed = true;
  }

  if (m_needs_redraw) {
    m_needs_redraw = false;
    selection_changed = true;
    m_display->clear(BACKGROUND);
    draw_heading(m_display, "MESSAGE MEMORY");
    draw_button_bar(m_display, OK_button, EDIT_button, UP_button, DOWN_button); 
  }

  if(offset_changed) {
    for (uint8_t idx = 0; idx < NUM_ITEMS_ON_SCREEN; ++idx) {
      m_display->fillRect(0, ITEMS_Y + (idx * ITEMS_INTERVAL) + 2, 16, DISPLAY_WIDTH, BACKGROUND);
    }
  }

  if(selection_changed) {
    for (uint8_t idx = 0; idx < NUM_ITEMS_ON_SCREEN; ++idx) {
      const uint8_t message_item_index = idx + m_message_offset;

      if (message_item_index < NUM_MESSAGES) {
        const int font_width = 12;
        const int display_width_chars = (DISPLAY_WIDTH - 10) / font_width;
        char buffer[50];
        memcpy(buffer, m_messages.text[message_item_index], display_width_chars);
        buffer[display_width_chars] = 0;
        const uint16_t text_width = strlen(buffer) * font_width;
        const uint16_t x = (DISPLAY_WIDTH - text_width) / 2;

        if (m_message_idx == message_item_index) {
          m_display->drawString(x, ITEMS_Y + (idx * ITEMS_INTERVAL) + 2, font_16x12, buffer,
                                MENU_ACTIVE, BACKGROUND);
        } else {
          m_display->drawString(x, ITEMS_Y + (idx * ITEMS_INTERVAL) + 2, font_16x12, buffer,
                                MENU_INACTIVE, BACKGROUND);
        }
      }
    }
  }

}

c_menu ::c_menu(ILI934X* display, button& button_left, button& button_right, button& button_down,
                button& button_up, const char* title, uint8_t& selection,
                const char* const* menu_items, const uint8_t num_selections)
    : m_display(display), m_selection(selection), m_menu_items(menu_items),
      m_num_selections(num_selections), m_title(title)
{
  m_buttons[0] = &button_left;
  m_buttons[1] = &button_right;
  m_buttons[2] = &button_down;
  m_buttons[3] = &button_up;
}

bool c_menu ::is_active()
{
  return m_active;
}

void c_menu ::raise()
{
  m_menu_item = m_selection;
  m_active = true;
  m_needs_redraw = true;
  m_offset = 0;
}

void c_menu ::run()
{
  const uint8_t num_items_on_screen = 7;

  if (m_selection > m_offset + num_items_on_screen - 1) {
    m_offset = m_menu_item - num_items_on_screen;
  }
  bool change_active_item = false;
  bool offset_changed = false;
  if (m_buttons[0]->is_pressed()) {
    m_active = false;
    m_selection = m_menu_item;
    m_ok = true;
    return; // ok
  }
  if (m_buttons[1]->is_pressed()) {
    m_active = false;
    m_ok = false;
    return; // cancel
  }
  if (m_buttons[2]->is_pressed() && m_menu_item > 0) {
    m_menu_item--;
    change_active_item = true;
  }
  if (m_buttons[3]->is_pressed() && m_menu_item < m_num_selections - 1) {
    m_menu_item++;
    change_active_item = true;
  }
  if (m_menu_item < m_offset) {
    m_offset--;
    offset_changed = true;
  }
  if (m_menu_item > m_offset + num_items_on_screen - 1) {
    m_offset++;
    offset_changed = true;
  }

  if (m_needs_redraw) {
    m_display->clear(BACKGROUND);
    draw_heading(m_display, m_title);
    draw_button_bar(m_display, OK_button, CANCEL_button, UP_button, DOWN_button);
    m_needs_redraw = false;
    change_active_item = true;
  }

  if(offset_changed) {
    for (uint8_t idx = 0; idx < num_items_on_screen; ++idx) {
        m_display->fillRect(0, 32 + ((idx) * 25), DISPLAY_WIDTH, 12, BACKGROUND);
    }
  }

  if(change_active_item) {
    for (uint8_t idx = 0; idx < num_items_on_screen; ++idx) {
      const uint8_t menu_item_index = idx + m_offset;
      if (menu_item_index < m_num_selections) {
        const bool active = m_menu_item == menu_item_index;
        uint16_t width = 12*strlen(m_menu_items[menu_item_index]);
        m_display->drawString((DISPLAY_WIDTH-width)/2, 32 + ((idx) * 25), font_16x12, m_menu_items[menu_item_index],
                              active ? MENU_ACTIVE : MENU_INACTIVE, BACKGROUND);
      }
    }
  }

}

c_int_entry ::c_int_entry(ILI934X* display, button& button_left, button& button_right,
                          button& button_down, button& button_up, const char* title, int& value,
                          int min, int max)
    : m_display(display), m_value(value), m_min(min), m_max(max), m_title(title)
{
  m_buttons[0] = &button_left;
  m_buttons[1] = &button_right;
  m_buttons[2] = &button_down;
  m_buttons[3] = &button_up;
}

bool c_int_entry ::is_active()
{
  return m_active;
}

void c_int_entry ::raise()
{
  m_edit_value = m_value;
  m_active = true;
  m_needs_redraw = true;
  m_full_redraw = true;
}

void c_int_entry ::run()
{

  if (m_buttons[0]->is_pressed()) {
    m_active = false;
    m_value = m_edit_value;
    m_ok = true;
    return; // ok
  }
  if (m_buttons[1]->is_pressed()) {
    m_active = false;
    m_ok = true;
    return; // cancel
  }
  if ((m_buttons[2]->is_pressed() || m_buttons[2]->is_held()) && m_edit_value < m_max) {
    m_edit_value++;
    m_needs_redraw = true;
  }
  if ((m_buttons[3]->is_pressed() || m_buttons[3]->is_held()) && m_edit_value > m_min) {
    m_edit_value--;
    m_needs_redraw = true;
  }

  if (m_needs_redraw) {
    if (m_full_redraw) {
      m_full_redraw = false;
      m_display->clear(BACKGROUND);
      draw_heading(m_display, m_title);
      draw_button_bar(m_display, OK_button, CANCEL_button, UP_button, DOWN_button);
    }

    char buffer[11];
    snprintf(buffer, 11, "%i", m_edit_value);
    int16_t width = strlen(buffer) * 12;
    m_display->drawString((DISPLAY_WIDTH - width) / 2, 100, font_16x12, buffer, COLOUR_WHITE,
                          COLOUR_BLACK);

    m_needs_redraw = false;
  }
}

