//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: cw_encoder.cpp
// description: class to encode CW
// License: MIT
//

#include "cw_encoder.h"

#include <cmath>
#include <cstring>
#include <unordered_map>
#include <string>

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

c_cw_encoder :: c_cw_encoder(double Fs_Hz)
{
    m_done = false;
    m_Fs_Hz = Fs_Hz;
    m_phase = 0;
    m_string = NULL;
    m_samples_to_send = 0;
    m_gap_samples = 0;


    for(uint16_t idx=0; idx<1024; ++idx)
    {
      m_sin_table[idx] = round(32767.0f*sin(2.0f*idx*M_PI/1024.0f));
    }
}

int16_t c_cw_encoder :: get_sample()
{

    //replenish samples
    if((!m_samples_to_send) && (!m_gap_samples)) {
      get_tone();
    }

    const uint32_t step = (static_cast<uint64_t>(m_frequency_Hz)<<32)/m_Fs_Hz;
    int32_t sample =  m_sin_table[m_phase >> 22];
    static uint8_t timer = 0;
    m_phase += step;
    
    int32_t shape;
    if(m_samples_to_send) {  
      m_samples_to_send--;
      if(timer == 100)
      {
        shape = 32767;
      }
      else
      {
        shape = (18307 + timer/2*(-18023 + timer/2*(11090 + (timer/2*-148))))>>8;
        timer++;
      }
    } else if(m_gap_samples) {
      m_gap_samples--;
      if(timer == 0)
      {
        shape = 0;
      }
      else
      {
        shape = (18307 + timer/2*(-18023 + timer/2*(11090 + (timer/2*-148))))>>8;
        timer--;
      }
    }

    sample = (sample * shape) >> 15;
    return sample;
}

static const std::unordered_map<char, std::string> MORSE_REVERSE = {
    {'A', ".-"},
    {'B', "-..."},
    {'C', "-.-."},
    {'D', "-.."},
    {'E', "."},
    {'F', "..-."},
    {'G', "--."},
    {'H', "...."},
    {'I', ".."},
    {'J', ".---"},
    {'K', "-.-"},
    {'L', ".-.."},
    {'M', "--"},
    {'N', "-."},
    {'O', "---"},
    {'P', ".--."},
    {'Q', "--.-"},
    {'R', ".-."},
    {'S', "..."},
    {'T', "-"},
    {'U', "..-"},
    {'V', "...-"},
    {'W', ".--"},
    {'X', "-..-"},
    {'Y', "-.--"},
    {'Z', "--.."},

    // numbers
    {'0', "-----"},
    {'1', ".----"},
    {'2', "..---"},
    {'3', "...--"},
    {'4', "....-"},
    {'5', "....."},
    {'6', "-...."},
    {'7', "--..."},
    {'8', "---.."},
    {'9', "----."},

    // common punctuation
    {'.', ".-.-.-"},
    {',', "--..--"},
    {'?', "..--.."},
    {'/', "-..-."},
    {'=', "-...-"},
    {'-', "-....-"},
    {':', "---..."},
    {'(', "-.--."},
    {')', "-.--.-"},
};

void c_cw_encoder :: set_string(const char * string)
{
  m_string = string;
  m_cursor = 0;
  m_done = false;
}

void c_cw_encoder :: set_wpm(uint16_t wpm)
{
  m_dit_samples = (m_Fs_Hz * 1200.0) / (1000.0*wpm);
}

void c_cw_encoder :: set_frequency_Hz(uint32_t frequency_Hz)
{
  m_frequency_Hz = frequency_Hz;
}

bool c_cw_encoder :: is_done()
{
  return m_done;
}

void c_cw_encoder :: get_tone()
{

  //we have no dots and dashes to send fetch a char
  if(m_dots_and_dashes.size() == 0) {
    //we have no string, send silence
    if(m_string == NULL || m_cursor >= strlen(m_string)) {
      m_samples_to_send = 0;
      m_gap_samples = 1;
      m_done = true;
      return;
    }

    //skip spaces at beginning of string
    m_char = toupper(m_string[m_cursor++]);
    while(m_char == ' ') {
      m_char = toupper(m_string[m_cursor++]);
    }

    //convert char to dots and dashes
    auto it = MORSE_REVERSE.find(m_char);
    if (it != MORSE_REVERSE.end()) m_dots_and_dashes = it->second;
  }

  //send either a dot or a dash
  char dot_or_dash = m_dots_and_dashes[0];
  m_dots_and_dashes.erase(0, 1);
  if(dot_or_dash == '.') {
    m_samples_to_send = m_dit_samples;
  } else if(dot_or_dash == '-') {
    m_samples_to_send = m_dit_samples * 3;
  }

  //send a gap
  if(m_dots_and_dashes.size()) {
    //symbol gap
    m_gap_samples = m_dit_samples;
  } else {
    if(m_string[m_cursor] != ' ') {
      //letter gap
      m_gap_samples = 3*m_dit_samples;
    } else {
      //word gap
      m_gap_samples = 7*m_dit_samples;
      m_cursor++;
    }
  }

}
