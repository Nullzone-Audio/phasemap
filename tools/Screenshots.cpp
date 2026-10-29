// Renders the editor with a music-like test signal and saves PNGs (README, website, design reviews).
//
//   PhasemapScreenshots <output folder> [scale]
//
// Signal: 120 BPM kick + bass, a slightly wide pad, off-beat hi-hat leaning right.
// For the "problem" shot, kick and bass have their polarity flipped in the right channel.

#include "plugin/PhasemapProcessor.h"
#include "ui/PhasemapEditor.h"

using namespace phasemap;

namespace
{
    struct TestSong
    {
        double time = 0;
        juce::Random random { 7 };
        juce::IIRFilter hatLeft, hatRight;

        TestSong()
        {
            hatLeft.setCoefficients (juce::IIRCoefficients::makeHighPass (48000, 6000));
            hatRight.setCoefficients (juce::IIRCoefficients::makeHighPass (48000, 6000));
        }

        void render (juce::AudioBuffer<float>& buffer, bool lowEndFlipped, float gain)
        {
            const double twoPi = juce::MathConstants<double>::twoPi;
            for (int i = 0; i < buffer.getNumSamples(); ++i, time += 1.0 / 48000)
            {
                const double beat = std::fmod (time, 0.5);
                const float kick = (float) (std::exp (-beat * 9) * std::sin (twoPi * (50 + 60 * std::exp (-beat * 30)) * beat));
                const float bass = 0.45f * (float) std::sin (twoPi * 55 * time) * (0.6f + 0.4f * (float) std::exp (-beat * 3));
                const float low = 0.6f * kick + bass;

                float padL = 0, padR = 0;
                for (double hz : { 220.0, 277.2, 329.6, 440.0 })
                {
                    padL += 0.05f * (float) std::sin (twoPi * hz * time);
                    padR += 0.05f * (float) std::sin (twoPi * hz * time + 0.35);
                }

                const float envelope = (float) std::exp (-std::fmod (time + 0.25, 0.5) * 40) * 0.35f;
                const float n1 = random.nextFloat() * 2 - 1, n2 = random.nextFloat() * 2 - 1;
                const float hatL = hatLeft.processSingleSampleRaw (envelope * 0.5f * n1);
                const float hatR = hatRight.processSingleSampleRaw (envelope * (0.8f * n1 + 0.45f * n2));

                buffer.setSample (0, i, gain * (low + padL + hatL));
                buffer.setSample (1, i, gain * ((lowEndFlipped ? -low : low) + padR + hatR));
            }
        }
    };

    struct Session
    {
        PhasemapProcessor processor;
        std::unique_ptr<PhasemapEditor> editor;
        TestSong song;

        Session()
        {
            processor.prepareToPlay (48000, 512);
            processor.setTarget (builtInTargets()[2].target);   // YouTube
            editor.reset (dynamic_cast<PhasemapEditor*> (processor.createEditor()));
        }

        /** Plays audio; the last second runs in real time so the display smoothing settles naturally. */
        void play (double seconds, bool lowEndFlipped)
        {
            juce::AudioBuffer<float> buffer (2, 512);
            juce::MidiBuffer midi;
            const int blocks = (int) (seconds * 48000 / 512);
            for (int b = 0; b < blocks; ++b)
            {
                song.render (buffer, lowEndFlipped, 0.62f);
                processor.processBlock (buffer, midi);
                const bool realTime = b > blocks - 60;
                if (realTime) juce::Thread::sleep (11);
                if (realTime || b % 4 == 0) editor->advanceOneFrame();
            }
        }

        void setParameter (const char* id, float value)
        {
            processor.parameters.getParameter (id)->setValueNotifyingHost (value);
        }

        /** Writes everything the display shows as numbers, for tools/export_design_svg.py. */
        void saveState (const juce::File& file)
        {
            auto& a = editor->getAnalysis();
            auto* root = new juce::DynamicObject();

            juce::Array<juce::var> bands;
            for (auto& b : a.getBands())
            {
                auto* o = new juce::DynamicObject();
                o->setProperty ("lowHz", b.lowHz);
                o->setProperty ("highHz", b.highHz);
                o->setProperty ("centreHz", b.centreHz);
                o->setProperty ("levelLeftDb", b.levelLeftDb);
                o->setProperty ("levelRightDb", b.levelRightDb);
                o->setProperty ("correlation", b.correlation);
                bands.add (juce::var (o));
            }
            root->setProperty ("bands", bands);

            juce::Array<juce::var> scopeL, scopeR;
            const int n = (int) a.getScopeLeft().size(), oldest = a.getScopeWritePosition();
            for (int k = 0; k < n; ++k)
            {
                scopeL.add (a.getScopeLeft()[(size_t) ((oldest + k) % n)]);
                scopeR.add (a.getScopeRight()[(size_t) ((oldest + k) % n)]);
            }
            root->setProperty ("scopeLeft", scopeL);
            root->setProperty ("scopeRight", scopeR);

            root->setProperty ("hasSignal", a.hasSignal());
            root->setProperty ("balanceDb", a.getBalanceDb());
            root->setProperty ("correlationShown", editor->getCorrelationBar().getDisplayedValue());
            root->setProperty ("correlationLowest", editor->getCorrelationBar().getLowestValue());

            auto param = [&] (const char* id) { return processor.parameters.getRawParameterValue (id)->load(); };
            root->setProperty ("phaseMode", param (ParamID::mode) > 0.5f);
            root->setProperty ("slow", param (ParamID::slow) > 0.5f);
            root->setProperty ("mono", param (ParamID::monoCheck) > 0.5f);
            root->setProperty ("holdMs", param (ParamID::holdMs));
            root->setProperty ("frozen", editor->isFrozen());

            auto& m = processor.loudness;
            root->setProperty ("momentary", m.momentary.load());
            root->setProperty ("shortTerm", m.shortTerm.load());
            root->setProperty ("integrated", m.integrated.load());
            root->setProperty ("loudnessRange", m.loudnessRange.load());
            root->setProperty ("truePeak", m.truePeak.load());

            const auto t = processor.getTarget();
            root->setProperty ("targetName", t.name);
            root->setProperty ("targetLufs", t.integratedLufs);
            root->setProperty ("targetTruePeak", t.truePeakLimitDb);
            root->setProperty ("targetTolerance", t.toleranceLu);

            file.replaceWithText (juce::JSON::toString (juce::var (root)));
            std::printf ("%s\n", file.getFullPathName().toRawUTF8());
        }

        void save (const juce::File& file, float scale)
        {
            auto& canvas = editor->getCanvas();
            auto image = canvas.createComponentSnapshot (canvas.getLocalBounds(), true, scale);
            file.deleteFile();
            juce::FileOutputStream stream (file);
            juce::PNGImageFormat().writeImageToStream (image, stream);
            std::printf ("%s\n", file.getFullPathName().toRawUTF8());
        }
    };
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const juce::File folder (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                      : juce::File::getCurrentWorkingDirectory());
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 1.5f;
    folder.createDirectory();

    Session s;
    s.play (12, false);
    s.save (folder.getChildFile ("energy.png"), scale);
    s.saveState (folder.getChildFile ("energy.json"));

    s.setParameter (ParamID::mode, 1.0f);
    s.play (3, false);
    s.save (folder.getChildFile ("phase.png"), scale);

    s.play (4, true);
    while ((juce::Time::getMillisecondCounter() / 200) % 2 != 0)   // catch the Low End lamp while it is lit
        juce::Thread::sleep (5);
    s.editor->advanceOneFrame();
    s.save (folder.getChildFile ("phase-problem.png"), scale);
    s.saveState (folder.getChildFile ("phase-problem.json"));

    s.setParameter (ParamID::mode, 0.0f);
    s.play (3, false);
    s.editor->showHoverTipFor (s.editor->getPhaseButton());
    s.save (folder.getChildFile ("hover-tip.png"), scale);

    s.editor->showSettings (true);
    for (int i = 0; i < 30; ++i) { juce::Thread::sleep (16); s.editor->advanceOneFrame(); }
    s.save (folder.getChildFile ("settings.png"), scale);
    return 0;
}
