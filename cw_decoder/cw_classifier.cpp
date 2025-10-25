#include "morse_classifier.h"

MorseTimingClassifier::MorseTimingClassifier() {
    // ON Model: dot/dash
    on.clusters = 2;
    on.initialized = false;
    on.g[0] = { 60.0f, 400.0f, 0.5f };
    on.g[1] = { 180.0f, 2000.0f, 0.5f };

    // OFF Model: elem/char/word spacing
    off.clusters = 3;
    off.initialized = false;
    off.g[0] = { 60.0f, 400.0f, 0.6f };  // element gap
    off.g[1] = { 180.0f, 2000.0f, 0.3f }; // char gap
    off.g[2] = { 420.0f, 6000.0f, 0.1f }; // word gap
}

float MorseTimingClassifier::gaussianPdf(float x, float mean, float var) {
    float d = x - mean;
    return expf(-0.5f * (d * d / var)) / sqrtf(2.0f * M_PI * var);
}

static void quickInit(TimingModel& m, const float* data, size_t n) {
    if (n < (size_t)m.clusters * 5) return;
    float sorted[256];
    n = std::min(n, sizeof(sorted)/sizeof(sorted[0]));
    for (size_t i=0;i<n;i++) sorted[i] = data[i];
    std::sort(sorted, sorted+n);

    size_t step = n / m.clusters;
    for (int k=0; k<m.clusters; k++) {
        size_t idx = k * step + step/2;
        if (idx >= n) idx = n-1;
        m.g[k].mean = sorted[idx];
        m.g[k].var = sorted[idx] * 0.4f;
        m.g[k].weight = 1.0f / m.clusters;
    }
    m.initialized = true;
}

void MorseTimingClassifier::adaptModel(TimingModel& m,
                                       const float* data,
                                       size_t n,
                                       float lr) {
    if (!m.initialized) { quickInit(m, data, n); return; }
    for (size_t i=0;i<n;i++) {
        float x = data[i];
        float denom = 0;
        float p[TimingModel::MAX_CLUSTERS];

        for (int k=0;k<m.clusters;k++) {
            p[k] = m.g[k].weight * gaussianPdf(x, m.g[k].mean, m.g[k].var);
            denom += p[k];
        }
        if (denom < 1e-6f) continue;

        for (int k=0;k<m.clusters;k++) {
            float q = p[k]/denom;
            m.g[k].mean += lr * q * (x - m.g[k].mean);
            m.g[k].weight = 0.95f*m.g[k].weight + 0.05f*q;
        }
    }
}

void MorseTimingClassifier::updateOnModel(const float* d, size_t n) {
    if (n >= 20) adaptModel(on, d, n, 0.05f);
}

void MorseTimingClassifier::updateOffModel(const float* d, size_t n) {
    if (n >= 20) adaptModel(off, d, n, 0.05f);
}

static inline float safeLog(float x) {
    if (x <= 1e-12f) return -30.0f; // clamp ~ln(1e-13)
    return logf(x);
}

bool MorseTimingClassifier::classifyOn(float d,
                                       float& logProbDot,
                                       float& logProbDash)
{
    if (!on.initialized) {
        logProbDot = logProbDash = -30.0f;
        return false;
    }

    // Unnormalized posteriors
    float pDot  = on.g[0].weight * gaussianPdf(d, on.g[0].mean, on.g[0].var);
    float pDash = on.g[1].weight * gaussianPdf(d, on.g[1].mean, on.g[1].var);

    float denom = pDot + pDash;
    if (denom < 1e-9f) {
        logProbDot = logProbDash = -30.0f;
        return false;
    }

    // Convert to posterior log-probs
    logProbDot  = safeLog(pDot  / denom);
    logProbDash = safeLog(pDash / denom);
    return true;
}

int MorseTimingClassifier::classifyOff(float d,
                                       float* logProbs,
                                       int maxClusters)
{
    if (!off.initialized) return -1;

    float p[TimingModel::MAX_CLUSTERS];
    float denom = 0;

    int K = std::min(maxClusters, off.clusters);

    for (int k = 0; k < K; k++) {
        p[k] = off.g[k].weight * gaussianPdf(d, off.g[k].mean, off.g[k].var);
        denom += p[k];
    }

    if (denom < 1e-9f) {
        for (int k=0;k<K;k++) logProbs[k] = -30.0f;
        return -1;
    }

    int best = 0;
    float maxp = p[0];

    for (int k = 0; k < K; k++) {
        float posterior = p[k] / denom;
        logProbs[k] = safeLog(posterior);
        if (p[k] > maxp) { maxp = p[k]; best = k; }
    }

    return best;
}

