#pragma once

#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

struct DiodeBridgeCompressorParameters
{
    bool compIn = true;
    float thresholdDbu = 0.0f;
    int ratioIndex = 2;
    bool externalSidechain = false;
    float sidechainHPFHz = 20.0f;
    float makeupGainDb = 0.0f;
    int timingIndex = 0;
    bool linkStereo = true;
    bool fastMode = false;
    float blendPercent = 100.0f;
};

struct DiodeBridgeCompressorMeters
{
    float inputDb = -80.0f;
    float outputDb = -80.0f;
    float gainReductionDb = 0.0f;
};

class DiodeBridgeCompressor
{
public:
    void prepare (double newSampleRate, int maximumChannels)
    {
        sampleRate = juce::jmax (1.0, newSampleRate);
        ensureChannelStorage (maximumChannels);
        updateOutputBandwidthCoefficient();
        reset();
    }

    void reset()
    {
        heldGainReductionDb = 0.0f;
        inputPeak = 0.0f;
        meterGainReductionDb = 0.0f;
        outputPeak = 0.0f;
        linkedEnvelope = 0.0f;
        linkedSlowPreviewEnvelope = 0.0f;
        linkedTransientPass = 0.0f;
        linkedPeakRestore = 0.0f;
        linkedPeakRestoreLockoutSamples = 0;
        linkedReleaseMemoryDb = 0.0f;
        linkedReleaseFastLevel = 0.0f;
        linkedReleaseSlowLevel = 0.0f;
        linkedReleaseEntryArmed = 1;
        linkedAttackDipFastLevel = 0.0f;
        linkedAttackDipSlowLevel = 0.0f;
        linkedAttackDipSample = -1;
        linkedAttackDipArmed = 1;
        std::fill (channelEnvelopes.begin(), channelEnvelopes.end(), 0.0f);
        std::fill (channelSlowPreviewEnvelopes.begin(), channelSlowPreviewEnvelopes.end(), 0.0f);
        std::fill (channelTransientPass.begin(), channelTransientPass.end(), 0.0f);
        std::fill (channelPeakRestore.begin(), channelPeakRestore.end(), 0.0f);
        std::fill (channelPeakRestoreLockoutSamples.begin(), channelPeakRestoreLockoutSamples.end(), 0);
        std::fill (channelReleaseMemoryDb.begin(), channelReleaseMemoryDb.end(), 0.0f);
        std::fill (channelReleaseFastLevel.begin(), channelReleaseFastLevel.end(), 0.0f);
        std::fill (channelReleaseSlowLevel.begin(), channelReleaseSlowLevel.end(), 0.0f);
        std::fill (channelReleaseEntryArmed.begin(), channelReleaseEntryArmed.end(), 1);
        std::fill (channelAttackDipFastLevel.begin(), channelAttackDipFastLevel.end(), 0.0f);
        std::fill (channelAttackDipSlowLevel.begin(), channelAttackDipSlowLevel.end(), 0.0f);
        std::fill (channelAttackDipSample.begin(), channelAttackDipSample.end(), -1);
        std::fill (channelAttackDipArmed.begin(), channelAttackDipArmed.end(), 1);
        std::fill (sidechainState.begin(), sidechainState.end(), 0.0f);
        std::fill (lastSidechain.begin(), lastSidechain.end(), 0.0f);
        std::fill (outputBandwidthState.begin(), outputBandwidthState.end(), 0.0f);
        std::fill (matchOutputBandwidthState.begin(), matchOutputBandwidthState.end(), 0.0f);
    }

    void resetMatchOutputTap() noexcept
    {
        std::fill (matchOutputBandwidthState.begin(), matchOutputBandwidthState.end(), 0.0f);
    }

    void setParameters (const DiodeBridgeCompressorParameters& newParameters)
    {
        parameters = newParameters;
    }

    void process (juce::AudioBuffer<float>& buffer,
                  const juce::AudioBuffer<float>* externalSidechain = nullptr,
                  juce::AudioBuffer<float>* wetBeforeMakeup = nullptr)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        if (numChannels == 0 || numSamples == 0)
            return;

        ensureChannelStorage (juce::jmax (numChannels, externalSidechain != nullptr ? externalSidechain->getNumChannels() : numChannels));
        meterInput (buffer);

        if (wetBeforeMakeup != nullptr
            && (wetBeforeMakeup->getNumChannels() < numChannels
                || wetBeforeMakeup->getNumSamples() < numSamples))
        {
            jassertfalse;
            wetBeforeMakeup = nullptr;
        }

        if (! parameters.compIn)
        {
            meters.inputDb = linearToDb (inputPeak);
            meters.outputDb = meters.inputDb;
            meters.gainReductionDb = 0.0f;
            inputPeak = 0.0f;
            meterGainReductionDb = 0.0f;
            outputPeak = 0.0f;
            heldGainReductionDb = 0.0f;
            linkedEnvelope = 0.0f;
            linkedSlowPreviewEnvelope = 0.0f;
            linkedTransientPass = 0.0f;
            linkedPeakRestore = 0.0f;
            linkedPeakRestoreLockoutSamples = 0;
            linkedReleaseMemoryDb = 0.0f;
            linkedReleaseFastLevel = 0.0f;
            linkedReleaseSlowLevel = 0.0f;
            linkedReleaseEntryArmed = 1;
            linkedAttackDipFastLevel = 0.0f;
            linkedAttackDipSlowLevel = 0.0f;
            linkedAttackDipSample = -1;
            linkedAttackDipArmed = 1;
            std::fill (channelPeakRestore.begin(), channelPeakRestore.end(), 0.0f);
            std::fill (channelEnvelopes.begin(), channelEnvelopes.end(), 0.0f);
            std::fill (channelSlowPreviewEnvelopes.begin(), channelSlowPreviewEnvelopes.end(), 0.0f);
            std::fill (channelPeakRestoreLockoutSamples.begin(), channelPeakRestoreLockoutSamples.end(), 0);
            std::fill (channelTransientPass.begin(), channelTransientPass.end(), 0.0f);
            std::fill (channelReleaseMemoryDb.begin(), channelReleaseMemoryDb.end(), 0.0f);
            std::fill (channelReleaseFastLevel.begin(), channelReleaseFastLevel.end(), 0.0f);
            std::fill (channelReleaseSlowLevel.begin(), channelReleaseSlowLevel.end(), 0.0f);
            std::fill (channelReleaseEntryArmed.begin(), channelReleaseEntryArmed.end(), 1);
            std::fill (channelAttackDipFastLevel.begin(), channelAttackDipFastLevel.end(), 0.0f);
            std::fill (channelAttackDipSlowLevel.begin(), channelAttackDipSlowLevel.end(), 0.0f);
            std::fill (channelAttackDipSample.begin(), channelAttackDipSample.end(), -1);
            std::fill (channelAttackDipArmed.begin(), channelAttackDipArmed.end(), 1);
            std::fill (sidechainState.begin(), sidechainState.end(), 0.0f);
            std::fill (lastSidechain.begin(), lastSidechain.end(), 0.0f);
            std::fill (outputBandwidthState.begin(), outputBandwidthState.end(), 0.0f);
            std::fill (matchOutputBandwidthState.begin(), matchOutputBandwidthState.end(), 0.0f);
            return;
        }

        const auto ratio = getRatio();
        const auto timing = resolveTiming (ratio);
        const auto hpfCoeff = std::exp (-2.0f * juce::MathConstants<float>::pi
                                      * juce::jmax (20.0f, parameters.sidechainHPFHz)
                                      / (float) sampleRate);
        auto timingOutputTrimDb = 0.0f;

        if (parameters.timingIndex == 3)
            timingOutputTrimDb = -1.0f;
        else if (parameters.timingIndex == 4)
            timingOutputTrimDb = parameters.fastMode ? -1.7f : -2.1f;

        const auto timingOutputGain = juce::Decibels::decibelsToGain (timingOutputTrimDb);
        const auto outputGain = juce::Decibels::decibelsToGain (parameters.makeupGainDb) * timingOutputGain;
        const auto wet = juce::jlimit (0.0f, 1.0f, parameters.blendPercent / 100.0f);
        const auto dry = 1.0f - wet;

        if (parameters.linkStereo || numChannels == 1)
            processLinked (buffer, externalSidechain, timing, hpfCoeff, outputGain, timingOutputGain,
                           wet, dry, wetBeforeMakeup);
        else
            processDualMono (buffer, externalSidechain, timing, hpfCoeff, outputGain, timingOutputGain,
                             wet, dry, wetBeforeMakeup);

        meters.inputDb = linearToDb (inputPeak);
        meters.outputDb = linearToDb (outputPeak);
        meters.gainReductionDb = meterGainReductionDb;

        inputPeak = 0.0f;
        outputPeak = 0.0f;
    }

    DiodeBridgeCompressorMeters getMeters() const
    {
        return meters;
    }

private:
    struct Timing
    {
        float minAttackMs = 1.0f;
        float maxAttackMs = 1.0f;
        float minReleaseMs = 150.0f;
        float maxReleaseMs = 150.0f;
        bool autoRelease = false;
        bool twoStageRelease = false;
    };

    struct GainResult
    {
        float gain = 1.0f;
        float gainReductionDb = 0.0f;
    };

    static float linearToDb (float value)
    {
        return juce::Decibels::gainToDecibels (juce::jmax (value, 0.000001f), -80.0f);
    }

    static float sign (float value)
    {
        if (value > 0.0f)
            return 1.0f;

        if (value < 0.0f)
            return -1.0f;

        return 0.0f;
    }

    float getRatio() const
    {
        static constexpr std::array<float, 6> ratios { 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
        return ratios[(size_t) juce::jlimit (0, (int) ratios.size() - 1, parameters.ratioIndex)];
    }

    float getRatioCompressionScale (float ratio) const
    {
        if (ratio <= 1.5f)
            return 1.88f;

        if (ratio <= 2.0f)
            return 1.28f;

        if (ratio < 3.0f)
            return 1.28f + (ratio - 2.0f) * (1.0f - 1.28f);

        if (ratio > 3.0f && ratio <= 4.0f)
            return 0.97f;

        if (ratio < 6.0f)
            return 0.97f + (ratio - 4.0f) * ((0.96f - 0.97f) / 2.0f);

        if (ratio <= 6.0f)
            return 0.96f;

        if (ratio < 8.0f)
            return 0.96f + (ratio - 6.0f) * ((0.93f - 0.96f) / 2.0f);

        if (ratio <= 8.0f)
            return 0.93f;

        return 1.0f;
    }

    float getRatioThresholdOffsetDb (float ratio) const
    {
        if (ratio <= 1.5f)
            return -0.50f;

        if (ratio <= 2.0f)
            return -0.18f;

        if (ratio < 3.0f)
            return juce::jmap (ratio, 2.0f, 3.0f, -0.18f, 0.0f);

        if (ratio <= 3.0f)
            return 0.0f;

        if (ratio < 4.0f)
            return juce::jmap (ratio, 3.0f, 4.0f, 0.0f, 0.9f);

        if (ratio <= 4.0f)
            return 0.9f;

        if (ratio < 6.0f)
            return juce::jmap (ratio, 4.0f, 6.0f, 0.9f, 1.52f);

        if (ratio <= 6.0f)
            return 1.52f;

        if (ratio < 8.0f)
            return juce::jmap (ratio, 6.0f, 8.0f, 1.52f, 1.48f);

        return 1.48f;
    }

    float getTimingThresholdOffsetDb (float ratio) const
    {
        if (parameters.timingIndex == 1)
        {
            if (ratio <= 1.5f)
                return 0.15f;

            if (ratio < 2.0f)
                return juce::jmap (ratio, 1.5f, 2.0f, 0.15f, 0.60f);

            if (ratio <= 2.0f)
                return 0.60f;

            if (ratio < 3.0f)
                return juce::jmap (ratio, 2.0f, 3.0f, 0.60f, 0.90f);

            if (ratio <= 3.0f)
                return 0.90f;

            if (ratio < 4.0f)
                return juce::jmap (ratio, 3.0f, 4.0f, -0.35f, 1.8f);

            if (ratio <= 4.0f)
                return 1.8f;

            if (ratio < 6.0f)
                return juce::jmap (ratio, 4.0f, 6.0f, 1.8f, 2.4f);

            if (ratio <= 6.0f)
                return 2.4f;

            if (ratio < 8.0f)
                return juce::jmap (ratio, 6.0f, 8.0f, 2.4f, 2.3f);

            return 2.3f;
        }

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 1.5f) < 0.01f)
            return -0.35f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 2.0f) < 0.01f)
            return -0.26f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 3.0f) < 0.01f)
            return -0.20f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 4.0f) < 0.01f)
            return 1.16f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 6.0f) < 0.01f)
            return 1.98f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 8.0f) < 0.01f)
            return 1.82f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 1.5f) < 0.01f)
            return 1.65f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 2.0f) < 0.01f)
            return 1.85f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 3.0f) < 0.01f)
            return 1.70f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 4.0f) < 0.01f)
            return 2.20f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 6.0f) < 0.01f)
            return 2.50f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 8.0f) < 0.01f)
            return 2.30f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 1.5f) < 0.01f)
            return 1.6f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 2.0f) < 0.01f)
            return 1.6f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 3.0f) < 0.01f)
            return 1.5f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 4.0f) < 0.01f)
            return 2.15f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 6.0f) < 0.01f)
            return 3.35f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 8.0f) < 0.01f)
            return 3.35f;

        if (parameters.timingIndex == 4)
            return interpolateRatioValue (ratio, { -8.35f, -5.45f, -3.35f, -0.25f, 1.30f, 2.40f });

        return 0.0f;
    }

    float getTimingFastModeThresholdOffsetDb (float ratio) const
    {
        if (parameters.timingIndex == 2 && parameters.fastMode && std::abs (ratio - 1.5f) < 0.01f)
            return -0.52f;

        if (parameters.timingIndex == 2 && parameters.fastMode && std::abs (ratio - 2.0f) < 0.01f)
            return 0.13f;

        if (parameters.timingIndex == 2 && parameters.fastMode && std::abs (ratio - 3.0f) < 0.01f)
            return 0.08f;

        if (parameters.timingIndex == 2 && parameters.fastMode && std::abs (ratio - 4.0f) < 0.01f)
            return 1.15f;

        if (parameters.timingIndex == 2 && parameters.fastMode && std::abs (ratio - 6.0f) < 0.01f)
            return 1.66f;

        if (parameters.timingIndex == 2 && parameters.fastMode && std::abs (ratio - 8.0f) < 0.01f)
            return 1.78f;

        if ((parameters.timingIndex == 1 || parameters.timingIndex == 2) && parameters.fastMode && std::abs (ratio - 1.5f) < 0.01f)
            return -0.30f;

        if ((parameters.timingIndex == 1 || parameters.timingIndex == 2) && parameters.fastMode && std::abs (ratio - 2.0f) < 0.01f)
            return -0.47f;

        if ((parameters.timingIndex == 1 || parameters.timingIndex == 2) && parameters.fastMode && std::abs (ratio - 3.0f) < 0.01f)
            return -0.33f;

        if ((parameters.timingIndex == 1 || parameters.timingIndex == 2) && parameters.fastMode && std::abs (ratio - 4.0f) < 0.01f)
            return -0.57f;

        if ((parameters.timingIndex == 1 || parameters.timingIndex == 2) && parameters.fastMode && std::abs (ratio - 6.0f) < 0.01f)
            return -0.52f;

        if ((parameters.timingIndex == 1 || parameters.timingIndex == 2) && parameters.fastMode && std::abs (ratio - 8.0f) < 0.01f)
            return -0.38f;

        if (parameters.timingIndex == 4 && parameters.fastMode)
        {
            constexpr auto slowFastModeOutputLiftDb = 0.4f;
            const auto compressionScale = interpolateRatioValue (ratio, { 0.36f, 0.42f, 0.50f, 0.60f, 0.72f, 0.80f });
            const auto ratioSlope = juce::jmax (0.001f, 1.0f - 1.0f / ratio);
            return -slowFastModeOutputLiftDb / (ratioSlope * compressionScale);
        }

        return 0.0f;
    }

    float getFastReleaseCurveLift (float) const
    {
        return 0.09f;
    }

    float getFastModeReleaseLift (float ratio) const
    {
        juce::ignoreUnused (ratio);

        switch (parameters.timingIndex)
        {
            case 0:  return 0.09f;
            case 1:  return 0.075f;
            case 2:  return 0.07f;
            case 3:  return 0.065f;
            case 4:  return 0.06f;
            case 5:  return 0.06f;
            default: return getFastReleaseCurveLift (ratio);
        }
    }

    float getTimingCompressionScale (float ratio) const
    {
        if (parameters.timingIndex == 1 && ! parameters.fastMode)
            return 1.06f;

        if (parameters.timingIndex == 4)
            return interpolateRatioValue (ratio, { 0.36f, 0.42f, 0.50f, 0.60f, 0.72f, 0.80f });

        return 1.0f;
    }

    float interpolateRatioValue (float ratio, const std::array<float, 6>& values) const
    {
        static constexpr std::array<float, 6> ratios { 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };

        if (ratio <= ratios.front())
            return values.front();

        for (size_t index = 1; index < ratios.size(); ++index)
        {
            if (ratio <= ratios[index])
                return juce::jmap (ratio, ratios[index - 1], ratios[index], values[index - 1], values[index]);
        }

        return values.back();
    }

    float getMediumFastAttackMs (float ratio) const
    {
        return interpolateRatioValue (ratio, { 31.0f, 27.0f, 20.5f, 13.8f, 7.8f, 5.6f });
    }

    float getMediumFastReleaseMs (float ratio) const
    {
        return interpolateRatioValue (ratio, { 625.0f, 675.0f, 775.0f, 795.0f, 845.0f, 895.0f });
    }

    float getMediumFastReleaseLift (float ratio) const
    {
        return interpolateRatioValue (ratio, { 0.055f, 0.055f, 0.055f, 0.04f, 0.022f, 0.014f });
    }

    bool isMediumTimingCalibratedRatio (float ratio) const
    {
        return parameters.timingIndex == 2
            && ! parameters.fastMode
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    bool isMediumTimingAttackDipRatio (float ratio) const
    {
        return parameters.timingIndex == 2
            && ! parameters.fastMode
            && (std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    float getMediumTimingAttackMs (float ratio) const
    {
        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 8.0f) < 0.01f)
            return 9.0f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 6.0f) < 0.01f)
            return 11.0f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 4.0f) < 0.01f)
            return 15.8f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 3.0f) < 0.01f)
            return 23.5f;

        if (isMediumTimingCalibratedRatio (ratio))
            return 26.0f;

        return 0.0f;
    }

    float getMediumTimingReleaseMs (float ratio) const
    {
        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 8.0f) < 0.01f)
            return 2150.0f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 6.0f) < 0.01f)
            return 2025.0f;

        if (parameters.timingIndex == 2 && ! parameters.fastMode && std::abs (ratio - 4.0f) < 0.01f)
            return 1680.0f;

        if (isMediumTimingCalibratedRatio (ratio))
            return 1250.0f;

        return 0.0f;
    }

    float getMediumTimingReleaseLift (float ratio) const
    {
        if (isMediumTimingCalibratedRatio (ratio))
            return 0.085f;

        return 0.0f;
    }

    bool isMediumSlowTimingCalibratedRatio (float ratio) const
    {
        return parameters.timingIndex == 3
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    bool isMediumSlowTimingAttackDipRatio (float ratio) const
    {
        return isMediumSlowTimingCalibratedRatio (ratio);
    }

    bool isMediumSlowTimingFastModeCalibratedRatio (float ratio) const
    {
        return parameters.timingIndex == 3 && parameters.fastMode && std::abs (ratio - 3.0f) < 0.01f;
    }

    float getMediumSlowTimingAttackMs (float ratio) const
    {
        if (parameters.timingIndex == 3 && std::abs (ratio - 3.0f) < 0.01f)
            return 24.0f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 4.0f) < 0.01f)
            return 20.0f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 6.0f) < 0.01f)
            return 16.0f;

        if (parameters.timingIndex == 3 && std::abs (ratio - 8.0f) < 0.01f)
            return 16.0f;

        if (isMediumSlowTimingCalibratedRatio (ratio))
            return 27.0f;

        return 0.0f;
    }

    float getMediumSlowTimingReleaseMs (float ratio) const
    {
        if (isMediumSlowTimingCalibratedRatio (ratio))
            return 1450.0f;

        return 0.0f;
    }

    float getMediumSlowTimingReleaseLift (float ratio) const
    {
        if (isMediumSlowTimingCalibratedRatio (ratio))
            return 0.0f;

        return 0.03f;
    }

    bool isSlowTimingCalibratedRatio (float ratio) const
    {
        return parameters.timingIndex == 4
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    float getSlowTimingAttackMs (float ratio) const
    {
        if (isSlowTimingCalibratedRatio (ratio))
            return interpolateRatioValue (ratio, { 260.0f, 180.0f, 224.1f, 197.2f, 161.4f, 134.5f });

        return 0.0f;
    }

    float getSlowTimingReleaseMs (float ratio) const
    {
        if (isSlowTimingCalibratedRatio (ratio))
            return interpolateRatioValue (ratio, { 1090.0f, 1112.0f, 1144.0f, 1320.0f, 1450.0f, 1550.0f });

        return 0.0f;
    }

    bool usesSlowAttackPreview (float ratio) const
    {
        return isSlowTimingCalibratedRatio (ratio) || isAutoTimingOneFiveRatio (ratio);
    }

    float tickSlowAttackPreviewEnvelope (float currentEnvelope, float detector) const
    {
        const auto isAutoAttackPreview = parameters.timingIndex == 5 && ! parameters.fastMode;
        const auto attackSeconds = isAutoAttackPreview ? 0.0016f : 0.0016f;
        const auto releaseSeconds = isAutoAttackPreview ? 0.035f : 0.035f;
        const auto attackCoeff = std::exp (-1.0f / (float) (sampleRate * attackSeconds));
        const auto releaseCoeff = std::exp (-1.0f / (float) (sampleRate * releaseSeconds));
        const auto coeff = detector > currentEnvelope ? attackCoeff : releaseCoeff;
        return coeff * currentEnvelope + (1.0f - coeff) * detector;
    }

    float getSlowAttackPreviewBlend (float ratio) const
    {
        if (parameters.timingIndex == 5 && std::abs (ratio - 2.0f) < 0.01f)
            return 0.09f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 3.0f) < 0.01f)
            return 0.095f;

        if (parameters.timingIndex == 5
            && (std::abs (ratio - 4.0f) < 0.01f || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f))
            return 0.11f;

        if (isAutoTimingOneFiveRatio (ratio))
            return 0.105f;

        if (std::abs (ratio - 1.5f) < 0.01f)
            return 0.050f;

        if (std::abs (ratio - 2.0f) < 0.01f)
            return 0.055f;

        if (std::abs (ratio - 4.0f) < 0.01f)
            return 0.25f;

        if (std::abs (ratio - 6.0f) < 0.01f)
            return 0.32f;

        if (std::abs (ratio - 8.0f) < 0.01f)
            return 0.48f;

        return 0.11f;
    }

    float getReleaseCurveLift (float ratio) const
    {
        switch (parameters.timingIndex)
        {
            case 2:
                if (const auto mediumLift = getMediumTimingReleaseLift (ratio); mediumLift > 0.0f)
                    return mediumLift;

                return 0.035f;

            case 3:  return getMediumSlowTimingReleaseLift (ratio);
            case 4:  return 0.025f;
            case 5:  return 0.03f;
            default: return 0.0f;
        }
    }

    float getReleaseShapeScale() const
    {
        return 0.92f;
    }

    bool isAutoTimingOneFiveRatio (float ratio) const
    {
        return parameters.timingIndex == 5
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    float getAutoTimingAttackMs (float ratio) const
    {
        if (parameters.timingIndex == 5 && std::abs (ratio - 6.0f) < 0.01f)
            return 12.0f;

        if (parameters.timingIndex == 5 && std::abs (ratio - 8.0f) < 0.01f)
            return 6.0f;

        if (isAutoTimingOneFiveRatio (ratio))
            return 205.0f;

        return 0.0f;
    }

    float getAutoTimingReleaseMs (float ratio) const
    {
        if (isAutoTimingOneFiveRatio (ratio))
            return 600.0f;

        return 0.0f;
    }

    float getAutoReleaseShape (float gainReductionDb) const
    {
        const auto releaseDepth = juce::jlimit (0.0f, 1.0f, (gainReductionDb - 0.8f) / 6.0f);
        return releaseDepth * releaseDepth * (3.0f - 2.0f * releaseDepth);
    }

    bool isFastTimingCalibratedRatio (float ratio) const
    {
        return parameters.timingIndex == 0
            && ! parameters.fastMode
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    bool isFastTimingSixToOne (float ratio) const
    {
        return parameters.timingIndex == 0 && ! parameters.fastMode && std::abs (ratio - 6.0f) < 0.01f;
    }

    bool isFastTimingFourToOne (float ratio) const
    {
        return parameters.timingIndex == 0 && ! parameters.fastMode && std::abs (ratio - 4.0f) < 0.01f;
    }

    bool isFastTimingEightToOne (float ratio) const
    {
        return parameters.timingIndex == 0 && ! parameters.fastMode && std::abs (ratio - 8.0f) < 0.01f;
    }

    bool isFastTimingFastModeCalibratedRatio (float ratio) const
    {
        return parameters.timingIndex == 0
            && parameters.fastMode
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    bool isFastTimingFastModeNormalAttackRatio (float ratio) const
    {
        return parameters.timingIndex == 0
            && parameters.fastMode
            && (std::abs (ratio - 4.0f) < 0.01f || std::abs (ratio - 6.0f) < 0.01f
                || std::abs (ratio - 8.0f) < 0.01f);
    }

    bool isMediumFastTimingCalibratedRatio (float ratio) const
    {
        return parameters.timingIndex == 1
            && ! parameters.fastMode
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    bool isMediumFastTimingFastModeCalibratedRatio (float ratio) const
    {
        return (parameters.timingIndex == 1 || parameters.timingIndex == 2)
            && parameters.fastMode
            && (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
                || std::abs (ratio - 3.0f) < 0.01f || std::abs (ratio - 4.0f) < 0.01f
                || std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f);
    }

    float getMediumFastAttackDipAmountDb (float ratio) const
    {
        return interpolateRatioValue (ratio, { 0.22f, 0.24f, 0.27f, 0.30f, 0.32f, 0.34f });
    }

    float getMediumFastFastModeAttackMultiplier (float ratio) const
    {
        return interpolateRatioValue (ratio, { 0.45f, 0.46f, 0.48f, 0.70f, 0.82f, 0.82f });
    }

    float getMediumFastFastModeReleaseMultiplier (float ratio) const
    {
        return interpolateRatioValue (ratio, { 0.46f, 0.46f, 0.47f, 0.50f, 0.49f, 0.48f });
    }

    float getMediumTimingFastModeAttackMultiplier (float ratio) const
    {
        if (std::abs (ratio - 1.5f) < 0.01f)
            return 0.90f;

        if (std::abs (ratio - 2.0f) < 0.01f)
            return 0.95f;

        if (std::abs (ratio - 3.0f) < 0.01f)
            return 0.95f;

        if (std::abs (ratio - 4.0f) < 0.01f)
            return 0.95f;

        if (std::abs (ratio - 6.0f) < 0.01f)
            return 0.82f;

        if (std::abs (ratio - 8.0f) < 0.01f)
            return 0.88f;

        return getMediumFastFastModeAttackMultiplier (ratio);
    }

    float getMediumTimingFastModeReleaseMultiplier (float ratio) const
    {
        if (std::abs (ratio - 1.5f) < 0.01f)
            return 2.60f;

        if (std::abs (ratio - 2.0f) < 0.01f)
            return 2.45f;

        if (std::abs (ratio - 3.0f) < 0.01f)
            return 2.35f;

        if (std::abs (ratio - 4.0f) < 0.01f)
            return 2.05f;

        if (std::abs (ratio - 6.0f) < 0.01f)
            return 1.70f;

        if (std::abs (ratio - 8.0f) < 0.01f)
            return 1.65f;

        return getMediumFastFastModeReleaseMultiplier (ratio);
    }

    float getMediumSlowFastModeAttackMultiplier (float ratio) const
    {
        if (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
            || std::abs (ratio - 4.0f) < 0.01f || std::abs (ratio - 6.0f) < 0.01f
            || std::abs (ratio - 8.0f) < 0.01f)
            return 0.92f;

        if (std::abs (ratio - 3.0f) < 0.01f)
            return 0.92f;

        return 1.0f;
    }

    float getMediumSlowFastModeReleaseMultiplier (float ratio) const
    {
        if (std::abs (ratio - 1.5f) < 0.01f || std::abs (ratio - 2.0f) < 0.01f
            || std::abs (ratio - 4.0f) < 0.01f || std::abs (ratio - 6.0f) < 0.01f
            || std::abs (ratio - 8.0f) < 0.01f)
            return 0.87f;

        if (std::abs (ratio - 3.0f) < 0.01f)
            return 0.72f;

        return 1.0f;
    }

    float getMediumFastFastModeAttackDipAmountDb (float ratio) const
    {
        if (parameters.timingIndex == 2 && parameters.fastMode && std::abs (ratio - 8.0f) < 0.01f)
            return 0.60f;

        return interpolateRatioValue (ratio, { 0.22f, 0.24f, 0.27f, 0.36f, 0.62f, 0.82f });
    }

    float getFastTimingThreeToOneReleaseScale (float ratio) const
    {
        juce::ignoreUnused (ratio);
        return 1.0f;
    }

    float getFastTimingFastModeMinReleaseScale (float ratio) const
    {
        return isFastTimingFastModeCalibratedRatio (ratio) ? 0.58f : 1.0f;
    }

    float getFastTimingMinReleaseScale (float ratio) const
    {
        if (isFastTimingFourToOne (ratio))
            return 0.88f;

        if (isFastTimingSixToOne (ratio))
            return 0.65f;

        if (isFastTimingEightToOne (ratio))
            return 0.55f;

        return 1.0f;
    }

    float getFastTimingThreeToOneReleaseMemoryScale (float ratio) const
    {
        if (isFastTimingFourToOne (ratio))
            return 0.34f;

        if (isFastTimingSixToOne (ratio))
            return 0.38f;

        if (isFastTimingEightToOne (ratio))
            return 0.30f;

        if (isFastTimingFastModeCalibratedRatio (ratio))
            return 1.0f;

        return isFastTimingCalibratedRatio (ratio) ? 0.55f : 1.0f;
    }

    float shapeFastTimingThreeToOneRelease (float ratio, float releaseDepth, float releaseShape) const
    {
        if (! isFastTimingCalibratedRatio (ratio))
            return releaseShape;

        const auto midRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.24f) / 0.34f);
        const auto shapedBoost = midRelease * midRelease * (3.0f - 2.0f * midRelease);
        const auto frontRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.14f) / 0.44f);
        const auto focusedLift = std::sin (frontRelease * juce::MathConstants<float>::pi);
        auto shapedAmount = 0.68f;
        auto focusedAmount = 0.10f;

        if (isFastTimingFourToOne (ratio))
        {
            shapedAmount = 0.78f;
            focusedAmount = 0.11f;
        }
        else if (isFastTimingSixToOne (ratio))
        {
            shapedAmount = 1.05f;
            focusedAmount = 0.18f;
        }
        else if (isFastTimingEightToOne (ratio))
        {
            shapedAmount = 0.96f;
            focusedAmount = 0.16f;
        }

        return juce::jlimit (0.0f, 1.0f, releaseShape + shapedBoost * shapedAmount + focusedLift * focusedAmount);
    }

    float shapeFastTimingFastModeRelease (float ratio, float releaseDepth, float releaseShape) const
    {
        if (! isFastTimingFastModeCalibratedRatio (ratio))
            return releaseShape;

        const auto frontRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.02f) / 0.18f);
        const auto frontBoost = frontRelease * frontRelease * (3.0f - 2.0f * frontRelease);
        const auto midRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.08f) / 0.28f);
        const auto midLift = std::sin (midRelease * juce::MathConstants<float>::pi);
        const auto tailTrim = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.42f) / 0.30f);

        return juce::jlimit (0.0f, 1.0f, releaseShape + frontBoost * 0.28f + midLift * 0.05f - tailTrim * 0.22f);
    }

    float shapeMediumFastRelease (float ratio, float releaseDepth, float releaseShape) const
    {
        if (parameters.timingIndex != 1 || parameters.fastMode)
            return releaseShape;

        const auto frontRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.08f) / 0.34f);
        const auto frontBoost = frontRelease * frontRelease * (3.0f - 2.0f * frontRelease);
        const auto midRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.18f) / 0.42f);
        const auto midLift = std::sin (midRelease * juce::MathConstants<float>::pi);
        const auto tailTrim = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.58f) / 0.28f);
        const auto shapedAmount = interpolateRatioValue (ratio, { 0.34f, 0.34f, 0.34f, 0.28f, 0.22f, 0.18f });
        const auto focusedAmount = interpolateRatioValue (ratio, { 0.10f, 0.10f, 0.10f, 0.08f, 0.06f, 0.05f });
        const auto tailAmount = interpolateRatioValue (ratio, { 0.08f, 0.08f, 0.08f, 0.07f, 0.06f, 0.05f });

        return juce::jlimit (0.0f, 1.0f, releaseShape + frontBoost * shapedAmount + midLift * focusedAmount - tailTrim * tailAmount);
    }

    float shapeMediumFastFastModeRelease (float ratio, float releaseDepth, float releaseShape) const
    {
        if (! isMediumFastTimingFastModeCalibratedRatio (ratio))
            return releaseShape;

        const auto frontRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.015f) / 0.18f);
        const auto frontBoost = frontRelease * frontRelease * (3.0f - 2.0f * frontRelease);
        const auto midRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.10f) / 0.34f);
        const auto midLift = std::sin (midRelease * juce::MathConstants<float>::pi);
        const auto tailTrim = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.46f) / 0.30f);
        const auto shapedAmount = interpolateRatioValue (ratio, { 0.55f, 0.56f, 0.54f, 0.42f, 0.46f, 0.90f });
        const auto focusedAmount = interpolateRatioValue (ratio, { 0.11f, 0.11f, 0.105f, 0.085f, 0.085f, 0.16f });
        const auto tailAmount = interpolateRatioValue (ratio, { 0.20f, 0.20f, 0.19f, 0.16f, 0.16f, 0.22f });

        return juce::jlimit (0.0f, 1.0f, releaseShape + frontBoost * shapedAmount + midLift * focusedAmount - tailTrim * tailAmount);
    }

    float shapeMediumSlowFastModeRelease (float ratio, float releaseDepth, float releaseShape) const
    {
        if (! parameters.fastMode || (parameters.timingIndex != 3 && parameters.timingIndex != 4 && parameters.timingIndex != 5)
            || (parameters.timingIndex == 3 && std::abs (ratio - 3.0f) >= 0.01f))
            return releaseShape;

        const auto frontRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.025f) / 0.20f);
        const auto frontBoost = frontRelease * frontRelease * (3.0f - 2.0f * frontRelease);
        const auto midRelease = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.10f) / 0.34f);
        const auto midLift = std::sin (midRelease * juce::MathConstants<float>::pi);
        const auto tailTrim = juce::jlimit (0.0f, 1.0f, (releaseDepth - 0.50f) / 0.28f);

        return juce::jlimit (0.0f, 1.0f, releaseShape + frontBoost * 0.38f + midLift * 0.08f - tailTrim * 0.10f);
    }

    GainResult applyFastTimingThreeToOneAttackDip (GainResult result,
                                                   float& fastLevel,
                                                   float& slowLevel,
                                                   int& dipSample,
                                                   int& dipArmed,
                                                   float detector,
                                                   float ratio) const
    {
        const auto mediumFastAttackDip = isMediumFastTimingCalibratedRatio (ratio) || isMediumFastTimingFastModeCalibratedRatio (ratio);
        const auto mediumTimingAttackDip = isMediumTimingAttackDipRatio (ratio);
        const auto mediumSlowAttackDip = isMediumSlowTimingAttackDipRatio (ratio);

        if (! isFastTimingCalibratedRatio (ratio) && ! isFastTimingFastModeCalibratedRatio (ratio) && ! mediumFastAttackDip && ! mediumTimingAttackDip && ! mediumSlowAttackDip)
        {
            fastLevel = 0.0f;
            slowLevel = 0.0f;
            dipSample = -1;
            dipArmed = 1;
            return result;
        }

        const auto shapedTimingDip = mediumFastAttackDip || mediumTimingAttackDip || mediumSlowAttackDip;
        const auto fastCoeff = std::exp (-1.0f / (float) (sampleRate * (shapedTimingDip ? 0.0012 : 0.0015)));
        const auto slowCoeff = std::exp (-1.0f / (float) (sampleRate * (shapedTimingDip ? 0.065 : 0.075)));
        fastLevel = fastCoeff * fastLevel + (1.0f - fastCoeff) * detector;
        slowLevel = slowCoeff * slowLevel + (1.0f - slowCoeff) * detector;

        if (slowLevel <= 0.000001f || result.gainReductionDb <= 0.05f)
        {
            dipSample = -1;
            dipArmed = 1;
            return result;
        }

        if (fastLevel < slowLevel * (shapedTimingDip ? 0.72f : 0.65f))
            dipArmed = 1;

        if (dipArmed != 0 && fastLevel > slowLevel * (shapedTimingDip ? 2.05f : 2.3f) && result.gainReductionDb > 1.5f)
        {
            dipSample = 0;
            dipArmed = 0;
        }

        if (dipSample < 0)
            return result;

        const auto elapsedSeconds = (float) dipSample / (float) sampleRate;
        ++dipSample;

        const auto delaySeconds = mediumSlowAttackDip ? 0.0060f : (mediumTimingAttackDip ? 0.0080f : (mediumFastAttackDip ? 0.0010f : 0.0012f));
        const auto riseSeconds = mediumSlowAttackDip ? 0.0200f : (mediumTimingAttackDip ? 0.0180f : (mediumFastAttackDip ? 0.0100f : 0.0080f));
        const auto holdSeconds = mediumSlowAttackDip ? 0.0160f : (mediumTimingAttackDip ? 0.0180f : (mediumFastAttackDip ? 0.0100f : 0.0120f));
        const auto decaySeconds = mediumSlowAttackDip ? 0.1200f : (mediumTimingAttackDip ? 0.0900f : (mediumFastAttackDip ? 0.0700f : 0.0950f));
        auto amountDb = mediumTimingAttackDip ? (std::abs (ratio - 8.0f) < 0.01f ? 0.58f : 0.18f)
                      : mediumSlowAttackDip ? 0.22f
                      : mediumFastAttackDip ? (parameters.fastMode ? getMediumFastFastModeAttackDipAmountDb (ratio)
                                                                   : getMediumFastAttackDipAmountDb (ratio))
                                            : 0.60f;

        if (! mediumFastAttackDip && ! mediumTimingAttackDip && ! mediumSlowAttackDip)
        {
            if (isFastTimingFourToOne (ratio) || (isFastTimingFastModeNormalAttackRatio (ratio) && std::abs (ratio - 4.0f) < 0.01f))
                amountDb = 0.78f;
            else if (isFastTimingSixToOne (ratio) || (isFastTimingFastModeNormalAttackRatio (ratio) && std::abs (ratio - 6.0f) < 0.01f))
                amountDb = 0.95f;
            else if (isFastTimingEightToOne (ratio) || (isFastTimingFastModeNormalAttackRatio (ratio) && std::abs (ratio - 8.0f) < 0.01f))
                amountDb = 1.28f;
        }

        auto dipDb = 0.0f;

        if (elapsedSeconds >= delaySeconds)
        {
            const auto activeSeconds = elapsedSeconds - delaySeconds;

            if (activeSeconds < riseSeconds)
            {
                const auto risePhase = juce::jlimit (0.0f, 1.0f, activeSeconds / riseSeconds);
                dipDb = amountDb * risePhase * risePhase * (3.0f - 2.0f * risePhase);
            }
            else
            {
                const auto decayTime = juce::jmax (0.0f, activeSeconds - riseSeconds - holdSeconds);
                dipDb = amountDb * std::exp (-decayTime / decaySeconds);
            }
        }

        if (elapsedSeconds > delaySeconds + riseSeconds + holdSeconds + decaySeconds * 4.5f)
            dipSample = -1;

        result.gain *= juce::Decibels::decibelsToGain (-dipDb);

        if (! mediumFastAttackDip && ! mediumTimingAttackDip && ! mediumSlowAttackDip)
            result.gainReductionDb += dipDb;

        return result;
    }

    float getPeakRestoreAmount (float ratio) const
    {
        juce::ignoreUnused (ratio);
        return 1.0f;
    }

    float getTransientPassAmount (float ratio) const
    {
        juce::ignoreUnused (ratio);

        switch (parameters.timingIndex)
        {
            // 1.1.0: the original 0.98 value could switch almost completely
            // from compressed audio to dry audio in a single sample.  Keep the
            // transient-preserving behaviour, but limit the gain discontinuity.
            case 0:  return 0.65f;
            case 1:  return 0.0f;
            case 2:  return 0.0f;
            case 3:  return 0.0f;
            case 4:  return 0.0f;
            case 5:  return 0.0f;
            default: return 0.0f;
        }
    }

    float getTransientPassLimit (float ratio) const
    {
        juce::ignoreUnused (ratio);

        switch (parameters.timingIndex)
        {
            case 0:  return 0.65f;
            case 1:  return 0.0f;
            case 2:  return 0.0f;
            case 3:  return 0.0f;
            case 4:  return 0.0f;
            case 5:  return 0.0f;
            default: return 0.0f;
        }
    }

    float getTransientPassDecaySeconds() const
    {
        switch (parameters.timingIndex)
        {
            case 0:  return 0.0010f;
            case 1:  return 0.00085f;
            case 2:  return 0.0011f;
            case 3:  return 0.00135f;
            case 4:  return 0.0016f;
            case 5:  return 0.0050f;
            default: return 0.0010f;
        }
    }

    float getPeakRestoreDecaySeconds() const
    {
        return 0.0010f;
    }

    float getPeakRestoreLockoutSeconds() const
    {
        switch (parameters.timingIndex)
        {
            case 0:  return 0.018f;
            case 1:  return 0.024f;
            case 2:  return 0.03f;
            case 3:  return 0.036f;
            case 4:  return 0.045f;
            case 5:  return 0.036f;
            default: return 0.03f;
        }
    }

    float getReleaseMemoryDecaySeconds() const
    {
        switch (parameters.timingIndex)
        {
            case 0:  return 0.105f;
            case 1:  return 0.13f;
            case 2:  return 0.15f;
            case 3:  return 0.17f;
            case 4:  return 0.2f;
            case 5:  return 0.17f;
            default: return 0.14f;
        }
    }

    Timing resolveTiming (float ratio) const
    {
        static constexpr std::array<Timing, 6> timings {{
            { 0.05f, 0.45f, 250.0f, 920.0f, false, true },
            { 12.0f, 32.0f, 480.0f, 980.0f, false, false },
            { 3.0f, 18.0f, 350.0f, 700.0f, false, false },
            { 5.0f, 40.0f, 600.0f, 1000.0f, false, false },
            { 10.0f, 80.0f, 800.0f, 1500.0f, false, false },
            { 5.0f, 40.0f, 400.0f, 2000.0f, true, true }
        }};

        auto timing = timings[(size_t) juce::jlimit (0, (int) timings.size() - 1, parameters.timingIndex)];
        const auto ratioNormalised = juce::jlimit (0.0f, 1.0f, (ratio - 1.5f) / (8.0f - 1.5f));

        auto attackMs = timing.maxAttackMs + ratioNormalised * (timing.minAttackMs - timing.maxAttackMs);
        auto releaseMs = timing.minReleaseMs + ratioNormalised * (timing.maxReleaseMs - timing.minReleaseMs);

        if (parameters.timingIndex == 1)
        {
            attackMs = getMediumFastAttackMs (ratio);
            releaseMs = getMediumFastReleaseMs (ratio);
        }
        else if (parameters.timingIndex == 2 && ! parameters.fastMode)
        {
            if (const auto mediumAttackMs = getMediumTimingAttackMs (ratio); mediumAttackMs > 0.0f)
                attackMs = mediumAttackMs;

            if (const auto mediumReleaseMs = getMediumTimingReleaseMs (ratio); mediumReleaseMs > 0.0f)
                releaseMs = mediumReleaseMs;
        }
        else if (parameters.timingIndex == 3)
        {
            if (const auto mediumSlowAttackMs = getMediumSlowTimingAttackMs (ratio); mediumSlowAttackMs > 0.0f)
                attackMs = mediumSlowAttackMs;

            if (const auto mediumSlowReleaseMs = getMediumSlowTimingReleaseMs (ratio); mediumSlowReleaseMs > 0.0f)
                releaseMs = mediumSlowReleaseMs;
        }
        else if (parameters.timingIndex == 4)
        {
            if (const auto slowAttackMs = getSlowTimingAttackMs (ratio); slowAttackMs > 0.0f)
                attackMs = slowAttackMs;

            if (const auto slowReleaseMs = getSlowTimingReleaseMs (ratio); slowReleaseMs > 0.0f)
                releaseMs = slowReleaseMs;
        }
        else if (parameters.timingIndex == 5)
        {
            if (const auto autoAttackMs = getAutoTimingAttackMs (ratio); autoAttackMs > 0.0f)
                attackMs = autoAttackMs;

            if (const auto autoReleaseMs = getAutoTimingReleaseMs (ratio); autoReleaseMs > 0.0f)
                releaseMs = autoReleaseMs;
        }

        const auto fastTimingAttackCalibration = (parameters.timingIndex == 0 && (! parameters.fastMode || isFastTimingFastModeNormalAttackRatio (ratio))) ? 4.00f : 1.0f;
        auto fastAttackMultiplier = (parameters.fastMode && ! isFastTimingFastModeNormalAttackRatio (ratio) ? 0.7f : 1.0f) * fastTimingAttackCalibration;
        auto fastReleaseMultiplier = parameters.fastMode ? 0.58f : 1.0f;

        if (parameters.timingIndex == 1 && parameters.fastMode)
        {
            fastAttackMultiplier = getMediumFastFastModeAttackMultiplier (ratio);
            fastReleaseMultiplier = getMediumFastFastModeReleaseMultiplier (ratio);
        }
        else if (parameters.timingIndex == 2 && parameters.fastMode)
        {
            fastAttackMultiplier = getMediumTimingFastModeAttackMultiplier (ratio);
            fastReleaseMultiplier = getMediumTimingFastModeReleaseMultiplier (ratio);
        }
        else if (parameters.timingIndex == 3 && parameters.fastMode)
        {
            fastAttackMultiplier = getMediumSlowFastModeAttackMultiplier (ratio);
            fastReleaseMultiplier = getMediumSlowFastModeReleaseMultiplier (ratio);
        }
        else if (parameters.timingIndex == 4 && parameters.fastMode)
        {
            fastAttackMultiplier = getMediumSlowFastModeAttackMultiplier (ratio);
            fastReleaseMultiplier = getMediumSlowFastModeReleaseMultiplier (ratio);
        }
        else if (parameters.timingIndex == 5 && parameters.fastMode)
        {
            fastAttackMultiplier = getMediumSlowFastModeAttackMultiplier (ratio);
            fastReleaseMultiplier = getMediumSlowFastModeReleaseMultiplier (ratio);
        }

        timing.minAttackMs = attackMs * fastAttackMultiplier;
        timing.maxAttackMs = timing.minAttackMs;
        timing.minReleaseMs = releaseMs * fastReleaseMultiplier;
        timing.maxReleaseMs = timing.maxReleaseMs * fastReleaseMultiplier;

        const auto releaseCalibration = getFastTimingThreeToOneReleaseScale (ratio);
        timing.minReleaseMs *= releaseCalibration;
        timing.maxReleaseMs *= releaseCalibration;
        timing.minReleaseMs *= getFastTimingFastModeMinReleaseScale (ratio);
        timing.minReleaseMs *= getFastTimingMinReleaseScale (ratio);
        return timing;
    }

    float getDetectorSample (int channel,
                             int sampleIndex,
                             const juce::AudioBuffer<float>& audioBuffer,
                             const juce::AudioBuffer<float>* externalSidechain,
                             float hpfCoeff)
    {
        const auto& detectorBuffer = (parameters.externalSidechain && externalSidechain != nullptr && externalSidechain->getNumSamples() > sampleIndex)
                                   ? *externalSidechain
                                   : audioBuffer;

        const auto detectorChannels = detectorBuffer.getNumChannels();

        if (detectorChannels == 0)
            return 0.0f;

        const auto mappedChannel = juce::jlimit (0, detectorChannels - 1, channel);
        const auto detectorSample = detectorBuffer.getSample (mappedChannel, sampleIndex);

        const auto stateIndex = (size_t) mappedChannel;
        const auto highPassed = detectorSample - lastSidechain[stateIndex] + hpfCoeff * sidechainState[stateIndex];
        lastSidechain[stateIndex] = detectorSample;
        sidechainState[stateIndex] = highPassed;
        return highPassed;
    }

    float tickEnvelope (float currentEnvelope, float detector, const Timing& timing, float gainReductionDb, float ratio) const
    {
        auto attackMs = timing.minAttackMs;

        if (isMediumSlowTimingCalibratedRatio (ratio) && detector > currentEnvelope && detector > 0.000001f)
        {
            const auto attackProgress = juce::jlimit (0.0f, 1.0f, currentEnvelope / detector);
            const auto tailProgress = juce::jlimit (0.0f, 1.0f, (attackProgress - 0.24f) / 0.50f);
            const auto tailShape = tailProgress * tailProgress * (3.0f - 2.0f * tailProgress);
            attackMs *= 0.78f + tailShape * 0.58f;
        }
        else if (parameters.timingIndex == 5 && std::abs (ratio - 2.0f) < 0.01f && detector > currentEnvelope && detector > 0.000001f)
        {
            const auto attackProgress = juce::jlimit (0.0f, 1.0f, currentEnvelope / detector);
            const auto tailProgress = juce::jlimit (0.0f, 1.0f, (attackProgress - 0.20f) / 0.48f);
            const auto tailShape = tailProgress * tailProgress * (3.0f - 2.0f * tailProgress);
            attackMs *= 0.22f + tailShape * 0.86f;
        }
        else if (parameters.timingIndex == 5 && std::abs (ratio - 3.0f) < 0.01f && detector > currentEnvelope && detector > 0.000001f)
        {
            const auto attackProgress = juce::jlimit (0.0f, 1.0f, currentEnvelope / detector);
            const auto tailProgress = juce::jlimit (0.0f, 1.0f, (attackProgress - 0.24f) / 0.54f);
            const auto tailShape = tailProgress * tailProgress * (3.0f - 2.0f * tailProgress);
            attackMs *= 0.30f + tailShape * 0.78f;
        }
        else if (parameters.timingIndex == 5
                 && (std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f)
                 && detector > currentEnvelope && detector > 0.000001f)
        {
            const auto attackProgress = juce::jlimit (0.0f, 1.0f, currentEnvelope / detector);
            const auto tailProgress = juce::jlimit (0.0f, 1.0f, (attackProgress - 0.22f) / 0.50f);
            const auto tailShape = tailProgress * tailProgress * (3.0f - 2.0f * tailProgress);
            attackMs *= 0.78f + tailShape * 0.50f;
        }
        else if (parameters.timingIndex == 5
                 && std::abs (ratio - 4.0f) < 0.01f
                 && detector > currentEnvelope && detector > 0.000001f)
        {
            const auto attackProgress = juce::jlimit (0.0f, 1.0f, currentEnvelope / detector);
            const auto tailProgress = juce::jlimit (0.0f, 1.0f, (attackProgress - 0.22f) / 0.50f);
            const auto tailShape = tailProgress * tailProgress * (3.0f - 2.0f * tailProgress);
            attackMs *= 0.23f + tailShape * 0.85f;
        }
        else if (isAutoTimingOneFiveRatio (ratio) && detector > currentEnvelope && detector > 0.000001f)
        {
            const auto attackProgress = juce::jlimit (0.0f, 1.0f, currentEnvelope / detector);
            const auto tailProgress = juce::jlimit (0.0f, 1.0f, (attackProgress - 0.26f) / 0.58f);
            const auto tailShape = tailProgress * tailProgress * (3.0f - 2.0f * tailProgress);
            attackMs *= 0.30f + tailShape * 0.78f;
        }

        const auto attackCoeff = std::exp (-1.0f / (float) (sampleRate * juce::jmax (0.00005f, attackMs / 1000.0f)));
        auto releaseMs = timing.minReleaseMs;

        if (parameters.fastMode && (parameters.timingIndex != 3 || isMediumSlowTimingFastModeCalibratedRatio (ratio)) && detector < currentEnvelope)
        {
            const auto releaseDepth = juce::jlimit (0.0f, 1.0f, gainReductionDb / 10.0f);
            auto releaseShape = std::pow (releaseDepth, 1.18f);
            releaseShape = juce::jlimit (0.0f, 1.0f,
                                         (releaseShape + std::sin (releaseDepth * juce::MathConstants<float>::pi) * getFastModeReleaseLift (ratio))
                                         * getReleaseShapeScale());
            releaseShape = (parameters.timingIndex == 3 || parameters.timingIndex == 4 || parameters.timingIndex == 5)
                         ? shapeMediumSlowFastModeRelease (ratio, releaseDepth, releaseShape)
                         : ((parameters.timingIndex == 1 || parameters.timingIndex == 2) ? shapeMediumFastFastModeRelease (ratio, releaseDepth, releaseShape)
                                                                                         : shapeFastTimingFastModeRelease (ratio, releaseDepth, releaseShape));

            releaseMs = timing.maxReleaseMs + releaseShape * (timing.minReleaseMs - timing.maxReleaseMs);
        }
        else if (timing.autoRelease)
        {
            const auto releaseShape = getAutoReleaseShape (gainReductionDb);
            releaseMs = timing.minReleaseMs + releaseShape * (timing.maxReleaseMs - timing.minReleaseMs);
        }
        else if (timing.twoStageRelease && detector < currentEnvelope)
        {
            const auto releaseDepth = juce::jlimit (0.0f, 1.0f, gainReductionDb / 10.0f);
            auto releaseShape = std::pow (releaseDepth, 1.18f);
            releaseShape = juce::jlimit (0.0f, 1.0f, releaseShape * getReleaseShapeScale());
            releaseShape = shapeFastTimingThreeToOneRelease (ratio, releaseDepth, releaseShape);

            releaseMs = timing.maxReleaseMs + releaseShape * (timing.minReleaseMs - timing.maxReleaseMs);
        }
        else if (parameters.timingIndex >= 2 && detector < currentEnvelope)
        {
            const auto releaseDepth = juce::jlimit (0.0f, 1.0f, gainReductionDb / 10.0f);
            const auto releaseLift = std::sin (releaseDepth * juce::MathConstants<float>::pi)
                                   * getReleaseCurveLift (ratio)
                                   * getReleaseShapeScale();
            releaseMs = timing.minReleaseMs * (1.0f - releaseLift);
        }
        else if (parameters.timingIndex == 1 && ! parameters.fastMode && detector < currentEnvelope)
        {
            const auto releaseDepth = juce::jlimit (0.0f, 1.0f, gainReductionDb / 10.0f);
            auto releaseLift = std::sin (releaseDepth * juce::MathConstants<float>::pi)
                             * getMediumFastReleaseLift (ratio)
                             * getReleaseShapeScale();
            releaseLift = shapeMediumFastRelease (ratio, releaseDepth, releaseLift);
            releaseMs = timing.minReleaseMs * (1.0f - releaseLift);
        }

        const auto releaseCoeff = std::exp (-1.0f / (float) (sampleRate * juce::jmax (0.0001f, releaseMs / 1000.0f)));
        const auto coeff = detector > currentEnvelope ? attackCoeff : releaseCoeff;
        return coeff * currentEnvelope + (1.0f - coeff) * detector;
    }

    float getGainEnvelope (float mainEnvelope, float previewEnvelope, float ratio) const
    {
        if (! usesSlowAttackPreview (ratio))
            return mainEnvelope;

        const auto previewBlend = getSlowAttackPreviewBlend (ratio);
        return mainEnvelope + (juce::jmax (mainEnvelope, previewEnvelope) - mainEnvelope) * previewBlend;
    }

    bool usesSlowInstantPeakControl (float ratio) const
    {
        return isSlowTimingCalibratedRatio (ratio) || isAutoTimingOneFiveRatio (ratio);
    }

    GainResult applySlowInstantPeakControl (GainResult result, float detector, float envelope, float ratio) const
    {
        if (! usesSlowInstantPeakControl (ratio) || detector <= 0.000001f)
            return result;

        const auto envelopeLead = juce::jlimit (0.0f, 1.0f, (detector - envelope) / detector);

        if (envelopeLead <= 0.0f)
            return result;

        const auto leadShape = envelopeLead * envelopeLead * (3.0f - 2.0f * envelopeLead);
        const auto directGainReductionDb = computeGain (detector, ratio).gainReductionDb;
        const auto availableReductionDb = juce::jmax (0.0f, directGainReductionDb - result.gainReductionDb);
        const auto peakControlScale = isAutoTimingOneFiveRatio (ratio) ? 0.55f : (std::abs (ratio - 2.0f) < 0.01f ? 0.21f : 0.24f);
        const auto peakControlLimitDb = isAutoTimingOneFiveRatio (ratio) ? 1.25f : (std::abs (ratio - 2.0f) < 0.01f ? 0.54f : 0.60f);
        const auto extraGainReductionDb = juce::jlimit (0.0f, peakControlLimitDb, availableReductionDb * peakControlScale * leadShape);

        result.gain *= juce::Decibels::decibelsToGain (-extraGainReductionDb);
        result.gainReductionDb += extraGainReductionDb;
        return result;
    }

    GainResult applyAutoAttackShoulderDip (GainResult result, float detector, float envelope, float ratio) const
    {
        if (! isAutoTimingOneFiveRatio (ratio) || detector <= 0.000001f || result.gainReductionDb <= 0.05f)
            return result;

        const auto attackProgress = juce::jlimit (0.0f, 1.0f, envelope / detector);
        const auto dipStart = 0.12f;
        const auto dipWidth = 0.22f;
        const auto dipDecayStart = 0.58f;
        const auto dipDecayWidth = 0.26f;
        const auto dipWindow = juce::jlimit (0.0f, 1.0f, (attackProgress - dipStart) / dipWidth);
        const auto dipDecay = 1.0f - juce::jlimit (0.0f, 1.0f, (attackProgress - dipDecayStart) / dipDecayWidth);
        const auto dipShape = dipWindow * dipWindow * (3.0f - 2.0f * dipWindow) * dipDecay;
        const auto isAutoRatioSixOrEight = std::abs (ratio - 6.0f) < 0.01f || std::abs (ratio - 8.0f) < 0.01f;
        const auto isAutoHighRatio = std::abs (ratio - 4.0f) < 0.01f || isAutoRatioSixOrEight;
        const auto dipScale = std::abs (ratio - 2.0f) < 0.01f ? 0.17f : (std::abs (ratio - 3.0f) < 0.01f ? 0.22f : (isAutoRatioSixOrEight ? 0.0f : (isAutoHighRatio ? 0.36f : 0.28f)));
        const auto dipLimitDb = std::abs (ratio - 2.0f) < 0.01f ? 0.60f : (std::abs (ratio - 3.0f) < 0.01f ? 0.75f : (isAutoRatioSixOrEight ? 0.0f : (isAutoHighRatio ? 1.18f : 0.95f)));
        const auto dipDb = juce::jlimit (0.0f, dipLimitDb, result.gainReductionDb * dipScale * dipShape);

        result.gain *= juce::Decibels::decibelsToGain (-dipDb);
        result.gainReductionDb += dipDb;
        return result;
    }

    float tickPeakRestore (float& currentRestore,
                           int& lockoutSamples,
                             float previousEnvelope,
                             float detector,
                             float gainReductionDb,
                             float ratio) const
    {
        const auto decayCoeff = std::exp (-1.0f / (float) (sampleRate * getPeakRestoreDecaySeconds()));
        currentRestore *= decayCoeff;

        if (lockoutSamples > 0)
            --lockoutSamples;

        if (lockoutSamples == 0 && detector > previousEnvelope && gainReductionDb < 9.0f)
        {
            const auto rise = (detector - previousEnvelope) / juce::jmax (detector, 0.000001f);

            if (rise > 0.08f)
            {
                currentRestore = juce::jmax (currentRestore, juce::jlimit (0.0f, 1.0f, rise * getPeakRestoreAmount (ratio)));

                if (currentRestore > 0.92f)
                    lockoutSamples = juce::jmax (1, (int) std::round (sampleRate * getPeakRestoreLockoutSeconds()));
            }
        }

        return currentRestore;
    }

    float tickTransientPass (float& currentPass,
                             float previousEnvelope,
                             float detector,
                             float gainReductionDb,
                             float ratio) const
    {
        const auto passAmount = getTransientPassAmount (ratio);
        const auto passLimit = getTransientPassLimit (ratio);

        if (passAmount <= 0.0f || passLimit <= 0.0f)
        {
            currentPass = 0.0f;
            return 0.0f;
        }

        auto targetPass = 0.0f;

        if (detector > previousEnvelope && gainReductionDb < 9.0f)
        {
            const auto rise = (detector - previousEnvelope) / juce::jmax (detector, 0.000001f);

            if (rise > 0.08f)
                targetPass = juce::jlimit (0.0f, passLimit, rise * passAmount);
        }

        // 1.1.0: slew the transient dry/wet crossfade in both directions.
        // This removes the one-sample jump that produced clicks, rasp and HF
        // splatter on unusually large vocal attacks while preserving the
        // intended short transient pass.
        constexpr auto attackSeconds = 0.00020f;
        const auto releaseSeconds = getTransientPassDecaySeconds();
        const auto attackCoeff = std::exp (-1.0f / (float) (sampleRate * attackSeconds));
        const auto releaseCoeff = std::exp (-1.0f / (float) (sampleRate * releaseSeconds));
        const auto coeff = targetPass > currentPass ? attackCoeff : releaseCoeff;
        currentPass = coeff * currentPass + (1.0f - coeff) * targetPass;

        if (currentPass < 0.000001f)
            currentPass = 0.0f;

        return currentPass;
    }

    GainResult computeGain (float envelope, float ratio) const
    {
        const auto thresholdDbFS = parameters.thresholdDbu - 18.0f
                                 + (parameters.timingIndex == 0 ? getRatioThresholdOffsetDb (ratio) : 0.0f)
                                 + getTimingThresholdOffsetDb (ratio)
                                 + getTimingFastModeThresholdOffsetDb (ratio);
        const auto kneeDb = getKneeDb (ratio);
        const auto kneeStart = thresholdDbFS - kneeDb * 0.5f;
        const auto kneeEnd = thresholdDbFS + kneeDb * 0.5f;
        const auto levelDb = linearToDb (envelope);

        auto gainDb = 0.0f;

        if (levelDb > kneeEnd)
        {
            const auto over = levelDb - thresholdDbFS;
            gainDb = -over * (1.0f - 1.0f / ratio);
        }
        else if (levelDb > kneeStart)
        {
            const auto x = levelDb - kneeStart;
            gainDb = -(1.0f - 1.0f / ratio) * x * x / (2.0f * kneeDb);
        }

        gainDb *= getRatioCompressionScale (ratio) * getTimingCompressionScale (ratio);

        const auto compressionDepth = juce::jlimit (0.0f, 1.0f, (-gainDb - 0.05f) / 3.0f);
        const auto diodeMemory = std::tanh (envelope * (3.75f + ratio * 0.5f));
        const auto dynamicSag = 1.0f - diodeMemory * (0.06f + ratio * 0.0075f) * compressionDepth;
        const auto gain = juce::Decibels::decibelsToGain (gainDb) * dynamicSag;

        return { gain, juce::jmax (0.0f, -linearToDb (gain)) };
    }

    float getKneeDb (float ratio) const
    {
        const auto baseKneeDb = 4.0f + ((ratio - 1.5f) / (8.0f - 1.5f)) * 3.0f;

        if (isAutoTimingOneFiveRatio (ratio))
            return 7.4f;

        if (parameters.timingIndex == 4 && std::abs (ratio - 2.0f) < 0.01f)
            return 10.0f;

        if (parameters.timingIndex == 4 && std::abs (ratio - 3.0f) < 0.01f)
            return 9.0f;

        if (parameters.timingIndex == 4
            && (std::abs (ratio - 4.0f) < 0.01f || std::abs (ratio - 6.0f) < 0.01f))
            return 9.0f;

        if (parameters.timingIndex == 4 && std::abs (ratio - 8.0f) < 0.01f)
            return 10.0f;

        return baseKneeDb;
    }

    GainResult applyReleaseEntryMemory (GainResult result,
                                        float& releaseMemoryDb,
                                        float& fastLevel,
                                        float& slowLevel,
                                        int& releaseEntryArmed,
                                        float detector,
                                        float ratio) const
    {
        const auto fastCoeff = std::exp (-1.0f / (float) (sampleRate * 0.0025));
        const auto slowCoeff = std::exp (-1.0f / (float) (sampleRate * 0.12));
        fastLevel = fastCoeff * fastLevel + (1.0f - fastCoeff) * detector;
        slowLevel = slowCoeff * slowLevel + (1.0f - slowCoeff) * detector;

        if (slowLevel <= 0.000001f)
        {
            releaseMemoryDb = 0.0f;
            releaseEntryArmed = 1;
            return result;
        }

        const auto isCharging = fastLevel >= slowLevel * 0.82f;
        auto targetMemoryDb = 0.0f;

        if (isCharging)
            releaseEntryArmed = 1;

        if (releaseEntryArmed != 0
            && result.gainReductionDb > 0.0f
            && fastLevel < slowLevel * 0.46f)
            releaseEntryArmed = 0;

        if (releaseEntryArmed == 0 && result.gainReductionDb > 0.0f)
        {
            const auto ratioNormalised = juce::jlimit (0.0f, 1.0f, (ratio - 1.5f) / (8.0f - 1.5f));
            const auto depth = juce::jlimit (0.0f, 1.0f, result.gainReductionDb / 10.0f);
            const auto entryDrop = juce::jlimit (0.0f, 1.0f, (slowLevel - fastLevel) / slowLevel);
            const auto memoryAmount = 0.26f + ratioNormalised * 0.04f;
            targetMemoryDb = result.gainReductionDb
                           * memoryAmount
                           * std::pow (entryDrop, 0.72f)
                           * (0.58f + depth * 0.42f)
                           * getFastTimingThreeToOneReleaseMemoryScale (ratio);
        }

        // 1.1.0: ramp into release memory rather than adding its full amount
        // in one sample. The original slow decay is retained.
        constexpr auto entrySeconds = 0.0020f;
        const auto entryCoeff = std::exp (-1.0f / (float) (sampleRate * entrySeconds));
        const auto decayCoeff = std::exp (-1.0f / (float) (sampleRate * getReleaseMemoryDecaySeconds()));

        if (targetMemoryDb > releaseMemoryDb)
            releaseMemoryDb = entryCoeff * releaseMemoryDb + (1.0f - entryCoeff) * targetMemoryDb;
        else
            releaseMemoryDb *= decayCoeff;

        if (releaseMemoryDb < 0.001f)
            releaseMemoryDb = 0.0f;

        const auto extraGainReductionDb = releaseMemoryDb;

        result.gain *= juce::Decibels::decibelsToGain (-extraGainReductionDb);
        result.gainReductionDb += extraGainReductionDb;
        return result;
    }

    float getColourAmountForGain (float gain) const
    {
        const auto gainReductionDb = juce::jmax (0.0f, -linearToDb (gain));
        const auto deepCompression = juce::jlimit (0.0f, 1.0f, (gainReductionDb - 3.0f) / 12.0f);
        return deepCompression * deepCompression;
    }

    float colourSample (float sample, float ratio, float colourAmount) const
    {
        juce::ignoreUnused (ratio);

        const auto amount = juce::jlimit (0.0f, 1.0f, colourAmount);

        if (amount <= 0.0001f)
            return sample;

        const auto odd = -0.004f * amount * sample * sample * sample;
        const auto even = 0.0008f * amount * sample * std::abs (sample);
        return sample + odd + even;
    }

    void processLinked (juce::AudioBuffer<float>& buffer,
                        const juce::AudioBuffer<float>* externalSidechain,
                        const Timing& timing,
                        float hpfCoeff,
                        float outputGain,
                        float timingOutputGain,
                        float wet,
                        float dry,
                        juce::AudioBuffer<float>* wetBeforeMakeup)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto ratio = getRatio();

        auto blockGainReduction = 0.0f;
        for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            auto detector = 0.0f;

            for (int channel = 0; channel < numChannels; ++channel)
                detector = juce::jmax (detector, std::abs (getDetectorSample (channel, sampleIndex, buffer, externalSidechain, hpfCoeff)));

            const auto previousEnvelope = linkedEnvelope;
            linkedEnvelope = tickEnvelope (linkedEnvelope, detector, timing, heldGainReductionDb, ratio);
            linkedSlowPreviewEnvelope = usesSlowAttackPreview (ratio) ? tickSlowAttackPreviewEnvelope (linkedSlowPreviewEnvelope, detector) : 0.0f;
            const auto gainEnvelope = getGainEnvelope (linkedEnvelope, linkedSlowPreviewEnvelope, ratio);
            const auto transientPass = tickTransientPass (linkedTransientPass,
                                                          previousEnvelope,
                                                          detector,
                                                          heldGainReductionDb,
                                                          ratio);
            auto gainResult = applyReleaseEntryMemory (computeGain (gainEnvelope, ratio),
                                                       linkedReleaseMemoryDb,
                                                       linkedReleaseFastLevel,
                                                       linkedReleaseSlowLevel,
                                                       linkedReleaseEntryArmed,
                                                       detector,
                                                       ratio);
            gainResult = applyFastTimingThreeToOneAttackDip (gainResult,
                                                             linkedAttackDipFastLevel,
                                                             linkedAttackDipSlowLevel,
                                                             linkedAttackDipSample,
                                                             linkedAttackDipArmed,
                                                             detector,
                                                             ratio);
            gainResult = applySlowInstantPeakControl (gainResult, detector, linkedEnvelope, ratio);
            gainResult = applyAutoAttackShoulderDip (gainResult, detector, linkedEnvelope, ratio);
            heldGainReductionDb = juce::jmax (heldGainReductionDb * 0.995f, gainResult.gainReductionDb);

            blockGainReduction = juce::jmax (blockGainReduction, gainResult.gainReductionDb);
            for (int channel = 0; channel < numChannels; ++channel)
                writeOutputSample (buffer, channel, sampleIndex, ratio, gainResult.gain,
                                   outputGain, timingOutputGain, wet, dry, transientPass, wetBeforeMakeup);
        }

        meterGainReductionDb = blockGainReduction;
    }

    void processDualMono (juce::AudioBuffer<float>& buffer,
                          const juce::AudioBuffer<float>* externalSidechain,
                          const Timing& timing,
                          float hpfCoeff,
                          float outputGain,
                          float timingOutputGain,
                          float wet,
                          float dry,
                          juce::AudioBuffer<float>* wetBeforeMakeup)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto ratio = getRatio();

        auto blockGainReduction = 0.0f;
        for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            auto sampleGainReduction = 0.0f;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto detector = std::abs (getDetectorSample (channel, sampleIndex, buffer, externalSidechain, hpfCoeff));
                auto& envelope = channelEnvelopes[(size_t) channel];
                const auto previousEnvelope = envelope;
                envelope = tickEnvelope (envelope, detector, timing, heldGainReductionDb, ratio);
                auto& previewEnvelope = channelSlowPreviewEnvelopes[(size_t) channel];
                previewEnvelope = usesSlowAttackPreview (ratio) ? tickSlowAttackPreviewEnvelope (previewEnvelope, detector) : 0.0f;
                const auto gainEnvelope = getGainEnvelope (envelope, previewEnvelope, ratio);
                const auto transientPass = tickTransientPass (channelTransientPass[(size_t) channel],
                                                              previousEnvelope,
                                                              detector,
                                                              heldGainReductionDb,
                                                              ratio);

                auto gainResult = applyReleaseEntryMemory (computeGain (gainEnvelope, ratio),
                                                           channelReleaseMemoryDb[(size_t) channel],
                                                           channelReleaseFastLevel[(size_t) channel],
                                                           channelReleaseSlowLevel[(size_t) channel],
                                                           channelReleaseEntryArmed[(size_t) channel],
                                                           detector,
                                                           ratio);
                gainResult = applyFastTimingThreeToOneAttackDip (gainResult,
                                                                 channelAttackDipFastLevel[(size_t) channel],
                                                                 channelAttackDipSlowLevel[(size_t) channel],
                                                                 channelAttackDipSample[(size_t) channel],
                                                                 channelAttackDipArmed[(size_t) channel],
                                                                 detector,
                                                                 ratio);
                gainResult = applySlowInstantPeakControl (gainResult, detector, envelope, ratio);
                gainResult = applyAutoAttackShoulderDip (gainResult, detector, envelope, ratio);
                sampleGainReduction = juce::jmax (sampleGainReduction, gainResult.gainReductionDb);
                writeOutputSample (buffer, channel, sampleIndex, ratio, gainResult.gain,
                                   outputGain, timingOutputGain, wet, dry, transientPass, wetBeforeMakeup);
            }

            blockGainReduction = juce::jmax (blockGainReduction, sampleGainReduction);
            heldGainReductionDb = juce::jmax (heldGainReductionDb * 0.995f, sampleGainReduction);
        }

        meterGainReductionDb = blockGainReduction;
    }

    void writeOutputSample (juce::AudioBuffer<float>& buffer,
                            int channel,
                            int sampleIndex,
                            float ratio,
                            float gain,
                            float outputGain,
                            float timingOutputGain,
                            float wet,
                            float dry,
                            float transientPass,
                            juce::AudioBuffer<float>* wetBeforeMakeup)
    {
        const auto drySample = buffer.getSample (channel, sampleIndex);
        const auto transientDry = dry + wet * transientPass;
        const auto transientWet = wet * (1.0f - transientPass);
        const auto transientLift = 1.0f;
        const auto gainReduced = colourSample (drySample, ratio, getColourAmountForGain (gain)) * gain;
        const auto compressed = gainReduced * outputGain;
        const auto output = (drySample * transientDry + compressed * transientWet) * transientLift;

        const auto voicedOutput = applyOutputBandwidth (channel, output);

        if (wetBeforeMakeup != nullptr)
        {
            // Reconstruct the fully Wet branch while excluding only the
            // user-facing Gain and Blend controls. Hidden Timing calibration,
            // transient pass and bandwidth voicing remain in the measurement.
            const auto wetPreMakeup = (drySample * transientPass
                                     + gainReduced * timingOutputGain * (1.0f - transientPass)) * transientLift;
            wetBeforeMakeup->setSample (channel, sampleIndex,
                                        applyMatchOutputBandwidth (channel, wetPreMakeup));
        }

        buffer.setSample (channel, sampleIndex, voicedOutput);
        outputPeak = juce::jmax (outputPeak, std::abs (voicedOutput));
    }

    void updateOutputBandwidthCoefficient()
    {
        const auto cutoffHz = juce::jlimit (20000.0f, 30000.0f, (float) sampleRate * 0.47f);
        outputBandwidthCoeff = std::exp (-2.0f * juce::MathConstants<float>::pi * cutoffHz / (float) sampleRate);
    }

    float applyOutputBandwidth (int channel, float sample)
    {
        return applyOutputBandwidthToState (outputBandwidthState, channel, sample);
    }

    float applyMatchOutputBandwidth (int channel, float sample)
    {
        return applyOutputBandwidthToState (matchOutputBandwidthState, channel, sample);
    }

    float applyOutputBandwidthToState (std::vector<float>& states, int channel, float sample)
    {
        const auto stateIndex = (size_t) juce::jlimit (0, (int) states.size() - 1, channel);
        auto& state = states[stateIndex];
        state = sample + outputBandwidthCoeff * (state - sample);

        const auto bandwidthAmount = 0.18f;
        return sample + (state - sample) * bandwidthAmount;
    }

    void meterInput (const juce::AudioBuffer<float>& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                inputPeak = juce::jmax (inputPeak, std::abs (buffer.getSample (channel, sample)));
    }

    void ensureChannelStorage (int numChannels)
    {
        const auto channels = (size_t) juce::jmax (1, numChannels);

        if (channelEnvelopes.size() < channels)
            channelEnvelopes.resize (channels, 0.0f);

        if (channelSlowPreviewEnvelopes.size() < channels)
            channelSlowPreviewEnvelopes.resize (channels, 0.0f);

        if (channelTransientPass.size() < channels)
            channelTransientPass.resize (channels, 0.0f);

        if (channelPeakRestore.size() < channels)
            channelPeakRestore.resize (channels, 0.0f);

        if (channelPeakRestoreLockoutSamples.size() < channels)
            channelPeakRestoreLockoutSamples.resize (channels, 0);

        if (channelReleaseMemoryDb.size() < channels)
            channelReleaseMemoryDb.resize (channels, 0.0f);

        if (channelReleaseFastLevel.size() < channels)
            channelReleaseFastLevel.resize (channels, 0.0f);

        if (channelReleaseSlowLevel.size() < channels)
            channelReleaseSlowLevel.resize (channels, 0.0f);

        if (channelReleaseEntryArmed.size() < channels)
            channelReleaseEntryArmed.resize (channels, 1);

        if (channelAttackDipFastLevel.size() < channels)
            channelAttackDipFastLevel.resize (channels, 0.0f);

        if (channelAttackDipSlowLevel.size() < channels)
            channelAttackDipSlowLevel.resize (channels, 0.0f);

        if (channelAttackDipSample.size() < channels)
            channelAttackDipSample.resize (channels, -1);

        if (channelAttackDipArmed.size() < channels)
            channelAttackDipArmed.resize (channels, 1);

        if (sidechainState.size() < channels)
            sidechainState.resize (channels, 0.0f);

        if (lastSidechain.size() < channels)
            lastSidechain.resize (channels, 0.0f);

        if (outputBandwidthState.size() < channels)
            outputBandwidthState.resize (channels, 0.0f);

        if (matchOutputBandwidthState.size() < channels)
            matchOutputBandwidthState.resize (channels, 0.0f);

    }

    DiodeBridgeCompressorParameters parameters;
    DiodeBridgeCompressorMeters meters;
    std::vector<float> channelEnvelopes;
    std::vector<float> channelSlowPreviewEnvelopes;
    std::vector<float> channelTransientPass;
    std::vector<float> channelPeakRestore;
    std::vector<int> channelPeakRestoreLockoutSamples;
    std::vector<float> channelReleaseMemoryDb;
    std::vector<float> channelReleaseFastLevel;
    std::vector<float> channelReleaseSlowLevel;
    std::vector<int> channelReleaseEntryArmed;
    std::vector<float> channelAttackDipFastLevel;
    std::vector<float> channelAttackDipSlowLevel;
    std::vector<int> channelAttackDipSample;
    std::vector<int> channelAttackDipArmed;
    std::vector<float> sidechainState;
    std::vector<float> lastSidechain;
    std::vector<float> outputBandwidthState;
    std::vector<float> matchOutputBandwidthState;
    double sampleRate = 44100.0;
    float outputBandwidthCoeff = 0.0f;
    float linkedEnvelope = 0.0f;
    float linkedSlowPreviewEnvelope = 0.0f;
    float linkedTransientPass = 0.0f;
    float linkedPeakRestore = 0.0f;
    int linkedPeakRestoreLockoutSamples = 0;
    float linkedReleaseMemoryDb = 0.0f;
    float linkedReleaseFastLevel = 0.0f;
    float linkedReleaseSlowLevel = 0.0f;
    int linkedReleaseEntryArmed = 1;
    float linkedAttackDipFastLevel = 0.0f;
    float linkedAttackDipSlowLevel = 0.0f;
    int linkedAttackDipSample = -1;
    int linkedAttackDipArmed = 1;
    float heldGainReductionDb = 0.0f;
    float inputPeak = 0.0f;
    float meterGainReductionDb = 0.0f;
    float outputPeak = 0.0f;
};
