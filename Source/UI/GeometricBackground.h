#pragma once
#include <JuceHeader.h>

// Draws subtle Persian/Arabic 8-point star geometric motif as background pattern
class GeometricBackground : public juce::Component
{
public:
    GeometricBackground() {}
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        // base already painted by parent, just overlay very subtle pattern
        g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.035f));
        const float step = 42.0f;
        for (float y = -step; y < bounds.getHeight()+step; y += step)
            for (float x = -step; x < bounds.getWidth()+step; x += step)
                drawStar(g, x, y, 14.0f);
    }
private:
    void drawStar(juce::Graphics& g, float cx, float cy, float r)
    {
        juce::Path p;
        for (int i=0;i<8;++i)
        {
            float a = juce::MathConstants<float>::pi * i / 4.0f;
            float rad = (i%2==0) ? r : r*0.45f;
            float x = cx + std::cos(a) * rad;
            float y = cy + std::sin(a) * rad;
            if(i==0) p.startNewSubPath(x,y);
            else p.lineTo(x,y);
        }
        p.closeSubPath();
        g.strokePath(p, juce::PathStrokeType(0.7f));
        // inner diamond
        juce::Path inner;
        inner.addRectangle(cx-1.2f, cy-1.2f, 2.4f, 2.4f);
        g.fillPath(inner);
        // apply rotation 45deg for diamond look handled via rect
    }
};
