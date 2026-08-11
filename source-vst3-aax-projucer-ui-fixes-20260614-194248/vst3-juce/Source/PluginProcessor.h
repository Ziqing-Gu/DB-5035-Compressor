#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include "DiodeBridgeCompressor.h"

class DB5035AudioProcessor final : public juce::AudioProcessor
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    DB5035AudioProcessor();
    ~DB5035AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS& getValueTreeState() { return parameters; }
    juce::UndoManager& getUndoManager() { return undoManager; }
    DiodeBridgeCompressorMeters getMeters() const;
    DiodeBridgeCompressorMeters consumeMeterPeaks();
    void selectCompareSlot (int slotIndex);
    void copyActiveCompareSlotToOther();
    int getActiveCompareSlot() const { return activeCompareSlot; }
    int getVuMode() const { return vuMode; }
    void setVuMode (int mode);
    int getUiStyle() const { return uiStyle; }
    void setUiStyle (int style);

    static APVTS::ParameterLayout createParameterLayout();

private:
    static constexpr size_t compareParameterCount = 10;
    using CompareSnapshot = std::array<float, compareParameterCount>;

    DiodeBridgeCompressorParameters readParameters() const;
    int readOversamplingIndex() const;
    int getOversamplingFactor() const;
    void prepareOversampling (int oversamplingIndex, int samplesPerBlock, int mainChannels, int sidechainChannels);
    void processOversampledBlock (juce::AudioBuffer<float>& mainBuffer, const juce::AudioBuffer<float>* sidechain);
    void processBlockInternal (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, bool forceHostBypass);
    bool readHostBypass() const;
    void prepareLatencyMatchedBypass (int maximumChannels, int maximumBlockSize);
    void processLatencyMatchedDry (juce::AudioBuffer<float>& buffer, bool writeDelayedSignalToOutput);
    void publishBypassMeters (float inputDb, float outputDb);
    static float measurePeakDb (const juce::AudioBuffer<float>& buffer);
    void initialiseCompareSlotsFromCurrent();
    void captureActiveCompareSlot();
    CompareSnapshot captureCompareSnapshot() const;
    void applyCompareSnapshot (const CompareSnapshot& snapshot);
    juce::ValueTree createCompareStateTree() const;
    void restoreCompareStateTree (const juce::ValueTree& state);

    juce::UndoManager undoManager;
    APVTS parameters;
    std::unique_ptr<juce::PropertiesFile> uiPreferences;
    DiodeBridgeCompressor compressor;
    CompareSnapshot compareSlotA {};
    CompareSnapshot compareSlotB {};
    int activeCompareSlot = 0;
    bool compareSlotsInitialised = false;
    std::unique_ptr<juce::dsp::Oversampling<float>> mainOversampler;
    juce::AudioBuffer<float> oversampledMainBuffer;
    juce::AudioBuffer<float> oversampledSidechainBuffer;
    juce::AudioBuffer<float> bypassDelayBuffer;
    int bypassDelayWritePosition = 0;
    int bypassDelaySamples = 0;
    double preparedSampleRate = 44100.0;
    int preparedBlockSize = 0;
    int preparedMainChannels = 0;
    int preparedSidechainChannels = 0;
    int activeOversamplingIndex = 0;
    bool wasHostBypassed = false;
    std::atomic<float> inputMeterDb { -80.0f };
    std::atomic<float> outputMeterDb { -80.0f };
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<float> pendingInputMeterPeakDb { -80.0f };
    std::atomic<float> pendingOutputMeterPeakDb { -80.0f };
    std::atomic<float> pendingGainReductionPeakDb { 0.0f };
    int vuMode = 0;
    int uiStyle = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DB5035AudioProcessor)
};
