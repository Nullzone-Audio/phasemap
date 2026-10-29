#pragma once

#include "TruePeakDetector.h"

#include <array>
#include <atomic>

namespace phasemap
{

/** Loudness measurement after ITU-R BS.1770-4 / EBU R128 (EBU Tech 3341 and 3342).

    Runs on the audio thread so that Integrated keeps measuring while the editor is closed.
    Results are published as atomics; the UI only reads them.

    Signal flow:   K-weighting  ->  100 ms energy blocks  ->  Momentary (4 blocks), Short-term (30 blocks)
                                                         ->  gated Integrated and Loudness Range (histograms)
    True peak is measured in parallel on the unweighted signal.

    Validated in tests/LoudnessTests.cpp with the EBU test signals.
*/
class LoudnessMeter
{
public:
    static constexpr float silence = -200.0f;

    // results (LUFS, LU, dBTP)
    std::atomic<float> momentary { silence }, shortTerm { silence }, integrated { silence },
                       loudnessRange { 0.0f }, truePeak { silence };

    /** Set from any thread; the measurement restarts at the beginning of the next block. */
    std::atomic<bool> resetRequested { false };

    void prepare (double sampleRate);
    void reset();

    /** Measures one block. Pass right == nullptr for a mono signal. */
    void process (const float* left, const float* right, int numSamples) noexcept;

    double getSampleRate() const { return sampleRate; }

private:
    // Second-order filter in direct form I, double precision
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        double process (double x) noexcept
        {
            const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            return y;
        }
        void clear() noexcept { x1 = x2 = y1 = y2 = 0; }
    };

    // Loudness values in 0.1 LU steps from -70 to +10 LUFS. Keeps count and exact energy per step.
    struct Histogram
    {
        static constexpr int numBins = 800;
        std::array<double, numBins> energy {};
        std::array<int, numBins> count {};

        void clear() noexcept                         { energy.fill (0.0); count.fill (0); }
        static int binFor (double lufs) noexcept;
        static double lufsOf (int bin) noexcept       { return -70.0 + (bin + 0.5) * 0.1; }
        void add (double meanSquare, double lufs) noexcept;
    };

    static double toLufs (double meanSquare) noexcept;

    void designKWeighting();
    void finishBlock() noexcept;
    double meanOfLastBlocks (int numBlocks) const noexcept;
    void updateIntegrated() noexcept;
    void updateLoudnessRange() noexcept;

    double sampleRate = 48000.0;
    Biquad shelving[2], highPass[2];   // K-weighting, stage 1 and 2, per channel

    static constexpr int blocksKept = 30;              // 30 x 100 ms = short-term window
    std::array<double, blocksKept> blockEnergy {};
    int blockLength = 4800, blocksDone = 0, blockWritePos = 0, samplesInBlock = 0;
    double energySum = 0.0;

    Histogram gatingBlocks, shortTermValues;
    TruePeakDetector truePeakDetector;
};

} // namespace phasemap
