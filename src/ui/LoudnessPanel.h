#pragma once

#include "Theme.h"
#include "../dsp/LoudnessMeter.h"
#include "../dsp/LoudnessTargets.h"

namespace phasemap
{

/** The loudness column: target name, M and S bars, Integrated, Range and True Peak.

    Coordinates inside are relative to theme::layout::loudnessPanel.
    Colours: green = on target, red = too loud, yellow = too quiet.
*/
class LoudnessPanel : public juce::Component
{
public:
    LoudnessPanel() { setInterceptsMouseClicks (false, false); }

    struct Values
    {
        float momentary = LoudnessMeter::silence, shortTerm = LoudnessMeter::silence,
              integrated = LoudnessMeter::silence, range = 0.0f, truePeak = LoudnessMeter::silence;
    };

    void setValues (const Values& newValues)            { values = newValues; repaint(); }
    void setTarget (const LoudnessTarget& newTarget)    { if (! (target == newTarget)) { target = newTarget; repaint(); } }

    void paint (juce::Graphics&) override;

    // sub-areas, used to place the hover explanations
    static juce::Rectangle<int> targetArea()        { return { 0, 0, 240, 26 }; }
    static juce::Rectangle<int> meterArea()         { return { 0, 40, 240, 298 }; }
    static juce::Rectangle<int> integratedArea()    { return { 0, 350, 240, 76 }; }
    static juce::Rectangle<int> rangeArea()         { return { 0, 440, 240, 26 }; }
    static juce::Rectangle<int> truePeakArea()      { return { 0, 476, 240, 26 }; }

private:
    void paintBar (juce::Graphics&, float x0, float x1, float lufs) const;

    Values values;
    LoudnessTarget target;

    static constexpr float scaleTop = 72, scaleBottom = 298;   // 0 LUFS and -48 LUFS
};

} // namespace phasemap
