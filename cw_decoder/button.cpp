//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: button.cpp
// description: button handling class
// License: MIT
//


#include "button.h"
#include "pico/stdlib.h"

#include <cstdio>

button ::button(uint8_t gpio_num) : gpio_num(gpio_num)
{
  button_state = up;
  time_pressed = 0;
  gpio_init(gpio_num);
  gpio_set_dir(gpio_num, GPIO_IN);
  gpio_pull_up(gpio_num);
}

void button ::update_state()
{
  if (button_state == up) {
    if (!gpio_get(gpio_num)) // on
    {
      time_pressed = time_us_32();
      button_state = down;
    }
  } else if (button_state == down) {
    if ((time_us_32() - time_pressed) > (500 * 1000)) {
      button_state = held;
    } else if (gpio_get(gpio_num)) // off
    {
      if ((time_us_32() - time_pressed) > (25 * 1000)) {
        button_state = pressed;
      } else {
        button_state = up;
      }
    }
  } else if (button_state == pressed) {
    button_state = up;
  } else if (button_state == held) {
    if (gpio_get(gpio_num)) // off
    {
      button_state = up;
    }
  }
}

bool button ::is_pressed()
{
  update_state();
  return button_state == pressed;
}

bool button ::is_held()
{
  update_state();
  return button_state == held;
}
