//  _  ___  _   _____ _     _
// / |/ _ \/ | |_   _| |__ (_)_ __   __ _ ___
// | | | | | |   | | | '_ \| | '_ \ / _` / __|
// | | |_| | |   | | | | | | | | | | (_| \__ \.
// |_|\___/|_|   |_| |_| |_|_|_| |_|\__, |___/
//                                  |___/
//
// Copyright (c) Jonathan P Dawson 2025
// filename: ADCAudio.h
// description: Audio Sink using Pico built-in ADC
// License: MIT
//

#ifdef ARDUINO_ARCH_RP2040

#ifndef ADC_AUDIO_H
#define ADC_AUDIO_H

#include "hardware/adc.h"
#include "hardware/dma.h"
#include <stdio.h>

const uint32_t ADC_BLOCK_SIZE = 2048;
const uint32_t ADC_DMA_SIZE = ADC_BLOCK_SIZE * 4;

class ADCAudio
{

public:
  ADCAudio();
  void begin(const uint8_t audio_pin, const uint32_t audio_sample_rate);
  void end();
  int16_t* input_samples();

private:
  int adc_dma;
  dma_channel_config cfg;
  uint16_t samples[2][8192];
  uint8_t buffer_number = 0;
  uint16_t dc = 0;
};

#endif
#endif
