#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace phasemap
{

/** Stereo analysis shown in the main display.

    Per frequency band:  level of L and R, and the phase correlation (-1 ... +1).
    Overall:             correlation and L/R balance.
    Goniometer:          the most recent sample pairs.

    Runs on the UI thread (fed from StereoFifo), so it may allocate in its setters.

    How the band correlation works:
        For each FFT frame we sum, per band, the power of L, the power of R and the real part of
        the cross spectrum L * conj(R). These three sums are averaged over time (Fast 0.3 s /
        Slow 1.5 s) and the correlation is  cross / sqrt(powerL * powerR).
        +1 = identical phase, 0 = unrelated, -1 = cancels when summed to mono.
*/
class SpectralCorrelation
{
public:
    static constexpr int fftOrder = 13;
    static constexpr int fftSize = 1 << fftOrder;   // 8192 samples, ~5.9 Hz bins at 48 kHz
    static constexpr int hopSize = fftSize / 4;
    static constexpr int scopeSize = 1024;          // samples kept for the goniometer
    static constexpr double lowestHz = 20.0, highestHz = 20000.0;

    struct Band
    {
        double lowHz = 0, highHz = 0, centreHz = 0;
        float levelLeftDb = -100.0f, levelRightDb = -100.0f;
        float correlation = 0.0f;
        double powerLeft = 0, powerRight = 0, crossPower = 0;   // time-averaged sums
    };

    SpectralCorrelation();

    void setSampleRate (double newSampleRate);
    void setSlowResponse (bool shouldBeSlow)        { slowResponse = shouldBeSlow; }
    void setBandsPerOctave (int bandsPerOctave);    // 3, 6 or 12

    /** Feeds new samples. Analysis frames are computed every hopSize samples. */
    void process (const float* left, const float* right, int numSamples);

    const std::vector<Band>& getBands() const       { return bands; }
    float getCorrelation() const                    { return correlation; }
    float getBalanceDb() const                      { return balanceDb; }   // > 0 = right louder
    bool hasSignal() const                          { return signalPresent; }

    const std::vector<float>& getScopeLeft() const  { return scopeLeft; }
    const std::vector<float>& getScopeRight() const { return scopeRight; }
    int getScopeWritePosition() const               { return scopePosition; }

private:
    void analyseFrame();
    double averagingSeconds() const                 { return slowResponse ? 1.5 : 0.3; }

    double sampleRate = 48000.0;
    bool slowResponse = false, signalPresent = false;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> ringLeft, ringRight, frameLeft, frameRight, scopeLeft, scopeRight;
    int ringPosition = 0, samplesSinceHop = 0, scopePosition = 0;

    std::vector<Band> bands;

    // running power sums for the overall correlation and balance
    double powerLeft = 0, powerRight = 0, crossPower = 0;
    float correlation = 0.0f, balanceDb = 0.0f;
};

} // namespace phasemap
