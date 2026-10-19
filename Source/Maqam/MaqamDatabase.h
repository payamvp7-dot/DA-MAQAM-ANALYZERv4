#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <map>

//==============================================================================
enum class Tradition
{
    Persian,
    Azerbaijani,
    Turkish,
    Arabic
};

inline juce::String traditionToString(Tradition t)
{
    switch(t)
    {
        case Tradition::Persian: return "Persian-Iranian";
        case Tradition::Azerbaijani: return "Azerbaijani Mugham";
        case Tradition::Turkish: return "Turkish Makam";
        case Tradition::Arabic: return "Arabic Maqam";
    }
    return "Unknown";
}

inline Tradition traditionFromIndex(int idx)
{
    switch(idx)
    {
        case 1: return Tradition::Persian;
        case 2: return Tradition::Azerbaijani;
        case 3: return Tradition::Turkish;
        case 4: return Tradition::Arabic;
        default: return Tradition::Persian;
    }
}

//==============================================================================
struct JinsTetrachord
{
    juce::String name; // e.g. "Rast tetrachord", "Bayati tetrachord"
    std::vector<int> intervalsCents; // from tonic of jins
    juce::String description;
};

struct MaqamEntry
{
    juce::String id; // unique, e.g. "persian_shur"
    juce::String displayName; // e.g. "Shur"
    juce::String transliteration; // e.g. "Shūr"
    std::vector<juce::String> aliases;
    Tradition tradition = Tradition::Persian;
    juce::String category; // "Dastgah", "Avaz", "Makam", "Mugham", "Maqam", "Jins"
    juce::String parentFamily; // e.g. for Avaz: "Shur"

    // Musical description
    juce::String description;
    juce::String seyir; // melodic movement
    juce::String characteristicDegrees; // e.g. "3rd, 6th emphasized"
    juce::String finalis; // resting tone
    juce::String dominant; // emphasized/ghan?
    juce::String cadences;

    // Pitch profile: intervals from tonic in cents (0..1200)
    // Always 7 or 8 notes per octave, but we allow variable length
    std::vector<int> intervalsCents; // e.g. {0, 200, 350, 500, 700, 850, 1000}
    std::vector<juce::String> degreeNames; // e.g. {"Karar","Segah",...}

    // Tetrachords / ajnas
    std::vector<JinsTetrachord> ajnas;

    // Modulation
    std::vector<juce::String> modulatesTo; // ids

    // Provenance
    juce::String source; // citation
    juce::String verificationStatus; // "verified", "tentative", "reconstructed"
    float confidenceWeight = 1.0f; // for ranking
    juce::String limitationNote;

    // Helper
    bool isMicrotonal() const
    {
        for (auto c : intervalsCents) if (c % 100 != 0) return true;
        return false;
    }
};

//==============================================================================
struct DetectedTonic
{
    juce::String noteName = "C"; // e.g. "G", "A ♭", "E half-flat"
    int midiNote = 60; // nearest 12-TET midi
    float frequencyHz = 261.63f;
    float centsOffsetFromA440 = 0.0f;
    float centsOffsetFrom12TET = 0.0f;
    bool isReliable = false;
    juce::String method; // e.g. "pitch histogram peak"
};

struct MaqamCandidate
{
    MaqamEntry entry;
    float similarityScore = 0.0f; // 0..1, explicitly labeled similarity, NOT probability
    juce::String evidence; // human readable
    juce::String limitation;
    DetectedTonic tonic;
    std::vector<float> observedVsExpectedCents; // per degree deviation
    bool insufficientEvidence = false;
};

//==============================================================================
class MaqamDatabase
{
public:
    MaqamDatabase();
    void buildDatabase();
    void loadFromJson(const juce::String& jsonText); // optional override

    const std::vector<MaqamEntry>& getAllEntries() const { return entries; }
    std::vector<MaqamEntry> getFiltered(Tradition t) const;
    std::vector<MaqamEntry> search(const juce::String& query, Tradition filter) const;
    const MaqamEntry* findById(const juce::String& id) const;
    std::vector<juce::String> getCategoriesForTradition(Tradition t) const;

    int size() const { return (int) entries.size(); }

private:
    std::vector<MaqamEntry> entries;

    void addPersianFamily();
    void addAzerbaijaniFamily();
    void addTurkishFamily();
    void addArabicFamily();
    void addEntry(MaqamEntry e);
};
