#pragma once
#include <JuceHeader.h>

// Simple persistence wrapper using APVTS + ValueTree for maqam analyzer state
class StateManager
{
public:
    static juce::String traditionFilterToString(int idx)
    {
        switch(idx)
        {
            case 0: return "All";
            case 1: return "Persian-Iranian";
            case 2: return "Azerbaijani Mugham";
            case 3: return "Turkish Makam";
            case 4: return "Arabic Maqam";
            default: return "All";
        }
    }
};
