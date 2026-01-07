//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: button.h
// description: Button handling class
// License: MIT
//

#ifndef __button__
#define __button__

#include <cstdint>

class button
{
public:
  button(uint8_t gpio_num);
  bool is_pressed();
  bool is_held();

public:
  uint8_t gpio_num;
  void update_state();

  enum e_button_state
  {
    up,
    down,
    pressed,
    held
  };
  e_button_state button_state = up;
  uint32_t time_pressed = 0;
};

#endif
