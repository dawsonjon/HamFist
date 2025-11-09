#include "cw_classifier.h"
#include <vector>

//#define LOGGING

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

c_morse_timing_classifier::c_morse_timing_classifier() {
  reset();
}

void c_morse_timing_classifier::reset()
{
  dot_mu = 20.0f;
  dash_mu = dot_mu*3.0f;
  gap1_mu = dot_mu*1.0f;
  gap3_mu = dot_mu*5.0f;
  gap7_mu = dot_mu*7.0f;
  dot_sigma = 10.0f;
  dash_sigma = dot_sigma;
  gap1_sigma = dot_sigma;
  gap3_sigma = dot_sigma;
  gap7_sigma = dot_sigma;
  for(int idx=0; idx<BIN_MAX/BIN_WIDTH; ++idx) { on_histogram[idx]=0; }
  for(int idx=0; idx<BIN_MAX/BIN_WIDTH; ++idx) { off_histogram[idx]=0; }
}

static float log_gaussian(float x, float mu, float sigma)
{
    return -0.5f * pow((x - mu) / sigma, 2.0f);
}

static float mean(const float* data, size_t n) {
    if (n == 0) return 0.0f;

    float sum = 0.0f;
    for (size_t idx = 0; idx < n; idx++) {
        sum += data[idx];
    }
    return sum / static_cast<float>(n);
}

static float histogram_mean(const int data[], const int begin, const int end, int bin_width) {
    float sum_data = 0.0f;
    float sum_counts = 0.0f;
    for (size_t idx = begin; idx <= end; idx++) {
	float bin_centre = (1.0f * idx * bin_width) + (0.5f*bin_width);
        sum_data += data[idx] * bin_centre;
        sum_counts += data[idx];
        DEBUG_PRINTF("sum_data %f sum_mean %f\n", sum_data, sum_counts);
    }
    DEBUG_PRINTF("sum_data %f sum_mean %f\n", sum_data, sum_counts);
    return sum_data / sum_counts;
}

static float histogram_stddev(const float mean, const int data[], const int begin, const int end, int bin_width) {
    float sum_data_squared = 0.0f;
    float sum_counts = 0.0f;
    for (size_t idx = begin; idx <= end; idx++) {
	float bin_centre = (1.0f * idx * bin_width) + (0.5f*bin_width);
        sum_data_squared += static_cast<float>(data[idx]) * static_cast<float>(bin_centre) * static_cast<float>(bin_centre);
        sum_counts += data[idx];
    }
    float m2 = sum_data_squared / sum_counts;
    DEBUG_PRINTF("m2 %f mean*mean %f\n", m2, mean*mean);
    float variance_binned = m2 - (mean * mean);
    DEBUG_PRINTF("variance %f\n", variance_binned);
    if (variance_binned < 0.0 && variance_binned > -1e-8) variance_binned = 0.0;
    float within_bin_var = (bin_width * bin_width) / 12.0;
    float variance = variance_binned + within_bin_var;
    return std::sqrt(variance);
}

static float stddev(const float* data, size_t n, float mean) {
    if (n == 0) return 0.0f;

    float sumSq = 0.0f;
    for (size_t idx = 0; idx < n; idx++) {
        float diff = data[idx] - mean;
        sumSq += diff * diff;
    }
    return std::sqrt(sumSq / static_cast<float>(n));  // population stddev
}

void c_morse_timing_classifier::update_on_model(const float* d, size_t n) {
  DEBUG_PRINTF("new data %lu\n", n);

  if(n < 2){ DEBUG_PRINTF("no samples to classify %lu\n", n); return;} //not enough samples
								
  //update histogram
  for(int idx=0; idx<n; ++idx) {
    int bin = d[idx]/BIN_WIDTH;
    bin = std::min(bin, BIN_MAX/BIN_WIDTH-1);
    DEBUG_PRINTF("duration %f, bin %u\n", d[idx], bin);
    on_histogram[bin]++;
  }

  //smooth histogram
  int smoothed_histogram[BIN_MAX/BIN_WIDTH];
  for(int idx=0; idx<BIN_MAX/BIN_WIDTH; ++idx) { smoothed_histogram[idx]=0; }
  for(int idx = 1; idx < BIN_MAX/BIN_WIDTH-1; idx++){
      smoothed_histogram[idx] = (on_histogram[idx-1] + on_histogram[idx] + on_histogram[idx+1]);
  }

  //print histogram
  for(int idx=0;idx<BIN_MAX/BIN_WIDTH;idx++){
      DEBUG_PRINTF("histogram %u, %u\n", idx, smoothed_histogram[idx]);
  }

  //find peaks
  std::vector<int> true_peaks;
  int idx = 1;
  while (idx < BIN_MAX/BIN_WIDTH - 1) {
      if (smoothed_histogram[idx] > smoothed_histogram[idx-1] && smoothed_histogram[idx] >= smoothed_histogram[idx+1]) {
          int start = idx;
          int end = idx;
          // Extend plateau
          while (end + 1 < BIN_MAX/BIN_WIDTH - 1 && smoothed_histogram[end+1] >= smoothed_histogram[idx]) end++;
          int center = (start + end) / 2;  // peak at plateau center
          true_peaks.push_back(center);
          idx = end + 1; // skip plateau
      } else {
          idx++;
      }
  }
  std::sort(true_peaks.begin(), true_peaks.end(),
      [&](const int a, const int b){ return smoothed_histogram[a] > smoothed_histogram[b]; });
  if(true_peaks.size()<2) return; //not bimodal return
				  
  for(auto peak : true_peaks) DEBUG_PRINTF("peak before %u\n", peak);
  std::sort(true_peaks.begin(), true_peaks.begin()+2);
  for(auto peak : true_peaks) DEBUG_PRINTF("peak after %u\n", peak);

  int peak1 = true_peaks[0];
  int peak2 = true_peaks[1];
  DEBUG_PRINTF("peak1 %u peak2 %u\n", peak1, peak2);

  // Find valley between peaks
  int valley_bin=peak1;
  int valley_value = smoothed_histogram[peak1];
  for(int idx=peak1+1;idx<peak2;idx++){
      if(smoothed_histogram[idx] < valley_value){
          valley_value = smoothed_histogram[idx];
          valley_bin=idx;
      }
  }
  DEBUG_PRINTF("valley_bin %u valley_value %u\n", valley_bin, valley_value);

  //cluster dots
  dot_mu = histogram_mean(smoothed_histogram, 0, valley_bin, BIN_WIDTH); 
  dot_sigma = histogram_stddev(dot_mu, smoothed_histogram, 0, valley_bin, BIN_WIDTH); 
  DEBUG_PRINTF("dot mu %f\n", dot_mu);
  DEBUG_PRINTF("dot sigma %f\n", dot_sigma);

  //cluster dashes
  int end = std::min(peak1 * 6, BIN_MAX/BIN_WIDTH-1);
  dash_mu = histogram_mean(smoothed_histogram, valley_bin, end, BIN_WIDTH); 
  dash_sigma = histogram_stddev(dash_mu, smoothed_histogram, valley_bin, end, BIN_WIDTH); 
  DEBUG_PRINTF("dash mu %f\n", dash_mu);
  DEBUG_PRINTF("dash sigma %f\n", dash_sigma);
  
}

void c_morse_timing_classifier::update_off_model(const float* d, size_t n) {
  DEBUG_PRINTF("new data %lu\n", n);
  if(n < 2){ DEBUG_PRINTF("no samples to classify %lu\n", n); return;} //not enough samples

  //update histogram
  for(int idx=0; idx<n; ++idx) {
    int bin = d[idx]/BIN_WIDTH;
    bin = std::min(bin, BIN_MAX/BIN_WIDTH-1);
    DEBUG_PRINTF("duration %f, bin %u\n", d[idx], bin);
    off_histogram[bin]++;
  }

  for(int idx=0;idx<BIN_MAX/BIN_WIDTH;idx++){
      DEBUG_PRINTF("histogram %u, %u\n", idx, off_histogram[idx]);
  }

  //smooth histogram
  int smoothed_histogram[BIN_MAX/BIN_WIDTH];
  for(int idx=0; idx<BIN_MAX/BIN_WIDTH; ++idx) { smoothed_histogram[idx]=0; }
  for(int idx = 1; idx < BIN_MAX/BIN_WIDTH-1; idx++){
      smoothed_histogram[idx] = (off_histogram[idx-1] + off_histogram[idx] + off_histogram[idx+1]);
  }

  //print histogram
  for(int idx=0;idx<BIN_MAX/BIN_WIDTH;idx++){
      DEBUG_PRINTF("smoothed histogram %u, %u\n", idx, smoothed_histogram[idx]);
  }

  //find peaks
  std::vector<int> true_peaks;
  int idx = 1;
  while (idx < BIN_MAX/BIN_WIDTH - 1) {
      if (smoothed_histogram[idx] > smoothed_histogram[idx-1] && smoothed_histogram[idx] >= smoothed_histogram[idx+1]) {
          int start = idx;
          int end = idx;
          // Extend plateau
          while ((end+1) < (BIN_MAX/BIN_WIDTH-1) && smoothed_histogram[end+1] >= smoothed_histogram[idx]) end++;
          int center = (start + end) / 2;  // peak at plateau center
          true_peaks.push_back(center);
          DEBUG_PRINTF("%u true peaks size %lu\n", idx, true_peaks.size());
          idx = end + 1; // skip plateau
      } else {
          idx++;
      }
  }
  std::sort(true_peaks.begin(), true_peaks.end(),
      [&](const int a, const int b){ return smoothed_histogram[a] > smoothed_histogram[b]; });

  //not trimodal
  if(true_peaks.size()<3) {

     //fallback to standard timings
     gap1_mu = dot_mu;
     gap1_sigma = dot_sigma;
     DEBUG_PRINTF("gap1 mu %f\n", gap1_mu);
     DEBUG_PRINTF("gap1 sigma %f\n", gap1_sigma);

     gap3_mu = 3*dot_mu;
     gap3_sigma = dot_sigma;
     DEBUG_PRINTF("gap3_mu %f\n", gap3_mu);
     DEBUG_PRINTF("gap3_sigma %f\n", gap3_sigma);

     gap7_mu = 7*dot_mu;
     gap7_sigma = dot_sigma;
     DEBUG_PRINTF("gap7_mu %f\n", gap7_mu);
     DEBUG_PRINTF("gap7_sigma %f\n", gap7_sigma);

     return;
  }

  for(auto peak : true_peaks) DEBUG_PRINTF("peak before %u\n", peak);
  std::sort(true_peaks.begin(), true_peaks.begin()+3,
      [](const int a, const int b){ return a < b; });
  for(auto peak : true_peaks) DEBUG_PRINTF("peak after %u\n", peak);


  int peak1 = true_peaks[0];
  int peak2 = true_peaks[1];
  int peak3 = true_peaks[2];
  DEBUG_PRINTF("peak1 %u peak2 %u peak3 %u\n", peak1, peak2, peak3);

  // Find valley between peaks
  int valley1_bin=peak1;
  int valley1_value = smoothed_histogram[peak1];
  for(int idx=peak1+1;idx<peak2;idx++){
      if(smoothed_histogram[idx] < valley1_value){
          valley1_value = smoothed_histogram[idx];
          valley1_bin=idx;
      }
  }
  DEBUG_PRINTF("valley1_bin %u valley1_value %u\n", valley1_bin, valley1_value);

  // Find valley between peaks
  int valley2_bin=peak2;
  int valley2_value = smoothed_histogram[peak2];
  for(int idx=peak2+1;idx<peak3;idx++){
      if(smoothed_histogram[idx] < valley2_value){
          valley2_value = smoothed_histogram[idx];
          valley2_bin=idx;
      }
  }
  DEBUG_PRINTF("valley2_bin %u valley2_value %u\n", valley2_bin, valley2_value);

  //cluster dots

  gap1_mu = histogram_mean(smoothed_histogram, 0, valley1_bin, BIN_WIDTH); 
  gap1_sigma = histogram_stddev(gap1_mu, smoothed_histogram, 0, valley1_bin, BIN_WIDTH); 
  gap1_sigma = std::max(0.1f*gap1_mu, gap1_sigma);
  DEBUG_PRINTF("gap1 mu %f\n", gap1_mu);
  DEBUG_PRINTF("gap1 sigma %f\n", gap1_sigma);

  gap3_mu = histogram_mean(smoothed_histogram, valley1_bin, valley2_bin, BIN_WIDTH); 
  gap3_sigma = histogram_stddev(gap3_mu, smoothed_histogram, valley1_bin, valley2_bin, BIN_WIDTH); 
  gap3_sigma = std::min(std::max(0.1f*gap3_mu, gap3_sigma), 2*gap1_sigma);
  DEBUG_PRINTF("gap3_mu %f\n", gap3_mu);
  DEBUG_PRINTF("gap3_sigma %f\n", gap3_sigma);

  gap7_mu = 7*gap1_mu;
  gap7_sigma = gap1_sigma;
  DEBUG_PRINTF("gap7_mu %f\n", gap7_mu);
  DEBUG_PRINTF("gap7_sigma %f\n", gap7_sigma);

}

void c_morse_timing_classifier::classify_on(float d, float& logp_dot, float& logp_dash, float& logp_dotdot, float& logp_dotdash, float& logp_dashdash) {
  logp_dot = log_gaussian(d, dot_mu, dot_sigma);
  logp_dash = log_gaussian(d, dash_mu, dash_sigma);
  logp_dotdot = log_gaussian(d, dot_mu + dot_mu + gap1_mu, dash_sigma);
  logp_dotdash = log_gaussian(d, dash_mu + dot_mu + gap1_mu, dash_sigma);
  logp_dashdash = log_gaussian(d, dash_mu + dash_mu + gap1_mu, dash_sigma);
}

void c_morse_timing_classifier::classify_off(float d, float log_probs[]) {
  log_probs[0] = log_gaussian(d, gap1_mu, gap1_sigma);
  log_probs[1] = log_gaussian(d, gap3_mu, gap3_sigma);
  log_probs[2] = d < gap7_mu?log_gaussian(d, gap7_mu, gap7_sigma):0;
}

