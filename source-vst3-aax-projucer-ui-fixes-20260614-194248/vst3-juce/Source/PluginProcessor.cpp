#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ParamID
{
    constexpr auto compIn = "compIn";
    constexpr auto threshold = "threshold";
    constexpr auto ratio = "ratio";
    constexpr auto externalSidechain = "externalSidechain";
    constexpr auto sidechainHPF = "sidechainHPF";
    constexpr auto makeupGain = "makeupGain";
    constexpr auto timing = "timing";
    constexpr auto linkStereo = "linkStereo";
    constexpr auto fastMode = "fastMode";
    constexpr auto blend = "blend";
    constexpr auto oversampling = "oversampling";
}

namespace
{
    constexpr auto compareStateTag = "CompareSlots";
    constexpr auto compareStateActive = "active";

    constexpr std::array<const char*, 10> compareParameterIds
    {{
        ParamID::compIn,
        ParamID::threshold,
        ParamID::ratio,
        ParamID::externalSidechain,
        ParamID::sidechainHPF,
        ParamID::makeupGain,
        ParamID::timing,
        ParamID::fastMode,
        ParamID::blend,
        ParamID::oversampling
    }};

    void copyBlockToBuffer (const juce::dsp::AudioBlock<float>& source, juce::AudioBuffer<float>& destination)
    {
        const auto numChannels = (int) source.getNumChannels();
        const auto numSamples = (int) source.getNumSamples();
        destination.setSize (numChannels, numSamples, false, false, true);

        for (int channel = 0; channel < numChannels; ++channel)
            destination.copyFrom (channel, 0, source.getChannelPointer ((size_t) channel), numSamples);
    }

    void copyBufferToBlock (const juce::AudioBuffer<float>& source, juce::dsp::AudioBlock<float>& destination)
    {
        const auto numChannels = juce::jmin (source.getNumChannels(), (int) destination.getNumChannels());
        const auto numSamples = juce::jmin (source.getNumSamples(), (int) destination.getNumSamples());

        for (int channel = 0; channel < numChannels; ++channel)
            std::copy_n (source.getReadPointer (channel), numSamples, destination.getChannelPointer ((size_t) channel));
    }

    void stretchDetectorWithoutFiltering (const juce::AudioBuffer<float>& source,
                                          juce::AudioBuffer<float>& destination,
                                          int factor,
                                          int targetNumSamples)
    {
        const auto numChannels = source.getNumChannels();
        destination.setSize (numChannels, targetNumSamples, false, false, true);

        if (numChannels == 0 || source.getNumSamples() == 0 || targetNumSamples == 0)
        {
            destination.clear();
            return;
        }

        const auto safeFactor = juce::jmax (1, factor);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* out = destination.getWritePointer (channel);
            const auto* in = source.getReadPointer (channel);

            for (int sample = 0; sample < targetNumSamples; ++sample)
                out[sample] = in[juce::jmin (source.getNumSamples() - 1, sample / safeFactor)];
        }
    }
}

DB5035AudioProcessor::DB5035AudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, &undoManager, "Parameters", createParameterLayout())
{
}

void DB5035AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preparedSampleRate = sampleRate;
    preparedBlockSize = samplesPerBlock;
    prepareOversampling (readOversamplingIndex(),
                         samplesPerBlock,
                         juce::jmax (1, getMainBusNumOutputChannels()),
                         juce::jmax (1, getBusCount (true) > 1 ? getChannelCountOfBus (true, 1) : getMainBusNumInputChannels()));
}

void DB5035AudioProcessor::releaseResources()
{
    compressor.reset();
    if (mainOversampler != nullptr)
        mainOversampler->reset();
}

bool DB5035AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getChannelSet (true, 0);
    const auto& output = layouts.getChannelSet (false, 0);

    if (input != output)
        return false;

    if (input != juce::AudioChannelSet::mono() && input != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto& sidechain = layouts.getChannelSet (true, 1);

        if (! sidechain.isDisabled()
            && sidechain != juce::AudioChannelSet::mono()
            && sidechain != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void DB5035AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto mainBuffer = getBusBuffer (buffer, false, 0);
    const auto totalInputChannels = getBusBuffer (buffer, true, 0).getNumChannels();
    const auto totalOutputChannels = mainBuffer.getNumChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        mainBuffer.clear (channel, 0, mainBuffer.getNumSamples());

    const auto useSidechain = getBusCount (true) > 1 && getBus (true, 1)->isEnabled();
    auto sidechainBuffer = useSidechain ? getBusBuffer (buffer, true, 1) : juce::AudioBuffer<float>();
    const auto* sidechain = useSidechain ? &sidechainBuffer : nullptr;

    const auto currentParameters = readParameters();
    compressor.setParameters (currentParameters);

    const auto requestedOversamplingIndex = readOversamplingIndex();
    const auto sidechainChannels = sidechain != nullptr ? sidechain->getNumChannels() : mainBuffer.getNumChannels();
    if (requestedOversamplingIndex != activeOversamplingIndex
        || mainBuffer.getNumSamples() > preparedBlockSize
        || mainBuffer.getNumChannels() > preparedMainChannels
        || sidechainChannels > preparedSidechainChannels)
    {
        prepareOversampling (requestedOversamplingIndex,
                             juce::jmax (mainBuffer.getNumSamples(), preparedBlockSize),
                             juce::jmax (1, mainBuffer.getNumChannels()),
                             juce::jmax (1, sidechainChannels));
        compressor.setParameters (currentParameters);
    }

    const auto* activeSidechain = currentParameters.externalSidechain ? sidechain : nullptr;
    if (getOversamplingFactor() <= 1)
        compressor.process (mainBuffer, activeSidechain);
    else
        processOversampledBlock (mainBuffer, activeSidechain);

    const auto meters = compressor.getMeters();
    inputMeterDb.store (meters.inputDb, std::memory_order_relaxed);
    outputMeterDb.store (meters.outputDb, std::memory_order_relaxed);
    gainReductionDb.store (meters.gainReductionDb, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* DB5035AudioProcessor::createEditor()
{
    return new DB5035AudioProcessorEditor (*this);
}

bool DB5035AudioProcessor::hasEditor() const
{
    return true;
}

const juce::String DB5035AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DB5035AudioProcessor::acceptsMidi() const
{
    return false;
}

bool DB5035AudioProcessor::producesMidi() const
{
    return false;
}

bool DB5035AudioProcessor::isMidiEffect() const
{
    return false;
}

double DB5035AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DB5035AudioProcessor::getNumPrograms()
{
    return 1;
}

int DB5035AudioProcessor::getCurrentProgram()
{
    return 0;
}

void DB5035AudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String DB5035AudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void DB5035AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void DB5035AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto stateTree = parameters.copyState();
    stateTree.appendChild (createCompareStateTree(), nullptr);
    stateTree.setProperty ("vuMode", vuMode, nullptr);

    if (auto state = stateTree.createXml())
        copyXmlToBinary (*state, destData);
}

void DB5035AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary (data, sizeInBytes))
        if (state->hasTagName (parameters.state.getType()))
        {
            auto stateTree = juce::ValueTree::fromXml (*state);
            const auto compareState = stateTree.getChildWithName (compareStateTag);

            if (compareState.isValid())
                stateTree.removeChild (compareState, nullptr);

            parameters.replaceState (stateTree);

            if (auto* compInParam = parameters.getParameter (ParamID::compIn))
            {
                compInParam->beginChangeGesture();
                compInParam->setValueNotifyingHost (compInParam->convertTo0to1 (1.0f));
                compInParam->endChangeGesture();
            }

            if (compareState.isValid())
                restoreCompareStateTree (compareState);
            else
                initialiseCompareSlotsFromCurrent();

            if (stateTree.hasProperty ("vuMode"))
                vuMode = (int) stateTree.getProperty ("vuMode");
        }
}

DiodeBridgeCompressorMeters DB5035AudioProcessor::getMeters() const
{
    return {
        inputMeterDb.load (std::memory_order_relaxed),
        outputMeterDb.load (std::memory_order_relaxed),
        gainReductionDb.load (std::memory_order_relaxed)
    };
}

void DB5035AudioProcessor::selectCompareSlot (int slotIndex)
{
    initialiseCompareSlotsFromCurrent();
    const auto targetSlot = juce::jlimit (0, 1, slotIndex);

    if (targetSlot == activeCompareSlot)
        return;

    captureActiveCompareSlot();
    activeCompareSlot = targetSlot;
    applyCompareSnapshot (activeCompareSlot == 0 ? compareSlotA : compareSlotB);
}

void DB5035AudioProcessor::copyCompareAToB()
{
    initialiseCompareSlotsFromCurrent();

    if (activeCompareSlot == 0)
        captureActiveCompareSlot();

    compareSlotB = compareSlotA;
    activeCompareSlot = 1;
    applyCompareSnapshot (compareSlotB);
}

void DB5035AudioProcessor::initialiseCompareSlotsFromCurrent()
{
    if (compareSlotsInitialised)
        return;

    compareSlotA = captureCompareSnapshot();
    compareSlotB = compareSlotA;
    activeCompareSlot = 0;
    compareSlotsInitialised = true;
}

void DB5035AudioProcessor::captureActiveCompareSlot()
{
    auto snapshot = captureCompareSnapshot();

    if (activeCompareSlot == 0)
        compareSlotA = snapshot;
    else
        compareSlotB = snapshot;
}

DB5035AudioProcessor::CompareSnapshot DB5035AudioProcessor::captureCompareSnapshot() const
{
    CompareSnapshot snapshot {};

    for (size_t index = 0; index < compareParameterIds.size(); ++index)
        if (auto* parameter = parameters.getParameter (compareParameterIds[index]))
            snapshot[index] = parameter->getValue();

    return snapshot;
}

void DB5035AudioProcessor::applyCompareSnapshot (const CompareSnapshot& snapshot)
{
    for (size_t index = 0; index < compareParameterIds.size(); ++index)
    {
        if (auto* parameter = parameters.getParameter (compareParameterIds[index]))
        {
            const auto value = juce::jlimit (0.0f, 1.0f, snapshot[index]);
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (value);
            parameter->endChangeGesture();
        }
    }
}

juce::ValueTree DB5035AudioProcessor::createCompareStateTree() const
{
    auto slotA = compareSlotA;
    auto slotB = compareSlotB;
    const auto current = captureCompareSnapshot();

    if (compareSlotsInitialised)
    {
        if (activeCompareSlot == 0)
            slotA = current;
        else
            slotB = current;
    }
    else
    {
        slotA = current;
        slotB = current;
    }

    juce::ValueTree compareState { compareStateTag };
    compareState.setProperty (compareStateActive, activeCompareSlot, nullptr);

    auto saveSlot = [] (const char* name, const CompareSnapshot& snapshot)
    {
        juce::ValueTree slot { name };

        for (size_t index = 0; index < snapshot.size(); ++index)
            slot.setProperty (juce::String ((int) index), snapshot[index], nullptr);

        return slot;
    };

    compareState.appendChild (saveSlot ("A", slotA), nullptr);
    compareState.appendChild (saveSlot ("B", slotB), nullptr);
    return compareState;
}

void DB5035AudioProcessor::restoreCompareStateTree (const juce::ValueTree& state)
{
    if (! state.isValid())
        return;

    auto loadSlot = [] (const juce::ValueTree& slot, CompareSnapshot& snapshot)
    {
        if (! slot.isValid())
            return;

        for (size_t index = 0; index < snapshot.size(); ++index)
            snapshot[index] = (float) slot.getProperty (juce::String ((int) index), snapshot[index]);
    };

    compareSlotA = captureCompareSnapshot();
    compareSlotB = compareSlotA;
    loadSlot (state.getChildWithName ("A"), compareSlotA);
    loadSlot (state.getChildWithName ("B"), compareSlotB);
    activeCompareSlot = juce::jlimit (0, 1, (int) state.getProperty (compareStateActive, 0));
    compareSlotsInitialised = true;
}

DB5035AudioProcessor::APVTS::ParameterLayout DB5035AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::compIn, 1 }, "Comp In", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::threshold, 1 }, "Threshold",
        juce::NormalisableRange<float> (-25.0f, 20.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::ratio, 1 }, "Ratio",
        juce::StringArray { "1.5:1", "2:1", "3:1", "4:1", "6:1", "8:1" }, 2));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::externalSidechain, 1 }, "External S/C", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::sidechainHPF, 1 }, "S/C HPF",
        juce::NormalisableRange<float> (20.0f, 300.0f, 1.0f), 20.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::makeupGain, 1 }, "Gain",
        juce::NormalisableRange<float> (-6.0f, 20.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::timing, 1 }, "Timing",
        juce::StringArray { "FAST", "MF", "MED", "MS", "SLOW", "AUTO" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::linkStereo, 1 }, "Link", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::fastMode, 1 }, "Fast", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::blend, 1 }, "Blend",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::oversampling, 1 }, "Oversampling",
        juce::StringArray { "1x", "2x", "4x", "8x" }, 0));

    return { params.begin(), params.end() };
}

DiodeBridgeCompressorParameters DB5035AudioProcessor::readParameters() const
{
    auto load = [this] (const char* id)
    {
        if (auto* value = parameters.getRawParameterValue (id))
            return value->load (std::memory_order_relaxed);

        jassertfalse;
        return 0.0f;
    };

    auto loadBool = [this] (const char* id)
    {
        if (auto* value = parameters.getRawParameterValue (id))
            return value->load (std::memory_order_relaxed) >= 0.5f;

        jassertfalse;
        return false;
    };

    return {
        loadBool (ParamID::compIn),
        load (ParamID::threshold),
        (int) std::round (load (ParamID::ratio)),
        loadBool (ParamID::externalSidechain),
        load (ParamID::sidechainHPF),
        load (ParamID::makeupGain),
        (int) std::round (load (ParamID::timing)),
        true,
        loadBool (ParamID::fastMode),
        load (ParamID::blend)
    };
}

int DB5035AudioProcessor::readOversamplingIndex() const
{
    if (auto* value = parameters.getRawParameterValue (ParamID::oversampling))
        return (int) juce::jlimit (0.0f, 3.0f, std::round (value->load (std::memory_order_relaxed)));

    jassertfalse;
    return 0;
}

int DB5035AudioProcessor::getOversamplingFactor() const
{
    return 1 << juce::jlimit (0, 3, activeOversamplingIndex);
}

void DB5035AudioProcessor::prepareOversampling (int oversamplingIndex,
                                                int samplesPerBlock,
                                                int mainChannels,
                                                int sidechainChannels)
{
    activeOversamplingIndex = juce::jlimit (0, 3, oversamplingIndex);
    preparedBlockSize = juce::jmax (1, samplesPerBlock);
    preparedMainChannels = juce::jmax (1, mainChannels);
    preparedSidechainChannels = juce::jmax (1, sidechainChannels);

    mainOversampler.reset();

    if (activeOversamplingIndex > 0)
    {
        mainOversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            (size_t) preparedMainChannels,
            (size_t) activeOversamplingIndex,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true,
            true);

        mainOversampler->initProcessing ((size_t) preparedBlockSize);
        setLatencySamples ((int) std::round (mainOversampler->getLatencyInSamples()));
    }
    else
    {
        setLatencySamples (0);
    }

    compressor.prepare (preparedSampleRate * (double) getOversamplingFactor(), preparedMainChannels);
}

void DB5035AudioProcessor::processOversampledBlock (juce::AudioBuffer<float>& mainBuffer,
                                                    const juce::AudioBuffer<float>* sidechain)
{
    if (mainOversampler == nullptr)
    {
        compressor.process (mainBuffer, sidechain);
        return;
    }

    auto mainInputBlock = juce::dsp::AudioBlock<const float> (mainBuffer);
    auto upsampledMainBlock = mainOversampler->processSamplesUp (mainInputBlock);

    if (upsampledMainBlock.getNumSamples() == 0)
    {
        compressor.process (mainBuffer, sidechain);
        return;
    }

    copyBlockToBuffer (upsampledMainBlock, oversampledMainBuffer);

    const juce::AudioBuffer<float>* oversampledSidechain = nullptr;
    if (sidechain != nullptr && sidechain->getNumSamples() > 0)
    {
        stretchDetectorWithoutFiltering (*sidechain,
                                         oversampledSidechainBuffer,
                                         getOversamplingFactor(),
                                         oversampledMainBuffer.getNumSamples());
        oversampledSidechain = &oversampledSidechainBuffer;
    }

    compressor.process (oversampledMainBuffer, oversampledSidechain);
    copyBufferToBlock (oversampledMainBuffer, upsampledMainBlock);

    auto mainOutputBlock = juce::dsp::AudioBlock<float> (mainBuffer);
    mainOversampler->processSamplesDown (mainOutputBlock);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DB5035AudioProcessor();
}
