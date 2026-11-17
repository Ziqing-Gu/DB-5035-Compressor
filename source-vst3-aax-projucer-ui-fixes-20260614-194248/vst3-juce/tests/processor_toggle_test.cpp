#include "PluginProcessor.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

struct DB5035ProcessorTestAccess
{
    static size_t blocks (const DB5035AudioProcessor& p) { return p.loudnessMatch.getBlockCount(); }
    static std::vector<unsigned char> measurementSnapshot (const DB5035AudioProcessor& p)
    {
        const auto* bytes = reinterpret_cast<const unsigned char*> (&p.loudnessMatch);
        std::vector<unsigned char> result (bytes, bytes + sizeof (p.loudnessMatch));
        for (const auto* b : { &p.matchDryBuffer, &p.matchWetBuffer,
                              &p.oversampledMatchDryBuffer, &p.oversampledMatchWetBuffer })
            for (int c = 0; c < b->getNumChannels(); ++c)
            {
                const auto* start = reinterpret_cast<const unsigned char*> (b->getReadPointer (c));
                result.insert (result.end(), start, start + sizeof (float) * b->getNumSamples());
            }
        return result;
    }
};

namespace
{
int checks = 0;
void check (bool ok, const char* message)
{
    ++checks;
    if (! ok) throw std::runtime_error (message);
}
void set (DB5035AudioProcessor& p, const char* id, float plain)
{
    auto* param = p.getValueTreeState().getParameter (id);
    if (param == nullptr) throw std::runtime_error ("missing parameter");
    param->setValueNotifyingHost (param->convertTo0to1 (plain));
}
float gain (DB5035AudioProcessor& p)
{
    return p.getValueTreeState().getRawParameterValue ("makeupGain")->load();
}
struct PlayHead : juce::AudioPlayHead
{
    int64_t sample = 0;
    bool playing = true;
    mutable int reads = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        ++reads;
        PositionInfo info;
        info.setIsPlaying (playing);
        info.setTimeInSamples (sample);
        return info;
    }
};
void fill (juce::AudioBuffer<float>& audio, int64_t start, double rate)
{
    for (int c = 0; c < audio.getNumChannels(); ++c)
        for (int n = 0; n < audio.getNumSamples(); ++n)
            audio.setSample (c, n, 0.3f * (float) std::sin ((start + n) * (c + 1) * 700.0 * 6.283185307179586 / rate));
}
void block (DB5035AudioProcessor& p, PlayHead& head, int size = 480)
{
    juce::AudioBuffer<float> b (2, size);
    fill (b, head.sample, 48000.0);
    juce::MidiBuffer midi;
    p.processBlock (b, midi);
    head.sample += size;
}
void stateTests()
{
    DB5035AudioProcessor p;
    check (p.isLoudnessMeasurementEnabled(), "default must be ON");
    const auto& params = p.getParameters();
    bool auOrder = params.size() == 13 && params.getLast()->getVersionHint() == 2;
    for (int i = 0; i < params.size() - 1; ++i)
        auOrder = auOrder && params[i]->getVersionHint() == 1;
    check (auOrder, "new parameter must follow old parameters in both VST3 and AU ordering");
    juce::MemoryBlock on, off;
    p.getStateInformation (on);
    set (p, "loudnessMeasurement", 0);
    p.getStateInformation (off);
    p.setStateInformation (on.getData(), (int) on.getSize());
    check (p.isLoudnessMeasurementEnabled(), "ON round trip failed");
    p.setStateInformation (off.getData(), (int) off.getSize());
    check (! p.isLoudnessMeasurementEnabled(), "OFF round trip failed");
    auto xml = juce::AudioProcessor::getXmlFromBinary (on.getData(), (int) on.getSize());
    auto legacy = juce::ValueTree::fromXml (*xml);
    legacy.removeChild (legacy.getChildWithProperty ("id", "loudnessMeasurement"), nullptr);
    legacy.removeProperty ("loudnessMeasurement", nullptr);
    juce::MemoryBlock legacyBytes;
    juce::AudioProcessor::copyXmlToBinary (*legacy.createXml(), legacyBytes);
    p.setStateInformation (legacyBytes.getData(), (int) legacyBytes.getSize());
    check (p.isLoudnessMeasurementEnabled(), "legacy session loaded after OFF must default ON");
}
void lifecycleTests()
{
    DB5035AudioProcessor p;
    PlayHead head;
    p.setPlayHead (&head);
    p.prepareToPlay (48000, 480);
    for (int i = 0; i < 40; ++i) block (p, head);
    check (DB5035ProcessorTestAccess::blocks (p) == 1, "first 400 ms must form one block, without a second reset");
    check (p.hasMatchData(), "ON playback must enable Match when data is ready");
    set (p, "loudnessMeasurement", 0);
    auto frozen = DB5035ProcessorTestAccess::measurementSnapshot (p);
    auto oldGain = gain (p);
    auto reads = head.reads;
    for (int i = 0; i < 100; ++i) block (p, head);
    check (! p.hasMatchData() && ! p.isMatchMeasuring(), "OFF must disable Match and measuring");
    check (! p.applyLoudnessMatch() && gain (p) == oldGain, "OFF Match must not write Gain");
    check (reads == head.reads, "OFF must not poll the playhead");
    check (frozen == DB5035ProcessorTestAccess::measurementSnapshot (p), "OFF must freeze measurement state and buffers");
    set (p, "loudnessMeasurement", 1);
    check (! p.hasMatchData(), "ON must not expose stale result");
    for (int i = 0; i < 39; ++i) block (p, head);
    check (! p.hasMatchData() && DB5035ProcessorTestAccess::blocks (p) == 0, "ON must start a fresh 400 ms window");
    block (p, head);
    check (p.hasMatchData(), "new ON measurement must become ready at 400 ms");
    const auto target = p.getMatchGainDb();
    check (p.applyLoudnessMatch() && std::abs (gain (p) - target) <= 0.011f, "single-click Match must apply target Gain");
    for (int i = 0; i < 40; ++i) block (p, head);
    check (p.hasMatchData(), "Match must begin a new measurement");
    head.playing = false;
    frozen = DB5035ProcessorTestAccess::measurementSnapshot (p);
    for (int i = 0; i < 20; ++i) block (p, head);
    check (frozen == DB5035ProcessorTestAccess::measurementSnapshot (p), "stopped transport must not accumulate");
    juce::MemoryBlock sameOn;
    p.getStateInformation (sameOn);
    p.setStateInformation (sameOn.getData(), (int) sameOn.getSize());
    check (! p.hasMatchData(), "preset restore must discard old measurement even when toggle stays ON");
    p.releaseResources();
}
void audioTests()
{
    for (int os = 0; os < 4; ++os)
        for (int channels : { 1, 2 })
            for (double rate : { 44100.0, 48000.0, 96000.0 })
            {
                DB5035AudioProcessor on, off;
                PlayHead a, b;
                for (auto* p : { &on, &off })
                {
                    p->setPlayConfigDetails (channels, channels, rate, 256);
                    set (*p, "oversampling", (float) os);
                    set (*p, "makeupGain", 2.37f);
                    set (*p, "blend", 37.0f);
                    set (*p, "threshold", -20.0f);
                    p->prepareToPlay (rate, 256);
                }
                on.setPlayHead (&a); off.setPlayHead (&b);
                set (off, "loudnessMeasurement", 0);
                auto frozen = DB5035ProcessorTestAccess::measurementSnapshot (off);
                bool identical = true;
                juce::MidiBuffer midi;
                for (int n = 0; n < 200; ++n)
                {
                    juce::AudioBuffer<float> x (channels, 256), y (channels, 256);
                    fill (x, a.sample, rate); y.makeCopyOf (x);
                    if (n == 100) set (off, "loudnessMeasurement", 1);
                    if (n == 150) set (off, "loudnessMeasurement", 0);
                    on.processBlock (x, midi); off.processBlock (y, midi);
                    if (n == 99)
                        check (frozen == DB5035ProcessorTestAccess::measurementSnapshot (off),
                               "OFF measurement buffers/filters changed in audio matrix");
                    for (int c = 0; c < channels; ++c)
                        identical = identical && std::memcmp (x.getReadPointer (c), y.getReadPointer (c), 256 * sizeof (float)) == 0;
                    a.sample += 256; b.sample += 256;
                }
                check (identical, "ON/OFF/toggling changed audio output");
                check (gain (on) == gain (off), "toggle changed Gain");
                on.releaseResources(); off.releaseResources();
            }
}

void editorTests()
{
    DB5035AudioProcessor p;
    const auto oldStyle = p.getUiStyle();
    PlayHead head;
    p.setPlayHead (&head);
    p.prepareToPlay (48000, 480);
    for (int style : { 0, 1 })
    {
        p.setUiStyle (style);
        for (bool enabled : { false, true })
        {
            set (p, "loudnessMeasurement", enabled ? 1.0f : 0.0f);
            for (int i = 0; i < 40; ++i) block (p, head);
            std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
            juce::TextButton* toggle = nullptr;
            juce::TextButton* match = nullptr;
            for (auto* child : editor->getChildren())
                if (auto* button = dynamic_cast<juce::TextButton*> (child))
                {
                    if (button->getButtonText().startsWith ("LUFS ")) toggle = button;
                    if (button->getButtonText() == "MATCH"
                        || button->getButtonText().startsWith ("M ")) match = button;
                }
            check (toggle != nullptr && match != nullptr, "both skins must contain the toggle and Match");
            check (toggle->getToggleState() == enabled, "toggle UI disagrees with saved parameter");
            check (match->isEnabled() == enabled, "Match must be disabled OFF and usable with ready ON data");
            for (int width : { 400, 640, 1120, 1200, 1920 })
            {
                editor->setSize (width, style == 0 ? width * 450 / 1120 : 28 + width * 199 / 1200);
                std::vector<juce::Rectangle<int>> bounds;
                for (auto* child : editor->getChildren())
                    if (dynamic_cast<juce::TextButton*> (child) != nullptr && child->isVisible())
                        bounds.push_back (child->getBoundsInParent());
                bool fits = true;
                for (size_t i = 0; i < bounds.size(); ++i)
                {
                    fits = fits && bounds[i].getX() >= 0 && bounds[i].getRight() <= width;
                    for (size_t j = i + 1; j < bounds.size(); ++j)
                        fits = fits && ! bounds[i].intersects (bounds[j]);
                }
                check (fits, "command buttons overlap or clip at supported window width");
            }
        }
    }
    p.setUiStyle (oldStyle);
    p.releaseResources();
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        stateTests(); lifecycleTests(); audioTests(); editorTests();
        std::cout << "PASS: " << checks << " processor runtime checks\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL after " << checks << " checks: " << e.what() << '\n';
        return 1;
    }
}
