#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
class TuningPanelComponent : public juce::Component
{
public:
    explicit TuningPanelComponent(MaqamAnalyzerAudioProcessor& p) : processor(p)
    {
        addAndMakeVisible(titleLbl);
        titleLbl.setText("TUNING GUIDANCE", juce::dontSendNotification);
        titleLbl.setJustificationType(juce::Justification::centredLeft);
        titleLbl.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        titleLbl.setColour(juce::Label::textColourId, juce::Colour(0xFFC2A878));

        addAndMakeVisible(infoLbl);
        infoLbl.setJustificationType(juce::Justification::topLeft);
        infoLbl.setColour(juce::Label::textColourId, juce::Colour(0xFF9AA0B0));
        infoLbl.setFont(juce::Font(juce::FontOptions(11.0f)));

        addAndMakeVisible(table);
        table.setModel(this);
        table.getHeader().addColumn("Degree", 1, 55);
        table.getHeader().addColumn("Pitch", 2, 50);
        table.getHeader().addColumn("Theoretical ¢", 3, 90);
        table.getHeader().addColumn("Observed ¢", 4, 90);
        table.getHeader().addColumn("Offset ¢", 5, 70);
        table.getHeader().addColumn("Auto-Tune", 6, 90);
        table.setHeaderHeight(22);
        table.setRowHeight(20);
        table.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF181E2A));

        addAndMakeVisible(a4Label);
        a4Label.setText("A4:", juce::dontSendNotification);
        a4Label.setColour(juce::Label::textColourId, juce::Colour(0xFF9AA0B0));
        addAndMakeVisible(a4Slider);
        a4Slider.setRange(415, 465, 0.1);
        a4Slider.setValue(processor.uiState.referenceA4);
        a4Slider.setSliderStyle(juce::Slider::LinearHorizontal);
        a4Slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 18);
        a4Slider.onValueChange = [this]{ processor.uiState.referenceA4 = a4Slider.getValue(); processor.apvts.getParameter("a4ref")->setValueNotifyingHost((float)((a4Slider.getValue()-415)/50.0)); buildFromCandidate(); };

        addAndMakeVisible(copyBtn); copyBtn.setButtonText("Copy TXT");
        addAndMakeVisible(csvBtn); csvBtn.setButtonText("Export CSV");
        addAndMakeVisible(sclBtn); sclBtn.setButtonText("Export SCL");

        copyBtn.onClick = [this]{ juce::SystemClipboard::copyTextToClipboard(processor.generateTXTReport()); copyBtn.setButtonText("Copied!"); juce::Timer::callAfterDelay(1500, [this]{ copyBtn.setButtonText("Copy TXT");}); };
        csvBtn.onClick = [this]{ exportCSV(); };
        sclBtn.onClick = [this]{ exportSCL(); };

        // candidate selector
        addAndMakeVisible(candBox);
        candBox.addItem("Candidate 1 (top)", 1);
        candBox.addItem("Candidate 2", 2);
        candBox.addItem("Candidate 3", 3);
        candBox.setSelectedId(1);
        candBox.onChange = [this]{ buildFromCandidate(); };

        // manual tonic edit
        addAndMakeVisible(tonicLbl); tonicLbl.setText("Tonic (manual, empty=auto):", juce::dontSendNotification);
        tonicLbl.setColour(juce::Label::textColourId, juce::Colour(0xFF9AA0B0));
        tonicLbl.setFont(juce::Font(juce::FontOptions(11.0f)));
        addAndMakeVisible(tonicEdit);
        tonicEdit.setText(processor.uiState.manualTonic);
        tonicEdit.onTextChange = [this]{ processor.uiState.manualTonic = tonicEdit.getText().trim(); };

        buildFromCandidate();
    }

    void rebuild()
    {
        buildFromCandidate();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF181E2A));
        // subtle inner shadow
        g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.12f));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 8, 1.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(8);
        titleLbl.setBounds(r.removeFromTop(18));
        r.removeFromTop(4);
        auto topRow = r.removeFromTop(22);
        candBox.setBounds(topRow.removeFromLeft(160));
        tonicLbl.setBounds(topRow.removeFromLeft(145).expanded(4,0));
        tonicEdit.setBounds(topRow.removeFromLeft(62));
        r.removeFromTop(6);
        auto a4Row = r.removeFromTop(20);
        a4Label.setBounds(a4Row.removeFromLeft(24));
        a4Slider.setBounds(a4Row);
        r.removeFromTop(6);
        infoLbl.setBounds(r.removeFromTop(44));
        r.removeFromTop(4);
        table.setBounds(r.removeFromTop(170));
        r.removeFromTop(8);
        auto btnRow = r.removeFromTop(26);
        copyBtn.setBounds(btnRow.removeFromLeft(78)); btnRow.removeFromLeft(6);
        csvBtn.setBounds(btnRow.removeFromLeft(82)); btnRow.removeFromLeft(6);
        sclBtn.setBounds(btnRow.removeFromLeft(82));
        r.removeFromTop(6);
        // note below
        gNoteBounds = r;
    }

    // TableListBoxModel
    int getNumRows() override { return (int) rows.size(); }
    void paintRowBackground(juce::Graphics& g, int row, int w, int h, bool sel) override
    {
        g.fillAll(row%2==0 ? juce::Colour(0xFF1E2636) : juce::Colour(0xFF181E2A));
        if (sel) g.fillAll(juce::Colour(0xFFC2A878).withAlpha(0.12f));
        // microtonal highlight: if offset >30c
        if (row < (int)rows.size() && std::abs(rows[row].offsetCents) > 30.0f)
        {
            g.setColour(juce::Colour(0xFFC2A878).withAlpha(0.10f));
            g.fillRect(0,0,w,h);
        }
    }
    void paintCell(juce::Graphics& g, int row, int colId, int w, int h, bool sel) override
    {
        if (row >= (int)rows.size()) return;
        auto& r = rows[row];
        juce::String txt;
        juce::Colour col = juce::Colour(0xFFE8E6E1);
        switch(colId)
        {
            case 1: txt = r.degree; break;
            case 2: txt = r.noteName; break;
            case 3: txt = juce::String(r.theoreticalCents) + "¢"; break;
            case 4: txt = r.hasObserved ? juce::String(r.observedCents,1) + "¢" : "—"; break;
            case 5: { txt = (r.offsetCents>=0?"+":"") + juce::String(r.offsetCents,1) + "¢";
                      col = std::abs(r.offsetCents) > 30 ? juce::Colour(0xFFC2A878) : juce::Colour(0xFF6BA3A8);
                      break; }
            case 6: txt = r.autoTuneNote; col = r.requiresManual ? juce::Colour(0xFFC98B4A) : juce::Colour(0xFF9AA0B0); break;
        }
        g.setColour(col);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(txt, juce::Rectangle<int>(2,0,w-4,h), juce::Justification::centred);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        if (gNoteBounds.isEmpty()) return;
        g.setColour(juce::Colour(0xFF9AA0B0));
        g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Italic")));
        juce::String note = "Distinguish detected sung intonation (observed) from theoretical maqam intervals. "
                            "Standard Auto-Tune/ Waves Tune key-scale selectors alone do NOT implement maqam cents; "
                            "use manual note offsets or a microtuning-capable host/plug-in. No direct control of third-party plug-ins is claimed.";
        g.drawFittedText(note, gNoteBounds, juce::Justification::topLeft, 4);
    }

private:
    MaqamAnalyzerAudioProcessor& processor;

    struct Row
    {
        juce::String degree;
        juce::String noteName;
        int theoreticalCents=0;
        float observedCents=0; bool hasObserved=false;
        float offsetCents=0;
        juce::String autoTuneNote;
        bool requiresManual=false;
    };
    std::vector<Row> rows;

    juce::Label titleLbl, infoLbl, a4Label, tonicLbl;
    juce::ComboBox candBox;
    juce::TextEditor tonicEdit;
    juce::Slider a4Slider;
    juce::TextButton copyBtn, csvBtn, sclBtn;
    juce::TableListBox table { "Tuning", this };
    juce::Rectangle<int> gNoteBounds;

    void buildFromCandidate()
    {
        rows.clear();
        auto res = processor.getLatestResult();
        int idx = candBox.getSelectedId()-1;
        if (!res.valid || idx <0 || idx >= (int)res.candidates.size())
        {
            infoLbl.setText("No analysis yet - press LEARN on a vocal phrase.", juce::dontSendNotification);
            table.updateContent(); table.repaint();
            return;
        }
        auto& cand = res.candidates[idx];
        infoLbl.setText("Tuning for: " + cand.entry.displayName + " (" + traditionToString(cand.entry.tradition) + ") • Tonic " + cand.tonic.noteName + " • " + cand.entry.description.substring(0, 110) + "...",
                        juce::dontSendNotification);

        // Build rows per degree
        for (size_t i=0;i<cand.entry.intervalsCents.size();++i)
        {
            Row r;
            r.degree = juce::String((int)i+1);
            if (i < cand.entry.degreeNames.size()) r.degree = cand.entry.degreeNames[i] + " (" + r.degree + ")";
            // note name from tonic + interval
            int tonicMidi = cand.tonic.midiNote;
            float midiExact = tonicMidi + cand.entry.intervalsCents[i] / 100.0f;
            int midiRound = (int) std::round(midiExact);
            float centOff = (midiExact - midiRound)*100.0f;
            static const char* n12[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            int pc=((midiRound%12)+12)%12; int oct=midiRound/12-1;
            r.noteName = juce::String(n12[pc]) + juce::String(oct);
            if (std::abs(centOff) > 12) r.noteName += (centOff>0 ? " +":" ") + juce::String((int)centOff) + "c";
            r.theoreticalCents = cand.entry.intervalsCents[i];
            if (idx < (int)cand.observedVsExpectedCents.size())
            {
                r.hasObserved = true;
                r.observedCents = cand.observedVsExpectedCents[i];
                r.offsetCents = r.observedCents; // deviation from theoretical
            }
            // Auto-Tune guidance: which notes need manual cents
            if (std::abs(centOff) > 12 || std::abs(r.offsetCents) > 20)
            {
                r.autoTuneNote = "Manual offset " + juce::String((int)centOff) + "c";
                r.requiresManual = true;
            }
            else r.autoTuneNote = "Standard 12-TET";
            rows.push_back(r);
        }
        table.updateContent(); table.repaint();
    }

    void exportCSV()
    {
        auto txt = processor.generateCSVReport();
        juce::FileChooser fc("Save CSV report", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("DA_Maqam_Report.csv"), "*.csv");
        fc.launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [txt](const juce::FileChooser& c)
        {
            auto f = c.getResult(); if(f != juce::File()) f.replaceWithText(txt);
        });
    }
    void exportSCL()
    {
        int idx = candBox.getSelectedId()-1;
        auto txt = processor.generateScalaSCL(idx);
        juce::FileChooser fc("Save Scala SCL", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("maqam.scl"), "*.scl");
        fc.launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [txt](const juce::FileChooser& c)
        {
            auto f = c.getResult(); if(f != juce::File()) f.replaceWithText(txt);
        });
    }
};
