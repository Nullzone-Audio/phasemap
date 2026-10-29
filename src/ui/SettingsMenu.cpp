#include "SettingsMenu.h"
#include "UserSettings.h"

namespace phasemap
{

namespace
{
    const double windowScales[] = { 0.75, 1.0, 1.25, 1.5, 2.0, 3.0 };
    const int bandResolutions[] = { 3, 6, 12 };

    juce::String listEntry (const juce::String& name, float lufs, int decimals)
    {
        return name + "   " + juce::String (lufs, decimals) + " LUFS";
    }

    /** Reads a number from a text field, or returns the fallback if nothing numeric was typed. */
    float parseNumber (const juce::Label& field, float minimum, float maximum, float fallback)
    {
        const auto text = field.getText().retainCharacters ("-+.,0123456789").replaceCharacter (',', '.');
        return text.containsAnyOf ("0123456789") ? juce::jlimit (minimum, maximum, text.getFloatValue()) : fallback;
    }
}

SettingsMenu::SettingsMenu (PhasemapProcessor& p) : processor (p)
{
    setWantsKeyboardFocus (true);

    resolution.addItemList ({ "1/3 octave", "1/6 octave", "1/12 octave" }, 1);
    resolution.onChange = [this]
    {
        if (resolution.getSelectedItemIndex() >= 0 && onResolutionChosen != nullptr)
            onResolutionChosen (bandResolutions[resolution.getSelectedItemIndex()]);
    };

    for (int i = 0; i < (int) std::size (windowScales); ++i)
        windowSize.addItem (juce::String (juce::roundToInt (windowScales[i] * 100)) + " %", i + 1);
    windowSize.addItem ("Fit to screen", fitToScreenId);
    windowSize.onChange = [this]
    {
        if (windowSize.getSelectedId() == fitToScreenId) { if (onFitToScreen != nullptr) onFitToScreen(); }
        else if (windowSize.getSelectedItemIndex() >= 0 && onWindowScaleChosen != nullptr)
            onWindowScaleChosen (windowScales[windowSize.getSelectedItemIndex()]);
    };

    rebuildTargetList();
    targetList.onChange = [this] { targetChosen(); };

    for (auto* field : { &targetName, &targetLufs, &targetTruePeak, &targetTolerance })
    {
        field->setEditable (true, true, false);
        field->setFont (theme::font (14));
        field->setJustificationType (juce::Justification::centred);
        field->setColour (juce::Label::backgroundColourId, theme::fieldBack);
        field->setColour (juce::Label::outlineColourId, theme::outline);
        field->setColour (juce::Label::textColourId, theme::text);
        field->setColour (juce::Label::textWhenEditingColourId, theme::text);
        field->setColour (juce::TextEditor::highlightColourId, theme::accent.withAlpha (0.4f));
        field->setMouseCursor (juce::MouseCursor::IBeamCursor);
        field->onTextChange = [this] { applyTypedTarget(); };
        addAndMakeVisible (field);
    }
    targetName.setJustificationType (juce::Justification::centredLeft);

    saveButton.onClick = [this] { saveTypedTarget(); };
    deleteButton.onClick = [this] { deleteCurrentTarget(); };
    resetOnPlay.onClick = [this] { processor.setResetOnPlay (resetOnPlay.getToggleState()); };
    closeButton.onClick = [this] { if (onClose != nullptr) onClose(); };

    for (auto* c : std::initializer_list<juce::Component*> { &resolution, &windowSize, &targetList, &saveButton,
                                                              &deleteButton, &resetOnPlay, &closeButton })
        addAndMakeVisible (c);
}

//==============================================================================
void SettingsMenu::rebuildTargetList()
{
    targetList.clear (juce::dontSendNotification);

    const auto& presets = builtInTargets();
    for (int i = 0; i < (int) presets.size(); ++i)
        targetList.addItem (listEntry (presets[(size_t) i].longName, presets[(size_t) i].target.integratedLufs, 0), i + 1);

    targetList.addSeparator();
    targetList.addSectionHeading ("My Targets");
    const auto saved = UserSettings::getSavedTargets();
    for (int i = 0; i < (int) saved.size(); ++i)
        targetList.addItem (listEntry (saved[(size_t) i].name, saved[(size_t) i].integratedLufs, 1), firstSavedTargetId + i);
    targetList.addItem ("New custom target...", newTargetId);
}

void SettingsMenu::targetChosen()
{
    const int id = targetList.getSelectedId();
    const auto& presets = builtInTargets();
    const auto saved = UserSettings::getSavedTargets();

    if (id >= 1 && id <= (int) presets.size())
        processor.setTarget (presets[(size_t) id - 1].target);
    else if (id >= firstSavedTargetId && id - firstSavedTargetId < (int) saved.size())
        processor.setTarget (saved[(size_t) (id - firstSavedTargetId)]);
    else if (id == newTargetId)
    {
        auto target = processor.getTarget();
        target.name = "My Target";
        processor.setTarget (target);
        refresh();
        targetName.showEditor();
        return;
    }
    refresh();
}

void SettingsMenu::applyTypedTarget()
{
    const auto previous = processor.getTarget();
    LoudnessTarget target;
    target.name = targetName.getText().trim().substring (0, 32);
    if (target.name.isEmpty())
        target.name = "My Target";
    target.integratedLufs  = parseNumber (targetLufs, -40.0f, 0.0f, previous.integratedLufs);
    target.truePeakLimitDb = parseNumber (targetTruePeak, -10.0f, 0.0f, previous.truePeakLimitDb);
    target.toleranceLu     = parseNumber (targetTolerance, 0.1f, 6.0f, previous.toleranceLu);
    processor.setTarget (target);
    refresh();
}

void SettingsMenu::saveTypedTarget()
{
    UserSettings::saveTarget (processor.getTarget());
    rebuildTargetList();
    refresh();
}

void SettingsMenu::deleteCurrentTarget()
{
    UserSettings::deleteTarget (processor.getTarget().name);
    rebuildTargetList();
    refresh();
}

void SettingsMenu::refresh()
{
    const int bands = UserSettings::getBandsPerOctave();
    resolution.setSelectedItemIndex (bands == 3 ? 0 : (bands == 12 ? 2 : 1), juce::dontSendNotification);

    const double scale = UserSettings::getWindowScale();
    int nearest = 1;
    for (int i = 0; i < (int) std::size (windowScales); ++i)
        if (std::abs (windowScales[i] - scale) < std::abs (windowScales[nearest] - scale))
            nearest = i;
    if (std::abs (windowScales[nearest] - scale) > 0.02)
        windowSize.setText (juce::String (juce::roundToInt (scale * 100)) + " %", juce::dontSendNotification);
    else
        windowSize.setSelectedItemIndex (nearest, juce::dontSendNotification);

    // select the matching list entry, or show the typed values as "not saved"
    const auto target = processor.getTarget();
    const auto& presets = builtInTargets();
    const auto saved = UserSettings::getSavedTargets();
    int matchingId = 0;
    for (int i = 0; i < (int) presets.size() && matchingId == 0; ++i)
        if (presets[(size_t) i].target == target)
            matchingId = i + 1;
    for (int i = 0; i < (int) saved.size() && matchingId == 0; ++i)
        if (saved[(size_t) i] == target)
            matchingId = firstSavedTargetId + i;

    if (matchingId > 0)
        targetList.setSelectedId (matchingId, juce::dontSendNotification);
    else
        targetList.setText (listEntry (target.name, target.integratedLufs, 1) + "  (not saved)", juce::dontSendNotification);

    targetName.setText (target.name, juce::dontSendNotification);
    targetLufs.setText (juce::String (target.integratedLufs, 1), juce::dontSendNotification);
    targetTruePeak.setText (juce::String (target.truePeakLimitDb, 1), juce::dontSendNotification);
    targetTolerance.setText (juce::String (target.toleranceLu, 1), juce::dontSendNotification);

    const bool nameIsSaved = std::any_of (saved.begin(), saved.end(), [&] (const auto& t) { return t.name == target.name; });
    deleteButton.setEnabled (matchingId >= firstSavedTargetId);
    saveButton.setButtonText (nameIsSaved ? "Update" : "Save");

    resetOnPlay.setToggleState (processor.getResetOnPlay(), juce::dontSendNotification);
    repaint();
}

//==============================================================================
void SettingsMenu::resized()
{
    const auto p = panelBounds().getPosition();
    closeButton.setBounds (p.x + panelBounds().getWidth() - 66, p.y + 22, 30, 30);
    resolution.setBounds (p.x + 220, p.y + 90, 170, 30);
    windowSize.setBounds (p.x + 220, p.y + 145, 170, 30);
    targetList.setBounds (p.x + 220, p.y + 200, 330, 30);
    targetName.setBounds (p.x + 220, p.y + 250, 190, 30);
    saveButton.setBounds (p.x + 420, p.y + 250, 80, 30);
    deleteButton.setBounds (p.x + 508, p.y + 250, 80, 30);
    targetLufs.setBounds (p.x + 220, p.y + 292, 64, 30);
    targetTruePeak.setBounds (p.x + 368, p.y + 292, 64, 30);
    targetTolerance.setBounds (p.x + 504, p.y + 292, 50, 30);
    resetOnPlay.setBounds (p.x + 216, p.y + 350, 400, 30);
}

void SettingsMenu::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.5f));

    const auto panel = panelBounds().toFloat();
    g.setColour (juce::Colour (0xff1c1c1c));
    g.fillRoundedRectangle (panel, 12);
    g.setColour (theme::menuBack);
    g.fillRoundedRectangle (panel.reduced (5), 8);

    auto text = [&] (const juce::String& s, int x, int y, juce::Colour colour, float size, bool bold = false)
    {
        g.setColour (colour);
        g.setFont (theme::font (size, bold));
        g.drawText (s, (int) panel.getX() + x, (int) panel.getY() + y, 560, 30, juce::Justification::centredLeft);
    };

    text ("PHASEMAP", 36, 24, theme::accent, 20, true);
   #ifdef PHASEMAP_VERSION_LABEL
    text (PHASEMAP_VERSION_LABEL, 160, 24, theme::textDim, 14);
   #endif
    text ("Resolution", 36, 90, theme::accent, 14, true);
    text ("bands in the spectrum", 402, 90, theme::textDim, 14);
    text ("Window Size", 36, 145, theme::accent, 14, true);
    text ("or drag the lower right corner", 402, 145, theme::textDim, 13);
    text ("Loudness Target", 36, 200, theme::accent, 14, true);
    text ("Custom Target", 36, 250, theme::accent, 14, true);
    text ("Target", 36, 292, theme::textDim, 13);
    text ("LUFS", 290, 292, theme::textDim, 13);
    text ("TP", 344, 292, theme::textDim, 13);
    text ("dBTP", 438, 292, theme::textDim, 13);
    text (juce::String::fromUTF8 ("\xc2\xb1"), 490, 292, theme::textDim, 14);
    text ("LU", 560, 292, theme::textDim, 13);
    text ("Integrated", 36, 350, theme::accent, 14, true);
    text ("Type a name and values, then Save. Saved targets appear under My Targets in every project.", 36, 405, theme::textDim, 13);
    text ("Loudness after ITU-R BS.1770-4 / EBU R128, measured after Mono Check.", 36, 430, theme::textDim, 13);
    text ("Phasemap by Nullzone Audio - free and open source - nullzone-audio.org", 36, 455, theme::textDim, 13);
}

void SettingsMenu::mouseDown (const juce::MouseEvent& e)
{
    if (! panelBounds().contains (e.getPosition()) && onClose != nullptr)
        onClose();
}

bool SettingsMenu::keyPressed (const juce::KeyPress& key)
{
    if (key != juce::KeyPress::escapeKey)
        return false;
    if (onClose != nullptr)
        onClose();
    return true;
}

} // namespace phasemap
