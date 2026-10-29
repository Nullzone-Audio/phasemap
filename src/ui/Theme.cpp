#include "Theme.h"

namespace phasemap::theme
{

static const juce::String& familyName()
{
    static const juce::String family = []
    {
        const auto installed = juce::Font::findAllTypefaceNames();
        for (auto* name : { "Helvetica Neue", "Helvetica", "Arial", "Inter", "DejaVu Sans" })
            if (installed.contains (name, true))
                return juce::String (name);
        return juce::Font::getDefaultSansSerifFontName();
    }();
    return family;
}

juce::Font font (float height, bool bold)
{
    return juce::Font (juce::FontOptions (familyName(), height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font designFont (float pixelSize, bool bold)
{
    return juce::Font (juce::FontOptions (familyName(), 10.0f, bold ? juce::Font::bold : juce::Font::plain)
                           .withPointHeight (pixelSize));
}

} // namespace phasemap::theme
