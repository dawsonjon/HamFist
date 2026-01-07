//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: sstv_encoder.h
// description: class to encode letters into morse code audio
// License: MIT
//

#ifndef __SSTV_ENCODER_H__
#define __SSTV_ENCODER_H__

#include <cstdint>
#include <string>

class c_cw_encoder
{

private:
  double m_Fs_Hz;
  uint32_t m_phase;
  int16_t m_sin_table[1024];
  const char* m_string;
  char m_char;
  uint32_t m_gap_samples;
  uint32_t m_samples_to_send;
  uint16_t m_cursor;
  uint32_t m_frequency_Hz;
  uint32_t m_dit_samples;
  bool m_done;
  std::string m_dots_and_dashes;
  void get_tone();

public:
  c_cw_encoder(double fs_Hz);
  int16_t get_sample();
  void set_string(const char* string);
  void set_wpm(uint16_t wpm);
  void set_frequency_Hz(uint32_t frequency_Hz);
  bool is_done();
  uint16_t get_cursor();
  void terminate();
};

#endif
