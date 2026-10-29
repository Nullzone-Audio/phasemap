#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace phasemap
{

/** A loudness goal: integrated level, true-peak ceiling and how close counts as "on target". */
struct LoudnessTarget
{
    juce::String name { "EBU R128" };
    float integratedLufs = -23.0f;
    float truePeakLimitDb = -1.0f;
    float toleranceLu = 0.5f;

    bool hasSameValues (const LoudnessTarget& other) const
    {
        return std::abs (integratedLufs - other.integratedLufs) < 0.01f
            && std::abs (truePeakLimitDb - other.truePeakLimitDb) < 0.01f
            && std::abs (toleranceLu - other.toleranceLu) < 0.01f;
    }

    bool operator== (const LoudnessTarget& other) const { return name == other.name && hasSameValues (other); }
};

/** Built-in delivery targets.

    Values as published by the platforms / standards (checked 2026). Please update them
    in a pull request when a platform changes its normalisation.
*/
struct Preset
{
    const char* longName;   // shown in the menu
    LoudnessTarget target;  // target.name is the short name shown in the meter
};

inline const std::vector<Preset>& builtInTargets()
{
    static const std::vector<Preset> presets {
        { "EBU R128 (Broadcast EU)",  { "EBU R128",     -23.0f, -1.0f, 0.5f } },
        { "ATSC A/85 (Broadcast US)", { "ATSC A/85",    -24.0f, -2.0f, 2.0f } },
        { "YouTube",                  { "YouTube",      -14.0f, -1.0f, 1.0f } },
        { "Spotify",                  { "Spotify",      -14.0f, -1.0f, 1.0f } },
        { "Spotify Loud",             { "Spotify Loud", -11.0f, -1.0f, 1.0f } },
        { "Apple Music",              { "Apple Music",  -16.0f, -1.0f, 1.0f } },
        { "Amazon Music",             { "Amazon Music", -14.0f, -1.0f, 1.0f } },
        { "TIDAL",                    { "TIDAL",        -14.0f, -1.0f, 1.0f } },
        { "Deezer",                   { "Deezer",       -15.0f, -1.0f, 1.0f } },
        { "SoundCloud",               { "SoundCloud",   -14.0f, -1.0f, 1.0f } },
        { "Apple Podcasts",           { "Podcast",      -16.0f, -1.0f, 1.0f } },
        { "AES Streaming Speech",     { "AES Speech",   -18.0f, -1.0f, 1.0f } },
    };
    return presets;
}

} // namespace phasemap
