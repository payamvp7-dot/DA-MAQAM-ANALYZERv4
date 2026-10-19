#pragma once
#include "MaqamDatabase.h"
#include "../DSP/PitchTracker.h"
#include <vector>

// Forward declare processor's pitch frame to avoid circular
struct AnalyzerPitchFrame
{
    double timeSec = 0.0;
    float freqHz = 0.0f;
    float confidence = 0.0f;
    bool voiced = false;
};

//==============================================================================
/**
 * Maqam Analyzer - evidence-based candidate ranking
 * Uses pitch trajectory, distribution, durations, transitions, contour, phrase endings.
 * NOT just histogram.
 */
class MaqamAnalyzer
{
public:
    explicit MaqamAnalyzer(MaqamDatabase& db) : database(db) {}

    struct Config
    {
        Tradition traditionFilter = Tradition::Persian; // All handled externally
        bool allTraditions = true;
        double refA4 = 440.0;
        juce::String tonicOverride; // empty = auto
        float minVoicedSec = 3.0f; // insufficient evidence threshold
    };

    struct AnalysisInput
    {
        std::vector<AnalyzerPitchFrame> frames;
        double sampleRate = 44100.0;
        double durationSec = 0.0;
        Config config;
    };

    struct Result
    {
        std::vector<MaqamCandidate> candidates; // 0..3 ranked
        DetectedTonic tonic;
        bool insufficient = false;
        juce::String uncertaintyNote;
        juce::String limitationNote;
        juce::String debugInfo;
    };

    Result analyze(const AnalysisInput& input);

    // Helpers exposed for testing
    static DetectedTonic estimateTonic(const std::vector<AnalyzerPitchFrame>& frames, double refA4);
    static std::vector<float> buildPitchClassHistogram(const std::vector<AnalyzerPitchFrame>& frames, int binsPerOctave = 24);
    static std::vector<float> buildIntervalProfile(const MaqamEntry& e, int binsPerOctave = 24);

private:
    MaqamDatabase& database;

    float scoreEntry(const MaqamEntry& e, const DetectedTonic& tonic,
                     const std::vector<AnalyzerPitchFrame>& frames,
                     const std::vector<float>& pcHist,
                     juce::String& evidenceOut,
                     std::vector<float>& obsVsExpectedOut) const;

    static float centsDistance(float a, float b) { return std::abs(a - b); }
    static float wrapCents(float c) { while(c < 0) c+=1200; while(c>=1200) c-=1200; return c; }
};
