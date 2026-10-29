#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace phasemap
{

/** Lock-free single-producer / single-consumer ring buffer for stereo samples.

    The audio thread pushes, the UI thread pulls. If the UI is closed or too slow,
    new samples are simply dropped - the audio thread never waits.
*/
class StereoFifo
{
public:
    static constexpr int capacity = 1 << 15;

    void push (const float* left, const float* right, int numSamples) noexcept
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite (numSamples, start1, size1, start2, size2);
        std::copy_n (left, size1, bufferL.begin() + start1);
        std::copy_n (right, size1, bufferR.begin() + start1);
        std::copy_n (left + size1, size2, bufferL.begin() + start2);
        std::copy_n (right + size1, size2, bufferR.begin() + start2);
        fifo.finishedWrite (size1 + size2);
    }

    /** Copies up to maxSamples into the destination arrays and returns how many were copied. */
    int pull (float* left, float* right, int maxSamples) noexcept
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead (maxSamples, start1, size1, start2, size2);
        std::copy_n (bufferL.begin() + start1, size1, left);
        std::copy_n (bufferR.begin() + start1, size1, right);
        std::copy_n (bufferL.begin() + start2, size2, left + size1);
        std::copy_n (bufferR.begin() + start2, size2, right + size1);
        fifo.finishedRead (size1 + size2);
        return size1 + size2;
    }

private:
    juce::AbstractFifo fifo { capacity };
    std::array<float, capacity> bufferL {}, bufferR {};
};

} // namespace phasemap
