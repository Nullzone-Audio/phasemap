#pragma once

#include "Controls.h"
#include "../plugin/PhasemapProcessor.h"

namespace phasemap
{

/** Settings overlay, opened by clicking the product name.

    Resolution, window size, loudness target (built-in or the user's own) and
    "reset Integrated on play". Covers the whole editor; clicking outside the panel or
    pressing Escape closes it.
*/
class SettingsMenu : public juce::Component
{
public:
    explicit SettingsMenu (PhasemapProcessor&);

    // wired up by the editor
    std::function<void()> onClose;
    std::function<void (double)> onWindowScaleChosen;
    std::function<void()> onFitToScreen;
    std::function<void (int)> onResolutionChosen;

    /** Reads all values again (call before showing). */
    void refresh();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    static juce::Rectangle<int> panelBounds()       { return { 470, 170, 660, 500 }; }

private:
    void rebuildTargetList();
    void targetChosen();
    void applyTypedTarget();
    void saveTypedTarget();
    void deleteCurrentTarget();

    PhasemapProcessor& processor;

    juce::ComboBox resolution, windowSize, targetList;
    juce::Label targetName, targetLufs, targetTruePeak, targetTolerance;
    juce::TextButton saveButton { "Save" }, deleteButton { "Delete" }, closeButton { "X" };
    juce::ToggleButton resetOnPlay { "Reset Integrated when playback starts" };

    static constexpr int firstSavedTargetId = 101, newTargetId = 999, fitToScreenId = 100;
};

} // namespace phasemap
