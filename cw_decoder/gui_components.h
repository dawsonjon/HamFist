//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: sstv_encoder.h
// description: GUI components for ILI934X
// License: MIT
//


#ifndef __c_text_entry__
#define __c_text_entry__

#include <cstddef>
#include <cstdint>
#include <string>

#include "button.h"
#include "ili934x.h"

enum e_state
{
  INACTIVE,
  INITIALISE,
  GETCHAR1_DRAW,
  GETCHAR1_GET,
  GETCHAR2_DRAW,
  GETCHAR2_GET,
  GETCHAR3_DRAW,
  GETCHAR3_GET,
  NEXT_CHAR,
  NEXT_CHAR_DRAW
};
struct s_messages
{
  char text[16][251];
};

void draw_button_bar(ILI934X *display, const char* btn1, const char* btn2, const char* btn3, const char* btn4);
void draw_button_bar(ILI934X *display, const uint16_t* btn1, const uint16_t* btn2, const uint16_t* btn3, const uint16_t* btn4);

const uint8_t NUM_MESSAGES = 16;

class c_text_entry
{

private:
  ILI934X* m_display;
  button* m_buttons[4];
  uint8_t m_sel1 = 0;
  uint8_t m_sel2 = 0;
  uint8_t m_sel3 = 0;
  uint16_t m_cursor = 0;
  char* m_string = NULL;
  uint16_t m_n = 0;
  std::string m_autocomplete_suggestion;
  std::string m_second;
  std::string m_third;
  e_state m_state = INACTIVE;
  uint8_t m_message_offset = 0;
  uint8_t m_message_idx = 0;

public:
  c_text_entry(ILI934X* display, button& button_left, button& button_right, button& button_down,
               button& button_up);
  bool is_active();
  void raise(char* string, uint16_t n);
  void run();
  void choose_message();
};

class c_multi_text_entry
{

private:
  ILI934X* m_display;
  button* m_buttons[4];
  uint8_t m_message_offset = 0;
  uint8_t m_message_idx = 0;
  bool m_active = false;
  bool m_needs_redraw = true;
  c_text_entry text_entry;
  s_messages& m_messages;

public:
  c_multi_text_entry(ILI934X* display, button& button_left, button& button_right,
                     button& button_down, button& button_up, s_messages& messages);
  uint8_t get_selection() { return m_message_idx; }
  bool is_active();
  void raise();
  void run();
};

class c_menu
{

private:
  ILI934X* m_display;
  button* m_buttons[4];
  uint8_t m_offset = 0;
  bool m_active = false;
  bool m_needs_redraw = true;
  uint16_t m_num_selections;
  const char* const* m_menu_items;
  uint8_t& m_selection;
  uint8_t m_menu_item = 0;
  const char* m_title;
  bool m_ok;

public:
  c_menu(ILI934X* display, button& button_left, button& button_right, button& button_down,
         button& button_up, const char* title, uint8_t& selection, const char* const* menu_items,
         const uint8_t num_selections);
  bool is_active();
  bool is_ok() { return m_ok; }
  void raise();
  void run();
};

class c_int_entry
{

private:
  ILI934X* m_display;
  button* m_buttons[4];
  bool m_active = false;
  bool m_needs_redraw = true;
  bool m_full_redraw = true;
  int& m_value;
  int m_edit_value;
  int m_min;
  int m_max;
  const char* m_title;
  bool m_ok;

public:
  c_int_entry(ILI934X* display, button& button_left, button& button_right, button& button_down,
              button& button_up, const char* title, int& value, int min, int max);
  bool is_active();
  bool is_ok() { return m_ok; }
  void raise();
  void run();
};

#endif
