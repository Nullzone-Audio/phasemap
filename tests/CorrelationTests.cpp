// Spectral correlation with signals whose answer is known.

#include "TestSupport.h"
#include "dsp/SpectralCorrelation.h"

using phasemap::SpectralCorrelation;

namespace
{
    constexpr double sampleRate = 48000.0;

    std::unique_ptr<SpectralCorrelation> analyse (const test::Generator& generate, double seconds = 3.0)
    {
        auto analysis = std::make_unique<SpectralCorrelation>();
        analysis->setSampleRate (sampleRate);
        std::vector<float> l, r;
        test::render ((int) (seconds * sampleRate), generate, l, r);
        for (size_t i = 0; i < l.size(); i += 512)
            analysis->process (l.data() + i, r.data() + i, (int) std::min<size_t> (512, l.size() - i));
        return analysis;
    }

    const SpectralCorrelation::Band& bandAt (const SpectralCorrelation& a, double hz)
    {
        for (auto& b : a.getBands())
            if (hz >= b.lowHz && hz < b.highHz)
                return b;
        return a.getBands().back();
    }
}

int main()
{
    juce::Random random (42);
    auto noise = [&] { return random.nextFloat() * 2.0f - 1.0f; };

    {
        const auto a = analyse ([&] (int, float& l, float& r) { l = r = 0.3f * noise(); });
        test::near ("mono noise: overall correlation", a->getCorrelation(), 1.0, 0.01);
        test::near ("mono noise: band at 1 kHz", bandAt (*a, 1000).correlation, 1.0, 0.01);
        test::near ("mono noise: balance (dB)", a->getBalanceDb(), 0.0, 0.1);
    }
    {
        const auto a = analyse ([&] (int, float& l, float& r) { l = 0.3f * noise(); r = -l; });
        test::near ("polarity inverted: overall correlation", a->getCorrelation(), -1.0, 0.01);
        test::near ("polarity inverted: band at 100 Hz", bandAt (*a, 100).correlation, -1.0, 0.01);
    }
    {
        const auto a = analyse ([&] (int, float& l, float& r) { l = 0.3f * noise(); r = 0.3f * noise(); });
        test::near ("two unrelated noises: overall correlation", a->getCorrelation(), 0.0, 0.1);
        test::near ("two unrelated noises: band at 2 kHz", bandAt (*a, 2000).correlation, 0.0, 0.25);
    }
    {
        const auto a = analyse ([&] (int, float& l, float& r) { l = 0.3f * noise(); r = 0.5f * l; });
        test::near ("R at half level: balance (dB)", a->getBalanceDb(), -6.02, 0.1);
        const auto& b = bandAt (*a, 1000);
        test::near ("R at half level: L-R level difference at 1 kHz", b.levelLeftDb - b.levelRightDb, 6.02, 0.5);
    }
    {
        const auto a = analyse ([&] (int i, float& l, float& r) { l = 0.5f * (float) std::sin (test::twoPi * 1000 * i / sampleRate); r = 0; });
        test::near ("1 kHz sine at 0.5 in L: level (dBFS)", bandAt (*a, 1000).levelLeftDb, -6.02, 1.0);
    }
    {
        // R = L delayed by 0.5 ms: 180 degrees at 1 kHz (cancels in mono), 360 degrees at 2 kHz
        std::vector<float> history (24, 0.0f);
        int position = 0;
        const auto a = analyse ([&] (int, float& l, float& r)
        {
            l = 0.3f * noise();
            r = history[(size_t) position];
            history[(size_t) position] = l;
            position = (position + 1) % 24;
        });
        test::near ("0.5 ms delay: band at 1 kHz cancels", bandAt (*a, 1000).correlation, -1.0, 0.1);
        test::near ("0.5 ms delay: band at 2 kHz in phase", bandAt (*a, 2000).correlation, 1.0, 0.1);
    }
    {
        const auto a = analyse ([&] (int, float& l, float& r) { l = r = 0.0f; });
        test::isTrue ("silence: no signal, correlation 0", ! a->hasSignal() && a->getCorrelation() == 0.0f);
    }
    {
        SpectralCorrelation a;
        for (int bands : { 3, 6, 12 })
        {
            a.setBandsPerOctave (bands);
            test::isTrue ("bands cover 20 Hz to 20 kHz", a.getBands().front().lowHz <= 20.0 && a.getBands().back().highHz >= 19999.0);
        }
    }
    return test::finish();
}
