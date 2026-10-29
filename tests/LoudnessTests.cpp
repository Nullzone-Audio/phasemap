// Loudness measurement against the EBU test signals (Tech 3341 "minimum requirements" and Tech 3342 LRA).

#include "TestSupport.h"
#include "dsp/LoudnessMeter.h"

using phasemap::LoudnessMeter;

namespace
{
    /** Plays sine segments through a meter, 512 samples at a time. */
    struct SinePlayer
    {
        LoudnessMeter& meter;
        double sampleRate, phase = 0;

        void play (double seconds, double dbfs, double hz = 1000.0, bool rightOnly = false, bool mono = false)
        {
            const double amplitude = std::pow (10.0, dbfs / 20.0);
            const int total = (int) std::lround (seconds * sampleRate);
            std::vector<float> l ((size_t) total), r ((size_t) total);
            for (int i = 0; i < total; ++i)
            {
                const float v = (float) (amplitude * std::sin (phase));
                phase += test::twoPi * hz / sampleRate;
                l[(size_t) i] = rightOnly ? 0.0f : v;
                r[(size_t) i] = v;
            }
            for (int i = 0; i < total; i += 512)
                meter.process (l.data() + i, mono ? nullptr : r.data() + i, std::min (512, total - i));
        }
    };

    void runAt (double sampleRate)
    {
        std::printf ("\n--- %.0f Hz\n", sampleRate);
        LoudnessMeter meter;
        meter.prepare (sampleRate);
        SinePlayer player { meter, sampleRate };

        player.play (20, -23);   // Tech 3341, case 1
        test::near ("1 kHz -23 dBFS stereo: Momentary", meter.momentary, -23, 0.1);
        test::near ("1 kHz -23 dBFS stereo: Short-term", meter.shortTerm, -23, 0.1);
        test::near ("1 kHz -23 dBFS stereo: Integrated", meter.integrated, -23, 0.1);

        meter.reset();
        player.play (20, -33);   // case 2
        test::near ("1 kHz -33 dBFS: Integrated", meter.integrated, -33, 0.1);

        meter.reset();           // case 3: relative gate
        player.play (10, -36); player.play (60, -23); player.play (10, -36);
        test::near ("-36/-23/-36 dBFS (relative gate): Integrated", meter.integrated, -23, 0.1);

        meter.reset();           // case 4: absolute and relative gate
        player.play (10, -72); player.play (10, -36); player.play (60, -23); player.play (10, -36); player.play (10, -72);
        test::near ("-72/-36/-23/-36/-72 dBFS: Integrated", meter.integrated, -23, 0.1);

        meter.reset();           // Tech 3342, case 1 and 3
        player.play (20, -20); player.play (20, -30);
        test::near ("LRA, -20 then -30 dBFS", meter.loudnessRange, 10, 1);
        meter.reset();
        player.play (20, -40); player.play (20, -20);
        test::near ("LRA, -40 then -20 dBFS", meter.loudnessRange, 20, 1);

        meter.reset();           // one channel only is 3 dB quieter than both
        player.play (10, -20, 1000, true);
        test::near ("-20 dBFS in one channel: Integrated", meter.integrated, -23, 0.1);

        meter.reset();           // a mono signal counts once (channel weight 1)
        player.play (10, -20, 1000, false, true);
        test::near ("-20 dBFS mono: Integrated", meter.integrated, -23, 0.1);

        // true peak: a sine at fs/4 with 45 degree phase has samples at +-0.707 but peaks at 1.0.
        // Above 96 kHz no oversampling is needed (BS.1770: the signal is already at >= 192 kHz),
        // and fs/4 would be an ultrasonic 48 kHz tone there, so the case is skipped.
        meter.reset();
        if (sampleRate <= 96000.0)
        {
            std::vector<float> x (4800);
            for (size_t i = 0; i < x.size(); ++i)
                x[i] = (float) std::sin (test::twoPi / 4 * (double) i + test::twoPi / 8);
            meter.process (x.data(), x.data(), (int) x.size());
            test::near ("fs/4 sine, sample peak -3 dB: True Peak (dBTP)", meter.truePeak, 0.0, 0.3);   // Tech 3341 allows +0.2/-0.4
        }

        meter.reset();
        player.play (2, -6, 997);
        test::near ("997 Hz -6 dBFS: True Peak (dBTP)", meter.truePeak, -6.0, 0.2);

        meter.reset();
        std::vector<float> silence (48000, 0.0f);
        meter.process (silence.data(), silence.data(), (int) silence.size());
        test::isTrue ("silence stays unmeasured", meter.integrated.load() <= -70.0f && meter.truePeak.load() <= -100.0f);
    }
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
        runAt (rate);
    return test::finish();
}
