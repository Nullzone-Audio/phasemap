#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace phasemap
{

/** Parameter IDs. Never rename these: hosts store them in their projects. */
namespace ParamID
{
    inline constexpr auto mode      = "mode";   // 0 = Energy, 1 = Phase
    inline constexpr auto slow      = "slow";   // response: false = Fast (0.3 s), true = Slow (1.5 s)
    inline constexpr auto monoCheck = "mono";   // sums L and R on the output
    inline constexpr auto holdMs    = "hold";   // hold time of the minimum-correlation marker
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::mode, 1 }, "Mode",
                                                              StringArray { "Energy", "Phase" }, 0));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::slow, 1 }, "Slow Response", false));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::monoCheck, 1 }, "Mono Check", false));
    params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::holdMs, 1 }, "Hold",
                                                             NormalisableRange<float> (0.0f, 10000.0f, 10.0f, 0.5f), 3000.0f,
                                                             AudioParameterFloatAttributes().withLabel ("ms")));
    return { params.begin(), params.end() };
}

} // namespace phasemap
