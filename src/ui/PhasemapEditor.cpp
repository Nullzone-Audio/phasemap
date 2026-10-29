#include "PhasemapEditor.h"
#include "Background.h"
#include "UserSettings.h"

namespace phasemap
{

namespace layout = theme::layout;

//==============================================================================
void PhasemapEditor::Canvas::paint (juce::Graphics& g)
{
    // re-render the background whenever the number of physical pixels changes (resize, other screen)
    const float pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int width = juce::roundToInt (theme::designWidth * pixelScale);
    const int height = juce::roundToInt (theme::designHeight * pixelScale);

    if (cachedBackground.getWidth() != width || cachedBackground.getHeight() != height)
    {
        cachedBackground = juce::Image (juce::Image::RGB, juce::jmax (1, width), juce::jmax (1, height), false);
        juce::Graphics imageGraphics (cachedBackground);
        imageGraphics.addTransform (juce::AffineTransform::scale (pixelScale));
        Background::paint (imageGraphics);
    }

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (cachedBackground, getLocalBounds().toFloat());
}

//==============================================================================
PhasemapEditor::PhasemapEditor (PhasemapProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    pullLeft.resize (8192);
    pullRight.resize (8192);
    analysis.setSampleRate (processor.getCurrentSampleRate());
    analysis.setBandsPerOctave (UserSettings::getBandsPerOctave());

    canvas.setBounds (0, 0, theme::designWidth, theme::designHeight);
    canvas.setOpaque (true);
    addAndMakeVisible (canvas);

    spectrum.setBounds (layout::spectrumDisplay);
    correlationBar.setBounds (layout::correlationBar);
    goniometer.setBounds (layout::goniometer);
    balanceMeter.setBounds (layout::balance);
    for (auto* c : std::initializer_list<juce::Component*> { &spectrum, &correlationBar, &goniometer, &balanceMeter })
        canvas.addAndMakeVisible (c);

    correlationValue.setBounds (layout::correlationValue);
    correlationValue.setFont (theme::font (15));
    correlationValue.setJustificationType (juce::Justification::centred);
    correlationValue.setColour (juce::Label::textColourId, theme::text);
    correlationValue.setInterceptsMouseClicks (false, false);
    canvas.addAndMakeVisible (correlationValue);

    holdBox.setBounds (layout::holdBox);
    holdBox.setDoubleClickReturnValue (true, 3000.0);
    holdAttachment = std::make_unique<juce::SliderParameterAttachment> (*processor.parameters.getParameter (ParamID::holdMs), holdBox);
    canvas.addAndMakeVisible (holdBox);

    setUpSwitches();
    setUpLoudness();

    settingsHotspot.setBounds (layout::settingsHotspot);
    settingsHotspot.onClick = [this] { showSettings (! settingsOpen); };
    canvas.addAndMakeVisible (settingsHotspot);

    settingsMenu.setBounds (0, 0, theme::designWidth, theme::designHeight);
    settingsMenu.onClose = [this] { showSettings (false); };
    settingsMenu.onWindowScaleChosen = [this] (double s) { setWindowScale (s); };
    settingsMenu.onFitToScreen = [this] { fitToScreen(); };
    settingsMenu.onResolutionChosen = [this] (int bands)
    {
        UserSettings::setBandsPerOctave (bands);
        analysis.setBandsPerOctave (bands);
    };
    canvas.addChildComponent (settingsMenu);

    setUpHoverTips();

    const double scale = UserSettings::getWindowScale();
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (theme::baseWidth * theme::minScale), juce::roundToInt (theme::baseHeight * theme::minScale),
                     juce::roundToInt (theme::baseWidth * theme::maxScale), juce::roundToInt (theme::baseHeight * theme::maxScale));
    getConstrainer()->setFixedAspectRatio ((double) theme::designWidth / theme::designHeight);
    setWindowScale (scale);
    windowReady = true;   // from now on, remember size changes

    lastFrameMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

PhasemapEditor::~PhasemapEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void PhasemapEditor::setUpSwitches()
{
    energyButton.setBounds (layout::energyButton);
    phaseButton.setBounds (layout::phaseButton);
    fastButton.setBounds (layout::fastButton);
    slowButton.setBounds (layout::slowButton);
    monoButton.setBounds (layout::monoButton);
    freezeButton.setBounds (layout::freezeButton);
    for (auto* b : { &energyButton, &phaseButton, &fastButton, &slowButton, &monoButton, &freezeButton })
        canvas.addAndMakeVisible (b);

    bindSwitchPair (energyButton, &phaseButton, ParamID::mode, 0.0f);
    bindSwitchPair (fastButton, &slowButton, ParamID::slow, 0.0f);
    bindSwitchPair (monoButton, nullptr, ParamID::monoCheck, 1.0f);
    freezeButton.setClickingTogglesState (true);   // display only, not a parameter
}

/** Connects one switch (toggle) or a pair of switches (either/or) to a parameter. */
void PhasemapEditor::bindSwitchPair (HardwareButton& onButton, HardwareButton* offButton, const char* parameterId, float onValue)
{
    auto* parameter = processor.parameters.getParameter (parameterId);
    const juce::String id (parameterId);

    switchAttachments.push_back (std::make_unique<juce::ParameterAttachment> (*parameter,
        [this, &onButton, offButton, onValue, id] (float value)
        {
            const bool isOn = std::abs (value - onValue) < 0.5f;
            onButton.setToggleState (isOn, juce::dontSendNotification);
            if (offButton != nullptr)
                offButton->setToggleState (! isOn, juce::dontSendNotification);

            if (id == ParamID::mode) spectrum.setPhaseMode (value > 0.5f);
            if (id == ParamID::slow) analysis.setSlowResponse (value > 0.5f);
        }));

    auto* attachment = switchAttachments.back().get();
    if (offButton != nullptr)
    {
        onButton.onClick = [attachment, onValue] { attachment->setValueAsCompleteGesture (onValue); };
        offButton->onClick = [attachment, onValue] { attachment->setValueAsCompleteGesture (1.0f - onValue); };
    }
    else
    {
        onButton.onClick = [attachment, &onButton, onValue]
        {
            attachment->setValueAsCompleteGesture (onButton.getToggleState() ? 1.0f - onValue : onValue);
        };
    }
    attachment->sendInitialUpdate();
}

void PhasemapEditor::setUpLoudness()
{
    loudnessPanel.setBounds (layout::loudnessPanel);
    canvas.addAndMakeVisible (loudnessPanel);

    resetButton.setBounds (layout::resetButton);
    resetButton.onClick = [this] { processor.loudness.resetRequested = true; };
    canvas.addAndMakeVisible (resetButton);

    const std::pair<TipArea*, juce::Rectangle<int>> areas[] = {
        { &tipTarget, LoudnessPanel::targetArea() },       { &tipMeter, LoudnessPanel::meterArea() },
        { &tipIntegrated, LoudnessPanel::integratedArea() }, { &tipRange, LoudnessPanel::rangeArea() },
        { &tipTruePeak, LoudnessPanel::truePeakArea() } };
    for (auto& [area, bounds] : areas)
    {
        area->setBounds (bounds + layout::loudnessPanel.getPosition());
        canvas.addAndMakeVisible (area);
    }
}

void PhasemapEditor::setUpHoverTips()
{
    hoverTips.add (energyButton, "Level of L and R per frequency band. Left bars = left channel, right bars = right channel. "
                                 "A lopsided shape means the band is panned or recorded unevenly.");
    hoverTips.add (phaseButton, "Phase correlation per band, no matter how loud. Centre = +1 (in phase), edge = -1 (cancels in mono). "
                                "Low End blinks red when something below 160 Hz is out of phase.");
    hoverTips.add (fastButton, "Display reacts quickly (0.3 s averaging). Good for spotting single hits.");
    hoverTips.add (slowButton, "Calm display (1.5 s averaging). Good for judging the overall picture.");
    hoverTips.add (monoButton, "Sums L and R on the output so you can hear what survives in mono.");
    hoverTips.add (freezeButton, "Stops the display so you can take a closer look. Audio and loudness measurement keep running.");
    hoverTips.add (holdBox, "How long the white marker holds the lowest correlation before it moves back. "
                            "Drag up or down, double-click for 3000 ms.");
    hoverTips.add (correlationBar, "Overall correlation. +1 = mono, 0 = wide, below 0 = parts cancel in mono.");
    hoverTips.add (goniometer, "Goniometer. Vertical = mono, wide cloud = stereo, horizontal = out of phase.");
    hoverTips.add (balanceMeter, "Overall level difference between L and R in dB.");
    hoverTips.add (tipTarget, "Loudness target. Choose a platform or make your own: click PHASEMAP at the top right.");
    hoverTips.add (tipMeter, "M = Momentary (400 ms), S = Short-term (3 s). The orange line is the target. "
                             "Green below the target, yellow up to 3 LU above, red beyond.");
    hoverTips.add (tipIntegrated, "Integrated loudness since the last reset, gated as in EBU R128. "
                                  "Green = on target, red = too loud, yellow = too quiet. The small number is the distance to the target.");
    hoverTips.add (tipRange, "Loudness Range (LRA): how much the loudness moves between quiet and loud parts, in LU.");
    hoverTips.add (tipTruePeak, "Highest true peak since the last reset (4x oversampled). Red when above the limit of the target.");
    hoverTips.add (resetButton, "Starts Integrated, Range and True Peak from zero.");
    hoverTips.add (settingsHotspot, "Settings: loudness target, resolution, window size.");

    hoverTips.setBounds (0, 0, theme::designWidth, theme::designHeight);
    canvas.addChildComponent (hoverTips);
}

//==============================================================================
void PhasemapEditor::showSettings (bool shouldShow)
{
    settingsOpen = shouldShow;
    if (shouldShow)
    {
        settingsMenu.refresh();
        settingsMenu.setVisible (true);
        settingsMenu.toFront (true);
        settingsMenu.grabKeyboardFocus();
    }
}

void PhasemapEditor::setWindowScale (double scale)
{
    scale = juce::jlimit (theme::minScale, theme::maxScale, scale);
    setSize (juce::roundToInt (theme::baseWidth * scale), juce::roundToInt (theme::baseHeight * scale));
}

/** Makes the window as large as the screen it is on, leaving room for the host's title bar. */
void PhasemapEditor::fitToScreen()
{
    if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForPoint (getScreenBounds().getCentre()))
    {
        const auto area = display->userArea;
        setWindowScale (juce::jmin ((double) area.getWidth() / theme::baseWidth, (area.getHeight() - 90.0) / theme::baseHeight));
        if (auto* window = getTopLevelComponent(); window != nullptr && window != this)
            window->setTopLeftPosition (area.getX() + (area.getWidth() - window->getWidth()) / 2, area.getY());
    }
}

void PhasemapEditor::resized()
{
    canvas.setTransform (juce::AffineTransform::scale ((float) getWidth() / theme::designWidth));
    if (windowReady)
        UserSettings::setWindowScale ((double) getWidth() / theme::baseWidth);
}

//==============================================================================
void PhasemapEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit (0.0, 0.1, (now - lastFrameMs) / 1000.0);
    lastFrameMs = now;

    // always empty the FIFO, so nothing old shows up after Freeze is released
    const bool frozen = freezeButton.getToggleState();
    analysis.setSampleRate (processor.getCurrentSampleRate());
    int received;
    while ((received = processor.analysisFifo.pull (pullLeft.data(), pullRight.data(), (int) pullLeft.size())) > 0)
        if (! frozen)
            analysis.process (pullLeft.data(), pullRight.data(), received);

    loudnessPanel.setTarget (processor.getTarget());

    if (! frozen)
    {
        const double holdSeconds = processor.parameters.getRawParameterValue (ParamID::holdMs)->load() / 1000.0;
        correlationBar.update (dt, holdSeconds);

        const auto& meter = processor.loudness;
        loudnessPanel.setValues ({ meter.momentary, meter.shortTerm, meter.integrated, meter.loudnessRange, meter.truePeak });

        spectrum.repaint();
        goniometer.repaint();
        balanceMeter.repaint();

        const float value = correlationBar.getDisplayedValue();
        correlationValue.setText (analysis.hasSignal() ? (value >= 0.005f ? "+" : "") + juce::String (value, 2) : juce::String ("--"),
                                  juce::dontSendNotification);
        correlationValue.setColour (juce::Label::textColourId, analysis.hasSignal() && value < 0.0f ? theme::red : theme::text);
    }

    hoverTips.advance (dt);
    updateMenuFade (dt);
}

void PhasemapEditor::updateMenuFade (double dt)
{
    const double before = settingsVisibility;
    settingsVisibility = settingsOpen ? juce::jmin (1.0, settingsVisibility + dt / 0.18)
                                      : juce::jmax (0.0, settingsVisibility - dt / 0.30);
    if (juce::exactlyEqual (before, settingsVisibility))
        return;

    settingsMenu.setAlpha (theme::easeInOut ((float) settingsVisibility));
    if (settingsVisibility <= 0.0)
        settingsMenu.setVisible (false);
}

} // namespace phasemap
