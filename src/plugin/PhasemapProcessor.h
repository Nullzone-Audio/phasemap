#pragma once

#include "Parameters.h"
#include "../dsp/LoudnessMeter.h"
#include "../dsp/LoudnessTargets.h"
#include "../dsp/StereoFifo.h"

namespace phasemap
{

/** The audio side of Phasemap.

    Audio passes through unchanged (except when Mono Check is on). The processor
      - measures loudness (LoudnessMeter, on the audio thread), and
      - copies the samples into a FIFO for the spectral analysis in the editor.
    Supported layouts: mono -> mono and stereo -> stereo.
*/
class PhasemapProcessor : public juce::AudioProcessor
{
public:
    PhasemapProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                     { return true; }

    const juce::String getName() const override        { return "Phasemap"; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    bool isMidiEffect() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0.0; }

    int getNumPrograms() override                       { return 1; }
    int getCurrentProgram() override                    { return 0; }
    void setCurrentProgram (int) override               {}
    const juce::String getProgramName (int) override    { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    StereoFifo analysisFifo;     // audio thread -> editor
    LoudnessMeter loudness;      // results are atomics, safe to read from the editor

    double getCurrentSampleRate() const                 { return currentSampleRate.load(); }

    /** The loudness target is stored in the project with all its values, so it survives even
        if the user later deletes it from their saved targets. Message thread only. */
    LoudnessTarget getTarget() const;
    void setTarget (const LoudnessTarget&);

    bool getResetOnPlay() const                         { return resetOnPlay.load(); }
    void setResetOnPlay (bool shouldReset);

private:
    std::atomic<double> currentSampleRate { 48000.0 };
    std::atomic<float>* monoCheckParam = nullptr;
    juce::SmoothedValue<float> monoAmount { 0.0f };    // 30 ms crossfade, no clicks
    std::atomic<bool> resetOnPlay { false };
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhasemapProcessor)
};

} // namespace phasemap
