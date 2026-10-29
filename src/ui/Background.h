#pragma once

#include "Theme.h"

namespace phasemap
{

/** Everything static in the editor: panels, displays, scales and labels.

    Drawn in code, in design pixels (1600 x 900). The look is grey hardware lit from the top right:
    raised panels have a bright top and right edge, inset displays a dark top and left edge.

    The editor caches the result as an image at the current screen resolution, so this only
    runs when the window size changes.
*/
namespace Background
{
    void paint (juce::Graphics&);

    // building blocks, also useful for new parts of the UI
    void raisedPanel (juce::Graphics&, juce::Rectangle<float> area, float bevel = 11.0f);
    void insetField (juce::Graphics&, juce::Rectangle<float> area, float bevel = 9.0f,
                     juce::Colour fill = juce::Colour (0xff535353));
    void label (juce::Graphics&, const juce::String& text, float x, float baseline, float size,
                juce::Colour colour, juce::Justification horizontal = juce::Justification::left, bool bold = false);
}

} // namespace phasemap
