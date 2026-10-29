#pragma once

#include "Controls.h"
#include "HoverTips.h"
#include "LoudnessPanel.h"
#include "MeterViews.h"
#include "SettingsMenu.h"
#include "../plugin/PhasemapProcessor.h"

namespace phasemap
{

/** The Phasemap window.

    Everything lives on a 1600 x 900 "canvas" that is scaled to the window size, so the layout
    never changes - only its size. The window keeps a 16:9 ratio and can be 50 % to 400 %.

    A 60 Hz timer pulls the samples from the processor, updates the analysis and repaints.
*/
class PhasemapEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PhasemapEditor (PhasemapProcessor&);
    ~PhasemapEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

    void setWindowScale (double scale);
    void fitToScreen();

    // used by tools/Screenshots.cpp ------------------------------------------
    juce::Component& getCanvas()                    { return canvas; }
    void advanceOneFrame()                          { timerCallback(); }
    void showSettings (bool shouldShow);
    void showHoverTipFor (juce::Component& c)       { hoverTips.showImmediately (c); }
    juce::Component& getPhaseButton()               { return phaseButton; }
    juce::Component& getHoldBox()                   { return holdBox; }

private:
    /** The scaled drawing surface. Caches the code-drawn background at the current resolution. */
    struct Canvas : public juce::Component
    {
        void paint (juce::Graphics&) override;
        juce::Image cachedBackground;
    };

    /** Invisible area that only exists to show a hover tip for part of a drawn panel. */
    struct TipArea : public juce::Component {};

    void timerCallback() override;
    void setUpSwitches();
    void setUpLoudness();
    void setUpHoverTips();
    void bindSwitchPair (HardwareButton& onButton, HardwareButton* offButton, const char* parameterId, float onValue);
    void updateMenuFade (double dt);

    PhasemapProcessor& processor;
    PhasemapLookAndFeel lookAndFeel;
    SpectralCorrelation analysis;

    Canvas canvas;
    SpectrumView spectrum { analysis };
    CorrelationBar correlationBar { analysis };
    Goniometer goniometer { analysis };
    BalanceMeter balanceMeter { analysis };
    juce::Label correlationValue;
    DragValueBox holdBox { "ms" };
    std::unique_ptr<juce::SliderParameterAttachment> holdAttachment;

    LoudnessPanel loudnessPanel;
    HardwareButton resetButton { "Reset" };
    TipArea tipTarget, tipMeter, tipIntegrated, tipRange, tipTruePeak;

    HardwareButton energyButton { "Energy", icons::energy }, phaseButton { "Phase", icons::phase },
                   fastButton { "Fast" }, slowButton { "Slow" }, monoButton { "Mono" }, freezeButton { "Freeze" };
    std::vector<std::unique_ptr<juce::ParameterAttachment>> switchAttachments;

    ClickArea settingsHotspot;
    SettingsMenu settingsMenu { processor };
    HoverTips hoverTips;

    bool settingsOpen = false, windowReady = false;
    double settingsVisibility = 0.0, lastFrameMs = 0.0;
    std::vector<float> pullLeft, pullRight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhasemapEditor)
};

} // namespace phasemap
