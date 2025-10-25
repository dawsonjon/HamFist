#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cassert>

#include <algorithm>
#include "cw_dsp.h"
#include "fft.h"
#include "utils.h"

#define LOGGING

#ifdef LOGGING
#ifdef ARDUINO
  #include <Arduino.h>
  #define DEBUG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #include <cstdio>
  #define DEBUG_PRINTF(...)  std::printf(__VA_ARGS__)
#endif
#else
  #define DEBUG_PRINTF(...)
#endif



// Function to generate a window
void generate_window(int32_t *window, int size) {
  for (uint16_t i = 0; i < size; i++) 
  {
    float multiplier = 0.5 * (1 - cosf(2*M_PI*i/size));
    window[i] = multiplier * 65536;
  }
}

//apply a flat-topped window to reduce scalloping loss
void apply_window(int16_t i[], int16_t q[], int32_t window[], const uint16_t n)
{
  for(uint16_t idx=0; idx<n; ++idx)
  {
    assert(idx >= 0);
    assert(idx < FRAME_SIZE);
    int32_t i_val = i[idx];
    int32_t q_val = q[idx];
    i_val = window[idx] * i_val / 65536;
    q_val = window[idx] * q_val / 65536;
    i[idx] = i_val;
    q[idx] = q_val;
  }
}

void c_cw_dsp :: decode(uint16_t cluster, std::string text)
{
  DEBUG_PRINTF("decode on channel %u %s\n", cluster, text.c_str());
}

static void max_magnitude(uint32_t magnitude[], uint8_t start_bin, uint8_t stop_bin, uint16_t &max_bin, uint32_t &max) 
{
    max = 0;
    max_bin = 0;
    for(uint16_t idx=start_bin; idx<stop_bin; ++idx) {
      assert(idx >= 0);
      assert(idx < FRAME_SIZE/2);
      if(magnitude[idx] > max) {
        max_bin = idx;
        max = magnitude[idx];
      }
    }
}

void c_cw_dsp :: flush()
{
  for (uint8_t cluster = 0; cluster < NUM_CLUSTERS; cluster++) {
    print_element("decode_bins", cluster * CLUSTER_SIZE);
    clusters[cluster].decoder.decode(clusters[cluster].observations, clusters[cluster].num_observations);
    decode(cluster, clusters[cluster].decoder.get_text());
    clusters[cluster].num_observations = 0;
    clusters[cluster].duration = 0;
  }
}

void c_cw_dsp :: process_clusters(uint32_t threshold)
{

  for (uint8_t cluster = 0; cluster < NUM_CLUSTERS; cluster++) {

    uint8_t start_bin = cluster * CLUSTER_SIZE;
    uint8_t stop_bin = start_bin + CLUSTER_SIZE - 1;

    uint16_t max_bin;
    uint32_t max;
    max_magnitude(magnitude, start_bin, stop_bin, max_bin, max);

    //measure signal present periods
    bool value = max > threshold;
    clusters[cluster].duration++;
    if(value != clusters[cluster].value) {
      s_observation observation = {clusters[cluster].value, FRAME_MS * clusters[cluster].duration};
      clusters[cluster].duration = 0;
      clusters[cluster].value = value;
      clusters[cluster].observations[clusters[cluster].num_observations++] = observation;
    }

    //decode messages
    if((clusters[cluster].num_observations && clusters[cluster].duration == TIMEOUT) || (clusters[cluster].num_observations == OBSERVATION_BUFFER_SIZE)) {
      print_element("decode_bins", cluster * CLUSTER_SIZE);
      clusters[cluster].decoder.decode(clusters[cluster].observations, clusters[cluster].num_observations);
      std::string text = clusters[cluster].decoder.get_text();
      decode(cluster, text);
      clusters[cluster].num_observations = 0;
      clusters[cluster].duration = 0;
    }

  }
}

void c_cw_dsp :: process_frame()
{
  print_frame("magnitudes", magnitude);
  // estimate noise with median
  std::vector<double> tmp(magnitude, magnitude+FRAME_SIZE/2);
  std::nth_element(tmp.begin(), tmp.begin()+FRAME_SIZE/4, tmp.end());
  uint32_t noise_estimate = tmp[FRAME_SIZE/4];
  print_element("noise_estimate", noise_estimate);

  //calculate threshold
  uint32_t threshold = 32+noise_estimate * 6;
  smoothed_threshold = ((smoothed_threshold << 5) - smoothed_threshold + threshold) >> 5;
  print_element("threshold", smoothed_threshold);

  //process active signals
  process_clusters(smoothed_threshold);
  frame_count++;

}


c_cw_dsp :: c_cw_dsp()
{
  //clear buffer
  for(uint16_t idx = 0; idx<FRAME_SIZE; idx++)
  {
    i[idx]=0; 
    q[idx]=0;
  }
  //initialise clusters
  for (uint8_t cluster = 0; cluster < NUM_CLUSTERS; cluster++) {
    clusters[cluster].duration = 0;
    clusters[cluster].value = false;
    clusters[cluster].observations[clusters[cluster].num_observations++];
    clusters[cluster].num_observations = 0;
  }
  generate_window(window, FRAME_SIZE);
  smoothed_threshold = 0;
  frame_count = 0;
}

void c_cw_dsp :: process_sample(int16_t sample)
{
  //resample from 15000Hz to 7500 Hz
  uint32_t sample_ratio_f16 = 7500 * 65536 / 15000;
  sample_number_f16 += sample_ratio_f16;
  if(sample_number_f16 < 65536) return;
  sample_number_f16 -= 65536;
  i[frequency_count] = sample;    

  //sample rate is 15000, 1st Nyquist -3750 to +3750, but only 0 to ~2500Hz is used.
  //at 7500Hz, each bin in a 64 point FFT represents 117Hz
  frequency_count++;
  if(frequency_count == FRAME_SIZE)
  {
    frequency_count = 0;
    apply_window(i, q, window, FRAME_SIZE);
    fixed_fft(i, q, 6);

    uint32_t total = 0;
    for(uint16_t idx=0; idx<FRAME_SIZE/2; idx++)
    {
      magnitude[idx] = rectangular_2_magnitude(i[idx], q[idx]);
    }
    process_frame();
  }
}

//********** debug code from here down **********

void c_cw_dsp :: print_frame(const char filename[], uint32_t frame[])
{
  #ifdef LOG_TO_FILE
  FILE *outf = fopen(filename, "a");
  for(uint16_t f=0; f<FRAME_SIZE/2; f++)
  {
    fprintf(outf, "%i ", (int)frame[f]);
    fprintf(outf, " ");
  }
  fprintf(outf, "\n");
  fclose(outf);
  #endif
}

void c_cw_dsp :: print_element(const char filename[], uint32_t element)
{
  #ifdef LOG_TO_FILE
  FILE *outf = fopen(filename, "a");
  fprintf(outf, "%i", (int)element);
  fprintf(outf, "\n");
  fclose(outf);
  #endif
}

void c_cw_dsp :: print_bool(const char filename[], bool element)
{
  #ifdef LOG_TO_FILE
  FILE *outf = fopen(filename, "a");
  fprintf(outf, "%i", (int)element);
  fprintf(outf, "\n");
  fclose(outf);
  #endif
}
