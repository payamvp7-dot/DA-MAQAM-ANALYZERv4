#pragma once
#include <JuceHeader.h>
#include "DSP/PitchTracker.h"
#include "Maqam/MaqamDatabase.h"
#include "Maqam/MaqamAnalyzer.h"
#include "Utils/StateManager.h"

//==============================================================================
/**
 * D.A Maqam Analyzer - Audio Processor
 * Pass-through analyzer: never modifies audio, only listens.
 */
class MaqamAnalyzerAudioProcessor : public juce::AudioProcessor
{
public:
    MaqamAnalyzerAudioProcessor();
    ~MaqamAnalyzerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "D.A Maqam Analyzer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Public state for editor binding
    enum class AnalysisState { Idle, Learning, Analyzing, Completed };
    enum class TraditionFilter { All = 0, Persian = 1, Azerbaijani = 2, Turkish = 3, Arabic = 4 };

    struct ProcessorState
    {
        TraditionFilter traditionFilter = TraditionFilter::All;
        int learnDurationSec = 20; // 10,20,30,60
        double referenceA4 = 440.0;
        juce::String manualTonic = ""; // e.g. "C" or empty = auto
        juce::String manualMaqamId = ""; // override
        std::array<float, 12> manualCentOffsets{}; // per pitch-class offset vs 12-TET
        bool hasManualMaqam = false;
    } uiState;

    // Analysis control
    void startLearn();
    void stopLearn();
    void resetAnalysis();
    AnalysisState getAnalysisState() const { return analysisState.load(); }
    double getLearnProgress() const; // 0..1
    int getVoicedFrameCount() const { return voicedFrames.load(); }
    float getCurrentRMS() const { return currentRMS.load(); }

    // Pitch data access (lock-free)
    struct PitchFrame
    {
        double timeSec = 0.0;
        float frequencyHz = 0.0f;
        float confidence = 0.0f;
        bool voiced = false;
        float centsOffset = 0.0f; // vs nearest 12-TET
        juce::String noteName;
    };

    // Thread-safe ring for editor to pull
    static constexpr int kRingSize = 8192;
    juce::AbstractFifo fifo { kRingSize };
    std::array<PitchFrame, kRingSize> pitchRing {};
    std::atomic<int> writePos { 0 };

    // Last analysis results
    struct AnalysisResultSnapshot
    {
        bool valid = false;
        std::vector<MaqamCandidate> candidates; // up to 3
        DetectedTonic detectedTonic;
        double analysisDurationSec = 0.0;
        int totalVoicedFrames = 0;
        juce::String uncertaintyNote;
        juce::String limitationNote;
        juce::Time timestamp;
    };

    AnalysisResultSnapshot getLatestResult() const;
    void setLatestResult(const AnalysisResultSnapshot& r);

    // Live pitch for bottom strip
    PitchFrame getLastPitchFrame() const;

    // Export
    juce::String generateTXTReport() const;
    juce::String generateCSVReport() const;
    juce::String generateScalaSCL(int candidateIndex) const;
    juce::String generateScalaKBM(int candidateIndex, const juce::String& tonic) const;

    // APVTS for automatable params (A4 only automatable)
    juce::AudioProcessorValueTreeState apvts;

    MaqamDatabase& getDatabase() { return maqamDatabase; }
    const MaqamDatabase& getDatabase() const { return maqamDatabase; }

    // For editor timer polling
    std::atomic<float> currentInputLevelDb { -100.0f };

private:
    // DSP
    std::unique_ptr<PitchTracker> pitchTracker;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Analysis engine (runs on audio thread preallocation + worker thread)
    MaqamDatabase maqamDatabase;
    MaqamAnalyzer maqamAnalyzer;

    // Worker thread for heavy analysis
    juce::TimeSliceThread workerThread { "MaqamWorker" };
    struct AnalysisJob : public juce::TimeSliceClient
    {
        MaqamAnalyzerAudioProcessor* owner = nullptr;
        int useResult() override;
    } analysisJob;

    // Learn state
    std::atomic<AnalysisState> analysisState { AnalysisState::Idle };
    std::atomic<int> voicedFrames { 0 };
    std::atomic<float> currentRMS { 0.0f };
    double learnStartTimeSec = 0.0;
    double learnElapsedSec = 0.0;
    juce::AudioBuffer<float> learnBuffer; // preallocated mono
    int learnWritePos = 0;
    int learnMaxSamples = 0;

    // Collected pitch trajectory for analysis
    juce::CriticalSection resultLock;
    AnalysisResultSnapshot latestResult;
    PitchFrame lastFrame;

    // Background pitch history for analysis
    std::vector<PitchFrame> collectedFrames; // protected by resultLock, written on audio thread via fifo copy
    std::vector<PitchFrame> analysisCopy; // worker thread copy

    void pushPitchFrame(const PitchFrame& f);
    void triggerAnalysis();

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MaqamAnalyzerAudioProcessor)
};
