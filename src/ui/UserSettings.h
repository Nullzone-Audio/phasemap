#pragma once

#include "../dsp/LoudnessTargets.h"
#include <juce_data_structures/juce_data_structures.h>

namespace phasemap
{

/** Settings shared by all Phasemap instances on this computer (not stored in projects):
    window size, band resolution and the user's saved loudness targets.

    macOS:   ~/Library/Application Support/Nullzone/Phasemap.settings
    Windows: %APPDATA%\Nullzone\Phasemap.settings
    Linux:   ~/.config/Nullzone/Phasemap.settings
*/
namespace UserSettings
{
    double getWindowScale();
    void setWindowScale (double scale);

    int getBandsPerOctave();                   // 3, 6 or 12
    void setBandsPerOctave (int bandsPerOctave);

    std::vector<LoudnessTarget> getSavedTargets();
    void saveTarget (const LoudnessTarget&);   // replaces a target with the same name
    void deleteTarget (const juce::String& name);
}

} // namespace phasemap
