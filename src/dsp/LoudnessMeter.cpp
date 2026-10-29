#include "LoudnessMeter.h"

#include <algorithm>
#include <cmath>

namespace phasemap
{

//==============================================================================
int LoudnessMeter::Histogram::binFor (double lufs) noexcept
{
    return std::clamp ((int) std::floor ((lufs + 70.0) * 10.0), 0, numBins - 1);
}

void LoudnessMeter::Histogram::add (double meanSquare, double lufs) noexcept
{
    const int bin = binFor (lufs);
    energy[(size_t) bin] += meanSquare;
    ++count[(size_t) bin];
}

double LoudnessMeter::toLufs (double meanSquare) noexcept
{
    return meanSquare > 0.0 ? -0.691 + 10.0 * std::log10 (meanSquare) : (double) silence;
}

//==============================================================================
void LoudnessMeter::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    designKWeighting();
    blockLength = std::max (1, (int) std::lround (sampleRate / 10.0));   // 100 ms
    truePeakDetector.prepare (sampleRate);
    reset();
}

// K-weighting coefficients for any sample rate, derived from the analogue prototype
// (same derivation as libebur128). At 48 kHz they match the table in BS.1770.
void LoudnessMeter::designKWeighting()
{
    const double pi = 3.14159265358979323846;
    Biquad shelf, hp;

    {   // stage 1: high shelf, +4 dB above ~1.7 kHz (head acoustics)
        const double f0 = 1681.974450955533, gainDb = 3.999843853973347, q = 0.7071752369554196;
        const double k = std::tan (pi * f0 / sampleRate);
        const double vh = std::pow (10.0, gainDb / 20.0), vb = std::pow (vh, 0.4996667741545416);
        const double a0 = 1.0 + k / q + k * k;
        shelf.b0 = (vh + vb * k / q + k * k) / a0;
        shelf.b1 = 2.0 * (k * k - vh) / a0;
        shelf.b2 = (vh - vb * k / q + k * k) / a0;
        shelf.a1 = 2.0 * (k * k - 1.0) / a0;
        shelf.a2 = (1.0 - k / q + k * k) / a0;
    }
    {   // stage 2: high pass at ~38 Hz (RLB weighting)
        const double f0 = 38.13547087602444, q = 0.5003270373238773;
        const double k = std::tan (pi * f0 / sampleRate);
        const double a0 = 1.0 + k / q + k * k;
        hp.b0 = 1.0; hp.b1 = -2.0; hp.b2 = 1.0;
        hp.a1 = 2.0 * (k * k - 1.0) / a0;
        hp.a2 = (1.0 - k / q + k * k) / a0;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        shelving[ch] = shelf;
        highPass[ch] = hp;
    }
}

void LoudnessMeter::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        shelving[ch].clear();
        highPass[ch].clear();
    }
    blockEnergy.fill (0.0);
    blocksDone = blockWritePos = samplesInBlock = 0;
    energySum = 0.0;
    gatingBlocks.clear();
    shortTermValues.clear();
    truePeakDetector.reset();

    momentary = silence;
    shortTerm = silence;
    integrated = silence;
    loudnessRange = 0.0f;
    truePeak = silence;
}

void LoudnessMeter::process (const float* left, const float* right, int numSamples) noexcept
{
    if (resetRequested.exchange (false))
        reset();

    for (int i = 0; i < numSamples; ++i)
    {
        const double l = highPass[0].process (shelving[0].process (left[i]));
        energySum += l * l;   // channel weight 1.0 for L, R and mono

        if (right != nullptr)
        {
            const double r = highPass[1].process (shelving[1].process (right[i]));
            energySum += r * r;
            truePeakDetector.process (left[i], right[i]);
        }
        else
        {
            truePeakDetector.process (left[i], left[i]);
        }

        if (++samplesInBlock >= blockLength)
            finishBlock();
    }

    const float peak = truePeakDetector.getPeakGain();
    truePeak = peak > 0.0f ? (float) (20.0 * std::log10 ((double) peak)) : silence;
}

double LoudnessMeter::meanOfLastBlocks (int numBlocks) const noexcept
{
    double sum = 0.0;
    for (int i = 0; i < numBlocks; ++i)
        sum += blockEnergy[(size_t) ((blockWritePos - 1 - i + blocksKept) % blocksKept)];
    return sum / (numBlocks * (double) blockLength);
}

void LoudnessMeter::finishBlock() noexcept
{
    blockEnergy[(size_t) blockWritePos] = energySum;
    blockWritePos = (blockWritePos + 1) % blocksKept;
    blocksDone = std::min (blocksDone + 1, 1 << 30);
    energySum = 0.0;
    samplesInBlock = 0;

    // Momentary = 400 ms window. Every new one is also a gating block (75 % overlap).
    if (blocksDone >= 4)
    {
        const double meanSquare = meanOfLastBlocks (4), lufs = toLufs (meanSquare);
        momentary = (float) lufs;
        if (lufs > -70.0)   // absolute gate
        {
            gatingBlocks.add (meanSquare, lufs);
            updateIntegrated();
        }
    }

    // Short-term = 3 s window. While it is still filling up, show the time so far.
    if (blocksDone >= blocksKept)
    {
        const double meanSquare = meanOfLastBlocks (blocksKept), lufs = toLufs (meanSquare);
        shortTerm = (float) lufs;
        if (lufs > -70.0)
        {
            shortTermValues.add (meanSquare, lufs);
            updateLoudnessRange();
        }
    }
    else if (blocksDone >= 4)
    {
        shortTerm = (float) toLufs (meanOfLastBlocks (blocksDone));
    }
}

void LoudnessMeter::updateIntegrated() noexcept
{
    // relative gate: 10 LU below the mean of all blocks that passed the absolute gate
    double energy = 0.0;
    long count = 0;
    for (int b = 0; b < Histogram::numBins; ++b)
    {
        energy += gatingBlocks.energy[(size_t) b];
        count += gatingBlocks.count[(size_t) b];
    }
    if (count == 0)
        return;

    const double gate = toLufs (energy / (double) count) - 10.0;
    double gatedEnergy = 0.0;
    long gatedCount = 0;
    for (int b = Histogram::binFor (gate); b < Histogram::numBins; ++b)
    {
        if (Histogram::lufsOf (b) < gate)
            continue;
        gatedEnergy += gatingBlocks.energy[(size_t) b];
        gatedCount += gatingBlocks.count[(size_t) b];
    }

    integrated = gatedCount > 0 ? (float) toLufs (gatedEnergy / (double) gatedCount) : silence;
}

void LoudnessMeter::updateLoudnessRange() noexcept
{
    // EBU Tech 3342: relative gate 20 LU below the mean, then the 10th to 95th percentile
    double energy = 0.0;
    long count = 0;
    for (int b = 0; b < Histogram::numBins; ++b)
    {
        energy += shortTermValues.energy[(size_t) b];
        count += shortTermValues.count[(size_t) b];
    }
    if (count == 0)
        return;

    const int firstBin = Histogram::binFor (toLufs (energy / (double) count) - 20.0);
    long total = 0;
    for (int b = firstBin; b < Histogram::numBins; ++b)
        total += shortTermValues.count[(size_t) b];

    if (total < 2)
    {
        loudnessRange = 0.0f;
        return;
    }

    auto percentile = [&] (double p)
    {
        const long wanted = (long) std::floor (p * (double) (total - 1));
        long seen = 0;
        for (int b = firstBin; b < Histogram::numBins; ++b)
            if ((seen += shortTermValues.count[(size_t) b]) > wanted)
                return Histogram::lufsOf (b);
        return Histogram::lufsOf (Histogram::numBins - 1);
    };

    loudnessRange = (float) std::max (0.0, percentile (0.95) - percentile (0.10));
}

} // namespace phasemap
