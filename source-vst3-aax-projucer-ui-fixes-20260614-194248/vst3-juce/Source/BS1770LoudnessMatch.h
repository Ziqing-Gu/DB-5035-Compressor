#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace db5035
{
class BS1770LoudnessMatch
{
public:
    struct Result
    {
        float targetGainDb = 0.0f;
        double dryLufs = 0.0;
        double wetLufs = 0.0;
        bool valid = false;
    };

    void prepare (double newSampleRate)
    {
        sampleRate = std::max (1.0, newSampleRate);
        blockSamples = std::max (1, static_cast<int> (std::llround (sampleRate * 0.400)));
        hopSamples = std::max (1, static_cast<int> (std::llround (sampleRate * 0.100)));

        squareHistory.assign (static_cast<size_t> (blockSamples), {});

        // 400 ms blocks with a 100 ms hop produce ten completed blocks per
        // second. Four hours of reserved storage avoids normal audio-thread
        // allocation while keeping the Integrated LUFS result exact.
        blocks.clear();
        blocks.reserve (4u * 60u * 60u * 10u);

        for (auto& filter : filters)
            filter.prepare (sampleRate);

        reset();
    }

    void reset() noexcept
    {
        for (auto& filter : filters)
            filter.reset();

        for (auto& frame : squareHistory)
            frame.fill (0.0f);

        runningSums.fill (0.0);
        historyIndex = 0;
        samplesUntilBlock = blockSamples;
        blocks.clear();
        latest = {};
    }

    // The Wet signal must be tapped after the compressor's dynamics, colour,
    // transient handling and fixed topology trims, but before the user Gain
    // and Blend controls. Dry and Wet must share the same processing-rate time
    // domain (native or oversampled).
    void processSample (float dryL, float dryR, float wetL, float wetR) noexcept
    {
        const std::array<float, streamCount> input { dryL, dryR, wetL, wetR };
        auto& oldFrame = squareHistory[static_cast<size_t> (historyIndex)];

        for (size_t index = 0; index < streamCount; ++index)
        {
            const auto weighted = filters[index].process (input[index]);
            const auto square = weighted * weighted;
            runningSums[index] += static_cast<double> (square) - static_cast<double> (oldFrame[index]);
            oldFrame[index] = square;
        }

        if (++historyIndex >= blockSamples)
            historyIndex = 0;

        if (--samplesUntilBlock > 0)
            return;

        const auto inverseBlockSize = 1.0 / static_cast<double> (blockSamples);
        const auto mean = [&] (size_t index) noexcept
        {
            return static_cast<float> (std::max (0.0, runningSums[index] * inverseBlockSize));
        };

        // BS.1770 assigns both front channels a weight of 1.0. A mono bus is
        // represented by a zero right channel and therefore uses the same path.
        blocks.push_back ({ mean (dryLIndex) + mean (dryRIndex),
                            mean (wetLIndex) + mean (wetRIndex) });
        samplesUntilBlock = hopSamples;
        recompute();
    }

    const Result& getLatestResult() const noexcept { return latest; }
    size_t getBlockCount() const noexcept { return blocks.size(); }

    // Public reference helper for the standalone conformance test.
    static double integratedLoudnessFromEnergies (const std::vector<float>& energies) noexcept
    {
        if (energies.empty())
            return negativeInfinity();

        constexpr double absoluteGateLufs = -70.0;
        const auto absoluteGateEnergy = std::pow (10.0, (absoluteGateLufs + 0.691) / 10.0);

        double absoluteSum = 0.0;
        uint64_t absoluteCount = 0;
        for (const auto energyValue : energies)
        {
            const auto energy = static_cast<double> (energyValue);
            if (energy > absoluteGateEnergy)
            {
                absoluteSum += energy;
                ++absoluteCount;
            }
        }

        if (absoluteCount == 0)
            return negativeInfinity();

        const auto absoluteMean = absoluteSum / static_cast<double> (absoluteCount);
        const auto finalGateEnergy = std::max (absoluteGateEnergy, absoluteMean * 0.1);

        double finalSum = 0.0;
        uint64_t finalCount = 0;
        for (const auto energyValue : energies)
        {
            const auto energy = static_cast<double> (energyValue);
            if (energy > finalGateEnergy)
            {
                finalSum += energy;
                ++finalCount;
            }
        }

        if (finalCount == 0)
            return negativeInfinity();

        return -0.691 + 10.0 * std::log10 (finalSum / static_cast<double> (finalCount));
    }

private:
    class Biquad
    {
    public:
        void set (double newB0, double newB1, double newB2,
                  double newA1, double newA2) noexcept
        {
            b0 = newB0;
            b1 = newB1;
            b2 = newB2;
            a1 = newA1;
            a2 = newA2;
            reset();
        }

        void reset() noexcept
        {
            x1 = x2 = y1 = y2 = 0.0;
        }

        float process (float input) noexcept
        {
            const auto x0 = static_cast<double> (input);
            const auto y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = x0;
            y2 = y1;
            y1 = y0;
            return static_cast<float> (y0);
        }

    private:
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    };

    class KWeightingFilter
    {
    public:
        void prepare (double filterSampleRate)
        {
            // De Man / BS.1770-compatible coefficient derivation. At 48 kHz
            // these reproduce the published BS.1770 K-weighting response.
            constexpr double shelfGainDb = 3.99984385397;
            constexpr double shelfQ = 0.7071752369554193;
            constexpr double shelfHz = 1681.9744509555319;
            constexpr double shelfExponent = 0.499666774155;

            const auto kShelf = std::tan (pi * shelfHz / filterSampleRate);
            const auto vh = std::pow (10.0, shelfGainDb / 20.0);
            const auto vb = std::pow (vh, shelfExponent);
            const auto a0Shelf = 1.0 + kShelf / shelfQ + kShelf * kShelf;

            shelf.set ((vh + vb * kShelf / shelfQ + kShelf * kShelf) / a0Shelf,
                       2.0 * (kShelf * kShelf - vh) / a0Shelf,
                       (vh - vb * kShelf / shelfQ + kShelf * kShelf) / a0Shelf,
                       2.0 * (kShelf * kShelf - 1.0) / a0Shelf,
                       (1.0 - kShelf / shelfQ + kShelf * kShelf) / a0Shelf);

            constexpr double highPassQ = 0.5003270373253953;
            constexpr double highPassHz = 38.13547087613982;
            const auto kHighPass = std::tan (pi * highPassHz / filterSampleRate);
            const auto a0HighPass = 1.0 + kHighPass / highPassQ + kHighPass * kHighPass;

            highPass.set (1.0,
                          -2.0,
                          1.0,
                          2.0 * (kHighPass * kHighPass - 1.0) / a0HighPass,
                          (1.0 - kHighPass / highPassQ + kHighPass * kHighPass) / a0HighPass);
        }

        void reset() noexcept
        {
            shelf.reset();
            highPass.reset();
        }

        float process (float input) noexcept
        {
            return highPass.process (shelf.process (input));
        }

    private:
        static constexpr double pi = 3.141592653589793238462643383279502884;
        Biquad shelf;
        Biquad highPass;
    };

    struct BlockEnergies
    {
        float dry = 0.0f;
        float wet = 0.0f;
    };

    template <typename Member>
    double integratedFor (Member member) const noexcept
    {
        if (blocks.empty())
            return negativeInfinity();

        constexpr double absoluteGateLufs = -70.0;
        const auto absoluteGateEnergy = std::pow (10.0, (absoluteGateLufs + 0.691) / 10.0);

        double absoluteSum = 0.0;
        uint64_t absoluteCount = 0;
        for (const auto& block : blocks)
        {
            const auto energy = static_cast<double> (block.*member);
            if (energy > absoluteGateEnergy)
            {
                absoluteSum += energy;
                ++absoluteCount;
            }
        }

        if (absoluteCount == 0)
            return negativeInfinity();

        const auto absoluteMean = absoluteSum / static_cast<double> (absoluteCount);
        const auto finalGateEnergy = std::max (absoluteGateEnergy, absoluteMean * 0.1);

        double finalSum = 0.0;
        uint64_t finalCount = 0;
        for (const auto& block : blocks)
        {
            const auto energy = static_cast<double> (block.*member);
            if (energy > finalGateEnergy)
            {
                finalSum += energy;
                ++finalCount;
            }
        }

        if (finalCount == 0)
            return negativeInfinity();

        return -0.691 + 10.0 * std::log10 (finalSum / static_cast<double> (finalCount));
    }

    void recompute() noexcept
    {
        const auto dry = integratedFor (&BlockEnergies::dry);
        const auto wet = integratedFor (&BlockEnergies::wet);

        latest = {};
        latest.dryLufs = dry;
        latest.wetLufs = wet;
        latest.valid = isFiniteLoudness (dry) && isFiniteLoudness (wet);

        if (latest.valid)
            latest.targetGainDb = std::clamp (static_cast<float> (dry - wet), -36.0f, 36.0f);
    }

    static constexpr double negativeInfinity() noexcept { return -1.0e300; }

    static bool isFiniteLoudness (double value) noexcept
    {
        return value > -1.0e200 && std::isfinite (value);
    }

    static constexpr size_t dryLIndex = 0;
    static constexpr size_t dryRIndex = 1;
    static constexpr size_t wetLIndex = 2;
    static constexpr size_t wetRIndex = 3;
    static constexpr size_t streamCount = 4;

    double sampleRate = 44100.0;
    int blockSamples = 17640;
    int hopSamples = 4410;
    int historyIndex = 0;
    int samplesUntilBlock = 17640;

    std::array<KWeightingFilter, streamCount> filters;
    std::vector<std::array<float, streamCount>> squareHistory;
    std::array<double, streamCount> runningSums {};
    std::vector<BlockEnergies> blocks;
    Result latest;
};
}
