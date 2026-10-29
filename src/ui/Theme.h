#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Colours, fonts and layout of the Phasemap editor.

    All positions are in design pixels of a 1600 x 900 canvas. The static parts (panels, scales,
    labels) are a pre-rendered background image (design/make_background.py); everything that moves
    is drawn in code at the positions below. If you move something here, move it in the
    design script too.
*/
namespace phasemap::theme
{
    // canvas and window
    constexpr int designWidth = 1600, designHeight = 900;
    constexpr int baseWidth = 1024, baseHeight = 576;      // window size at 100 %
    constexpr double minScale = 0.5, maxScale = 4.0;       // 4.0 = 4096 x 2304, enough for a 4K screen

    // colours
    const juce::Colour text      { 0xffe6e6e6 };
    const juce::Colour textDim   { 0xff9a9a9a };
    const juce::Colour textDark  { 0xff1e1e1e };   // labels on grey hardware buttons
    const juce::Colour accent    { 0xfff29a2e };   // orange
    const juce::Colour menuBack  { 0xff3a3a3a };
    const juce::Colour fieldBack { 0xff2c2c2c };
    const juce::Colour outline   { 0xff6a6a6a };
    const juce::Colour red       { 0xffe0362c };
    const juce::Colour yellow    { 0xfff2c230 };
    const juce::Colour green     { 0xff3fc24a };

    /** Helvetica Neue where available, otherwise the closest sans serif. */
    juce::Font font (float height, bool bold = false);

    /** Same typeface, sized like CSS / design tools ("15 px" = em size). Used for the static labels. */
    juce::Font designFont (float pixelSize, bool bold = false);

    /** Smooth start and end, used for fades (menu, hover tips). */
    inline float easeInOut (float p)
    {
        return p < 0.5f ? 4.0f * p * p * p : 1.0f - std::pow (-2.0f * p + 2.0f, 3.0f) / 2.0f;
    }

    /** Green for in phase, yellow for wide, red for out of phase. */
    inline juce::Colour correlationColour (float correlation)
    {
        return correlation < 0.0f ? red : (correlation < 0.3f ? yellow : green);
    }

    namespace layout
    {
        // left: spectrum
        const juce::Rectangle<int> spectrumDisplay { 68, 70, 829, 602 };
        const juce::Rectangle<int> spectrumPlot    { 150, 86, 727, 550 };   // 20 Hz at the bottom, 20 kHz at the top

        // middle: loudness
        const juce::Rectangle<int> loudnessPanel   { 976, 108, 240, 504 };
        const juce::Rectangle<int> resetButton     { 976, 624, 240, 42 };

        // bottom: overall correlation
        const juce::Rectangle<int> correlationBar  { 232, 772, 976, 60 };
        const juce::Rectangle<int> correlationValue { 70, 790, 126, 26 };
        const juce::Rectangle<int> holdBox         { 112, 830, 84, 24 };

        // right: goniometer, switches, balance
        const juce::Rectangle<int> goniometer      { 1295, 74, 256, 256 };
        const juce::Rectangle<int> energyButton    { 1295, 384, 124, 64 };
        const juce::Rectangle<int> phaseButton     { 1427, 384, 124, 64 };
        const juce::Rectangle<int> fastButton      { 1295, 512, 124, 54 };
        const juce::Rectangle<int> slowButton      { 1427, 512, 124, 54 };
        const juce::Rectangle<int> balance         { 1295, 632, 256, 32 };
        const juce::Rectangle<int> monoButton      { 1295, 752, 124, 64 };
        const juce::Rectangle<int> freezeButton    { 1427, 752, 124, 64 };

        // top right: product name, click opens the settings
        const juce::Rectangle<int> settingsHotspot { 1380, 8, 200, 40 };
    }
}
