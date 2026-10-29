#include "HoverTips.h"

namespace phasemap
{

HoverTips::HoverTips()
{
    setInterceptsMouseClicks (false, false);
    setAlwaysOnTop (true);
}

void HoverTips::add (juce::Component& target, juce::String text)
{
    tips.push_back ({ &target, std::move (text) });
}

const HoverTips::Tip* HoverTips::findTipUnderMouse() const
{
    auto* under = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
    if (under == nullptr)
        return nullptr;

    for (auto& tip : tips)
        if ((under == tip.target || tip.target->isParentOf (under)) && tip.target->isShowing())
            return &tip;
    return nullptr;
}

void HoverTips::advance (double dt)
{
    const auto* now = findTipUnderMouse();
    if (juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown())
        hoverSeconds = 0.0;

    if (now != hovered)
    {
        hovered = now;
        hoverSeconds = 0.0;
    }
    else if (hovered != nullptr)
    {
        hoverSeconds += dt;
    }

    const bool wanted = hovered != nullptr && hoverSeconds >= delaySeconds;
    const double before = visibility;

    if (wanted && shown == hovered)
    {
        visibility = juce::jmin (1.0, visibility + dt / fadeInSeconds);
    }
    else
    {
        // fade the current tip out completely before the next one appears
        visibility = juce::jmax (0.0, visibility - dt / fadeOutSeconds);
        if (visibility <= 0.0)
        {
            shown = wanted ? hovered : nullptr;
            if (shown != nullptr)
                layOut();
        }
    }

    if (! juce::exactlyEqual (visibility, before) || (visibility <= 0.0 && shown != nullptr))
    {
        setVisible (visibility > 0.0 || shown != nullptr);
        repaint();
    }
}

void HoverTips::showImmediately (juce::Component& target)
{
    for (auto& tip : tips)
        if (tip.target == &target)
        {
            shown = hovered = &tip;
            layOut();
            visibility = 1.0;
            setVisible (true);
            repaint();
        }
}

void HoverTips::layOut()
{
    constexpr float width = 300, padding = 14;

    juce::AttributedString text;
    text.append (shown->text, theme::font (15), theme::text);
    text.setLineSpacing (3);
    textLayout.createLayout (text, width - 2 * padding);

    const float height = textLayout.getHeight() + 2 * padding;
    const auto target = getLocalArea (shown->target->getParentComponent(), shown->target->getBounds()).toFloat();
    const auto inside = getLocalBounds().toFloat().reduced (10);

    float y = target.getBottom() + 10;                  // below the control ...
    if (y + height > inside.getBottom())
        y = target.getY() - 10 - height;                // ... or above if there is no room
    const float x = juce::jlimit (inside.getX(), inside.getRight() - width, target.getCentreX() - width / 2);
    box = { x, y, width, height };
}

void HoverTips::paint (juce::Graphics& g)
{
    if (shown == nullptr || visibility <= 0.0)
        return;

    g.beginTransparencyLayer (theme::easeInOut ((float) visibility));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (box.translated (0, 4), 10);
    g.setColour (juce::Colour (0xff1c1c1c));
    g.fillRoundedRectangle (box, 10);
    g.setColour (theme::menuBack);
    g.fillRoundedRectangle (box.reduced (3), 8);
    textLayout.draw (g, box.reduced (14));
    g.endTransparencyLayer();
}

} // namespace phasemap
