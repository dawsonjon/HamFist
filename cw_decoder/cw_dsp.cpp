#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cassert>

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

void c_cw_dsp :: decode(uint8_t bin, bool state, uint16_t duration)
{
  printf("output_state : %u %u %u\n", bin, state, duration);
}

static uint16_t clear_surrounding_bins(uint32_t magnitude[], uint16_t bin, uint16_t &max_magnitude_bin) 
{
    int start = std::max(0,              (int)bin-(int)CLUSTER_WIDTH);
    int stop =  std::min(FRAME_SIZE/2-1, (int)bin+(int)CLUSTER_WIDTH);

    uint32_t max_magnitude = 0;
    max_magnitude_bin = bin;
    for(uint16_t bin=start; bin<=stop; ++bin) {
      assert(bin >= 0);
      assert(bin < FRAME_SIZE/2);
      if(magnitude[bin] > max_magnitude){
        max_magnitude = magnitude[bin];
        max_magnitude_bin = bin;
      }
      magnitude[bin]=0;
    }

    return max_magnitude;
}

static uint16_t max_magnitude(uint32_t magnitude[], uint32_t threshold, uint16_t &max_bin, uint32_t &max) 
{
    max = 0;
    max_bin = 0;
    uint16_t num_detections = 0;
    for(uint16_t idx=1; idx<FRAME_SIZE/2; ++idx) {
      assert(idx >= 0);
      assert(idx < FRAME_SIZE/2);
      if(magnitude[idx] > threshold ) {
        num_detections++;
        if(magnitude[idx] > max) {
          max_bin = idx;
          max = magnitude[idx];
        }
      }
    }
    return num_detections;
}

void c_cw_dsp :: cluster_detections(uint32_t threshold)
{

  //refresh existing clusters
  for (s_cluster &cluster : clusters) {
      uint16_t max_magnitude_bin;
      uint32_t max_magnitude = clear_surrounding_bins(magnitude, cluster.bin, max_magnitude_bin);

      //track frequency changes
      if(max_magnitude_bin > cluster.bin) {
        cluster.move_up_count++;
        cluster.move_down_count = 0;
      } else if(max_magnitude_bin < cluster.bin) {
        cluster.move_up_count = 0;
        cluster.move_down_count++;
      } else {
        cluster.move_up_count = 0;
        cluster.move_down_count = 0;
      }
      if(cluster.move_up_count > 10) {
        printf("moving up %u\n", cluster.bin);
        cluster.bin++;
        cluster.move_up_count = 0;
        cluster.move_down_count = 0;
      } 
      if(cluster.move_down_count > 10) {
        printf("moving down %u\n", cluster.bin);
        cluster.bin--;
        cluster.move_up_count = 0;
        cluster.move_down_count = 0;
      }

      cluster.value = max_magnitude > threshold;
  }

  //find new clusters
  while(clusters.size() < MAX_DECODERS) { 
    uint16_t max_bin;
    uint32_t max;

    //if no detections found break
    if(max_magnitude(magnitude, threshold, max_bin, max) == 0) break;

    //clear surrounding bins
    uint16_t discard;
    clear_surrounding_bins(magnitude, max_bin, discard);

    //create a new cluster
    printf("starting %u @frame %u\n", max_bin, frame_count);
    s_cluster cluster;
    cluster.bin = max_bin;
    cluster.value = true;
    cluster.last_value = true;
    cluster.duration = 0;
    cluster.timeout = TIMEOUT;
    cluster.num_observations = 0;
    cluster.move_up_count = 0;
    cluster.move_down_count = 0;
    cluster.frame_count = frame_count;
    clusters.push_back(cluster);
  }
  
}

void c_cw_dsp :: flush()
{

  for (auto &cluster : clusters) {

      printf("completing: %u\n", cluster.bin);
      print_element("decode_bins", cluster.bin);
      cluster.decoder.decode(cluster.observations, cluster.num_observations);
      printf("Decode %u %u %u: %s\n", cluster.bin, cluster.frame_count, cluster.num_observations, cluster.decoder.get_text().c_str());
  }

  clusters.clear();

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
      if(cluster.num_observations == OBSERVATION_BUFFER_SIZE) {
        print_element("decode_bins", cluster.bin);
        cluster.decoder.decode(cluster.observations, cluster.num_observations);
        printf("Decode %u %u %u: %s\n", cluster.bin, cluster.frame_count, cluster.num_observations, cluster.decoder.get_text().c_str());
        cluster.num_observations = 0;
      }
    }

    //decrement timeout
    if(cluster.value) {
      cluster.timeout = TIMEOUT; //keep alive
    } else {
      if(cluster.timeout) cluster.timeout--;
    }

    //force decode on timeout
    if(cluster.timeout == 0 ) {
      printf("Stopping %u\n", cluster.bin);
      float active_time = frame_count - cluster.frame_count;
      if(cluster.num_observations/active_time > 0.01) {
        print_element("decode_bins", cluster.bin);
        cluster.decoder.decode(cluster.observations, cluster.num_observations);
        printf("Decode %u %u: %s\n", cluster.num_observations, cluster.bin, cluster.decoder.get_text().c_str());
        cluster.num_observations = 0;
      }
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
  uint32_t threshold = noise_estimate * 6;
  smoothed_threshold = ((smoothed_threshold << 5) - smoothed_threshold + threshold) >> 5;
  print_element("threshold", smoothed_threshold);

  //find live signals
  cluster_detections(smoothed_threshold);

  //process active signals
  process_clusters(smoothed_threshold);
  frame_count++;

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

