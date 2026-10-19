#include "CustomLookAndFeel.h"

const juce::Colour DALookAndFeel::midnightNavy  { 0xFF0F1623 };
const juce::Colour DALookAndFeel::charcoalDark  { 0xFF181E2A };
const juce::Colour DALookAndFeel::charcoalMid   { 0xFF222B3A };
const juce::Colour DALookAndFeel::charcoalLight { 0xFF2E384B };
const juce::Colour DALookAndFeel::goldMuted     { 0xFFC2A878 };
const juce::Colour DALookAndFeel::goldBright    { 0xFFE2C99A };
const juce::Colour DALookAndFeel::goldGlow      { 0x66C2A878 };
const juce::Colour DALookAndFeel::textPrimary   { 0xFFE8E6E1 };
const juce::Colour DALookAndFeel::textSecondary { 0xFF9AA0B0 };
const juce::Colour DALookAndFeel::accentTeal    { 0xFF6BA3A8 };
const juce::Colour DALookAndFeel::dangerAmber   { 0xFFC98B4A };

DALookAndFeel::DALookAndFeel()
{
    setColour(juce::Label::textColourId, textPrimary);
    setColour(juce::ComboBox::backgroundColourId, charcoalMid);
    setColour(juce::ComboBox::textColourId, textPrimary);
    setColour(juce::ComboBox::outlineColourId, goldMuted.withAlpha(0.3f));
    setColour(juce::TextButton::buttonColourId, charcoalMid);
    setColour(juce::TextButton::buttonOnColourId, goldMuted);
    setColour(juce::TextButton::textColourOffId, textPrimary);
    setColour(juce::TextButton::textColourOnId, midnightNavy);
    setColour(juce::Slider::thumbColourId, goldMuted);
    setColour(juce::Slider::trackColourId, charcoalLight);
    setColour(juce::Slider::backgroundColourId, charcoalDark);
    setColour(juce::TableHeaderComponent::backgroundColourId, charcoalMid);
    setColour(juce::TableHeaderComponent::textColourId, goldMuted);
    setColour(juce::PopupMenu::backgroundColourId, charcoalDark);
    setColour(juce::PopupMenu::textColourId, textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, goldMuted);
    setColour(juce::PopupMenu::highlightedTextColourId, midnightNavy);
}

void DALookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                         const juce::Colour&, bool hl, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced(1);
    bool isOn = b.getToggleState();
    juce::Colour base = isOn ? goldMuted : (down ? charcoalLight : charcoalMid);
    if (hl && !isOn) base = base.brighter(0.08f);

    // subtle outer glow for primary
    if (b.getName() == "LEARN" || b.getName() == "learnPrimary")
    {
        if (isOn || hl)
        {
            g.setColour(goldGlow);
            g.fillRoundedRectangle(bounds.expanded(3), 8);
        }
        base = isOn ? juce::Colour(0xFFD44A3A) : goldMuted; // red when learning
    }

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 7);

    // gold hairline border
    g.setColour(isOn ? goldBright.withAlpha(0.6f) : goldMuted.withAlpha(0.35f));
    g.drawRoundedRectangle(bounds, 7, 1.0f);

    // inner highlight
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawRoundedRectangle(bounds.reduced(1), 6, 0.8f);
}

void DALookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool down,
                                 int bx, int by, int bw, int bh, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(w,h).toFloat().reduced(1);
    g.setColour(charcoalMid);
    g.fillRoundedRectangle(bounds, 6);
    g.setColour(goldMuted.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 6, 1.0f);

    // arrow
    auto arrow = juce::Rectangle<float>(bx+2, by+2, bw-4, bh-4);
    juce::Path p;
    p.addTriangle(arrow.getCentreX()-4, arrow.getCentreY()-2,
                  arrow.getCentreX()+4, arrow.getCentreY()-2,
                  arrow.getCentreX(),   arrow.getCentreY()+4);
    g.setColour(textSecondary);
    g.fillPath(p);
}

juce::Font DALookAndFeel::getComboBoxFont(juce::ComboBox&) { return juce::Font(juce::FontOptions(13.0f)); }
juce::Font DALookAndFeel::getLabelFont(juce::Label&) { return juce::Font(juce::FontOptions(13.0f)); }

void DALookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                     float pos, float minPos, float maxPos,
                                     const juce::Slider::SliderStyle style, juce::Slider& s)
{
    auto bounds = juce::Rectangle<float>(x, y, w, h);
    auto track = bounds.withHeight(4).withCentre(bounds.getCentre());

    // background track
    g.setColour(charcoalLight);
    g.fillRoundedRectangle(track, 2);

    // filled
    auto filled = track.withWidth(pos - track.getX());
    g.setColour(goldMuted);
    g.fillRoundedRectangle(filled, 2);

    // thumb
    g.setColour(goldBright);
    g.fillEllipse(juce::Rectangle<float>(pos-7, track.getCentreY()-7, 14, 14));
    g.setColour(midnightNavy.withAlpha(0.6f));
    g.drawEllipse(juce::Rectangle<float>(pos-7, track.getCentreY()-7, 14,14), 1.2f);
}

void DALookAndFeel::drawTableHeaderBackground(juce::Graphics& g, juce::TableHeaderComponent& h)
{
    g.fillAll(charcoalMid);
    g.setColour(goldMuted.withAlpha(0.4f));
    g.drawHorizontalLine(h.getHeight()-1, 0, h.getWidth());
}
