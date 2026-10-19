#pragma once
#include <JuceHeader.h>
#include <vector>

//==============================================================================
/**
 * YIN pitch estimator - de Cheveigné & Kawahara 2002
 * Monophonic, robust, with confidence and voiced/unvoiced.
 * Real-time safe: no allocation in process().
 * Reference implementation adapted for audio thread, with octave-error mitigation.
 */
class YinPitchDetector
{
public:
    YinPitchDetector() = default;
    ~YinPitchDetector() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset() { yinBuffer.assign(yinBuffer.size(), 0.0f); }

    struct Result
    {
        float frequencyHz = 0.0f; // 0 = unvoiced
        float confidence = 0.0f;  // 0..1 (1 = high, derived from aperiodicity)
        bool voiced = false;
        float rmsDb = -100.0f;
    };

    // Process a block of mono samples, return per-block estimate
    // blockSize should be 1024-4096 for 44.1k
    Result processBlock(const float* samples, int numSamples);

    void setThreshold(float t) { threshold = juce::jlimit(0.05f, 0.5f, t); }
    void setMinFreq(float f) { minFreq = f; }
    void setMaxFreq(float f) { maxFreq = f; }

private:
    double sampleRate = 44100.0;
    float threshold = 0.15f;
    float minFreq = 60.0f;
    float maxFreq = 1200.0f;

    std::vector<float> yinBuffer; // size = window
    std::vector<float> inputBuffer;
    int windowSize = 2048;

    int tauMin = 36;
    int tauMax = 735;

    float parabolicInterpolation(const std::vector<float>& buf, int tau) const;
    void cumulativeMeanNormalizedDifference(const float* samples, int numSamples);
};
