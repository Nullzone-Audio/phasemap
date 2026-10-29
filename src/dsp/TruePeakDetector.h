#pragma once

#include <array>
#include <vector>

namespace phasemap
{

/** Inter-sample ("true") peak detection after ITU-R BS.1770-4, Annex 2.

    The signal is oversampled with a windowed-sinc interpolator (4x up to 48 kHz,
    2x up to 96 kHz, none above) and the largest absolute value is kept.
    Allocates only in prepare().
*/
class TruePeakDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    /** Processes one sample per channel (right may equal left for mono). */
    void process (float left, float right) noexcept;

    float getPeakGain() const noexcept       { return peak; }
    int getOversamplingFactor() const        { return factor; }

private:
    static constexpr int filterLength = 96;

    int factor = 4, tapsPerPhase = filterLength / 4;
    std::vector<std::vector<float>> phases;   // polyphase interpolation filters
    std::array<std::vector<float>, 2> history;
    int writePosition = 0;
    float peak = 0.0f;
};

} // namespace phasemap
