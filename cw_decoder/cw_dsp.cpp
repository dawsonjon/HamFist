#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>

#include <algorithm>
#include "cw_dsp.h"
#include "fft.h"
#include "utils.h"

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
    int32_t i_val = i[idx];
    int32_t q_val = q[idx];
    i_val = window[idx] * i_val / 65536;
    q_val = window[idx] * q_val / 65536;
    i[idx] = i_val;
    q[idx] = q_val;
  }
}

void c_cw_dsp :: decode(uint8_t bin, bool state, uint16_t duration)
{
  printf("output_state : %u %u %u\n", bin, state, duration);
}

void c_cw_dsp :: cluster_detections(uint32_t threshold)
{

  //refresh existing clusters
  for (s_cluster &cluster : clusters) {
      int start = cluster.bin>0?cluster.bin-1:cluster.bin;
      int stop = cluster.bin<FRAME_SIZE/2-1?cluster.bin+1:cluster.bin;

      cluster.value = magnitude[cluster.bin] > threshold;

      uint32_t max_magnitude = 0;
      for(uint16_t bin=start; bin<=stop; ++bin) {
        if(magnitude[bin] > max_magnitude) max_magnitude = magnitude[bin];
        magnitude[bin]=0;
      }
      //cluster.value = max_magnitude > threshold;
  }

  //find new clusters
  while(clusters.size() < MAX_DECODERS) { 
    uint32_t max = 0;
    uint16_t max_bin = 0;
    uint16_t num_detections = 0;
    for(uint16_t idx=1; idx<FRAME_SIZE/2; ++idx) {
      if(magnitude[idx] > threshold ) {
        num_detections++;
        if(magnitude[idx] > max) {
          max_bin = idx;
          max = magnitude[idx];
          if(idx>0) magnitude[idx] = 0;
          magnitude[idx+1] = 0;
          if(idx+1 < FRAME_SIZE/2) magnitude[idx-1] = 0;
        }
      }
    }
    if(num_detections == 0) break;
    printf("starting %u\n", max_bin);
    s_cluster cluster;
    cluster.bin = max_bin;
    cluster.value = true;
    cluster.last_value = true;
    cluster.duration = 0;
    cluster.timeout = TIMEOUT;
    cluster.num_observations = 0;
    clusters.push_back(cluster);
  }
  
}

void c_cw_dsp :: process_clusters(uint32_t threshold)
{

  for (auto &cluster : clusters) {

    //measure duration of signal present and signal absent periods.
    cluster.duration++;
    if(cluster.value != cluster.last_value) {
      s_observation observation = {cluster.last_value, cluster.duration};
      if(cluster.num_observations < OBSERVATION_BUFFER_SIZE) cluster.observations[cluster.num_observations++] = observation;
      cluster.duration = 0;
      cluster.last_value = cluster.value;

      //decode when buffer is full
      if( cluster.bin==22 && cluster.value == 0 && cluster.num_observations > OBSERVATION_BUFFER_SIZE-3 ) {
        printf("Decode %u:\n", cluster.bin);
        decode_cw(cluster.observations, cluster.num_observations, 5);
        cluster.num_observations = 0;
      }

      //for(s_observation observation : cluster->observations) {
        //printf("{%u %f} ", observation.mark, observation.duration);
      //}
      //printf("\n");
    }

    //decrement timeout
    if(cluster.value) {
      cluster.timeout = TIMEOUT; //keep alive
    } else {
      if(cluster.timeout) cluster.timeout--;
    }

    //force decode on timeout
    if(cluster.timeout == 0 ) {
      printf("Stopping %u:\n", cluster.bin);
      //decode_cw(cluster.observations, cluster.num_observations, 5);
      cluster.num_observations = 0;
    }
    

  }

  //remove stale clusters
  for (auto cluster = clusters.begin(); cluster != clusters.end(); ) {
    if(cluster->timeout == 0)
      cluster = clusters.erase(cluster);
    else ++cluster;
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
  uint32_t threshold = noise_estimate * 5;
  smoothed_threshold = ((smoothed_threshold << 2) - smoothed_threshold + threshold) >> 2;
  print_element("threshold", smoothed_threshold);

  //find live signals
  cluster_detections(smoothed_threshold);

  //process active signals
  process_clusters(smoothed_threshold);

}

void c_cw_dsp :: print_frame(const char filename[], uint32_t frame[])
{
  FILE *outf = fopen(filename, "a");
  for(uint16_t f=0; f<FRAME_SIZE/2; f++)
  {
    fprintf(outf, "%i ", (int)frame[f]);
    fprintf(outf, " ");
  }
  fprintf(outf, "\n");
  fclose(outf);
}

void c_cw_dsp :: print_element(const char filename[], uint32_t element)
{
  FILE *outf = fopen(filename, "a");
  fprintf(outf, "%i", (int)element);
  fprintf(outf, "\n");
  fclose(outf);
}

void c_cw_dsp :: print_bool(const char filename[], bool element)
{
  FILE *outf = fopen(filename, "a");
  fprintf(outf, "%i", (int)element);
  fprintf(outf, "\n");
  fclose(outf);
}

c_cw_dsp :: c_cw_dsp()
{
  //clear buffer
  for(uint16_t idx = 0; idx<FRAME_SIZE; idx++)
  {
    i[idx]=0; 
    q[idx]=0;
  }
  generate_window(window, FRAME_SIZE);
  smoothed_threshold = 0;
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

