#include "PluginEditor.h"

//==============================================================================
MaqamAnalyzerAudioProcessorEditor::MaqamAnalyzerAudioProcessorEditor(MaqamAnalyzerAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(geoBg);
    geoBg.setInterceptsMouseClicks(false,false);

    // Header
    brandLabel.setText("D.A Production", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    brandLabel.setColour(juce::Label::textColourId, DALookAndFeel::goldMuted);
    brandLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(brandLabel);

    titleLabel.setText("D.A MAQAM ANALYZER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, DALookAndFeel::textPrimary);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    versionLabel.setText("v1.0.0 • Windows x64 VST3 • Pass-through Analyzer", juce::dontSendNotification);
    versionLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    versionLabel.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    versionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(versionLabel);

    // Tradition tabs (as combo for compactness + tab feel)
    addAndMakeVisible(traditionBox);
    traditionBox.addItem("All Traditions", 1);
    traditionBox.addItem("Persian-Iranian", 2);
    traditionBox.addItem("Azerbaijani Mugham", 3);
    traditionBox.addItem("Turkish Makam", 4);
    traditionBox.addItem("Arabic Maqam", 5);
    traditionBox.setSelectedId(1);
    traditionBox.onChange = [this]{
        int id = traditionBox.getSelectedId();
        processor.uiState.traditionFilter = (MaqamAnalyzerAudioProcessor::TraditionFilter)(id-1);
        processor.apvts.getParameter("tradition")->setValueNotifyingHost((id-1)/4.0f);
        rebuildMaqamList();
    };

    // Search
    searchBox.setPlaceholder("Search maqam, dastgah, mugham, seyir...");
    searchBox.setColour(juce::TextEditor::backgroundColourId, DALookAndFeel::charcoalMid);
    searchBox.setColour(juce::TextEditor::textColourId, DALookAndFeel::textPrimary);
    searchBox.setColour(juce::TextEditor::outlineColourId, DALookAndFeel::goldMuted.withAlpha(0.25f));
    searchBox.onTextChange = [this]{ rebuildMaqamList(); };
    addAndMakeVisible(searchBox);

    // Browser title
    browserTitle.setText("MAQAM BROWSER", juce::dontSendNotification);
    browserTitle.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    browserTitle.setColour(juce::Label::textColourId, DALookAndFeel::goldMuted);
    addAndMakeVisible(browserTitle);

    // ListBox
    listModel.owner = this;
    maqamListBox.setModel(&listModel);
    maqamListBox.setRowHeight(26);
    maqamListBox.setColour(juce::ListBox::backgroundColourId, DALookAndFeel::charcoalDark);
    maqamListBox.setOutlineThickness(1);
    addAndMakeVisible(maqamListBox);

    // Desc
    descBox.setMultiLine(true); descBox.setReadOnly(true);
    descBox.setColour(juce::TextEditor::backgroundColourId, DALookAndFeel::charcoalDark);
    descBox.setColour(juce::TextEditor::textColourId, DALookAndFeel::textSecondary);
    descBox.setFont(juce::Font(juce::FontOptions(11.0f)));
    descBox.setText("Select a maqam to view tradition-specific intervals, seyir, cadences, and provenance. Double-click to set as manual override.", false);
    addAndMakeVisible(descBox);

    selectedInfo.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    selectedInfo.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Italic")));
    addAndMakeVisible(selectedInfo);

    // Center dial
    addAndMakeVisible(dial);

    // Learn controls
    learnBtn.setButtonText("● LEARN");
    learnBtn.setName("learnPrimary");
    learnBtn.setClickingTogglesState(true);
    learnBtn.setColour(juce::TextButton::buttonColourId, DALookAndFeel::goldMuted);
    learnBtn.setColour(juce::TextButton::textColourOffId, DALookAndFeel::midnightNavy);
    learnBtn.onClick = [this]{
        if (learnBtn.getToggleState())
        {
            processor.startLearn();
            learnBtn.setButtonText("● LEARNING…");
            stopBtn.setEnabled(true);
        }
        else
        {
            processor.stopLearn();
            learnBtn.setButtonText("● LEARN");
        }
    };
    addAndMakeVisible(learnBtn);

    stopBtn.setButtonText("STOP");
    stopBtn.setEnabled(false);
    stopBtn.onClick = [this]{
        processor.stopLearn();
        learnBtn.setToggleState(false, juce::dontSendNotification);
        learnBtn.setButtonText("● LEARN");
        stopBtn.setEnabled(false);
    };
    addAndMakeVisible(stopBtn);

    resetBtn.setButtonText("RESET");
    resetBtn.onClick = [this]{
        processor.resetAnalysis();
        learnBtn.setToggleState(false, juce::dontSendNotification);
        learnBtn.setButtonText("● LEARN");
        stopBtn.setEnabled(false);
        if (pitchGraph) pitchGraph->clear();
        updateFromProcessor();
    };
    addAndMakeVisible(resetBtn);

    durationBox.addItem("10 sec", 1);
    durationBox.addItem("20 sec", 2);
    durationBox.addItem("30 sec", 3);
    durationBox.addItem("60 sec", 4);
    durationBox.addItem("Manual STOP", 5);
    durationBox.setSelectedId(2);
    durationBox.onChange = [this]{
        int id = durationBox.getSelectedId();
        int map[5]={10,20,30,60, 999};
        processor.uiState.learnDurationSec = map[id-1];
        juce::StringArray opts{"10 sec","20 sec","30 sec","60 sec"};
        int idx = juce::jlimit(0,3, id-1);
        if (id<=4) processor.apvts.getParameter("learnDur")->setValueNotifyingHost(idx/3.0f);
    };
    addAndMakeVisible(durationBox);

    // Pitch graph
    pitchGraph = std::make_unique<PitchGraphComponent>(processor);
    addAndMakeVisible(*pitchGraph);

    liveNoteLbl.setText("Note: —  Hz: —  Cents: —  Confidence: —", juce::dontSendNotification);
    liveNoteLbl.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    liveNoteLbl.setColour(juce::Label::textColourId, DALookAndFeel::textPrimary);
    liveNoteLbl.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(liveNoteLbl);

    levelLbl.setFont(juce::Font(juce::FontOptions(10.0f)));
    levelLbl.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    addAndMakeVisible(levelLbl);

    // Candidates
    candidatesTitle.setText("TOP CANDIDATES", juce::dontSendNotification);
    candidatesTitle.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    candidatesTitle.setColour(juce::Label::textColourId, DALookAndFeel::goldMuted);
    addAndMakeVisible(candidatesTitle);
    for (int i=0;i<3;++i)
    {
        addAndMakeVisible(candidateRows[i]);
    }

    uncertaintyLbl.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Italic")));
    uncertaintyLbl.setColour(juce::Label::textColourId, DALookAndFeel::dangerAmber);
    uncertaintyLbl.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(uncertaintyLbl);

    limitationLbl.setFont(juce::Font(juce::FontOptions(10.0f)));
    limitationLbl.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    limitationLbl.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(limitationLbl);

    // Tuning panel
    tuningPanel = std::make_unique<TuningPanelComponent>(processor);
    addAndMakeVisible(*tuningPanel);

    // Bottom status
    statusSampleRate.setFont(juce::Font(juce::FontOptions(10.0f)));
    statusSampleRate.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    addAndMakeVisible(statusSampleRate);
    statusVoiced.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    statusVoiced.setColour(juce::Label::textColourId, DALookAndFeel::accentTeal);
    addAndMakeVisible(statusVoiced);
    statusState.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    statusState.setColour(juce::Label::textColourId, DALookAndFeel::goldMuted);
    addAndMakeVisible(statusState);
    statusCPU.setFont(juce::Font(juce::FontOptions(10.0f)));
    statusCPU.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    addAndMakeVisible(statusCPU);

    setSize(1280, 800);
    setResizable(true, true);
    setResizeLimits(1100, 680, 1920, 1080);

    rebuildMaqamList();
    startTimerHz(20);
}

MaqamAnalyzerAudioProcessorEditor::~MaqamAnalyzerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void MaqamAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    // base gradient
    juce::ColourGradient grad(DALookAndFeel::midnightNavy, bounds.getTopLeft(),
                              DALookAndFeel::charcoalDark, bounds.getBottomLeft(), false);
    g.setGradientFill(grad);
    g.fillAll();

    // header strip
    auto header = bounds.removeFromTop(54);
    g.setColour(DALookAndFeel::charcoalDark);
    g.fillRect(header);
    g.setColour(DALookAndFeel::goldMuted.withAlpha(0.35f));
    g.drawHorizontalLine((int)header.getBottom()-1, header.getX(), header.getRight());

    // gold accent under title
    g.setColour(DALookAndFeel::goldMuted.withAlpha(0.55f));
    g.fillRoundedRectangle(juce::Rectangle<float>(header.getCentreX()-60, header.getBottom()-3, 120, 1.5f), 1);

    // decorative left column hint: very subtle vertical gold line separating left
    // (actual layout lines drawn in resized via components borders)

    // bottom status strip
    auto bottom = bounds.sliceFromBottom(22);
    g.setColour(DALookAndFeel::charcoalMid);
    g.fillRect(bottom);
    g.setColour(DALookAndFeel::goldMuted.withAlpha(0.20f));
    g.drawHorizontalLine((int)bottom.getY(), bottom.getX(), bottom.getRight());
}

void MaqamAnalyzerAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();
    auto header = b.removeFromTop(54);
    brandLabel.setBounds(header.removeFromLeft(160).reduced(12, 8));
    versionLabel.setBounds(header.removeFromRight(260).reduced(8));
    titleLabel.setBounds(header.reduced(8));

    auto bottom = b.removeFromBottom(22);
    statusSampleRate.setBounds(bottom.removeFromLeft(150).reduced(6,2));
    statusVoiced.setBounds(bottom.removeFromLeft(160).reduced(6,2));
    statusState.setBounds(bottom.removeFromLeft(180).reduced(6,2));
    statusCPU.setBounds(bottom.reduced(6,2));
    bottom = b; // keep for reference

    geoBg.setBounds(b);

    // grid: left 300, center flexible, right 380
    int leftW = 300;
    int rightW = 380;
    auto left = b.removeFromLeft(leftW);
    auto right = b.removeFromRight(rightW);
    auto center = b;

    left.reduce(8,6);
    right.reduce(8,6);
    center.reduce(6,6);

    // Left column: oud image (vector placeholder) 110px, tradition, search, list, desc
    // Oud area
    auto oudArea = left.removeFromTop(110);
    // we don't have separate oud component; left column top will show outline via paint elsewhere, but we keep space
    // tradition
    auto tradRow = left.removeFromTop(30);
    traditionBox.setBounds(tradRow.reduced(2));
    left.removeFromTop(6);
    auto searchRow = left.removeFromTop(28);
    searchBox.setBounds(searchRow);
    left.removeFromTop(6);
    auto browserTitleRow = left.removeFromTop(16);
    browserTitle.setBounds(browserTitleRow);
    left.removeFromTop(2);
    // list takes most
    auto descArea = left.removeFromBottom(180);
    auto listArea = left;
    left = listArea;
    maqamListBox.setBounds(listArea);
    descArea.removeFromTop(4);
    auto infoRow = descArea.removeFromTop(14);
    selectedInfo.setBounds(infoRow);
    descArea.removeFromTop(2);
    descBox.setBounds(descArea);

    // Right column
    auto candTitleRow = right.removeFromTop(18);
    candidatesTitle.setBounds(candTitleRow);
    right.removeFromTop(6);
    for (int i=0;i<3;++i)
    {
        auto row = right.removeFromTop(88);
        candidateRows[i].setBounds(row.reduced(1));
        right.removeFromTop(4);
    }
    auto uncArea = right.removeFromTop(56);
    uncertaintyLbl.setBounds(uncArea);
    right.removeFromTop(4);
    auto limArea = right.removeFromTop(56);
    limitationLbl.setBounds(limArea);
    right.removeFromTop(6);
    tuningPanel->setBounds(right);

    // Center column
    auto dialArea = center.removeFromTop(240);
    // dial centered
    int dialSize = juce::jmin(dialArea.getWidth(), dialArea.getHeight()) - 10;
    dial.setBounds(juce::Rectangle<int>(dialSize, dialSize).withCentre(dialArea.getCentre()));

    // controls under dial
    auto ctrlRow = center.removeFromTop(40);
    ctrlRow.reduce(8,4);
    auto durBox = ctrlRow.removeFromRight(110);
    durationBox.setBounds(durBox.reduced(4,2));
    auto reset = ctrlRow.removeFromRight(70);
    resetBtn.setBounds(reset.reduced(4,2));
    auto stop = ctrlRow.removeFromRight(70);
    stopBtn.setBounds(stop.reduced(4,2));
    auto learn = ctrlRow;
    learnBtn.setBounds(learn.reduced(4,2));

    center.removeFromTop(4);
    auto liveRow = center.removeFromTop(18);
    liveNoteLbl.setBounds(liveRow.removeFromLeft(liveRow.getWidth()-90));
    levelLbl.setBounds(liveRow);
    center.removeFromTop(4);
    // graph occupies rest
    pitchGraph->setBounds(center);

    // oud placeholder draw hint: repaint left area overlay? handled via editor paint's geoBg already
    (void)oudArea;
}

void MaqamAnalyzerAudioProcessorEditor::timerCallback()
{
    updateFromProcessor();
}

void MaqamAnalyzerAudioProcessorEditor::rebuildMaqamList()
{
    auto query = searchBox.getText();
    int tradId = traditionBox.getSelectedId();
    std::vector<MaqamEntry> base;
    if (tradId == 1)
    {
        base = processor.getDatabase().getAllEntries();
    }
    else
    {
        Tradition t = (Tradition)(tradId-2);
        base = processor.getDatabase().getFiltered(t);
    }

    if (query.isEmpty())
    {
        filteredEntries = base;
    }
    else
    {
        filteredEntries.clear();
        auto q = query.toLowerCase();
        for (auto& e: base)
        {
            if (e.displayName.toLowerCase().contains(q) ||
                e.transliteration.toLowerCase().contains(q) ||
                e.description.toLowerCase().contains(q) ||
                e.category.toLowerCase().contains(q) ||
                traditionToString(e.tradition).toLowerCase().contains(q))
                filteredEntries.push_back(e);
            else
            {
                for (auto& a: e.aliases) if (a.toLowerCase().contains(q)) { filteredEntries.push_back(e); break; }
            }
        }
    }

    // sort by tradition then name for stable browsing
    std::sort(filteredEntries.begin(), filteredEntries.end(), [](const MaqamEntry& a, const MaqamEntry& b){
        if ((int)a.tradition != (int)b.tradition) return (int)a.tradition < (int)b.tradition;
        return a.displayName < b.displayName;
    });

    maqamListBox.updateContent();
    maqamListBox.repaint();
}

void MaqamAnalyzerAudioProcessorEditor::applyTraditionFilter()
{
    rebuildMaqamList();
}

void MaqamAnalyzerAudioProcessorEditor::updateFromProcessor()
{
    auto state = processor.getAnalysisState();
    float prog = (float) processor.getLearnProgress();
    int voiced = processor.getVoicedFrameCount();
    float rmsDb = processor.currentInputLevelDb.load();

    // Dial
    auto snap = processor.getLatestResult();
    if (snap.valid) dial.setCandidates(snap.candidates);
    else dial.setCandidates({});

    int dialState = 0;
    if (state == MaqamAnalyzerAudioProcessor::AnalysisState::Learning) dialState = 1;
    else if (state == MaqamAnalyzerAudioProcessor::AnalysisState::Completed) dialState = 2;
    dial.setAnalysisState(dialState);
    dial.setProgress(prog);

    // live note
    auto pf = processor.getLastPitchFrame();
    juce::String noteTxt;
    if (pf.voiced && pf.frequencyHz > 20)
        noteTxt = "Note: " + pf.noteName + "  Hz: " + juce::String(pf.frequencyHz,1) + "  Cents: " + (pf.centsOffset>=0?"+":"") + juce::String(pf.centsOffset,1) + "¢  Conf: " + juce::String(pf.confidence,2);
    else
        noteTxt = "Note: —  Hz: —  Cents: —  Confidence: —  (sing steady monophonic tone)";
    liveNoteLbl.setText(noteTxt, juce::dontSendNotification);

    // level
    juce::String lvl = juce::String(rmsDb,1) + " dB  ";
    if (rmsDb < -48) lvl += "◼ Low";
    else if (rmsDb < -18) lvl += "◼ Good";
    else if (rmsDb < -6) lvl += "◼ Strong";
    else lvl += "◼ Clip!";
    levelLbl.setText(lvl, juce::dontSendNotification);

    // candidates
    if (snap.valid && !snap.candidates.empty())
    {
        for (int i=0;i<3;++i)
        {
            if (i < (int)snap.candidates.size()) candidateRows[i].setCandidate(snap.candidates[i], i+1);
            else candidateRows[i].setCandidate(MaqamCandidate{}, i+1);
        }
        uncertaintyLbl.setText("Uncertainty: " + snap.uncertaintyNote, juce::dontSendNotification);
        limitationLbl.setText("Limits: " + snap.limitationNote, juce::dontSendNotification);
        if (tuningPanel) tuningPanel->rebuild();
    }
    else
    {
        for (int i=0;i<3;++i) candidateRows[i].setCandidate(MaqamCandidate{}, i+1);
        if (state == MaqamAnalyzerAudioProcessor::AnalysisState::Learning)
        {
            uncertaintyLbl.setText("Learning… " + juce::String(prog*100,0) + "%  Voiced frames: " + juce::String(voiced) + " / ~" + juce::String(processor.uiState.learnDurationSec * 50) + " (50 fps)", juce::dontSendNotification);
            limitationLbl.setText("Keep singing - stable, monophonic, dry vocal. Reverb/polyphony will degrade tracking.", juce::dontSendNotification);
        }
        else if (state == MaqamAnalyzerAudioProcessor::AnalysisState::Idle)
        {
            uncertaintyLbl.setText("Press LEARN - capture 10-60 sec of monophonic vocal for analysis. Results show similarity scores (not probabilities).", juce::dontSendNotification);
            limitationLbl.setText("Analyzer only: audio passes unchanged. Monophonic only; polyphonic masters are unsupported and will be flagged insufficient.", juce::dontSendNotification);
        }
    }

    // bottom status
    statusSampleRate.setText("SR: " + juce::String((int)processor.getSampleRate()) + " Hz  •  Block: " + juce::String(processor.getBlockSize()), juce::dontSendNotification);
    statusVoiced.setText("Voiced: " + juce::String(voiced) + " frames  •  Level: " + juce::String(rmsDb,1) + " dB", juce::dontSendNotification);
    juce::String st;
    if (state==MaqamAnalyzerAudioProcessor::AnalysisState::Idle) st="Idle";
    else if (state==MaqamAnalyzerAudioProcessor::AnalysisState::Learning) st="LEARNING " + juce::String(prog*100,0) + "%";
    else if (state==MaqamAnalyzerAudioProcessor::AnalysisState::Analyzing) st="Analyzing…";
    else st="Completed";
    statusState.setText("State: " + st, juce::dontSendNotification);
    statusCPU.setText("CPU: ~ analyzer 2-4%  •  VST3 • D.A Production", juce::dontSendNotification);

    repaint();
}

//==============================================================================
void MaqamAnalyzerAudioProcessorEditor::CandidateRow::setCandidate(const MaqamCandidate& c, int rnk)
{
    cand = c; rank = rnk; has = (c.similarityScore > 0.01f);
    if (!has)
    {
        nameLbl.setText("—", juce::dontSendNotification);
        tonicLbl.setText("", juce::dontSendNotification);
        scoreLbl.setText("", juce::dontSendNotification);
        evidLbl.setText(rank==1 ? "No result - press LEARN to analyze incoming vocal." : "", juce::dontSendNotification);
        repaint();
        return;
    }
    nameLbl.setText(juce::String(rank) + ". " + c.entry.displayName + " (" + traditionToString(c.entry.tradition) + " - " + c.entry.category + ")", juce::dontSendNotification);
    tonicLbl.setText("Tonic " + c.tonic.noteName + "  •  " + juce::String(c.tonic.frequencyHz,1) + " Hz  •  " + c.entry.finalis, juce::dontSendNotification);
    scoreLbl.setText("Similarity " + juce::String(c.similarityScore,2) + (c.insufficientEvidence ? " • insufficient evidence" : ""), juce::dontSendNotification);
    juce::String ev = c.evidence;
    if (c.limitation.isNotEmpty()) ev += "  Limitation: " + c.limitation;
    evidLbl.setText(ev, juce::dontSendNotification);
    repaint();
}

void MaqamAnalyzerAudioProcessorEditor::CandidateRow::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(has ? (rank==1 ? juce::Colour(0xFF222B3A) : juce::Colour(0xFF1E2636)) : juce::Colour(0xFF181E2A));
    if (rank==1 && has)
    {
        // gold left accent
        g.fillRoundedRectangle(b, 8);
        g.setColour(DALookAndFeel::goldMuted.withAlpha(0.9f));
        g.fillRoundedRectangle(b.removeFromLeft(4), 3);
        // outer glow
        g.setColour(DALookAndFeel::goldGlow);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 8, 1.0f);
    }
    else
    {
        g.fillRoundedRectangle(b, 7);
        g.setColour(juce::Colour(0xFF2E384B).withAlpha(0.6f));
        g.drawRoundedRectangle(b, 7, 0.9f);
    }
}

void MaqamAnalyzerAudioProcessorEditor::CandidateRow::resized()
{
    auto r = getLocalBounds().reduced(8,6);
    nameLbl.setBounds(r.removeFromTop(16));
    tonicLbl.setBounds(r.removeFromTop(13));
    scoreLbl.setBounds(r.removeFromTop(13));
    evidLbl.setBounds(r);
    for (auto* lbl : {&nameLbl,&tonicLbl,&scoreLbl,&evidLbl})
    {
        lbl->setColour(juce::Label::textColourId, juce::Colour(0xFFE8E6E1));
        addAndMakeVisible(*lbl);
    }
    nameLbl.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
    nameLbl.setColour(juce::Label::textColourId, has && rank==1 ? DALookAndFeel::goldBright : DALookAndFeel::textPrimary);
    tonicLbl.setFont(juce::Font(juce::FontOptions(10.5f)));
    tonicLbl.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    scoreLbl.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")));
    scoreLbl.setColour(juce::Label::textColourId, DALookAndFeel::goldMuted);
    evidLbl.setFont(juce::Font(juce::FontOptions(10.0f)));
    evidLbl.setColour(juce::Label::textColourId, DALookAndFeel::textSecondary);
    evidLbl.setJustificationType(juce::Justification::topLeft);
}
