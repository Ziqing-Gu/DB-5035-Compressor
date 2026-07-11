#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>
#include "PluginProcessor.h"

class PanelConstrainer : public juce::ComponentBoundsConstrainer
{
public:
    void checkBounds (juce::Rectangle<int>& bounds,
                      const juce::Rectangle<int>&,
                      const juce::Rectangle<int>& limits,
                      bool, bool, bool, bool) override
    {
        bounds = bounds.constrainedWithin (limits);
        const auto scale = (float) bounds.getWidth() / (float) 1200;
        const auto h = 28 + juce::roundToInt ((float) 199 * scale);
        bounds = bounds.withHeight (h);
    }
};

class DB5035AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit DB5035AudioProcessorEditor (DB5035AudioProcessor&);
    ~DB5035AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class HardwareLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider (juce::Graphics& g,
                               int x,
                               int y,
                               int width,
                               int height,
                               float sliderPos,
                               float rotaryStartAngle,
                               float rotaryEndAngle,
                               juce::Slider& slider) override;

        void drawButtonBackground (juce::Graphics& g,
                                   juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    };

    class FlatCommandLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawButtonBackground (juce::Graphics& g,
                                   juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    };

    class ScrewLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawButtonBackground (juce::Graphics& g,
                                   juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    };

    class VUMeter final : public juce::Component
    {
    public:
        enum class Mode { input, output, reduction };
        void setMode (Mode newMode);
        Mode getMode() const { return mode; }
        void setValue (float newValueDb, float newMinimumDb, float newMaximumDb);
        void setPeakHold (float newPeakDb);
        void paint (juce::Graphics& g) override;
        void mouseDown (const juce::MouseEvent& event) override;

        std::function<void()> onResetPeak;

    private:
        static float dbToNormalised (float dB)
        {
            constexpr auto meterMin = -24.0f;
            constexpr auto meterMax = 3.0f;
            const auto levelMin = std::pow (10.0f, meterMin / 20.0f);
            const auto levelMax = std::pow (10.0f, meterMax / 20.0f);
            const auto level = std::pow (10.0f, juce::jlimit (meterMin, meterMax, dB) / 20.0f);
            return (level - levelMin) / (levelMax - levelMin);
        }

        float valueDb = -80.0f;
        float minimumDb = -80.0f;
        float maximumDb = 6.0f;
        float heldPeakDb = 0.0f;
        bool showPeakHold = false;
        Mode mode = Mode::output;
        float smoothedAngle = 0.0f;
        float targetAngle = 0.0f;
    };

    class ParameterSlider final : public juce::Slider
    {
    public:
        void setValueLabel (juce::Label* labelToUse);

        void mouseDown (const juce::MouseEvent& event) override;
        void mouseDoubleClick (const juce::MouseEvent& event) override;

        bool editable = true;

    private:
        juce::Label* valueLabel = nullptr;
    };

    class KnobComponent final : public juce::Component
    {
    public:
        KnobComponent()
        {
            setPaintingIsUnclipped (true);
            slider.setPaintingIsUnclipped (true);
            addAndMakeVisible (slider);
            addAndMakeVisible (nameLabel);
            addChildComponent (valueLabel);
        }

        void paint (juce::Graphics& g) override;
        void resized() override;

        ParameterSlider slider;
        juce::Label nameLabel;
        juce::Label valueLabel;
        juce::Colour knobColour;
        bool stepped = false;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::StringArray scaleLabels;
        float scaleStartAngle = 0.0f;
        float scaleEndAngle = 0.0f;
        int scaleTickCount = 0;
        int labelYOffset = 0;
    };

    struct ButtonControl
    {
        juce::TextButton button;
        juce::Label name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    struct CommandButtonControl
    {
        juce::TextButton button;
        juce::Label name;
    };

    class HelpOverlay final : public juce::Component
    {
    public:
        HelpOverlay();

        void paint (juce::Graphics& g) override;
        void resized() override;

        std::function<void()> onClose;

    private:
        class HelpContent final : public juce::Component
        {
        public:
            HelpContent();
            void paint (juce::Graphics& g) override;
        };

        juce::TextButton closeButton;
        HelpContent helpContent;
        juce::Viewport helpViewport;
    };

    void timerCallback() override;
    void configureKnob (KnobComponent& control,
                        const juce::String& parameterId,
                        const juce::String& labelText,
                        juce::Colour knobColour,
                        float startAngle,
                        float endAngle,
                        bool stepped);
    void configureButton (ButtonControl& control, const juce::String& parameterId, const juce::String& labelText);
    void configureCommandButton (CommandButtonControl& control, const juce::String& labelText);
    void layoutContent();
    void layoutCommandStrip();
    void layoutButton (ButtonControl& control, juce::Rectangle<int> bounds);

    static void drawSignature (juce::Graphics& g);

    void updateValueLabels();
    void updateUndoRedoButtons();
    void updateCompareButtons();
    void updateOversamplingButton();

    DB5035AudioProcessor& audioProcessor;
    PanelConstrainer panelConstrainer;
    HardwareLookAndFeel hardwareLookAndFeel;
    FlatCommandLookAndFeel flatCommandLookAndFeel;
    ScrewLookAndFeel screwLookAndFeel;
    juce::Component scaledContent;
    juce::ImageComponent panelOverlay;
    struct TextOverlay final : public juce::Component
    {
        void paint (juce::Graphics& g) override;
        bool hitTest (int, int) override { return false; }
    } textOverlay;
    std::array<KnobComponent, 6> knobs;
    std::array<juce::String, 6> knobParameterIds;
    std::array<ButtonControl, 3> buttons;
    std::array<juce::String, 3> buttonParameterIds;
    std::array<CommandButtonControl, 2> historyButtons;
    std::array<CommandButtonControl, 3> compareButtons;
    CommandButtonControl helpButton;
    CommandButtonControl oversamplingButton;
    HelpOverlay helpOverlay;
    VUMeter vuMeter;
    juce::TextButton vuModeButton;
    float gainReductionPeakHoldDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DB5035AudioProcessorEditor)
};
