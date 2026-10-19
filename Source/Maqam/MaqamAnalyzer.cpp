#include "MaqamAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <map>

//==============================================================================
DetectedTonic MaqamAnalyzer::estimateTonic(const std::vector<AnalyzerPitchFrame>& frames, double refA4)
{
    DetectedTonic t;
    if (frames.empty()) return t;

    // Collect voiced frequencies
    std::vector<float> voicedFreqs;
    for (auto& f : frames) if (f.voiced && f.freqHz > 50) voicedFreqs.push_back(f.freqHz);
    if (voicedFreqs.empty()) return t;

    // Histogram in MIDI cents (12-TET with 10c resolution for tonic search)
    // Count occurrences per pitch-class, find most stable / frequent final tone
    // Heuristic: most common pitch-class that also appears at phrase endings (last 20% of frames)

    // Build 12-TET pitch class histogram (weighted by duration assumed uniform)
    std::map<int,int> pcCount;
    for (auto freq : voicedFreqs)
    {
        float midi = 69 + 12 * std::log2(freq / 440.0f);
        int pc = ((int) std::round(midi) % 12 + 12) % 12;
        pcCount[pc]++;
    }

    // Phrase endings: look at last voiced frame in each contiguous voiced phrase
    // Phrase = gap > 300ms of unvoiced
    std::vector<float> endingFreqs;
    bool inPhrase = false;
    float lastVoicedInPhrase = 0;
    double lastVoicedTime = 0;
    for (auto& f : frames)
    {
        if (f.voiced)
        {
            lastVoicedInPhrase = f.freqHz;
            lastVoicedTime = f.timeSec;
            inPhrase = true;
        }
        else if (inPhrase && (f.timeSec - lastVoicedTime) > 0.3)
        {
            if (lastVoicedInPhrase > 0) endingFreqs.push_back(lastVoicedInPhrase);
            inPhrase = false;
        }
    }
    if (inPhrase && lastVoicedInPhrase > 0) endingFreqs.push_back(lastVoicedInPhrase);

    // Score each pitch class by: frequency count + ending bonus
    int bestPc = -1;
    int bestScore = -1;
    for (auto& kv : pcCount)
    {
        int score = kv.second;
        // bonus if this pc appears as ending
        for (auto ef : endingFreqs)
        {
            float midi = 69 + 12 * std::log2(ef / 440.0f);
            int epc = ((int) std::round(midi) % 12 + 12) % 12;
            if (epc == kv.first) score += 5;
        }
        if (score > bestScore) { bestScore = score; bestPc = kv.first; }
    }

    if (bestPc < 0) return t;

    // Determine octave: median octave of that pitch class
    std::vector<int> octs;
    for (auto freq : voicedFreqs)
    {
        float midi = 69 + 12 * std::log2(freq / 440.0f);
        int rm = (int) std::round(midi);
        if (((rm % 12)+12)%12 == bestPc) octs.push_back(rm);
    }
    int medianMidi = 60;
    if (!octs.empty())
    {
        std::sort(octs.begin(), octs.end());
        medianMidi = octs[octs.size()/2];
    }
    else medianMidi = bestPc + 60; // fallback to C4 region

    float freq = 440.0f * std::pow(2.0f, (medianMidi - 69)/12.0f);
    // Refine tonic frequency: average of all voiced frames that map to this pc within ±40c
    double sum = 0; int cnt = 0;
    for (auto freqV : voicedFreqs)
    {
        float midi = 69 + 12 * std::log2(freqV / 440.0f);
        int rm = (int) std::round(midi);
        if (((rm%12)+12)%12 == bestPc)
        {
            // within 1 semitone
            float centsFromPc = 100 * (midi - rm);
            if (std::abs(centsFromPc) < 50)
            { sum += freqV; cnt++; }
        }
    }
    if (cnt > 0) freq = (float)(sum / cnt);

    t.midiNote = medianMidi;
    t.frequencyHz = freq;
    static const char* names[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int oct = medianMidi /12 -1;
    t.noteName = juce::String(names[bestPc]) + juce::String(oct);
    t.centsOffsetFromA440 = 1200.0f * std::log2(freq / 440.0f) - (medianMidi - 69)*100.0f;
    t.centsOffsetFrom12TET = t.centsOffsetFromA440; // same for tonic
    t.isReliable = endingFreqs.size() >= 2 && bestScore > 8;
    t.method = endingFreqs.empty() ? "most frequent pitch-class (no clear phrase endings)"
                                   : "phrase-ending + frequency-weighted histogram (voiced " + juce::String(voicedFreqs.size()) + " frames)";

    return t;
}

std::vector<float> MaqamAnalyzer::buildPitchClassHistogram(const std::vector<AnalyzerPitchFrame>& frames, int binsPerOctave)
{
    int nBins = binsPerOctave; // 24 = 50c resolution for quarter tones
    std::vector<float> hist(nBins, 0.0f);
    for (auto& f : frames)
    {
        if (!f.voiced || f.freqHz < 20) continue;
        float midi = 69 + 12 * std::log2(f.freqHz / 440.0f);
        float cents = midi * 100.0f; // cents from C-1
        // pitch class cents 0..1200
        float pcCents = std::fmod(cents, 1200.0f);
        if (pcCents < 0) pcCents += 1200.0f;
        int bin = (int) (pcCents / 1200.0f * nBins + 0.5f) % nBins;
        hist[bin] += f.confidence; // weight by confidence
    }
    // normalize
    float sum = 0; for (auto v: hist) sum+=v;
    if (sum > 0) for (auto& v: hist) v/=sum;
    return hist;
}

std::vector<float> MaqamAnalyzer::buildIntervalProfile(const MaqamEntry& e, int binsPerOctave)
{
    int nBins = binsPerOctave;
    std::vector<float> profile(nBins, 0.0f);
    float binWidth = 1200.0f / nBins;
    // Each interval contributes Gaussian bump width ~ binWidth
    for (int c : e.intervalsCents)
    {
        float pc = std::fmod((float)c, 1200.0f);
        if (pc < 0) pc+=1200;
        int bin = (int) (pc / binWidth + 0.5f) % nBins;
        profile[bin] += 1.0f;
        // spread to neighbors for tolerance (microtonal tolerance ±15c)
        int prev = (bin -1 + nBins) % nBins;
        int next = (bin +1) % nBins;
        profile[prev] += 0.3f;
        profile[next] += 0.3f;
    }
    float sum = 0; for (auto v: profile) sum+=v;
    if (sum > 0) for (auto& v: profile) v/=sum;
    return profile;
}

float MaqamAnalyzer::scoreEntry(const MaqamEntry& e, const DetectedTonic& tonic,
                                const std::vector<AnalyzerPitchFrame>& frames,
                                const std::vector<float>& pcHist,
                                juce::String& evidenceOut,
                                std::vector<float>& obsVsExpectedOut) const
{
    // --- 1. Build shifted histogram so tonic = bin 0 ---
    // We need to know tonic pitch class in 1200c space
    float tonicMidi = 69 + 12 * std::log2(tonic.frequencyHz / 440.0f);
    float tonicPcCents = std::fmod(tonicMidi * 100.0f, 1200.0f);
    if (tonicPcCents < 0) tonicPcCents += 1200.0f;

    int nBins = (int) pcHist.size();
    float binWidth = 1200.0f / nBins;
    int tonicBinShift = (int) (tonicPcCents / binWidth + 0.5f) % nBins;

    // Rotate pcHist so tonicBinShift -> 0
    std::vector<float> rotated(nBins,0);
    for (int i=0;i<nBins;++i)
    {
        int src = (i + tonicBinShift) % nBins;
        rotated[i] = pcHist[src];
    }

    auto profile = buildIntervalProfile(e, nBins);

    // --- 2. Histogram similarity (Bhattacharyya or cosine) ---
    // Use cosine similarity for interpretability
    float dot=0, normA=0, normB=0;
    for (int i=0;i<nBins;++i)
    {
        dot += rotated[i] * profile[i];
        normA += rotated[i]*rotated[i];
        normB += profile[i]*profile[i];
    }
    float histScore = (normA>0 && normB>0) ? dot / std::sqrt(normA*normB) : 0.0f;

    // --- 3. Duration-weighted interval match ---
    // For each interval in e, count how many voiced frames lie within ±35c
    std::vector<float> perDegreeCount;
    perDegreeCount.reserve(e.intervalsCents.size());
    float coverageSum = 0;
    for (int cents : e.intervalsCents)
    {
        float targetPc = std::fmod(tonicPcCents + (float)cents, 1200.0f);
        int hits = 0; int tot=0;
        for (auto& f: frames) if (f.voiced)
        {
            float midi = 69+12*std::log2(f.freqHz/440.0f);
            float pc = std::fmod(midi*100.0f, 1200.0f); if(pc<0) pc+=1200;
            float dist = std::abs(pc - targetPc);
            if (dist > 600) dist = 1200 - dist;
            if (dist < 35) hits++;
            tot++;
        }
        float ratio = tot>0 ? (float)hits / (float)tot : 0;
        perDegreeCount.push_back(ratio);
        coverageSum += ratio;
    }
    float coverageScore = e.intervalsCents.empty() ? 0 : coverageSum / (float)e.intervalsCents.size();

    // Observed vs expected: for each degree, median observed cents offset
    obsVsExpectedOut.clear();
    for (size_t di=0; di<e.intervalsCents.size(); ++di)
    {
        int expected = e.intervalsCents[di];
        float targetPc = std::fmod(tonicPcCents + (float)expected, 1200.0f);
        std::vector<float> centsDeltas;
        for (auto& f: frames) if (f.voiced)
        {
            float midi = 69+12*std::log2(f.freqHz/440.0f);
            float pc = std::fmod(midi*100.0f, 1200.0f); if(pc<0) pc+=1200;
            float dist = std::abs(pc - targetPc); if(dist>600) dist=1200-dist;
            if (dist < 35)
            {
                // deviation from expected
                float signedDist = pc - targetPc;
                if (signedDist > 600) signedDist -=1200;
                if (signedDist < -600) signedDist +=1200;
                centsDeltas.push_back(signedDist);
            }
        }
        if (!centsDeltas.empty())
        {
            std::sort(centsDeltas.begin(), centsDeltas.end());
            obsVsExpectedOut.push_back(centsDeltas[centsDeltas.size()/2]);
        }
        else obsVsExpectedOut.push_back(0.0f); // no observation
    }

    // --- 4. Melodic features ---
    // a) Transition matrix score: count transitions between degrees that are allowed in seyir
    // Simplified: reward stepwise motion (2 semitones) vs large leaps (except Chahargah)
    // b) Contour: ratio of ascending vs descending moves
    // c) Emphasized tones: check if most frequent degrees match entry's characteristic
    // For first release we compute a simple transition consistency score
    float transitionScore = 0.5f; // neutral baseline
    if (frames.size() > 5)
    {
        // Map each voiced frame to nearest degree index of this entry
        std::vector<int> degreeSeq;
        for (auto& f: frames) if (f.voiced)
        {
            float midi = 69+12*std::log2(f.freqHz/440.0f);
            float pc = std::fmod(midi*100.0f, 1200.0f); if(pc<0) pc+=1200;
            float rel = pc - tonicPcCents; if(rel<0) rel+=1200;
            int bestIdx=-1; float bestDist=999;
            for (size_t i=0;i<e.intervalsCents.size();++i)
            {
                float d = std::abs(rel - (float)e.intervalsCents[i]);
                if(d>600) d=1200-d;
                if(d<bestDist){bestDist=d; bestIdx=(int)i;}
            }
            if(bestDist < 40) degreeSeq.push_back(bestIdx);
        }
        // Count stepwise vs leap: stepwise if |deg diff| ==1
        int stepwise=0, leaps=0;
        for (size_t i=1;i<degreeSeq.size();++i)
        {
            int diff = std::abs(degreeSeq[i]-degreeSeq[i-1]);
            if(diff==1) stepwise++;
            else if(diff>2) leaps++;
        }
        if(stepwise+leaps>0)
        {
            float stepRatio = (float)stepwise / (stepwise+leaps);
            // For most maqams stepwise is good, except Chahargah where leaps are expected
            if(e.id.contains("chahargah")) transitionScore = 0.5f + 0.3f * (1.0f - stepRatio); // leaps increase score for chahargah
            else transitionScore = 0.5f + 0.3f * stepRatio;
        }
        // Emphasis check: does mode's finalis/güçlü correspond to frequent degrees?
        // Simple: if entry's degreeNames[0] is most frequent, boost
        if(degreeSeq.size()>10)
        {
            std::map<int,int> cnt;
            for(int d: degreeSeq) cnt[d]++;
            auto maxIt = std::max_element(cnt.begin(), cnt.end(), [](auto&a, auto&b){return a.second < b.second;});
            if(maxIt != cnt.end() && maxIt->first==0) transitionScore += 0.1f; // finalis frequent
        }
    }

    // --- 5. Combine scores ---
    // Weighted combination: hist 40%, coverage 35%, transition 25%
    float combined = 0.40f * histScore + 0.35f * coverageScore + 0.25f * transitionScore;
    combined = juce::jlimit(0.0f, 1.0f, combined);

    // Evidence string
    evidenceOut = "Pitch-class histogram match " + juce::String(histScore,2)
                + ", interval coverage " + juce::String(coverageScore,2)
                + ", melodic consistency " + juce::String(transitionScore,2)
                + ". Observed degrees: ";
    for (size_t i=0;i<perDegreeCount.size();++i)
        evidenceOut += juce::String(perDegreeCount[i],2) + (i+1<perDegreeCount.size() ? ", " : "");
    if (e.isMicrotonal())
        evidenceOut += ". Microtonal degrees present (~ -50c koron/half-flat).";

    // Penalize if very few frames cover signature microtonal intervals
    // e.g., for Shur/Hijaz with neutral second, need at least some evidence of that bin
    if (e.isMicrotonal() && coverageScore < 0.15f)
        combined *= 0.7f;

    return combined;
}

MaqamAnalyzer::Result MaqamAnalyzer::analyze(const AnalysisInput& input)
{
    Result res;
    res.limitationNote = "Analysis is monophonic only - polyphonic material will give unreliable results. ";
    res.limitationNote += "Requires dry, isolated vocal; reverb/polyphony/chorus degrades tracking. ";
    res.limitationNote += "Short excerpts (< 10 sec or < 3 sec voiced) marked insufficient - cannot distinguish similar modes (e.g. Ussak vs Huseyni/Saba share pitches, only seyir differs). ";

    if (input.frames.empty())
    {
        res.insufficient = true;
        res.uncertaintyNote = "No voiced frames captured - check input level, ensure monophonic vocal, disable noise gate.";
        return res;
    }

    // Count voiced
    int voicedCount=0; double voicedSec=0;
    for (auto& f: input.frames) if(f.voiced) {voicedCount++; voicedSec+=0.02;} // assume ~20ms frames
    double voicedSecApprox = voicedCount * 0.02; // 50 fps
    if (voicedSecApprox < input.config.minVoicedSec || voicedCount < 40)
    {
        res.insufficient = true;
        res.uncertaintyNote = "Insufficient evidence: only " + juce::String(voicedSecApprox,1) + " sec voiced (" + juce::String(voicedCount) + " frames). Need ≥ " + juce::String(input.config.minVoicedSec,1) + " sec voiced for reliable mode distinction. Extend LEARN duration or sing longer phrases with clear finalis.";
        res.limitationNote += "Result below is tentative.";
    }

    // Estimate tonic (or use override)
    DetectedTonic tonic;
    if (input.config.tonicOverride.isNotEmpty())
    {
        // Parse note name e.g. "G", "C#", "Eb", "A half-flat"
        // Simplified: map to midi
        juce::String s = input.config.tonicOverride.trim().toUpperCase();
        std::map<juce::String,int> map = {{"C",0},{"C#",1},{"DB",1},{"D",2},{"D#",3},{"EB",3},{"E",4},{"F",5},{"F#",6},{"GB",6},{"G",7},{"G#",8},{"AB",8},{"A",9},{"A#",10},{"BB",10},{"B",11}};
        int pc = 0; // default C
        for (auto& kv: map) if(s.contains(kv.first)) {pc=kv.second; break;}
        int midi = 60 + pc - 0; // C4 =60
        float freq = 440.0f * std::pow(2.0f, (midi-69)/12.0f);
        tonic.midiNote = midi;
        tonic.frequencyHz = freq;
        tonic.noteName = s + " (manual)";
        tonic.isReliable = true;
        tonic.method = "manual override";
    }
    else
    {
        tonic = estimateTonic(input.frames, input.config.refA4);
    }
    res.tonic = tonic;

    // Build pitch class histogram (24 bins = 50c)
    auto pcHist = buildPitchClassHistogram(input.frames, 24);

    // Score all entries matching tradition filter
    std::vector<std::pair<MaqamEntry,float>> scored;
    juce::StringArray evidenceList;
    std::vector<std::vector<float>> obsList;
    std::vector<juce::String> evidenceStrs;

    auto all = database.getAllEntries();
    for (auto& e : all)
    {
        if (!input.config.allTraditions)
        {
            if (e.tradition != input.config.traditionFilter) continue;
        }
        juce::String evidence;
        std::vector<float> obs;
        float score = scoreEntry(e, tonic, input.frames, pcHist, evidence, obs);
        scored.push_back({e, score});
        evidenceStrs.push_back(evidence);
        obsList.push_back(obs);
    }

    std::sort(scored.begin(), scored.end(), [](auto& a, auto& b){ return a.second > b.second; });

    // Take top 3, but apply ambiguity threshold
    int take = juce::jmin(3, (int) scored.size());
    for (int i=0;i<take;++i)
    {
        MaqamCandidate cand;
        cand.entry = scored[i].first;
        cand.similarityScore = scored[i].second;
        cand.evidence = evidenceStrs[i]; // note: indices misaligned after sort - fix by re-scoring top
        // Re-score to get obs correctly (since we sorted without mapping)
        // Simpler: find index of this entry in original for obs - but we already have mapping error above
        // For correctness, re-score this entry:
        juce::String ev2; std::vector<float> obs2;
        cand.similarityScore = scoreEntry(cand.entry, tonic, input.frames, pcHist, ev2, obs2);
        cand.evidence = ev2;
        cand.observedVsExpectedCents = obs2;
        cand.tonic = tonic;
        // Insufficient if score <0.35 and voiced short
        cand.insufficientEvidence = cand.similarityScore < 0.35f || res.insufficient;
        // Limitation per candidate
        cand.limitation = cand.entry.limitationNote;
        if (cand.entry.id.contains("ussak") || cand.entry.id.contains("huseyni") || cand.entry.id.contains("saba"))
            cand.limitation += " Ussak/Huseyni/Saba share pitch set - ranking relies on contour (seyir) and is uncertain on short excerpts.";
        if (cand.entry.id.contains("persian_shur") || cand.entry.id.contains("azerbaijani_shur"))
            cand.limitation += " Shur neutral second varies 135-160c by performer - configurable cents must be calibrated to singer.";

        res.candidates.push_back(cand);
    }

    // Uncertainty note based on score separation
    if (!res.candidates.empty())
    {
        float top = res.candidates[0].similarityScore;
        float second = res.candidates.size()>1 ? res.candidates[1].similarityScore : 0.0f;
        float gap = top - second;
        if (gap < 0.06f)
            res.uncertaintyNote += " HIGH AMBIGUITY: top two candidates are nearly equal (gap " + juce::String(gap,2) + "). Consider longer excerpt with clear finalis and characteristic modulation. ";
        else if (gap < 0.12f)
            res.uncertaintyNote += " Moderate ambiguity - top candidate leads by " + juce::String(gap,2) + ". Verify with seyir and emphasized tones. ";
        else
            res.uncertaintyNote += " Top candidate leads by " + juce::String(gap,2) + " similarity points - moderate confidence. ";

        if (top < 0.45f)
            res.uncertaintyNote += " Overall similarity low (<0.45). Material may be outside database, heavily modulatory, or insufficiently voiced.";
        else if (top > 0.70f)
            res.uncertaintyNote += " Overall similarity HIGH (>0.70) but this is NOT a probability - see evidence above.";
        res.uncertaintyNote += " Scores are deterministic similarity scores (0..1), NOT calibrated probabilities. No held-out test set calibration claimed.";
    }

    // Debug
    res.debugInfo = "Voiced frames: " + juce::String(voicedCount) + ", voiced sec ~" + juce::String(voicedSecApprox,1)
                  + ", tonic " + tonic.noteName + " (" + juce::String(tonic.frequencyHz,1) + " Hz, method: " + tonic.method + ")"
                  + ", total entries scored: " + juce::String((int)all.size());

    return res;
}
