#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

const int BIN_WIDTH = 5;
const int BIN_MAX = 500;

class c_morse_timing_classifier {
  public:
    c_morse_timing_classifier();

    // Element ON durations (dots/dashes)
    void update_on_model(const float* durations, size_t count);
    void update_off_model(const float* durations, size_t count);

    void classify_on(float d, float& logp_dot, float& logp_dash, float& logp_dotdot, float& logp_dotdash, float& logp_dashdash);
    void classify_off(float duration_ms, float* logProbs);

    float get_dot_length() const { return dot_mu; }
    float get_WPM() const {return 1200.0f / get_dot_length();}

    void reset();

  private:
    int on_histogram[BIN_MAX/BIN_WIDTH];
    int off_histogram[BIN_MAX/BIN_WIDTH];
    float dot_mu, dot_sigma;
    float dash_mu, dash_sigma;
    float gap1_mu, gap1_sigma;
    float gap3_mu, gap3_sigma;
    float gap7_mu, gap7_sigma;

};

