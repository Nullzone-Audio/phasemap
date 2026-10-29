#pragma once

#include "Theme.h"

namespace phasemap
{

/** Short explanations that fade in after resting the mouse on a control for 3 seconds.

    Register components with add(); call advance() once per UI frame. Fades use theme::easeInOut
    (180 ms in, 300 ms out). Clicking or dragging restarts the waiting time.
*/
class HoverTips : public juce::Component
{
public:
    static constexpr double delaySeconds = 3.0, fadeInSeconds = 0.18, fadeOutSeconds = 0.30;

    HoverTips();

    void add (juce::Component& target, juce::String text);
    void advance (double secondsSinceLastFrame);

    /** Shows the tip of a component immediately (used for screenshots). */
    void showImmediately (juce::Component& target);

    void paint (juce::Graphics&) override;

private:
    struct Tip
    {
        juce::Component* target;
        juce::String text;
    };

    const Tip* findTipUnderMouse() const;
    void layOut();

    std::vector<Tip> tips;
    const Tip* hovered = nullptr;   // under the mouse right now
    const Tip* shown = nullptr;     // visible or fading out
    double hoverSeconds = 0.0, visibility = 0.0;
    juce::Rectangle<float> box;
    juce::TextLayout textLayout;
};

} // namespace phasemap
