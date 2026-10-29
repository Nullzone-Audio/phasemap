#pragma once

#include "Theme.h"
#include "../dsp/SpectralCorrelation.h"

namespace phasemap
{

/** The main display. One horizontal bar per frequency band, 20 Hz at the bottom, 20 kHz at the top.

    Energy mode: level of L to the left, level of R to the right.
    Phase mode:  bar width = distance from +1 (centre = in phase, edge = -1, cancels in mono).
                 The "Low End" lamp blinks when anything below 160 Hz is out of phase.
*/
class SpectrumView : public juce::Component
{
public:
    explicit SpectrumView (const SpectralCorrelation& analysis);

    void setPhaseMode (bool shouldShowPhase)    { phaseMode = shouldShowPhase; repaint(); }
    bool isLowEndAlarmActive() const            { return lowEndAlarm; }

    void paint (juce::Graphics&) override;

    static constexpr double lowEndLimitHz = 160.0;

private:
    void paintGrid (juce::Graphics&, juce::Rectangle<float> plot);
    void paintLowEndLamp (juce::Graphics&, juce::Rectangle<float> plot);

    const SpectralCorrelation& analysis;
    bool phaseMode = false, lowEndAlarm = false;
};

/** Overall correlation as a row of LEDs from the centre, with a marker at the lowest recent value. */
class CorrelationBar : public juce::Component
{
public:
    explicit CorrelationBar (const SpectralCorrelation& analysis);

    /** Smooths the value and handles the minimum marker. Call once per UI frame. */
    void update (double secondsSinceLastFrame, double holdSeconds);
    float getDisplayedValue() const             { return displayed; }
    float getLowestValue() const                { return lowest; }

    void paint (juce::Graphics&) override;

private:
    const SpectralCorrelation& analysis;
    float displayed = 0.0f, lowest = 1.0f;
    double secondsSinceLowest = 0.0;
};

/** Mid/side display of the latest samples: vertical = mono, horizontal = out of phase. */
class Goniometer : public juce::Component
{
public:
    explicit Goniometer (const SpectralCorrelation& source) : analysis (source) {}
    void paint (juce::Graphics&) override;

private:
    const SpectralCorrelation& analysis;
    float zoom = 1.0f;   // automatic, fast out and slow in
};

/** Level difference between L and R, +-12 dB. */
class BalanceMeter : public juce::Component
{
public:
    explicit BalanceMeter (const SpectralCorrelation& source) : analysis (source) {}
    void paint (juce::Graphics&) override;

private:
    const SpectralCorrelation& analysis;
};

} // namespace phasemap
