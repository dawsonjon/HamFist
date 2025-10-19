#ifndef FT8_DECODER_H__
#define FT8_DECODER_H__

#include <cstdint>

#include "ft8_decoder.h"

class c_ft8_decoder
{

  uint16_t frequency_count=0; //0 -> 1023
  uint16_t time_count=0; //0 -> 92
  uint32_t sample_number_f16=0;
  int16_t i[1024];
  int16_t q[1024];
  int32_t window[1024];
  int16_t noise_estimate=0; 
  uint8_t log_noise_estimate=0; 

  uint8_t waterfall[93][512];
  uint32_t signal_strength[20][512];

  void print_waterfall();
  void print_signal_strength();
  void costas_search();
  void get_next_candidate(uint16_t &best_t, uint16_t &best_f);
  void fsk_to_symbol(uint8_t *tones, int8_t bits[]);
  void demodulate_fsk(uint16_t t, uint16_t f, int8_t bits[]);
  void process_frame();

  public:

  c_ft8_decoder();
  void process_sample(int16_t sample);

};

#endif
