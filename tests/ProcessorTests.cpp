// The whole plugin as a host would use it: layouts, sample rates, block sizes, state.

#include "TestSupport.h"
#include "plugin/PhasemapProcessor.h"

using phasemap::PhasemapProcessor;

namespace
{
    float maxDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        float worst = 0.0f;
        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                worst = std::max (worst, std::abs (a.getSample (ch, i) - b.getSample (ch, i)));
        return worst;
    }

    bool allFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i)))
                    return false;
        return true;
    }

    void fillSine (juce::AudioBuffer<float>& b, double& phase, double hz, double sampleRate, float gain)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = gain * (float) std::sin (phase);
            phase += test::twoPi * hz / sampleRate;
            for (int ch = 0; ch < b.getNumChannels(); ++ch)
                b.setSample (ch, i, ch == 1 ? 0.5f * v : v);
        }
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    juce::MidiBuffer midi;

    // audio passes through untouched (Mono Check off), at every common rate and block size
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int blockSize : { 1, 17, 64, 512, 4096 })
        {
            PhasemapProcessor p;
            p.prepareToPlay (rate, blockSize);
            juce::AudioBuffer<float> buffer (2, blockSize), original;
            double phase = 0;
            float worst = 0.0f;
            const int blocks = juce::jmax (1, (int) (rate * 0.5) / blockSize);
            for (int b = 0; b < blocks; ++b)
            {
                fillSine (buffer, phase, 440.0, rate, 0.5f);
                original.makeCopyOf (buffer);
                p.processBlock (buffer, midi);
                worst = std::max (worst, maxDifference (buffer, original));
            }
            char name[96];
            std::snprintf (name, sizeof (name), "pass-through at %.0f Hz, block %d", rate, blockSize);
            test::near (name, worst, 0.0, 0.0);
        }

    // layouts
    {
        PhasemapProcessor p;
        using Set = juce::AudioChannelSet;
        test::isTrue ("stereo -> stereo supported", p.checkBusesLayoutSupported ({ { Set::stereo() }, { Set::stereo() } }));
        test::isTrue ("mono -> mono supported", p.checkBusesLayoutSupported ({ { Set::mono() }, { Set::mono() } }));
        test::isTrue ("mono -> stereo rejected", ! p.checkBusesLayoutSupported ({ { Set::mono() }, { Set::stereo() } }));
        test::isTrue ("5.1 rejected", ! p.checkBusesLayoutSupported ({ { Set::create5point1() }, { Set::create5point1() } }));
    }

    // mono track: measures loudness, passes audio
    {
        PhasemapProcessor p;
        p.setPlayConfigDetails (1, 1, 48000.0, 512);
        p.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (1, 512);
        double phase = 0;
        for (int b = 0; b < 48000 * 5 / 512; ++b)
        {
            fillSine (buffer, phase, 1000.0, 48000.0, (float) std::pow (10.0, -20.0 / 20.0));
            p.processBlock (buffer, midi);
        }
        test::near ("mono track, 1 kHz -20 dBFS: Integrated (LUFS)", p.loudness.integrated, -23.0, 0.1);
    }

    // Mono Check sums both channels
    {
        PhasemapProcessor p;
        p.parameters.getParameter (phasemap::ParamID::monoCheck)->setValueNotifyingHost (1.0f);
        p.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (2, 512);
        double phase = 0;
        for (int b = 0; b < 10; ++b)
        {
            fillSine (buffer, phase, 300.0, 48000.0, 0.5f);
            p.processBlock (buffer, midi);
        }
        test::near ("Mono Check: L equals R", maxDifference (juce::AudioBuffer<float> (buffer.getArrayOfWritePointers(), 1, 512),
                                                             juce::AudioBuffer<float> (buffer.getArrayOfWritePointers() + 1, 1, 512)), 0.0, 1e-6);
    }

    // silence and extreme input stay finite
    {
        PhasemapProcessor p;
        p.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> buffer (2, 256);
        buffer.clear();
        p.processBlock (buffer, midi);
        for (int i = 0; i < 256; ++i) { buffer.setSample (0, i, 1.0e6f); buffer.setSample (1, i, -1.0e6f); }
        p.processBlock (buffer, midi);
        test::isTrue ("huge input: output finite", allFinite (buffer));
        test::isTrue ("huge input: loudness finite", std::isfinite (p.loudness.momentary.load()) && std::isfinite (p.loudness.truePeak.load()));
    }

    // state: parameters and the loudness target survive save and load
    {
        PhasemapProcessor a;
        a.parameters.getParameter (phasemap::ParamID::mode)->setValueNotifyingHost (1.0f);
        a.setTarget ({ "Club", -9.0f, -0.3f, 1.0f });
        a.setResetOnPlay (true);
        juce::MemoryBlock state;
        a.getStateInformation (state);

        PhasemapProcessor b;
        b.setStateInformation (state.getData(), (int) state.getSize());
        test::isTrue ("state: mode restored", b.parameters.getRawParameterValue (phasemap::ParamID::mode)->load() > 0.5f);
        test::isTrue ("state: target restored", b.getTarget() == phasemap::LoudnessTarget { "Club", -9.0f, -0.3f, 1.0f });
        test::isTrue ("state: reset-on-play restored", b.getResetOnPlay());
    }

    // the editor opens and closes without problems
    {
        PhasemapProcessor p;
        p.prepareToPlay (48000.0, 512);
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        test::isTrue ("editor opens with a 16:9 size", editor != nullptr && editor->getWidth() * 9 == editor->getHeight() * 16);
    }

    return test::finish();
}
