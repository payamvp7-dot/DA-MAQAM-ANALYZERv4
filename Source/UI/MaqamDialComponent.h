#pragma once
#include <JuceHeader.h>
#include "../Maqam/MaqamDatabase.h"
#include "../Maqam/MaqamAnalyzer.h"

class MaqamDialComponent : public juce::Component, public juce::Timer
{
public:
    MaqamDialComponent()
    {
        startTimerHz(30);
    }

    void setCandidates(const std::vector<MaqamCandidate>& cands)
    {
        candidates = cands;
        repaint();
    }

    void setAnalysisState(int s) { state = s; repaint(); }
    void setProgress(float p) { progress = p; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.44f;
        float innerR = radius * 0.62f;

        // outer glow
        g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.12f));
        g.fillEllipse(centre.x - radius - 8, centre.y - radius - 8, (radius+8)*2, (radius+8)*2);

        // outer ring gradient
        juce::ColourGradient cg(juce::Colour(0xFF222B3A), centre.x, centre.y - radius,
                                juce::Colour(0xFF181E2A), centre.x, centre.y + radius, false);
        g.setGradientFill(cg);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius*2, radius*2);

        // gold border
        g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.85f));
        g.drawEllipse(centre.x - radius, centre.y - radius, radius*2, radius*2, 1.6f);
        g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.25f));
        g.drawEllipse(centre.x - radius + 3, centre.y - radius + 3, (radius-3)*2, (radius-3)*2, 0.9f);

        // progress arc when learning
        if (state == 1) // Learning
        {
            g.setColour(juce::Colour(0xFFE2C99A).withAlpha(0.85f));
            juce::Path arc;
            float start = -juce::MathConstants<float>::pi * 0.5f;
            float end = start + juce::MathConstants<float>::twoPi * progress;
            arc.addCentredArc(centre.x, centre.y, radius+2, radius+2, 0, start, end, true);
            g.strokePath(arc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // inner circle
        juce::ColourGradient innerGrad(juce::Colour(0xFF1E2636), centre.x, centre.y - innerR,
                                       juce::Colour(0xFF0F1623), centre.x, centre.y + innerR, false);
        g.setGradientFill(innerGrad);
        g.fillEllipse(centre.x - innerR, centre.y - innerR, innerR*2, innerR*2);
        g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.45f));
        g.drawEllipse(centre.x - innerR, centre.y - innerR, innerR*2, innerR*2, 1.0f);

        // tick marks (12 semitones + microtonal hints)
        for (int i=0;i<12;++i)
        {
            float ang = juce::MathConstants<float>::twoPi * i / 12.0f - juce::MathConstants<float>::pi * 0.5f;
            float r1 = radius - 2;
            float r2 = radius - 9;
            if (i%3==0) r2 = radius - 13;
            float x1 = centre.x + std::cos(ang)*r1;
            float y1 = centre.y + std::sin(ang)*r1;
            float x2 = centre.x + std::cos(ang)*r2;
            float y2 = centre.y + std::sin(ang)*r2;
            g.setColour(i%3==0 ? juce::Colour(0xFFC2A878).withAlpha(0.85f) : juce::Colour(0xFF9AA0B0).withAlpha(0.55f));
            g.drawLine(x1,y1,x2,y2, i%3==0 ? 1.4f : 0.9f);
        }

        // needle / current top candidate indicator
        float needleAng = -juce::MathConstants<float>::pi * 0.5f;
        if (!candidates.empty())
        {
            // map similarity 0..1 to slight right bias for visual variety - keep centered
            needleAng += candidates[0].similarityScore * 0.35f; // small nudge
            // add slow shimmer
            needleAng += std::sin(juce::Time::getMillisecondCounter() * 0.001f) * 0.02f;
        }
        else
        {
            // idle drift
            needleAng += std::sin(juce::Time::getMillisecondCounter() * 0.0006f) * 0.15f;
        }
        juce::Path needle;
        float nx = centre.x + std::cos(needleAng) * (innerR - 10);
        float ny = centre.y + std::sin(needleAng) * (innerR - 10);
        needle.startNewSubPath(centre.x, centre.y);
        needle.lineTo(nx, ny);
        g.setColour(juce::Colour(0xFFE2C99A));
        g.strokePath(needle, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // center dot
        g.setColour(juce::Colour(0xFFC2A878));
        g.fillEllipse(centre.x - 5, centre.y -5, 10,10);
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.fillEllipse(centre.x -2, centre.y -2, 4,4);

        // text
        juce::String mainText = candidates.empty() ? "—" : candidates[0].entry.displayName;
        juce::String subText = candidates.empty() ? "Awaiting LEARN" : candidates[0].tonic.noteName + " • " + traditionToString(candidates[0].entry.tradition);

        g.setColour(juce::Colour(0xFFE8E6E1));
        g.setFont(juce::Font(juce::FontOptions(17.0f).withStyle("Bold")));
        g.drawText(mainText, juce::Rectangle<float>(centre.x - innerR, centre.y - 10, innerR*2, 20), juce::Justification::centred);
        g.setColour(juce::Colour(0xFFC2A878));
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(subText, juce::Rectangle<float>(centre.x - innerR, centre.y + 9, innerR*2, 14), juce::Justification::centred);

        if (!candidates.empty())
        {
            juce::String scoreTxt = "Similarity " + juce::String(candidates[0].similarityScore, 2) + "  (not probability)";
            g.setColour(juce::Colour(0xFF9AA0B0));
            g.setFont(juce::Font(juce::FontOptions(9.5f)));
            g.drawText(scoreTxt, juce::Rectangle<float>(centre.x - innerR, centre.y + 23, innerR*2, 12), juce::Justification::centred);
        }
    }

    void timerCallback() override
    {
        if (state == 1) repaint(); // animate when learning
    }

private:
    std::vector<MaqamCandidate> candidates;
    int state = 0; // 0 idle,1 learning,2 done
    float progress = 0.0f;
};
