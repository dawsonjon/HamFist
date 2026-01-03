#ifndef __c_text_entry__
#define __c_text_entry__

#include <cstdint>
#include <cstddef>
#include <string>

#include "ili934x.h"
#include "button.h"

enum e_state {INACTIVE, INITIALISE, GETCHAR1_DRAW, GETCHAR1_GET, GETCHAR2_DRAW, GETCHAR2_GET, GETCHAR3_DRAW, GETCHAR3_GET, NEXT_CHAR};
struct s_messages {
  char text[16][251];
};

const uint8_t NUM_MESSAGES = 16;

class c_text_entry
{

  private:

  ILI934X *m_display;
  button *m_buttons[4];
  uint8_t m_sel1 = 0;
  uint8_t m_sel2 = 0;
  uint8_t m_sel3 = 0;
  uint16_t m_cursor = 0;
  char *m_string = NULL;
  uint16_t m_n=0;
  std::string m_autocomplete_suggestion;
  std::string m_second;
  std::string m_third;
  e_state m_state = INACTIVE;
  uint8_t m_message_offset = 0;
  uint8_t m_message_idx = 0;

  public:

  c_text_entry(ILI934X *display, button &button_left, button &button_right, button &button_down, button &button_up);
  bool is_active();
  void raise(char* string, uint16_t n);
  void run();
  void choose_message();
  s_messages m_messages;

};

class c_multi_text_entry
{

  private:

  ILI934X *m_display;
  button *m_buttons[4];
  uint8_t m_message_offset = 0;
  uint8_t m_message_idx = 0;
  bool m_active = false;
  bool m_needs_redraw = true;
  c_text_entry text_entry;

  public:

  c_multi_text_entry(ILI934X *display, button &button_left, button &button_right, button &button_down, button &button_up);
  bool is_active();
  void raise();
  void run();
  s_messages m_messages;

};

#endif
