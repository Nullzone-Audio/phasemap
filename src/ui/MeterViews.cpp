#include "MeterViews.h"

namespace phasemap
{

//==============================================================================
SpectrumView::SpectrumView (const SpectralCorrelation& a) : analysis (a)
{
    setInterceptsMouseClicks (false, false);
}

void SpectrumView::paint (juce::Graphics& g)
{
    // this component covers the whole display; the plot area sits inside it
    const auto plot = (theme::layout::spectrumPlot - theme::layout::spectrumDisplay.getPosition()).toFloat();
    const float centreX = plot.getCentreX(), halfWidth = plot.getWidth() * 0.5f;

    paintGrid (g, plot);

    auto yForHz = [&] (double hz)
    {
        return plot.getBottom() - (float) ((std::log10 (hz) - std::log10 (20.0)) / 3.0) * plot.getHeight();
    };

    lowEndAlarm = false;
    for (const auto& band : analysis.getBands())
    {
        const float top = yForHz (std::min (band.highHz, 20000.0)), bottom = yForHz (band.lowHz);
        const float height = juce::jmax (1.5f, (bottom - top) - (bottom - top > 5 ? 1.5f : 0.5f));   // small gap between bars
        const float y = top + ((bottom - top) - height) * 0.5f;

        if (! phaseMode)
        {
            auto barLength = [&] (float db) { return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * halfWidth; };
            const float left = barLength (band.levelLeftDb), right = barLength (band.levelRightDb);

            // the clearly louder side (> 3 dB) is drawn brighter
            g.setColour (theme::accent.withAlpha (band.levelLeftDb > band.levelRightDb + 3 ? 1.0f : 0.7f));
            g.fillRect (centreX - left, y, left, height);
            g.setColour (theme::accent.withAlpha (band.levelRightDb > band.levelLeftDb + 3 ? 1.0f : 0.7f));
            g.fillRect (centreX, y, right, height);
            continue;
        }

        const bool audible = band.levelLeftDb > -72.0f || band.levelRightDb > -72.0f;
        if (! audible)
            continue;

        // bar width = distance from +1, independent of the level
        const float width = juce::jlimit (0.0f, 1.0f, (1.0f - band.correlation) * 0.5f) * halfWidth;
        g.setColour (theme::correlationColour (band.correlation).withAlpha (0.9f));
        g.fillRect (centreX - width, y, 2 * width, height);

        if (band.centreHz < lowEndLimitHz && band.correlation < 0.0f
            && (band.levelLeftDb > -60.0f || band.levelRightDb > -60.0f))
            lowEndAlarm = true;
    }

    if (phaseMode)
        paintLowEndLamp (g, plot);
}

void SpectrumView::paintGrid (juce::Graphics& g, juce::Rectangle<float> plot)
{
    const float centreX = plot.getCentreX(), halfWidth = plot.getWidth() * 0.5f;

    auto verticalLine = [&] (float x, const juce::String& text)
    {
        g.setColour (juce::Colour (0xff5e5e5e));
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        g.setColour (theme::textDim);
        g.setFont (theme::font (11));
        g.drawText (text, juce::Rectangle<float> (x - 40, plot.getBottom() + 10, 80, 16), juce::Justification::centred);
    };

    if (! phaseMode)
    {
        for (int db : { 0, -12, -24, -36, -48 })
        {
            const float offset = (db + 60) / 60.0f * halfWidth;
            const auto text = db == 0 ? juce::String ("0 dB") : juce::String (db);
            verticalLine (centreX - offset, text);
            verticalLine (centreX + offset, text);
        }
        g.setColour (theme::textDim);
        g.setFont (theme::font (13, true));
        g.drawText ("L", juce::Rectangle<float> (plot.getX(), plot.getY() - 2, 20, 16), juce::Justification::centredLeft);
        g.drawText ("R", juce::Rectangle<float> (plot.getRight() - 20, plot.getY() - 2, 20, 16), juce::Justification::centredRight);
        return;
    }

    // phase mode, mirrored: centre = +1, half width = 0, edge = -1
    verticalLine (centreX, "+1");
    for (float side : { -1.0f, 1.0f })
    {
        verticalLine (centreX + side * halfWidth * 0.5f, "0");
        verticalLine (centreX + side * halfWidth, "-1");
    }
    g.setColour (theme::textDim);
    g.setFont (theme::font (11));
    g.drawText ("out of phase", juce::Rectangle<float> (plot.getX() + 4, plot.getY() + 2, 120, 16), juce::Justification::centredLeft);
    g.drawText ("out of phase", juce::Rectangle<float> (plot.getRight() - 124, plot.getY() + 2, 120, 16), juce::Justification::centredRight);
}

void SpectrumView::paintLowEndLamp (juce::Graphics& g, juce::Rectangle<float> plot)
{
    const auto box = juce::Rectangle<float> (plot.getRight() - 112, plot.getBottom() - 28, 106, 22);
    g.setColour (juce::Colour (0xff2a2a2a).withAlpha (0.85f));
    g.fillRoundedRectangle (box, 4);

    const bool lit = lowEndAlarm && (juce::Time::getMillisecondCounter() / 200) % 2 == 0;   // blinks at 2.5 Hz
    const auto lamp = juce::Rectangle<float> (box.getX() + 8, box.getCentreY() - 5, 10, 10);
    g.setColour (lit ? theme::red : theme::red.withMultipliedBrightness (0.3f));
    g.fillEllipse (lamp);
    if (lit)
    {
        g.setColour (theme::red.withAlpha (0.35f));
        g.fillEllipse (lamp.expanded (4));
    }

    g.setColour (lowEndAlarm ? theme::text : theme::textDim);
    g.setFont (theme::font (12, true));
    g.drawText ("Low End", box.withTrimmedLeft (26), juce::Justification::centredLeft);
}

//==============================================================================
CorrelationBar::CorrelationBar (const SpectralCorrelation& a) : analysis (a) {}

void CorrelationBar::update (double dt, double holdSeconds)
{
    const float value = analysis.getCorrelation();
    displayed += (value - displayed) * (float) juce::jmin (1.0, dt * 12.0);

    if (value < lowest)
    {
        lowest = value;
        secondsSinceLowest = 0.0;
    }
    else if ((secondsSinceLowest += dt) > holdSeconds)
    {
        lowest = juce::jmin (1.0f, lowest + (float) dt * 0.5f);   // drifts back up after the hold time
    }
    repaint();
}

void CorrelationBar::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (12, 10);
    constexpr int numLeds = 40;
    const float ledWidth = area.getWidth() / numLeds;
    const bool active = analysis.hasSignal();

    for (int i = 0; i < numLeds; ++i)
    {
        const float from = -1.0f + 2.0f * i / numLeds, to = from + 2.0f / numLeds, mid = 0.5f * (from + to);
        // light from the centre towards the current value
        const bool lit = active && (displayed >= 0 ? (mid >= 0 && from < displayed) : (mid < 0 && to > displayed));
        const auto colour = theme::correlationColour (mid);
        g.setColour (lit ? colour : colour.withMultipliedBrightness (0.35f).withMultipliedSaturation (0.7f));
        g.fillRoundedRectangle (area.getX() + i * ledWidth + 1.5f, area.getY(), ledWidth - 3, area.getHeight(), 1.5f);
    }

    if (active && lowest < 0.99f)
    {
        const float x = area.getX() + (lowest + 1) * 0.5f * area.getWidth();
        g.setColour (theme::text);
        g.fillRect (x - 1.5f, area.getY() - 4, 3.0f, area.getHeight() + 8);
    }
}

//==============================================================================
void Goniometer::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const float radius = bounds.getWidth() * 0.5f - 14;
    const auto& left = analysis.getScopeLeft();
    const auto& right = analysis.getScopeRight();
    constexpr float sqrtHalf = 0.70710678f;

    float peak = 1.0e-6f;
    for (size_t i = 0; i < left.size(); ++i)
        peak = juce::jmax (peak, std::abs (left[i] + right[i]), std::abs (left[i] - right[i]));
    const float wantedZoom = juce::jlimit (0.5f, 200.0f, 0.9f / (peak * sqrtHalf));
    zoom = wantedZoom < zoom ? wantedZoom : zoom + (wantedZoom - zoom) * 0.05f;

    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (getLocalBounds().reduced (3));

    const int count = (int) left.size(), oldest = analysis.getScopeWritePosition();
    for (int k = 0; k < count; ++k)
    {
        const auto i = (size_t) ((oldest + k) % count);
        const float mid = (left[i] + right[i]) * sqrtHalf, side = (right[i] - left[i]) * sqrtHalf;
        g.setColour (theme::accent.withAlpha (0.15f + 0.6f * (float) k / (float) count));   // newer points brighter
        g.fillRect (centre.x + side * zoom * radius - 0.9f, centre.y - mid * zoom * radius - 0.9f, 1.8f, 1.8f);
    }
}

//==============================================================================
void BalanceMeter::paint (juce::Graphics& g)
{
    if (! analysis.hasSignal())
        return;

    const auto area = getLocalBounds().toFloat().reduced (8, 7);
    const float centreX = area.getCentreX();
    const float x = centreX + juce::jlimit (-12.0f, 12.0f, analysis.getBalanceDb()) / 12.0f * area.getWidth() * 0.5f;

    g.setColour (theme::accent.withAlpha (0.85f));
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (centreX, x), area.getY(), juce::jmax (centreX, x), area.getBottom()));
    g.setColour (theme::text);
    g.fillRect (x - 1.5f, area.getY() - 2, 3.0f, area.getHeight() + 4);
}

} // namespace phasemap
