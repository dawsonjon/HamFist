#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

class MorseTimingClassifier {
  public:
    MorseTimingClassifier();

    // Element ON durations (dots/dashes)
    void updateOnModel(const float* durations, size_t count);
    void updateOffModel(const float* durations, size_t count);

    void classifyOn(float duration_ms, float& logProbDot, float& logProbDash);
    void classifyOff(float duration_ms, float* logProbs);

    float getDot() const { return dot_mu; }
    float getWPM() const { return 1200.0f / getDot(); }

  private:
    float dot_mu, dot_sigma;
    float dash_mu, dash_sigma;
    float gap1_mu, gap1_sigma;
    float gap3_mu, gap3_sigma;
    float gap7_mu, gap7_sigma;

};

