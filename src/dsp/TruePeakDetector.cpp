#include "TruePeakDetector.h"

#include <algorithm>
#include <cmath>

namespace phasemap
{

void TruePeakDetector::prepare (double sampleRate)
{
    factor = sampleRate <= 50000.0 ? 4 : (sampleRate <= 100000.0 ? 2 : 1);
    tapsPerPhase = filterLength / factor;

    const double pi = 3.14159265358979323846;
    const double centre = filterLength / 2;          // integer centre: phase 0 hits the original samples
    const double cutoff = 0.5 / factor * 0.92;       // just below the original Nyquist frequency

    phases.assign ((size_t) factor, std::vector<float> ((size_t) tapsPerPhase, 0.0f));
    for (int i = 0; i < filterLength; ++i)
    {
        const double x = i - centre;
        const double sinc = std::abs (x) < 1e-9 ? 2.0 * cutoff : std::sin (2.0 * pi * cutoff * x) / (pi * x);
        const double blackman = 0.42 - 0.5 * std::cos (2.0 * pi * i / (filterLength - 1))
                                     + 0.08 * std::cos (4.0 * pi * i / (filterLength - 1));
        phases[(size_t) (i % factor)][(size_t) (i / factor)] = (float) (sinc * blackman);
    }

    for (auto& phase : phases)   // unity gain per phase
    {
        float sum = 0.0f;
        for (auto v : phase) sum += v;
        if (sum > 0.0f)
            for (auto& v : phase) v /= sum;
    }

    for (auto& h : history)
        h.assign ((size_t) tapsPerPhase, 0.0f);

    reset();
}

void TruePeakDetector::reset()
{
    for (auto& h : history)
        std::fill (h.begin(), h.end(), 0.0f);
    writePosition = 0;
    peak = 0.0f;
}

void TruePeakDetector::process (float left, float right) noexcept
{
    const float input[2] = { left, right };

    for (size_t channel = 0; channel < 2; ++channel)
    {
        auto& h = history[channel];
        h[(size_t) writePosition] = input[channel];
        peak = std::max (peak, std::abs (input[channel]));   // never below the sample peak

        if (factor == 1)
            continue;

        for (auto& phase : phases)
        {
            float y = 0.0f;
            for (int k = 0; k < tapsPerPhase; ++k)
                y += phase[(size_t) k] * h[(size_t) ((writePosition - k + tapsPerPhase) % tapsPerPhase)];
            peak = std::max (peak, std::abs (y));
        }
    }

    writePosition = (writePosition + 1) % tapsPerPhase;
}

} // namespace phasemap
