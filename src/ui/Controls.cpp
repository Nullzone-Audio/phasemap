#include "Controls.h"

namespace phasemap
{

PhasemapLookAndFeel::PhasemapLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, theme::menuBack);
    setColour (juce::PopupMenu::textColourId, theme::text);
    setColour (juce::PopupMenu::headerTextColourId, theme::textDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::ComboBox::textColourId, juce::Colours::transparentBlack);   // drawn in drawComboBox
    setColour (juce::TextButton::textColourOffId, theme::text);
    setColour (juce::ToggleButton::textColourId, theme::text);
    setColour (juce::ToggleButton::tickColourId, theme::accent);
    setColour (juce::ToggleButton::tickDisabledColourId, theme::textDim);
}

void PhasemapLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                        int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (theme::fieldBack);
    g.fillRoundedRectangle (r, 3);
    g.setColour (box.isMouseOver (true) ? theme::accent : theme::outline);
    g.drawRoundedRectangle (r, 3, 1);

    g.setColour (theme::text);
    g.setFont (theme::font (14));
    g.drawText (box.getText(), r.reduced (10, 0).withTrimmedRight (18), juce::Justification::centredLeft);

    const float arrowX = (float) width - 14, arrowY = (float) height * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (arrowX - 6, arrowY - 3);
    arrow.lineTo (arrowX, arrowY + 3);
    arrow.lineTo (arrowX + 6, arrowY - 3);
    g.setColour (juce::Colour (0xffc8c8c8));
    g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PhasemapLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                                bool isHighlighted, bool isDown)
{
    const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (isDown ? theme::accent.withAlpha (0.2f) : theme::fieldBack);
    g.fillRoundedRectangle (r, 3);
    g.setColour (! button.isEnabled() ? juce::Colour (0xff4a4a4a) : (isHighlighted ? theme::accent : theme::outline));
    g.drawRoundedRectangle (r, 3, 1);
}

//==============================================================================
void HardwareButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();

    // light from the top right: lighter face when engaged, fine highlight on the inner edge
    g.setColour (on ? juce::Colour (0xffa8a8a8) : juce::Colour (isDown ? 0xff7a7a7a : 0xff868686));
    g.fillRoundedRectangle (r, 10);
    g.setColour (juce::Colour (0xff2a2a2a));
    g.drawRoundedRectangle (r, 10, 1.6f);
    g.setColour (juce::Colour (isHighlighted ? 0xffc0c0c0 : 0xffa6a6a6).withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (1.8f), 8.5f, 0.9f);

    auto area = r.reduced (8, 6);
    if (paintIcon != nullptr)
        paintIcon (g, area.removeFromTop (area.getHeight() * 0.55f));

    g.setColour (theme::textDark);
    g.setFont (theme::font (15, on));
    g.drawText (getButtonText(), area, paintIcon != nullptr ? juce::Justification::centredTop : juce::Justification::centred);
}

void icons::energy (juce::Graphics& g, juce::Rectangle<float> area)
{
    const float centreX = area.getCentreX();
    const float halfWidths[] = { 10, 18, 14, 22 };
    g.setColour (juce::Colour (0xff2a2a2a));
    for (int i = 0; i < 4; ++i)
        g.fillRect (centreX - halfWidths[i], area.getY() + 2 + i * 6.0f, halfWidths[i] * (i % 2 ? 1.6f : 2.0f), 4.0f);
}

void icons::phase (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Path wave, inverted;
    for (int i = 0; i <= 30; ++i)
    {
        const float x = area.getCentreX() - 26 + i * 52.0f / 30;
        const float s = 8.0f * std::sin (i / 30.0f * juce::MathConstants<float>::twoPi);
        if (i == 0) { wave.startNewSubPath (x, area.getCentreY() + s); inverted.startNewSubPath (x, area.getCentreY() - s); }
        else        { wave.lineTo (x, area.getCentreY() + s);          inverted.lineTo (x, area.getCentreY() - s); }
    }
    g.setColour (juce::Colour (0xff2a2a2a));
    g.strokePath (wave, juce::PathStrokeType (2.0f));
    g.setColour (juce::Colour (0xff2a2a2a).withAlpha (0.5f));
    g.strokePath (inverted, juce::PathStrokeType (2.0f));
}

//==============================================================================
DragValueBox::DragValueBox (juce::String unitSuffix)
    : juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox), suffix (std::move (unitSuffix))
{
    setMouseDragSensitivity (300);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void DragValueBox::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    if (isMouseOverOrDragging())
    {
        g.setColour (theme::accent.withAlpha (0.12f));
        g.fillRect (r.reduced (3));
    }
    g.setColour (isMouseButtonDown() ? theme::accent : theme::text);
    g.setFont (theme::font (12.5f));
    g.drawText (juce::String (juce::roundToInt (getValue())) + " " + suffix, r.withTrimmedLeft (2), juce::Justification::centred);
}

} // namespace phasemap
