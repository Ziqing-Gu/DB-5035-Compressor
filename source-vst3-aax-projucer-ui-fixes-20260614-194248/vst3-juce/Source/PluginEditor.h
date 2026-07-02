#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>
#include "PluginProcessor.h"

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

    private:
        juce::Label* valueLabel = nullptr;
    };

    struct KnobControl
    {
        ParameterSlider slider;
        juce::Label name;
        juce::Label value;
        juce::Colour knobColour;
        bool stepped = false;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
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
    void configureKnob (KnobControl& control,
                        const juce::String& parameterId,
                        const juce::String& labelText,
                        juce::Colour knobColour,
                        bool stepped);
    void configureButton (ButtonControl& control, const juce::String& parameterId, const juce::String& labelText);
    void configureCommandButton (CommandButtonControl& control, const juce::String& labelText);
    void layoutContent();
    void layoutKnob (KnobControl& control, juce::Rectangle<int> bounds);
    void layoutButton (ButtonControl& control, juce::Rectangle<int> bounds);
    void layoutCommandButton (CommandButtonControl& control, juce::Rectangle<int> bounds);
    void drawHardwareFrame (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawSignature (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawKnobScale (juce::Graphics& g,
                        juce::Rectangle<int> bounds,
                        const juce::StringArray& labels,
                        float startAngle,
                        float endAngle,
                        bool majorLabels);
    void updateValueLabels();
    void updateUndoRedoButtons();
    void updateCompareButtons();
    void updateOversamplingButton();

    DB5035AudioProcessor& audioProcessor;
    HardwareLookAndFeel hardwareLookAndFeel;
    juce::Component scaledContent;
    std::array<KnobControl, 6> knobs;
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
