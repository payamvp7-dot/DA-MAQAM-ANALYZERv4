#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/GeometricBackground.h"
#include "UI/MaqamDialComponent.h"
#include "UI/PitchGraphComponent.h"
#include "UI/TuningPanelComponent.h"

//==============================================================================
class MaqamAnalyzerAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit MaqamAnalyzerAudioProcessorEditor(MaqamAnalyzerAudioProcessor& p);
    ~MaqamAnalyzerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MaqamAnalyzerAudioProcessor& processor;
    DALookAndFeel lookAndFeel;
    GeometricBackground geoBg;

    // Header
    juce::Label brandLabel, titleLabel, versionLabel;

    // Left column
    juce::ComboBox traditionBox;
    juce::TextEditor searchBox;
    juce::ListBox maqamListBox;
    juce::Label browserTitle, browserDesc;
    juce::TextEditor descBox; // multi-line
    juce::Label selectedInfo;

    // Center
    MaqamDialComponent dial;
    juce::TextButton learnBtn, stopBtn, resetBtn;
    juce::ComboBox durationBox;
    std::unique_ptr<PitchGraphComponent> pitchGraph;
    juce::Label liveNoteLbl, levelLbl;

    // Right
    struct CandidateRow : public juce::Component
    {
        void setCandidate(const MaqamCandidate& c, int rank);
        void paint(juce::Graphics& g) override;
        void resized() override;
        MaqamCandidate cand;
        int rank=0;
        bool has=false;
        juce::Label nameLbl, tonicLbl, scoreLbl, evidLbl;
    };
    std::array<CandidateRow,3> candidateRows;
    juce::Label candidatesTitle, uncertaintyLbl, limitationLbl;
    std::unique_ptr<TuningPanelComponent> tuningPanel;

    // Bottom
    juce::Label statusSampleRate, statusVoiced, statusState, statusCPU;

    // Data
    std::vector<MaqamEntry> filteredEntries;
    int selectedBrowserIndex = -1;

    void rebuildMaqamList();
    void updateFromProcessor();
    void applyTraditionFilter();

    struct MaqamListModel : public juce::ListBoxModel
    {
        MaqamAnalyzerAudioProcessorEditor* owner = nullptr;
        int getNumRows() override { return (int) owner->filteredEntries.size(); }
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override
        {
            if (row >= (int)owner->filteredEntries.size()) return;
            auto& e = owner->filteredEntries[row];
            g.fillAll(sel ? juce::Colour(0xFFC2A878).withAlpha(0.18f) : (row%2==0 ? juce::Colour(0xFF1E2636) : juce::Colour(0xFF181E2A)));
            // subtle gold left bar if selected
            if (sel) { g.setColour(juce::Colour(0xFFC2A878)); g.fillRect(0,0,3,h); }
            // microtonal dot
            bool micro = e.isMicrotonal();
            juce::String txt = e.displayName + " [" + e.category + "]" + (micro ? " • micro" : "");
            g.setColour(sel ? juce::Colour(0xFFE8E6E1) : juce::Colour(0xFF9AA0B0));
            g.setFont(juce::Font(juce::FontOptions(sel?12.0f:11.5f).withStyle(sel?"Bold":"Regular")));
            g.drawText(txt, juce::Rectangle<int>(8,0,w-10,h), juce::Justification::centredLeft, true);
            // tradition color dot
            juce::Colour dot = juce::Colour(0xFF6BA3A8);
            if (e.tradition==Tradition::Persian) dot = juce::Colour(0xFFC2A878);
            else if (e.tradition==Tradition::Turkish) dot = juce::Colour(0xFF8FA8C2);
            else if (e.tradition==Tradition::Arabic) dot = juce::Colour(0xFF9AC2A8);
            g.setColour(dot);
            g.fillEllipse(w-10, h/2-3, 6,6);
        }
        void listBoxItemClicked(int row, const juce::MouseEvent&) override
        {
            owner->selectedBrowserIndex = row;
            owner->maqamListBox.repaint();
            if (row >=0 && row < (int)owner->filteredEntries.size())
            {
                auto& e = owner->filteredEntries[row];
                juce::String desc = "» " + e.displayName + " (" + e.transliteration + ") — " + traditionToString(e.tradition) + " / " + e.category + "\n";
                if (e.parentFamily.isNotEmpty()) desc += "Family: " + e.parentFamily + "\n";
                desc += "Intervals (¢ from tonic): ";
                for (auto v: e.intervalsCents) desc += juce::String(v) + " ";
                desc += "\nDegrees: ";
                for (auto& d: e.degreeNames) desc += d + " ";
                desc += "\n\nDescription: " + e.description + "\n";
                desc += "Seyir: " + e.seyir + "\n";
                desc += "Finalis: " + e.finalis + " • Dominant: " + e.dominant + " • Cadences: " + e.cadences + "\n";
                if (!e.ajnas.empty())
                {
                    desc += "Ajnas/Tetrachords: ";
                    for (auto& j: e.ajnas) desc += j.name + " ";
                    desc += "\n";
                }
                if (!e.modulatesTo.empty())
                {
                    desc += "Modulates to: ";
                    for (auto& m: e.modulatesTo) desc += m + " ";
                    desc += "\n";
                }
                desc += "Source: " + e.source + "\nVerification: " + e.verificationStatus + "\n";
                if (e.limitationNote.isNotEmpty()) desc += "Note: " + e.limitationNote + "\n";
                owner->descBox.setText(desc, false);
                owner->selectedInfo.setText(e.displayName + " — tap to load as manual override (applies to tuning panel)", juce::dontSendNotification);
            }
        }
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
        {
            listBoxItemClicked(row, {});
            // set manual maqam override
            if (row>=0 && row < (int)owner->filteredEntries.size())
            {
                owner->processor.uiState.manualMaqamId = owner->filteredEntries[row].id;
                owner->processor.uiState.hasManualMaqam = true;
                owner->updateFromProcessor();
            }
        }
    } listModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MaqamAnalyzerAudioProcessorEditor)
};
