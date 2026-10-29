#pragma once

// A deliberately tiny test helper: no framework to learn, every check prints one line.

#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

namespace test
{
    inline int& failures() { static int count = 0; return count; }

    inline void near (const char* what, double actual, double expected, double tolerance)
    {
        const bool ok = std::abs (actual - expected) <= tolerance && ! std::isnan (actual);
        if (! ok) ++failures();
        std::printf ("%s  %-52s %9.3f   (expected %.3f +- %.3f)\n", ok ? "ok  " : "FAIL", what, actual, expected, tolerance);
    }

    inline void isTrue (const char* what, bool condition)
    {
        if (! condition) ++failures();
        std::printf ("%s  %s\n", condition ? "ok  " : "FAIL", what);
    }

    inline int finish()
    {
        std::printf (failures() == 0 ? "\nAll checks passed.\n" : "\n%d check(s) FAILED.\n", failures());
        return failures() == 0 ? 0 : 1;
    }

    /** Stereo test signal generator: fills L and R sample by sample. */
    using Generator = std::function<void (int sampleIndex, float& left, float& right)>;

    inline void render (int numSamples, const Generator& generate, std::vector<float>& left, std::vector<float>& right)
    {
        left.resize ((size_t) numSamples);
        right.resize ((size_t) numSamples);
        for (int i = 0; i < numSamples; ++i)
            generate (i, left[(size_t) i], right[(size_t) i]);
    }

    constexpr double twoPi = 6.283185307179586;
}
