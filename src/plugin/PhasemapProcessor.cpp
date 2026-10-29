#include "PhasemapProcessor.h"
#include "../ui/PhasemapEditor.h"

namespace phasemap
{

namespace StateKey
{
    static const juce::Identifier targetName ("targetName"), targetLufs ("target"), truePeakLimit ("tpLimit"),
                                  tolerance ("tolerance"), legacyFormatIndex ("format"), resetOnPlay ("resetOnPlay");
}

PhasemapProcessor::PhasemapProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "STATE", createParameterLayout())
{
    monoCheckParam = parameters.getRawParameterValue (ParamID::monoCheck);
}

bool PhasemapProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return layout.getMainInputChannelSet() == out
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void PhasemapProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    monoAmount.reset (sampleRate, 0.03);
    monoAmount.setCurrentAndTargetValue (monoCheckParam->load() > 0.5f ? 1.0f : 0.0f);
    loudness.prepare (sampleRate);
    wasPlaying = false;
}

void PhasemapProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), getTotalNumInputChannels());
    if (numChannels < 1 || numSamples < 1)
        return;

    // optional: restart Integrated when the host starts playing
    if (auto* host = getPlayHead())
        if (auto position = host->getPosition())
        {
            const bool playing = position->getIsPlaying();
            if (playing && ! wasPlaying && resetOnPlay.load())
                loudness.resetRequested = true;
            wasPlaying = playing;
        }

    if (numChannels == 1)
    {
        const auto* mono = buffer.getReadPointer (0);
        loudness.process (mono, nullptr, numSamples);
        analysisFifo.push (mono, mono, numSamples);
        return;
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    // Mono Check: both channels become (L + R) / 2
    monoAmount.setTargetValue (monoCheckParam->load() > 0.5f ? 1.0f : 0.0f);
    if (monoAmount.isSmoothing() || monoAmount.getTargetValue() > 0.5f)
        for (int i = 0; i < numSamples; ++i)
        {
            const float amount = monoAmount.getNextValue();
            const float sum = 0.5f * (left[i] + right[i]);
            left[i]  += amount * (sum - left[i]);
            right[i] += amount * (sum - right[i]);
        }

    loudness.process (left, right, numSamples);
    analysisFifo.push (left, right, numSamples);
}

//==============================================================================
LoudnessTarget PhasemapProcessor::getTarget() const
{
    const auto& state = parameters.state;
    if (state.hasProperty (StateKey::targetLufs))
        return { state.getProperty (StateKey::targetName).toString(),
                 (float) state.getProperty (StateKey::targetLufs),
                 (float) state.getProperty (StateKey::truePeakLimit),
                 (float) state.getProperty (StateKey::tolerance) };

    // projects saved by the pre-release "Correlation Meter" only stored an index
    const auto& presets = builtInTargets();
    const int index = juce::jlimit (0, (int) presets.size() - 1, (int) state.getProperty (StateKey::legacyFormatIndex, 0));
    return presets[(size_t) index].target;
}

void PhasemapProcessor::setTarget (const LoudnessTarget& target)
{
    auto& state = parameters.state;
    state.setProperty (StateKey::targetName, target.name, nullptr);
    state.setProperty (StateKey::targetLufs, target.integratedLufs, nullptr);
    state.setProperty (StateKey::truePeakLimit, target.truePeakLimitDb, nullptr);
    state.setProperty (StateKey::tolerance, target.toleranceLu, nullptr);
}

void PhasemapProcessor::setResetOnPlay (bool shouldReset)
{
    resetOnPlay = shouldReset;
    parameters.state.setProperty (StateKey::resetOnPlay, shouldReset, nullptr);
}

//==============================================================================
void PhasemapProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void PhasemapProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
            resetOnPlay = (bool) parameters.state.getProperty (StateKey::resetOnPlay, false);
        }
}

juce::AudioProcessorEditor* PhasemapProcessor::createEditor()
{
    return new PhasemapEditor (*this);
}

} // namespace phasemap

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new phasemap::PhasemapProcessor();
}
