#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Maqam/MaqamDatabase.h"

//==============================================================================
MaqamAnalyzerAudioProcessor::MaqamAnalyzerAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      maqamAnalyzer(maqamDatabase)
{
    workerThread.startThread();
    learnBuffer.setSize(1, 48000 * 64); // 64 sec max
    learnBuffer.clear();
}

MaqamAnalyzerAudioProcessor::~MaqamAnalyzerAudioProcessor()
{
    workerThread.stopThread(2000);
}

juce::AudioProcessorValueTreeState::ParameterLayout MaqamAnalyzerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"a4ref",1}, "A4 Reference", juce::NormalisableRange<float>(415.0f, 465.0f, 0.1f), 440.0f,
        juce::AudioParameterFloatAttributes().withLabel(" Hz")));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"tradition",1}, "Tradition Filter",
        juce::StringArray{"All","Persian-Iranian","Azerbaijani Mugham","Turkish Makam","Arabic Maqam"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"learnDur",1}, "Learn Duration",
        juce::StringArray{"10 sec","20 sec","30 sec","60 sec"}, 1));
    return { params.begin(), params.end() };
}

void MaqamAnalyzerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    pitchTracker = std::make_unique<PitchTracker>();
    pitchTracker->prepare(sampleRate, samplesPerBlock * 4);

    learnMaxSamples = (int) (sampleRate * 64);
    learnBuffer.setSize(1, learnMaxSamples, false, true);
    learnBuffer.clear();
    learnWritePos = 0;
    analysisState = AnalysisState::Idle;
}

void MaqamAnalyzerAudioProcessor::releaseResources() {}

bool MaqamAnalyzerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto in = layouts.getMainInputChannelSet();
    auto out = layouts.getMainOutputChannelSet();
    if (in.isDisabled() || out.isDisabled()) return false;
    return in == out || out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void MaqamAnalyzerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // PASS-THROUGH: never modify audio
    // (no processing - audio passes unchanged)

    // Sync APVTS -> uiState
    if (auto* p = apvts.getRawParameterValue("a4ref")) uiState.referenceA4 = p->load();
    if (auto* p = apvts.getRawParameterValue("tradition")) uiState.traditionFilter = (TraditionFilter)(int)p->load();
    if (auto* p = apvts.getRawParameterValue("learnDur"))
    {
        int idx = (int) p->load();
        int map[4]={10,20,30,60};
        uiState.learnDurationSec = map[juce::jlimit(0,3,idx)];
    }

    // RMS for metering
    float rms = 0;
    for (int ch=0; ch<buffer.getNumChannels(); ++ch)
        rms += buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
    rms /= juce::jmax(1, buffer.getNumChannels());
    currentRMS = rms;
    currentInputLevelDb = rms > 1e-6f ? 20.0f * std::log10(rms) : -100.0f;

    // Monophonic sum for pitch tracking
    int N = buffer.getNumSamples();
    juce::AudioBuffer<float> mono(1, N);
    mono.clear();
    for (int ch=0; ch<buffer.getNumChannels(); ++ch)
        for (int i=0;i<N;++i) mono.addSample(0, i, buffer.getSample(ch,i) * (1.0f / buffer.getNumChannels()));

    // Pitch tracking (every block)
    // Downsample to 2048 window for YIN
    const float* monoPtr = mono.getReadPointer(0);
    double blockTime = juce::Time::getMillisecondCounterHiRes() * 0.001; // fallback, better to use playhead
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
            if (pos->getTimeInSeconds().hasValue()) blockTime = *pos->getTimeInSeconds();
    }

    if (pitchTracker)
    {
        // For YIN we need at least 1024 samples; if block smaller, accumulate
        static std::vector<float> accum;
        if ((int)accum.size() < 2048)
        {
            accum.insert(accum.end(), monoPtr, monoPtr+N);
        }
        if ((int)accum.size() >= 2048)
        {
            auto frame = pitchTracker->process(accum.data(), 2048, blockTime, uiState.referenceA4);
            lastFrame.frequencyHz = frame.freqHz;
            lastFrame.confidence = frame.confidence;
            lastFrame.voiced = frame.voiced;
            lastFrame.noteName = frame.noteName;
            lastFrame.centsOffset = frame.centsFrom12TET;
            lastFrame.timeSec = frame.timeSec;

            // Push to ring for editor
            PitchFrame pf;
            pf.frequencyHz = frame.freqHz;
            pf.confidence = frame.confidence;
            pf.voiced = frame.voiced;
            pf.centsOffset = frame.centsFrom12TET;
            pf.noteName = frame.noteName;
            pf.timeSec = frame.timeSec;
            pushPitchFrame(pf);

            // If learning, collect frames
            if (analysisState == AnalysisState::Learning)
            {
                {
                    juce::ScopedLock sl(resultLock);
                    collectedFrames.push_back({frame.timeSec, frame.freqHz, frame.confidence, frame.voiced});
                }
                voicedFrames = std::count_if(collectedFrames.begin(), collectedFrames.end(),
                                             [](auto& f){ return f.voiced; });

                learnElapsedSec = (juce::Time::getMillisecondCounterHiRes()*0.001 - learnStartTimeSec);
                if (learnElapsedSec >= uiState.learnDurationSec)
                {
                    // auto-stop
                    stopLearn();
                }
                // also store raw audio for optional future use
                int toCopy = juce::jmin(N, learnMaxSamples - learnWritePos);
                if (toCopy>0)
                {
                    learnBuffer.copyFrom(0, learnWritePos, mono, 0, 0, toCopy);
                    learnWritePos += toCopy;
                }
            }

            accum.clear();
            // keep overlap 50%
            if (N >= 1024) accum.insert(accum.end(), monoPtr + N - 1024, monoPtr+N);
        }
    }
}

void MaqamAnalyzerAudioProcessor::pushPitchFrame(const PitchFrame& f)
{
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1>0) { pitchRing[start1 % kRingSize] = f; fifo.finishedWrite(1); }
    else if (size2>0) { pitchRing[start2 % kRingSize] = f; fifo.finishedWrite(1); }
}

void MaqamAnalyzerAudioProcessor::startLearn()
{
    {
        juce::ScopedLock sl(resultLock);
        collectedFrames.clear();
        analysisCopy.clear();
    }
    learnBuffer.clear();
    learnWritePos = 0;
    voicedFrames = 0;
    learnStartTimeSec = juce::Time::getMillisecondCounterHiRes()*0.001;
    learnElapsedSec = 0;
    analysisState = AnalysisState::Learning;
    // clear previous result
    {
        juce::ScopedLock sl(resultLock);
        latestResult.valid = false;
        latestResult.candidates.clear();
    }
}

void MaqamAnalyzerAudioProcessor::stopLearn()
{
    if (analysisState != AnalysisState::Learning) return;
    analysisState = AnalysisState::Analyzing;

    // Copy frames for worker thread
    {
        juce::ScopedLock sl(resultLock);
        analysisCopy = collectedFrames;
    }
    triggerAnalysis();
}

void MaqamAnalyzerAudioProcessor::resetAnalysis()
{
    analysisState = AnalysisState::Idle;
    voicedFrames = 0;
    learnElapsedSec = 0;
    {
        juce::ScopedLock sl(resultLock);
        latestResult.valid = false;
        collectedFrames.clear();
        analysisCopy.clear();
    }
    // clear fifo
    fifo.reset();
}

double MaqamAnalyzerAudioProcessor::getLearnProgress() const
{
    if (analysisState != AnalysisState::Learning) return 0;
    if (uiState.learnDurationSec <=0) return 0;
    return juce::jlimit(0.0, 1.0, learnElapsedSec / uiState.learnDurationSec);
}

MaqamAnalyzerAudioProcessor::AnalysisResultSnapshot MaqamAnalyzerAudioProcessor::getLatestResult() const
{
    juce::ScopedLock sl(resultLock);
    return latestResult;
}
void MaqamAnalyzerAudioProcessor::setLatestResult(const AnalysisResultSnapshot& r)
{
    juce::ScopedLock sl(resultLock);
    latestResult = r;
}

MaqamAnalyzerAudioProcessor::PitchFrame MaqamAnalyzerAudioProcessor::getLastPitchFrame() const
{
    juce::ScopedLock sl(resultLock);
    // lastFrame is atomic-ish; we return copy
    PitchFrame pf;
    pf.frequencyHz = lastFrame.frequencyHz;
    pf.confidence = lastFrame.confidence;
    pf.voiced = lastFrame.voiced;
    pf.centsOffset = lastFrame.centsOffset;
    pf.noteName = lastFrame.noteName;
    pf.timeSec = lastFrame.timeSec;
    return pf;
}

void MaqamAnalyzerAudioProcessor::triggerAnalysis()
{
    workerThread.addTimeSliceClient(&analysisJob);
}

int MaqamAnalyzerAudioProcessor::AnalysisJob::useResult()
{
    // Heavy analysis on worker thread
    MaqamAnalyzer::AnalysisInput input;
    input.frames.reserve(owner->analysisCopy.size());
    for (auto& f: owner->analysisCopy)
    {
        AnalyzerPitchFrame pf;
        pf.timeSec = f.timeSec; pf.freqHz = f.freqHz; pf.confidence = f.confidence; pf.voiced = f.voiced;
        input.frames.push_back(pf);
    }
    input.sampleRate = owner->currentSampleRate;
    input.durationSec = owner->learnElapsedSec;
    MaqamAnalyzer::Config cfg;
    cfg.allTraditions = (owner->uiState.traditionFilter == TraditionFilter::All);
    cfg.traditionFilter = cfg.allTraditions ? Tradition::Persian : (Tradition)((int)owner->uiState.traditionFilter -1);
    cfg.refA4 = owner->uiState.referenceA4;
    cfg.tonicOverride = owner->uiState.manualTonic;
    input.config = cfg;

    auto result = owner->maqamAnalyzer.analyze(input);

    MaqamAnalyzerAudioProcessor::AnalysisResultSnapshot snap;
    snap.valid = true;
    snap.candidates = result.candidates;
    snap.detectedTonic = result.tonic;
    snap.analysisDurationSec = input.durationSec;
    snap.totalVoicedFrames = (int) input.frames.size();
    snap.uncertaintyNote = result.uncertaintyNote;
    snap.limitationNote = result.limitationNote + " Debug: " + result.debugInfo;
    snap.timestamp = juce::Time::getCurrentTime();

    owner->setLatestResult(snap);
    owner->analysisState = AnalysisState::Completed;

    return -1; // remove client
}

//==============================================================================
void MaqamAnalyzerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    // add custom uiState
    state.setProperty("manualTonic", uiState.manualTonic, nullptr);
    state.setProperty("manualMaqamId", uiState.manualMaqamId, nullptr);
    state.setProperty("hasManualMaqam", uiState.hasManualMaqam, nullptr);
    for (int i=0;i<12;++i) state.setProperty("centOff"+juce::String(i), uiState.manualCentOffsets[i], nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void MaqamAnalyzerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid())
        {
            // restore custom before apvts replace
            uiState.manualTonic = state.getProperty("manualTonic", "").toString();
            uiState.manualMaqamId = state.getProperty("manualMaqamId", "").toString();
            uiState.hasManualMaqam = state.getProperty("hasManualMaqam", false);
            for (int i=0;i<12;++i) uiState.manualCentOffsets[i] = (float) state.getProperty("centOff"+juce::String(i), 0.0f);

            apvts.replaceState(state);
            // ensure apvts params still propagated
            if (auto* p = apvts.getRawParameterValue("a4ref")) uiState.referenceA4 = p->load();
        }
    }
}

juce::AudioProcessorEditor* MaqamAnalyzerAudioProcessor::createEditor() { return new MaqamAnalyzerAudioProcessorEditor(*this); }

juce::String MaqamAnalyzerAudioProcessor::generateTXTReport() const
{
    auto snap = getLatestResult();
    juce::String s;
    s += "D.A Maqam Analyzer - Report\n";
    s += "Generated: " + juce::Time::getCurrentTime().toString(true,true) + "\n";
    s += "Tradition filter: " + juce::String((int)uiState.traditionFilter) + "\n";
    s += "A4: " + juce::String(uiState.referenceA4,1) + " Hz\n";
    if (!snap.valid) { s += "No valid analysis yet.\n"; return s; }
    s += "Tonic: " + snap.detectedTonic.noteName + " (" + juce::String(snap.detectedTonic.frequencyHz,1) + " Hz, offset " + juce::String(snap.detectedTonic.centsOffsetFrom12TET,1) + "c) - Method: " + snap.detectedTonic.method + "\n";
    s += "Duration: " + juce::String(snap.analysisDurationSec,1) + " sec, voiced frames: " + juce::String(snap.totalVoicedFrames) + "\n";
    s += "Limitation: " + snap.limitationNote + "\n";
    s += "Uncertainty: " + snap.uncertaintyNote + "\n\n";
    int idx=1;
    for (auto& c: snap.candidates)
    {
        s += "--- Candidate " + juce::String(idx++) + ": " + c.entry.displayName + " (" + traditionToString(c.entry.tradition) + ") ---\n";
        s += "Similarity: " + juce::String(c.similarityScore,2) + " (NOT probability)\n";
        s += "Intervals (cents from tonic): ";
        for (auto v: c.entry.intervalsCents) s += juce::String(v) + " ";
        s += "\n";
        s += "Degrees: ";
        for (auto& n: c.entry.degreeNames) s += n + ", ";
        s += "\n";
        s += "Description: " + c.entry.description + "\n";
        s += "Seyir: " + c.entry.seyir + "\n";
        s += "Finalis: " + c.entry.finalis + "  Dominant: " + c.entry.dominant + "  Cadences: " + c.entry.cadences + "\n";
        s += "Source: " + c.entry.source + "  Verification: " + c.entry.verificationStatus + "\n";
        s += "Evidence: " + c.evidence + "\n";
        if (c.insufficientEvidence) s += "WARNING: Insufficient evidence - treat as tentative.\n";
        s += "Limitation: " + c.limitation + "\n";
        s += "Observed vs expected cents per degree: ";
        for (auto v: c.observedVsExpectedCents) s += juce::String(v,1) + "c ";
        s += "\n\n";
    }
    s += "Auto-Tune guidance: Standard key/scale selectors do NOT implement microtones. Use manual cents offsets below or a microtuning host.\n";
    // tuning table
    if (!snap.candidates.empty())
    {
        auto& c = snap.candidates[0];
        s += "\nTUNING TABLE (Top candidate: " + c.entry.displayName + "):\n";
        s += "Degree | Note | Theoretical¢ | ObservedΔ¢ | ManualOffset¢\n";
        for (size_t i=0;i<c.entry.intervalsCents.size();++i)
        {
            float obs = i < c.observedVsExpectedCents.size() ? c.observedVsExpectedCents[i] : 0;
            s += juce::String((int)i+1) + " | " + (i<c.entry.degreeNames.size()?c.entry.degreeNames[i]:"-") + " | " + juce::String(c.entry.intervalsCents[i]) + " | " + juce::String(obs,1) + " | 0\n";
        }
    }
    return s;
}

juce::String MaqamAnalyzerAudioProcessor::generateCSVReport() const
{
    juce::String s;
    s += "Degree,DegreeName,Tradition,Maqam,IntervalCents,ObservedDeltaCents,NoteName,Verification,Source\n";
    auto snap = getLatestResult();
    if (!snap.valid) return s;
    for (auto& c: snap.candidates)
    {
        for (size_t i=0;i<c.entry.intervalsCents.size();++i)
        {
            float obs = i < c.observedVsExpectedCents.size() ? c.observedVsExpectedCents[i] : 0;
            juce::String degName = i < c.entry.degreeNames.size() ? c.entry.degreeNames[i] : juce::String((int)i+1);
            s += juce::String((int)i+1) + "," + degName.replace(",",";") + "," + traditionToString(c.entry.tradition) + "," + c.entry.displayName + "," + juce::String(c.entry.intervalsCents[i]) + "," + juce::String(obs,1) + ",,," + c.entry.verificationStatus + "," + c.entry.source.replace(",",";") + "\n";
        }
    }
    return s;
}

juce::String MaqamAnalyzerAudioProcessor::generateScalaSCL(int candidateIndex) const
{
    auto snap = getLatestResult();
    if (!snap.valid || candidateIndex <0 || candidateIndex >= (int)snap.candidates.size()) return "! No valid candidate\n";
    auto& c = snap.candidates[candidateIndex];
    juce::String s;
    s += "! " + c.entry.displayName + " - " + traditionToString(c.entry.tradition) + " - D.A Production\n";
    s += "! Source: " + c.entry.source + "\n";
    s += "! Note: Scala SCL - intervals from tonic, microtonal degrees preserved. Verify cents match performer.\n";
    s += juce::String(c.entry.displayName) + "\n";
    s += juce::String((int)c.entry.intervalsCents.size()) + "\n";
    s += "!\n";
    for (size_t i=1;i<c.entry.intervalsCents.size();++i) // skip 0 (unison implied in some but we include)
    {
        s += juce::String(c.entry.intervalsCents[i]) + ".0\n";
    }
    s += "2/1\n"; // octave
    return s;
}

juce::String MaqamAnalyzerAudioProcessor::generateScalaKBM(int candidateIndex, const juce::String& tonic) const
{
    juce::String s;
    s += "! KBM for " + tonic + " tonic\n";
    s += "! Map " + juce::String(candidateIndex) + " candidate\n";
    s += "0\n0\n0\n60\n60\n69\n440.0\n";
    return s;
}

// Creator for host
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MaqamAnalyzerAudioProcessor(); }
