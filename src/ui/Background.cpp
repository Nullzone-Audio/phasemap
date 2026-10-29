#include "Background.h"

namespace phasemap::Background
{

namespace
{
    using Gradient = std::initializer_list<std::pair<double, juce::uint32>>;

    /** Fills a quadrilateral with a linear gradient running from 'from' to 'to'. */
    void fillQuad (juce::Graphics& g, std::initializer_list<juce::Point<float>> corners,
                   juce::Point<float> from, juce::Point<float> to, Gradient stops)
    {
        juce::Path quad;
        bool first = true;
        for (auto p : corners)
        {
            if (first) quad.startNewSubPath (p); else quad.lineTo (p);
            first = false;
        }
        quad.closeSubPath();

        juce::ColourGradient gradient;
        gradient.point1 = from;
        gradient.point2 = to;
        gradient.isRadial = false;
        for (auto& [position, colour] : stops)
            gradient.addColour (position, juce::Colour (colour));

        g.setGradientFill (gradient);
        g.fillPath (quad);
    }

    void line (juce::Graphics& g, float x1, float y1, float x2, float y2, juce::uint32 colour, float thickness = 1.0f)
    {
        g.setColour (juce::Colour (colour));
        g.drawLine (x1, y1, x2, y2, thickness);
    }

    const juce::Colour labelDark { 0xff2c2c2c }, scaleDark { 0xff333333 }, scaleGrey { 0xff9a9a9a }, captionGrey { 0xff4a4a4a };

    // individual sections ------------------------------------------------------
    void spectrumSection (juce::Graphics& g)
    {
        raisedPanel (g, { 46, 52, 873, 638 });
        insetField (g, { 68, 70, 829, 602 });

        const auto plot = theme::layout::spectrumPlot.toFloat();
        auto yForHz = [&] (double hz)
        {
            return plot.getBottom() - (float) ((std::log10 (hz) - std::log10 (20.0)) / 3.0) * plot.getHeight();
        };

        const std::pair<double, const char*> marks[] = { { 20, "20" }, { 50, "50" }, { 100, "100" }, { 200, "200" }, { 500, "500" },
                                                         { 1000, "1k" }, { 2000, "2k" }, { 5000, "5k" }, { 10000, "10k" }, { 20000, "20k" } };
        for (auto& [hz, text] : marks)
        {
            const float y = yForHz (hz);
            line (g, plot.getX(), y, plot.getRight(), y, 0xff5e5e5e);
            label (g, text, plot.getX() - 12, y + 4, 11, scaleGrey, juce::Justification::right);
        }
        line (g, plot.getCentreX(), plot.getY(), plot.getCentreX(), plot.getBottom(), 0xff7a7a7a, 1.2f);
    }

    void loudnessSection (juce::Graphics& g)
    {
        raisedPanel (g, { 954, 52, 284, 638 });
        label (g, "Loudness", 976, 96, 15, labelDark);
        insetField (g, { 976, 108, 240, 26 }, 4);

        // meter field with a shared scale between the M and S bars (like a mixing console)
        insetField (g, { 976, 148, 240, 298 });
        const float barM = (996 + 1062) / 2.0f, barS = (1130 + 1196) / 2.0f, scaleX = (1062 + 1130) / 2.0f;
        label (g, "M", barM, 168, 12, scaleGrey, juce::Justification::horizontallyCentred, true);
        label (g, "S", barS, 168, 12, scaleGrey, juce::Justification::horizontallyCentred, true);
        for (int db = 0; db >= -48; db -= 6)
        {
            const float y = 180.0f + (float) -db / 48.0f * (406.0f - 180.0f);
            line (g, 1065, y, 1073, y, 0xff7a7a7a);
            line (g, 1119, y, 1127, y, 0xff7a7a7a);
            label (g, juce::String (db), scaleX, y + 4, 11, scaleGrey, juce::Justification::horizontallyCentred);
        }
        label (g, "LUFS", scaleX, 432, 10, scaleGrey, juce::Justification::horizontallyCentred);

        label (g, "Integrated", 976, 474, 15, labelDark);
        insetField (g, { 976, 484, 240, 50 }, 5);
        label (g, "Range", 976, 566, 13, labelDark);
        insetField (g, { 1100, 548, 116, 26 }, 4);
        label (g, "True Peak", 976, 602, 13, labelDark);
        insetField (g, { 1100, 584, 116, 26 }, 4);
    }

    void correlationSection (juce::Graphics& g)
    {
        raisedPanel (g, { 46, 720, 1192, 155 });
        label (g, "Correlation", 70, 774, 15, labelDark);
        insetField (g, { 70, 790, 126, 26 }, 4);
        label (g, "Hold", 70, 847, 13, labelDark);
        insetField (g, { 112, 830, 84, 24 }, 4);

        const auto bar = theme::layout::correlationBar.toFloat();
        insetField (g, bar);
        const std::pair<float, const char*> marks[] = { { -1.0f, "-1" }, { -0.5f, "-0.5" }, { 0.0f, "0" }, { 0.5f, "+0.5" }, { 1.0f, "+1" } };
        for (auto& [value, text] : marks)
        {
            const float x = bar.getX() + 12 + (value + 1) / 2 * (bar.getWidth() - 24);
            line (g, x, bar.getY() - 9, x, bar.getY() - 3, 0xff3a3a3a, juce::exactlyEqual (value, 0.0f) ? 1.6f : 1.0f);
            label (g, text, x, bar.getY() - 13, 11, scaleDark, juce::Justification::horizontallyCentred);
        }
        label (g, "out of phase", bar.getX(), bar.getBottom() + 22, 11, captionGrey);
        label (g, "mono compatible", bar.getRight(), bar.getBottom() + 22, 11, captionGrey, juce::Justification::right);
    }

    void rightSection (juce::Graphics& g)
    {
        raisedPanel (g, { 1273, 52, 300, 823 });

        // goniometer: circle, diagonals, M / L / R / +S / -S
        const auto gonio = theme::layout::goniometer.toFloat();
        insetField (g, gonio);
        const auto c = gonio.getCentre();
        const float r = gonio.getWidth() / 2 - 14;
        g.setColour (juce::Colour (0xff5e5e5e));
        g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.0f);
        for (float degrees : { 0.0f, 45.0f, 90.0f, 135.0f })
        {
            const float a = juce::degreesToRadians (degrees);
            line (g, c.x - r * std::cos (a), c.y - r * std::sin (a), c.x + r * std::cos (a), c.y + r * std::sin (a), 0xff5e5e5e);
        }
        label (g, "M", c.x, gonio.getY() + 13, 11, scaleGrey, juce::Justification::horizontallyCentred);
        label (g, "L", gonio.getX() + 14, gonio.getY() + 22, 11, scaleGrey, juce::Justification::horizontallyCentred);
        label (g, "R", gonio.getRight() - 14, gonio.getY() + 22, 11, scaleGrey, juce::Justification::horizontallyCentred);
        label (g, "+S", gonio.getX() + 10, c.y - 5, 10, scaleGrey);
        label (g, "-S", gonio.getRight() - 10, c.y - 5, 10, scaleGrey, juce::Justification::right);

        label (g, "Mode", 1295, 374, 15, labelDark);
        label (g, "Response", 1295, 502, 15, labelDark);
        label (g, "Balance", 1295, 622, 15, labelDark);

        const auto balance = theme::layout::balance.toFloat();
        insetField (g, balance, 5);
        const std::pair<float, const char*> marks[] = { { -12.0f, "L 12" }, { -6.0f, "6" }, { 0.0f, "0" }, { 6.0f, "6" }, { 12.0f, "12 R" } };
        for (auto& [db, text] : marks)
        {
            const float x = balance.getX() + 8 + (db + 12) / 24 * (balance.getWidth() - 16);
            line (g, x, balance.getBottom() + 3, x, balance.getBottom() + 9, 0xff3a3a3a, juce::exactlyEqual (db, 0.0f) ? 1.6f : 1.0f);
            label (g, text, x, balance.getBottom() + 22, 11, scaleDark, juce::Justification::horizontallyCentred);
        }

        label (g, "Check", 1295, 742, 15, labelDark);
    }
}

//==============================================================================
void raisedPanel (juce::Graphics& g, juce::Rectangle<float> area, float b)
{
    const juce::uint32 face = 0xff828282;
    const float x0 = area.getX(), y0 = area.getY(), x1 = area.getRight(), y1 = area.getBottom();

    g.setColour (juce::Colour (face));
    g.fillRect (area);

    // top: bright (light from above)
    fillQuad (g, { { x0, y0 }, { x1, y0 }, { x1 - b, y0 + b }, { x0 + b, y0 + b } },
              { 0, y0 }, { 0, y0 + b }, { { 0.0, 0xff868686 }, { 0.45, 0xffb5b5b5 }, { 1.0, face } });
    // right: bright edge falling into shadow at the outside
    fillQuad (g, { { x1, y0 }, { x1, y1 }, { x1 - b, y1 - b }, { x1 - b, y0 + b } },
              { x1 - b, 0 }, { x1, 0 }, { { 0.0, face }, { 0.55, 0xffb6b6b6 }, { 0.85, 0xff8c8c8c }, { 1.0, 0xff3a3a3a } });
    // bottom: dark
    fillQuad (g, { { x0, y1 }, { x1, y1 }, { x1 - b, y1 - b }, { x0 + b, y1 - b } },
              { 0, y1 - b }, { 0, y1 }, { { 0.0, face }, { 0.85, 0xff363636 }, { 1.0, 0xff2e2e2e } });
    // left: dark
    fillQuad (g, { { x0, y0 }, { x0, y1 }, { x0 + b, y1 - b }, { x0 + b, y0 + b } },
              { x0, 0 }, { x0 + b, 0 }, { { 0.0, 0xff2e2e2e }, { 0.2, 0xff303030 }, { 1.0, face } });

    g.setColour (juce::Colour (0xff2b2b2b));
    g.drawRect (area, 1.2f);
}

void insetField (juce::Graphics& g, juce::Rectangle<float> area, float b, juce::Colour fill)
{
    const float x0 = area.getX(), y0 = area.getY(), x1 = area.getRight(), y1 = area.getBottom();
    b = juce::jmin (b, area.getWidth() / 2.2f, area.getHeight() / 2.2f);

    g.setColour (fill);
    g.fillRect (area);

    // shadow on the top and left inner edge, light catching the bottom and right lip
    fillQuad (g, { { x0, y0 }, { x1, y0 }, { x1 - b, y0 + b }, { x0 + b, y0 + b } },
              { 0, y0 }, { 0, y0 + b }, { { 0.0, 0xff161616 }, { 0.3, 0xff2a2a2a }, { 1.0, fill.getARGB() } });
    fillQuad (g, { { x0, y0 }, { x0, y1 }, { x0 + b, y1 - b }, { x0 + b, y0 + b } },
              { x0, 0 }, { x0 + b, 0 }, { { 0.0, 0xff141414 }, { 0.3, 0xff2c2c2c }, { 1.0, fill.getARGB() } });
    line (g, x0, y1, x1, y1, 0xff8a8a8a, 1.2f);
    line (g, x1, y0, x1, y1, 0xff7a7a7a, 1.2f);
}

void label (juce::Graphics& g, const juce::String& text, float x, float baseline, float size,
            juce::Colour colour, juce::Justification horizontal, bool bold)
{
    g.setColour (colour);
    g.setFont (theme::designFont (size, bold));
    g.drawSingleLineText (text, juce::roundToInt (x), juce::roundToInt (baseline), horizontal);
}

void paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff555555));

    spectrumSection (g);
    loudnessSection (g);
    correlationSection (g);
    rightSection (g);

    // product name, top right; a click opens the settings
    auto brand = theme::designFont (22, true);
    brand.setExtraKerningFactor (0.14f);
    g.setFont (brand);
    g.setColour (juce::Colour (0xff3a3a3a));
    g.drawSingleLineText ("PHASEMAP", 1573, 36, juce::Justification::right);
}

} // namespace phasemap::Background
