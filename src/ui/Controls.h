#pragma once

#include "Theme.h"

namespace phasemap
{

/** Combo boxes, popup menus and small text buttons in the dark menu style. */
class PhasemapLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PhasemapLookAndFeel();

    void drawComboBox (juce::Graphics&, int width, int height, bool isDown,
                       int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override  { label.setBounds (box.getLocalBounds()); }
    juce::Font getPopupMenuFont() override                                         { return theme::font (15.0f); }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool isHighlighted, bool isDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override                 { return theme::font (14.0f); }
};

/** Grey hardware-style switch with an optional icon above the label. */
class HardwareButton : public juce::Button
{
public:
    using IconPainter = std::function<void (juce::Graphics&, juce::Rectangle<float>)>;

    explicit HardwareButton (const juce::String& label, IconPainter icon = {})
        : juce::Button (label), paintIcon (std::move (icon)) {}

    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    IconPainter paintIcon;
};

namespace icons
{
    void energy (juce::Graphics&, juce::Rectangle<float>);   // bars to both sides
    void phase (juce::Graphics&, juce::Rectangle<float>);    // two opposite waves
}

/** A value in a field that changes when dragged up or down; double-click restores the default. */
class DragValueBox : public juce::Slider
{
public:
    explicit DragValueBox (juce::String unitSuffix);
    void paint (juce::Graphics&) override;

private:
    juce::String suffix;
};

/** Invisible area that runs a function when clicked (for the product name in the background). */
class ClickArea : public juce::Component
{
public:
    std::function<void()> onClick;

    ClickArea()                                             { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void mouseUp (const juce::MouseEvent& e) override       { if (onClick != nullptr && e.mouseWasClicked()) onClick(); }
};

} // namespace phasemap
