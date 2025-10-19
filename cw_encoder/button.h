#ifdef ARDUINO_ARCH_RP2040

#ifndef __button__
#define __button__

#include <cstdint>

class c_button
{
  public:
  c_button(uint8_t gpio_num);
  bool is_pressed();
  bool is_held();
  public:
  uint8_t gpio_num;
  void update_state();

  enum e_button_state {up, down, pressed, held};
  e_button_state button_state = up;
  uint32_t time_pressed = 0;

};

#endif
#endif
