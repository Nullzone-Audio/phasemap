#include "UserSettings.h"
#include "Theme.h"

namespace phasemap::UserSettings
{

namespace
{
    struct SettingsFile
    {
        SettingsFile()
        {
            juce::PropertiesFile::Options options;
            options.applicationName = "Phasemap";
            options.folderName = "Nullzone";
            options.filenameSuffix = ".settings";
            options.osxLibrarySubFolder = "Application Support";
            file = std::make_unique<juce::PropertiesFile> (options);
        }
        std::unique_ptr<juce::PropertiesFile> file;
    };

    juce::PropertiesFile& settings()
    {
        static juce::SharedResourcePointer<SettingsFile> shared;
        return *shared->file;
    }

    void store (const juce::String& key, const juce::var& value)
    {
        settings().setValue (key, value);
        settings().saveIfNeeded();
    }

    void storeTargets (const std::vector<LoudnessTarget>& targets)
    {
        juce::XmlElement root ("Targets");
        for (const auto& t : targets)
        {
            auto* e = root.createNewChildElement ("Target");
            e->setAttribute ("name", t.name);
            e->setAttribute ("integrated", t.integratedLufs);
            e->setAttribute ("truePeak", t.truePeakLimitDb);
            e->setAttribute ("tolerance", t.toleranceLu);
        }
        store ("savedTargets", root.toString (juce::XmlElement::TextFormat().singleLine()));
    }
}

double getWindowScale()
{
    return juce::jlimit (theme::minScale, theme::maxScale, settings().getDoubleValue ("windowScale", 1.0));
}

void setWindowScale (double scale)             { store ("windowScale", scale); }

int getBandsPerOctave()
{
    const int value = settings().getIntValue ("bandsPerOctave", 6);
    return value == 3 || value == 12 ? value : 6;
}

void setBandsPerOctave (int bandsPerOctave)    { store ("bandsPerOctave", bandsPerOctave); }

std::vector<LoudnessTarget> getSavedTargets()
{
    std::vector<LoudnessTarget> targets;
    if (auto xml = juce::parseXML (settings().getValue ("savedTargets")))
        for (auto* e : xml->getChildIterator())
            targets.push_back ({ e->getStringAttribute ("name"),
                                 (float) e->getDoubleAttribute ("integrated", -14.0),
                                 (float) e->getDoubleAttribute ("truePeak", -1.0),
                                 (float) e->getDoubleAttribute ("tolerance", 1.0) });
    return targets;
}

void saveTarget (const LoudnessTarget& target)
{
    auto targets = getSavedTargets();
    auto existing = std::find_if (targets.begin(), targets.end(), [&] (const auto& t) { return t.name == target.name; });
    if (existing != targets.end())
        *existing = target;
    else
        targets.push_back (target);
    storeTargets (targets);
}

void deleteTarget (const juce::String& name)
{
    auto targets = getSavedTargets();
    targets.erase (std::remove_if (targets.begin(), targets.end(), [&] (const auto& t) { return t.name == name; }),
                   targets.end());
    storeTargets (targets);
}

} // namespace phasemap::UserSettings
