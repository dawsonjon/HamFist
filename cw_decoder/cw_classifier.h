#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

enum SpaceType {
    SPACE_NONE = 0,
    SPACE_ELEM,   // between dots/dashes inside a character (~1 dot)
    SPACE_CHAR,   // between letters (~3 dots)
    SPACE_WORD    // between words (~7 dots)
};

struct Gaussian {
    float mean;
    float var;
    float weight;
};

// Configurable GMM (2 for ON, 3 for OFF)
struct TimingModel {
    static const int MAX_CLUSTERS = 3;
    int clusters;
    Gaussian g[MAX_CLUSTERS];
    bool initialized;
};

class MorseTimingClassifier {
public:
    MorseTimingClassifier();

    // Element ON durations (dots/dashes)
    void updateOnModel(const float* durations, size_t count);
    void updateOffModel(const float* durations, size_t count);

    // Returns true if valid classification.
    // Outputs log-probabilities for DOT and DASH via reference params.
    bool classifyOn(float duration_ms,
                    float& logProbDot,
                    float& logProbDash);

    // Outputs log-probabilities for OFF spacing clusters.
    // Returns best cluster (0..clusters-1), or -1 if invalid.
    int classifyOff(float duration_ms,
                    float* logProbs, // array of size off.clusters
                    int maxClusters);


    float getDot() const { return on.g[0].mean; }
    float getWPM() const { return 1200.0f / getDot(); }

private:
    TimingModel on;  // 2 clusters
    TimingModel off; // 3 clusters

    float gaussianPdf(float x, float mean, float var);
    void adaptModel(TimingModel& m, const float* data, size_t n, float learnRate);
    int bestCluster(const TimingModel& m, float x, float* probOut = nullptr);
};

