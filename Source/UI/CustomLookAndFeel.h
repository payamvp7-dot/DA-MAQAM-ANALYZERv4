#pragma once
#include <JuceHeader.h>

//==============================================================================
/**
 * D.A Production premium dark theme: midnight navy/charcoal + muted warm gold
 * Persian geometric subtle motifs, high-DPI aware.
 */
class DALookAndFeel : public juce::LookAndFeel_V4
{
public:
    DALookAndFeel();

    // Palette
    static const juce::Colour midnightNavy;   // #0F1623
    static const juce::Colour charcoalDark;    // #181E2A
    static const juce::Colour charcoalMid;     // #222B3A
    static const juce::Colour charcoalLight;   // #2E384B
    static const juce::Colour goldMuted;       // #C2A878
    static const juce::Colour goldBright;      // #E2C99A
    static const juce::Colour goldGlow;        // #C2A878 40%
    static const juce::Colour textPrimary;     // #E8E6E1
    static const juce::Colour textSecondary;   // #9AA0B0
    static const juce::Colour accentTeal;      // #6BA3A8 muted teal for level
    static const juce::Colour dangerAmber;     // #C98B4A

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider&) override;

    void drawTableHeaderBackground(juce::Graphics&, juce::TableHeaderComponent&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DALookAndFeel)
};
