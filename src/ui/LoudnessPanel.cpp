#include "LoudnessPanel.h"

namespace phasemap
{

namespace
{
    bool isMeasured (float lufs)                { return lufs > -69.95f; }
    juce::String lufsText (float lufs)          { return isMeasured (lufs) ? juce::String (lufs, 1) : juce::String ("--.-"); }

    juce::String targetText (const LoudnessTarget& t)
    {
        const bool whole = std::abs (t.integratedLufs - std::round (t.integratedLufs)) < 0.05f;
        return t.name + "  " + juce::String::fromUTF8 ("\xc2\xb7") + "  "
             + juce::String (t.integratedLufs, whole ? 0 : 1) + " LUFS";
    }
}

void LoudnessPanel::paintBar (juce::Graphics& g, float x0, float x1, float lufs) const
{
    constexpr int numSegments = 24;   // 2 LU each
    const float segmentHeight = (scaleBottom - scaleTop) / numSegments;
    const float goal = target.integratedLufs;

    for (int i = 0; i < numSegments; ++i)
    {
        const float from = -48.0f + 2.0f * i, to = from + 2.0f;
        const auto colour = to <= goal ? theme::green : (from < goal + 3.0f ? theme::yellow : theme::red);
        const bool lit = lufs > from;
        g.setColour (lit ? colour : colour.withMultipliedBrightness (0.3f).withMultipliedSaturation (0.6f));
        g.fillRect (x0, scaleBottom - (i + 1) * segmentHeight + 0.75f, x1 - x0, segmentHeight - 1.5f);
    }
}

void LoudnessPanel::paint (juce::Graphics& g)
{
    g.setFont (theme::font (12.5f));
    g.setColour (theme::text);
    g.drawText (targetText (target), targetArea().toFloat(), juce::Justification::centred);

    // M and S bars with the target as an orange line
    paintBar (g, 20, 86, values.momentary);
    paintBar (g, 154, 220, values.shortTerm);

    const float targetY = juce::jmap (juce::jlimit (-48.0f, 0.0f, target.integratedLufs), -48.0f, 0.0f, scaleBottom, scaleTop);
    g.setColour (theme::accent);
    g.fillRect (16.0f, targetY - 1, 74.0f, 2.0f);
    g.fillRect (150.0f, targetY - 1, 74.0f, 2.0f);
    juce::Path arrows;
    arrows.addTriangle (92, targetY - 5, 92, targetY + 5, 99, targetY);
    arrows.addTriangle (148, targetY - 5, 148, targetY + 5, 141, targetY);
    g.fillPath (arrows);

    g.setFont (theme::font (12));
    g.setColour (theme::text);
    g.drawText (lufsText (values.momentary), juce::Rectangle<float> (14, 304, 78, 18), juce::Justification::centred);
    g.drawText (lufsText (values.shortTerm), juce::Rectangle<float> (148, 304, 78, 18), juce::Justification::centred);

    // Integrated, coloured by the distance to the target
    const auto area = juce::Rectangle<float> (0, 376, 240, 50);
    const bool measured = isMeasured (values.integrated);
    const float distance = values.integrated - target.integratedLufs;
    auto colour = theme::text;
    if (measured)
        colour = std::abs (distance) <= target.toleranceLu ? theme::green : (distance > 0 ? theme::red : theme::yellow);

    g.setColour (colour);
    g.setFont (theme::font (30));
    g.drawText (lufsText (values.integrated), area.withTrimmedRight (92), juce::Justification::centredRight);
    g.setFont (theme::font (12.5f));
    g.setColour (theme::textDim);
    g.drawText ("LUFS", juce::Rectangle<float> (156, area.getY() + 6, 80, 18), juce::Justification::centredLeft);
    if (measured)
    {
        g.setColour (colour);
        g.drawText ((distance >= 0 ? "+" : "") + juce::String (distance, 1) + " LU",
                    juce::Rectangle<float> (156, area.getY() + 25, 80, 18), juce::Justification::centredLeft);
    }

    // Range and True Peak
    g.setFont (theme::font (12.5f));
    g.setColour (theme::text);
    g.drawText (measured ? juce::String (values.range, 1) + " LU" : juce::String ("--"),
                juce::Rectangle<float> (124, 440, 116, 26), juce::Justification::centred);
    g.setColour (values.truePeak > target.truePeakLimitDb ? theme::red : theme::text);
    g.drawText (values.truePeak <= -100.0f ? juce::String ("--") : juce::String (values.truePeak, 1) + " dBTP",
                juce::Rectangle<float> (124, 476, 116, 26), juce::Justification::centred);
}

} // namespace phasemap
