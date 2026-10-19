#pragma once
#include "YinPitchDetector.h"
#include <deque>

//==============================================================================
/**
 * PitchTracker wraps YIN with smoothing for display only, cent calculation,
 * note naming (including quarter-tone hints), and voiced frame history.
 */
class PitchTracker
{
public:
    PitchTracker() = default;

    void prepare(double sampleRate, int blockSize)
    {
        yin.prepare(sampleRate, juce::jmax(1024, blockSize * 2));
        sr = sampleRate;
        // median filter for display smoothing (not for analysis)
        displayHistory.clear();
        rawHistory.clear();
    }

    struct Frame
    {
        float freqHz = 0.0f;
        float confidence = 0.0f;
        bool voiced = false;
        float centsFromA4 = 0.0f; // vs 440, cents
        float centsFrom12TET = 0.0f; // offset from nearest 12-TET pitch
        int midiNearest = -1;
        juce::String noteName; // e.g. "A4", "E half-flat"
        float rmsDb = -100.0f;
        double timeSec = 0.0;
    };

    // call per audio block (mono summed input)
    Frame process(const float* samples, int numSamples, double timeSec, double refA4 = 440.0)
    {
        auto r = yin.processBlock(samples, numSamples);

        Frame f;
        f.freqHz = r.frequencyHz;
        f.confidence = r.confidence;
        f.voiced = r.voiced;
        f.rmsDb = r.rmsDb;
        f.timeSec = timeSec;

        if (f.voiced && f.freqHz > 20)
        {
            // cents from A4
            f.centsFromA4 = 1200.0f * std::log2(f.freqHz / (float) refA4);

            // nearest MIDI (12-TET)
            float midiFloat = 69.0f + 12.0f * std::log2(f.freqHz / 440.0f);
            f.midiNearest = (int) std::round(midiFloat);
            float midiFreq = 440.0f * std::pow(2.0f, (f.midiNearest - 69) / 12.0f);
            f.centsFrom12TET = 1200.0f * std::log2(f.freqHz / midiFreq);

            // note name
            static const char* names[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            int pc = ((f.midiNearest % 12) + 12) % 12;
            int oct = f.midiNearest / 12 - 1;
            f.noteName = juce::String(names[pc]) + juce::String(oct);

            // quarter-tone hint: if 30c - 70c off 12-TET, flag as microtonal
            if (std::abs(f.centsFrom12TET) > 30 && std::abs(f.centsFrom12TET) < 70)
            {
                if (f.centsFrom12TET > 0) f.noteName += " +¼";
                else f.noteName += " -¼";
            }

            // display smoothing: median of last 3 voiced frames (only for display)
            displayHistory.push_back(f.freqHz);
            if (displayHistory.size() > 3) displayHistory.pop_front();
            // note: we do NOT overwrite f.freqHz for analysis; the median is only
            // used by the graph component which reads smoothed value separately
        }
        else
        {
            f.noteName = "-";
        }

        // keep raw for debugging
        rawHistory.push_back(f);
        if (rawHistory.size() > 512) rawHistory.pop_front();

        lastFrame = f;
        return f;
    }

    Frame getLastFrame() const { return lastFrame; }

    // Smoothed display frequency (median)
    float getSmoothedFreqForDisplay() const
    {
        if (displayHistory.empty()) return 0.0f;
        std::vector<float> v(displayHistory.begin(), displayHistory.end());
        std::sort(v.begin(), v.end());
        return v[v.size()/2];
    }

private:
    YinPitchDetector yin;
    double sr = 44100.0;
    Frame lastFrame;
    std::deque<float> displayHistory;
    std::deque<Frame> rawHistory;
};
