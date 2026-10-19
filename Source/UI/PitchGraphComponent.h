#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
class PitchGraphComponent : public juce::Component, public juce::Timer
{
public:
    explicit PitchGraphComponent(MaqamAnalyzerAudioProcessor& p) : processor(p)
    {
        startTimerHz(30);
        graphData.reserve(1200);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        // background
        juce::ColourGradient bg(juce::Colour(0xFF141C2B), bounds.getTopLeft(),
                                juce::Colour(0xFF0F1623), bounds.getBottomLeft(), false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(bounds, 8);

        // border
        g.setColour(juce::Colour(0xFF2E384B));
        g.drawRoundedRectangle(bounds, 8, 1.0f);
        // subtle gold top edge
        g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.22f));
        g.drawHorizontalLine((int)bounds.getY()+1, bounds.getX()+8, bounds.getRight()-8);

        if (graphData.empty() && waveform.empty())
        {
            g.setColour(juce::Colour(0xFF9AA0B0).withAlpha(0.55f));
            g.setFont(juce::Font(juce::FontOptions(12.0f)));
            g.drawText("Press LEARN - live pitch contour appears here", bounds, juce::Justification::centred);
            return;
        }

        // draw semitone grid
        g.setColour(juce::Colour(0xFF2E384B).withAlpha(0.9f));
        // horizontal lines for octaves? We'll draw cents grid relative to tonic if available
        int midMidi = 60;
        if (processor.getLatestResult().valid) midMidi = processor.getLatestResult().detectedTonic.midiNote;
        // draw 12 semitone lines centered
        for (int sem = -12; sem <= 12; ++sem)
        {
            float y = midiToY(midMidi + sem, bounds);
            if (y < bounds.getY() || y > bounds.getBottom()) continue;
            bool isOctave = (sem % 12 == 0);
            bool isTonic = (sem == 0);
            g.setColour(isTonic ? juce::Colour(0xFFC2A878).withAlpha(0.85f) : (isOctave ? juce::Colour(0xFF9AA0B0).withAlpha(0.35f) : juce::Colour(0xFF2E384B).withAlpha(0.55f)));
            juce::Path line;
            line.startNewSubPath(bounds.getX()+40, y);
            line.lineTo(bounds.getRight()-6, y);
            float dash[2]={ isTonic? 1:4, 4};
            g.strokePath(line, juce::PathStrokeType(isTonic?1.2f:0.7f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt, 0, dash, isTonic?0:1));

            // label
            if (std::abs(sem) <= 12)
            {
                static const char* names[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
                int pc = ((midMidi + sem) %12+12)%12;
                int oct = (midMidi + sem)/12 -1;
                juce::String lbl = juce::String(names[pc]) + juce::String(oct);
                if (sem==0) lbl += " (tonic)";
                g.setColour(isTonic ? juce::Colour(0xFFC2A878) : juce::Colour(0xFF9AA0B0).withAlpha(0.85f));
                g.setFont(juce::Font(juce::FontOptions(isTonic?10.5f:9.0f).withStyle(isTonic?"Bold":"Regular")));
                g.drawText(lbl, juce::Rectangle<float>(bounds.getX()+4, y-7, 38, 12), juce::Justification::right);
            }
        }

        // draw pitch contour from graphData (midi float)
        if (graphData.size() > 1)
        {
            juce::Path path;
            bool started=false;
            for (size_t i=0;i<graphData.size();++i)
            {
                double t = graphData[i].timeSec;
                float x = timeToX(t, bounds);
                float y = midiToY(graphData[i].midi, bounds);
                if (!graphData[i].voiced)
                {
                    started=false;
                    continue;
                }
                if (!started) { path.startNewSubPath(x,y); started=true; }
                else path.lineTo(x,y);
            }
            // glow
            g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.18f));
            g.strokePath(path, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(juce::Colour(0xFFE2C99A));
            g.strokePath(path, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // dots for note events (where pitch stable > 200ms)
            for (size_t i=1;i<graphData.size();++i)
            {
                if (!graphData[i].voiced) continue;
                // detect stable pitch segments: 3 consecutive within 20c
                if (i>2 && std::abs(graphData[i].midi - graphData[i-1].midi) < 0.20f && std::abs(graphData[i-1].midi - graphData[i-2].midi) < 0.20f)
                {
                    float x = timeToX(graphData[i].timeSec, bounds);
                    float y = midiToY(graphData[i].midi, bounds);
                    g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.65f));
                    g.fillEllipse(x-1.2f, y-1.2f, 2.4f, 2.4f);
                }
            }
        }

        // waveform mini strip at bottom (last block RMS shape)
        if (!waveform.empty())
        {
            auto wfArea = bounds.removeFromBottom(26).reduced(40, 2);
            g.setColour(juce::Colour(0xFF1B2333));
            g.fillRoundedRectangle(wfArea, 4);
            juce::Path wf;
            for (size_t i=0;i<waveform.size();++i)
            {
                float x = wfArea.getX() + (float)i / (float)waveform.size() * wfArea.getWidth();
                float y = wfArea.getCentreY() - waveform[i] * wfArea.getHeight()*0.45f;
                if(i==0) wf.startNewSubPath(x,y);
                else wf.lineTo(x,y);
            }
            g.setColour(juce::Colour(0xFF6BA3A8).withAlpha(0.9f));
            g.strokePath(wf, juce::PathStrokeType(1.0f));
        }

        // playhead
        if (processor.getAnalysisState() == MaqamAnalyzerAudioProcessor::AnalysisState::Learning)
        {
            float prog = (float) processor.getLearnProgress();
            float x = bounds.getX()+40 + prog * (bounds.getWidth()-46);
            g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.95f));
            g.drawVerticalLine((int)x, bounds.getY()+2, bounds.getBottom()-28);
            g.fillEllipse(x-3, bounds.getY()+2, 6,6);
        }
    }

    void timerCallback() override
    {
        // Pull from processor fifo
        auto& fifo = processor.fifo;
        auto& ring = processor.pitchRing;
        int start1, size1, start2, size2;
        fifo.prepareToRead(fifo.getNumReady(), start1, size1, start2, size2);
        if (size1+size2 > 0)
        {
            for (int i=0;i<size1;++i)
            {
                int idx = (start1 + i) % MaqamAnalyzerAudioProcessor::kRingSize;
                auto& pf = ring[idx];
                if (pf.frequencyHz > 20 && pf.voiced)
                {
                    float midi = 69 + 12 * std::log2(pf.frequencyHz/440.0f);
                    DataPoint dp { pf.timeSec, midi, pf.voiced, pf.centsOffset };
                graphData.push_back({pf.timeSec, midi, pf.voiced, pf.centsOffset});
                }
                else
                {
                    // unvoiced gap marker
                    graphData.push_back({pf.timeSec, 0, false, 0});
                }
            }
            for (int i=0;i<size2;++i)
            {
                int idx = (start2 + i) % MaqamAnalyzerAudioProcessor::kRingSize;
                auto& pf = ring[idx];
                if (pf.frequencyHz > 20 && pf.voiced)
                {
                    float midi = 69 + 12 * std::log2(pf.frequencyHz/440.0f);
                    graphData.push_back({pf.timeSec, midi, pf.voiced, pf.centsOffset});
                }
                else graphData.push_back({pf.timeSec, 0, false, 0});
            }
            fifo.finishedRead(size1+size2);

            // window to last 30 sec
            if (graphData.size() > 1500)
            {
                graphData.erase(graphData.begin(), graphData.begin() + (graphData.size()-1500));
            }
            repaint();
        }

        // waveform update: simple animated placeholder driven by RMS
        float rms = processor.getCurrentRMS();
        waveform.push_back(juce::jlimit(-1.0f,1.0f, rms * 3.0f + 0.1f * std::sin((float)juce::Time::getMillisecondCounter()*0.01f)));
        if (waveform.size() > 180) waveform.erase(waveform.begin());
        if (processor.getAnalysisState() == MaqamAnalyzerAudioProcessor::AnalysisState::Learning)
            repaint();
    }

    void clear()
    {
        graphData.clear();
        waveform.clear();
        repaint();
    }

private:
    MaqamAnalyzerAudioProcessor& processor;

    struct DataPoint { double timeSec; float midi=0; bool voiced=false; float centsOff=0; };
    std::vector<DataPoint> graphData;
    std::vector<float> waveform;

    float midiToY(float midi, juce::Rectangle<float> bounds) const
    {
        // map midi to Y, centered around current tonic ±12 semitones
        int tonicMidi = processor.getLatestResult().valid ? processor.getLatestResult().detectedTonic.midiNote : 60;
        float visibleRange = 24.0f; // ±12 sem
        float midY = bounds.getCentreY() - 13; // reserve waveform strip
        float midiMin = tonicMidi - 12;
        float midiMax = tonicMidi + 12;
        float frac = (midi - midiMin) / (midiMax - midiMin);
        // invert Y (higher pitch higher on screen -> smaller Y)
        return bounds.getY()+6 + (1.0f - frac) * (bounds.getHeight()-32-12);
    }

    float timeToX(double t, juce::Rectangle<float> bounds) const
    {
        if (graphData.empty()) return bounds.getX()+40;
        double t0 = graphData.front().timeSec;
        double t1 = graphData.back().timeSec;
        double span = juce::jmax(10.0, t1 - t0); // at least 10 sec span
        float frac = (float)((t - t0) / span);
        return bounds.getX()+40 + frac * (bounds.getWidth()-46);
    }
};
