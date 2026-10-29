#include "SpectralCorrelation.h"

namespace phasemap
{

SpectralCorrelation::SpectralCorrelation()
    : window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    ringLeft.assign (fftSize, 0.0f);
    ringRight.assign (fftSize, 0.0f);
    frameLeft.assign (fftSize * 2, 0.0f);    // the real-only FFT needs 2 * size
    frameRight.assign (fftSize * 2, 0.0f);
    scopeLeft.assign (scopeSize, 0.0f);
    scopeRight.assign (scopeSize, 0.0f);
    setBandsPerOctave (6);
}

void SpectralCorrelation::setSampleRate (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
}

void SpectralCorrelation::setBandsPerOctave (int bandsPerOctave)
{
    bands.clear();
    const int count = (int) std::ceil (std::log2 (highestHz / lowestHz) * bandsPerOctave);

    for (int i = 0; i < count; ++i)
    {
        Band band;
        band.lowHz    = lowestHz * std::pow (2.0, (double) i / bandsPerOctave);
        band.highHz   = lowestHz * std::pow (2.0, (double) (i + 1) / bandsPerOctave);
        band.centreHz = std::sqrt (band.lowHz * band.highHz);
        bands.push_back (band);
    }
}

void SpectralCorrelation::process (const float* left, const float* right, int numSamples)
{
    const double smoothing = std::exp (-1.0 / (averagingSeconds() * sampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        const double l = left[i], r = right[i];
        powerLeft  = smoothing * powerLeft  + (1.0 - smoothing) * l * l;
        powerRight = smoothing * powerRight + (1.0 - smoothing) * r * r;
        crossPower = smoothing * crossPower + (1.0 - smoothing) * l * r;

        ringLeft[(size_t) ringPosition] = left[i];
        ringRight[(size_t) ringPosition] = right[i];
        ringPosition = (ringPosition + 1) % fftSize;

        scopeLeft[(size_t) scopePosition] = left[i];
        scopeRight[(size_t) scopePosition] = right[i];
        scopePosition = (scopePosition + 1) % scopeSize;

        if (++samplesSinceHop >= hopSize)
        {
            samplesSinceHop = 0;
            analyseFrame();
        }
    }

    constexpr double silence = 1.0e-8;   // about -80 dBFS
    signalPresent = powerLeft > silence || powerRight > silence;

    correlation = (powerLeft > silence && powerRight > silence)
                      ? (float) juce::jlimit (-1.0, 1.0, crossPower / std::sqrt (powerLeft * powerRight))
                      : 0.0f;

    balanceDb = signalPresent
                    ? (float) juce::jlimit (-24.0, 24.0, 10.0 * std::log10 ((powerRight + 1e-12) / (powerLeft + 1e-12)))
                    : 0.0f;
}

void SpectralCorrelation::analyseFrame()
{
    // unroll the ring buffer into the FFT frame, oldest sample first
    for (int i = 0; i < fftSize; ++i)
    {
        frameLeft[(size_t) i]  = ringLeft[(size_t) ((ringPosition + i) % fftSize)];
        frameRight[(size_t) i] = ringRight[(size_t) ((ringPosition + i) % fftSize)];
    }
    std::fill (frameLeft.begin() + fftSize, frameLeft.end(), 0.0f);
    std::fill (frameRight.begin() + fftSize, frameRight.end(), 0.0f);

    window.multiplyWithWindowingTable (frameLeft.data(), (size_t) fftSize);
    window.multiplyWithWindowingTable (frameRight.data(), (size_t) fftSize);
    fft.performRealOnlyForwardTransform (frameLeft.data(), true);    // result: interleaved re/im
    fft.performRealOnlyForwardTransform (frameRight.data(), true);

    const double smoothing = std::exp (-(double) hopSize / (averagingSeconds() * sampleRate));
    const double binHz = sampleRate / fftSize;
    const double amplitudeScale = 4.0 / fftSize;            // Hann window: sine of amplitude A -> |X| = A * N / 4
    const float levelFallDb = slowResponse ? 0.6f : 1.5f;  // per analysis frame

    for (auto& band : bands)
    {
        int firstBin = (int) std::ceil (band.lowHz / binHz);
        int lastBin  = (int) std::floor (band.highHz / binHz);
        if (lastBin < firstBin)                             // very narrow low bands: use the nearest bin
            firstBin = lastBin = (int) std::lround (band.centreHz / binHz);
        firstBin = juce::jlimit (1, fftSize / 2 - 1, firstBin);
        lastBin  = juce::jlimit (firstBin, fftSize / 2 - 1, lastBin);

        double pL = 0, pR = 0, cross = 0, peakL = 0, peakR = 0;
        for (int k = firstBin; k <= lastBin; ++k)
        {
            const double lRe = frameLeft[(size_t) (2 * k)],  lIm = frameLeft[(size_t) (2 * k + 1)];
            const double rRe = frameRight[(size_t) (2 * k)], rIm = frameRight[(size_t) (2 * k + 1)];
            pL += lRe * lRe + lIm * lIm;
            pR += rRe * rRe + rIm * rIm;
            cross += lRe * rRe + lIm * rIm;                 // Re (L * conj (R))
            peakL = std::max (peakL, std::sqrt (lRe * lRe + lIm * lIm));
            peakR = std::max (peakR, std::sqrt (rRe * rRe + rIm * rIm));
        }

        // average the sums first, then derive the correlation (more stable than averaging the ratio)
        band.powerLeft  = smoothing * band.powerLeft  + (1.0 - smoothing) * pL;
        band.powerRight = smoothing * band.powerRight + (1.0 - smoothing) * pR;
        band.crossPower = smoothing * band.crossPower + (1.0 - smoothing) * cross;
        const double denominator = std::sqrt (band.powerLeft * band.powerRight);
        band.correlation = denominator > 1.0e-14 ? (float) juce::jlimit (-1.0, 1.0, band.crossPower / denominator) : 0.0f;

        // level: strongest bin in the band, rises instantly, falls slowly
        const float dbL = juce::Decibels::gainToDecibels ((float) (peakL * amplitudeScale), -100.0f);
        const float dbR = juce::Decibels::gainToDecibels ((float) (peakR * amplitudeScale), -100.0f);
        band.levelLeftDb  = dbL > band.levelLeftDb  ? dbL : juce::jmax (dbL, band.levelLeftDb  - levelFallDb);
        band.levelRightDb = dbR > band.levelRightDb ? dbR : juce::jmax (dbR, band.levelRightDb - levelFallDb);
    }
}

} // namespace phasemap
