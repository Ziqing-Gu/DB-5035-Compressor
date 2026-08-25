// Standalone (no JUCE required) sanity test for DB-5035 Match Gain.
// Example:
//   c++ -std=c++17 -O2 tests/bs1770_match_selftest.cpp -I. -o bs1770_test

#include "Source/BS1770LoudnessMatch.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr double pi = 3.141592653589793238462643383279502884;

bool near (double actual, double expected, double tolerance)
{
    return std::abs (actual - expected) <= tolerance;
}
}

int main()
{
    constexpr double sampleRate = 48000.0;

    // BS.1770 reference point: a 0 dBFS 1 kHz sine in one front channel is
    // approximately -3.01 LKFS/LUFS.
    db5035::BS1770LoudnessMatch reference;
    reference.prepare (sampleRate);
    for (int sampleIndex = 0; sampleIndex < static_cast<int> (sampleRate * 5.0); ++sampleIndex)
    {
        const auto sample = static_cast<float> (
            std::sin (2.0 * pi * 1000.0 * static_cast<double> (sampleIndex) / sampleRate));
        reference.processSample (sample, 0.0f, sample, 0.0f);
    }

    const auto referenceResult = reference.getLatestResult();
    std::cout << "1 kHz 0 dBFS mono: " << referenceResult.dryLufs << " LUFS\n";
    if (! referenceResult.valid || ! near (referenceResult.dryLufs, -3.01, 0.05))
        return 1;

    // A fixed 6 dB Dry/Wet difference must produce a +6 dB Gain target.
    db5035::BS1770LoudnessMatch match;
    match.prepare (sampleRate);
    const auto dryAmplitude = std::pow (10.0f, -20.0f / 20.0f);
    const auto wetAmplitude = std::pow (10.0f, -26.0f / 20.0f);

    for (int sampleIndex = 0; sampleIndex < static_cast<int> (sampleRate * 10.0); ++sampleIndex)
    {
        const auto carrier = static_cast<float> (
            std::sin (2.0 * pi * 1000.0 * static_cast<double> (sampleIndex) / sampleRate));
        const auto dry = dryAmplitude * carrier;
        const auto wet = wetAmplitude * carrier;
        match.processSample (dry, dry, wet, wet);
    }

    const auto matchResult = match.getLatestResult();
    std::cout << "6 dB fixed difference: " << matchResult.targetGainDb << " dB\n";
    if (! matchResult.valid || ! near (matchResult.targetGainDb, 6.0, 0.03))
        return 2;

    std::cout << "PASS\n";
    return 0;
}
