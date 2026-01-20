//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: PWMAudio.h
// description: DMA based PWM Audio Outpus
// License: MIT
//

#ifdef ARDUINO_ARCH_RP2040

#ifndef PWM_AUDIO_H
#define PWM_AUDIO_H

#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

class PWMAudio
{

public:
  PWMAudio();
  void end();
  void begin(uint8_t audio_pin, const uint32_t audio_sample_rate, const uint32_t cpuFrequencyHz);
  void output_samples(const uint16_t samples[], const uint16_t len);

private:
  int pwm_dma;
  int dma_timer;
  dma_channel_config audio_cfg;
  int audio_pwm_slice_num;
};

#endif
#endif
