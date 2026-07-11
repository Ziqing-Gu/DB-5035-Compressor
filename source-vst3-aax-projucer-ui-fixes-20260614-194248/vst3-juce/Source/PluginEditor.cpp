#include "PluginEditor.h"
#include "BinaryData.h"

#include <cmath>
#include <cstdlib>

namespace
{
    const auto background = juce::Colour (0xff12120f);
    const auto panel = juce::Colour (0xff2d3945);
    const auto panelTop = juce::Colour (0xff3d4955);
    const auto panelDark = juce::Colour (0xff1d2935);
    const auto line = juce::Colours::white;
    const auto text = juce::Colours::white;
    const auto muted = juce::Colours::white;
    const auto cream = juce::Colour (0xffe6dfc5);
    const auto blackKnob = juce::Colour (0xff333f54);
    const auto redKnob = juce::Colour (0xffa23744);
    const auto creamKnob = juce::Colour (0xffccd1db);
    const auto amber = juce::Colour (0xffffcc22);
    const auto green = juce::Colour (0xff22ee22);

    constexpr auto rotaryStart = juce::MathConstants<float>::pi * 20.f / 30.f;
    constexpr auto rotaryEnd = juce::MathConstants<float>::pi * 70.f / 30.f;
    constexpr int designWidth = 1200;
    constexpr int designHeight = 199;
    constexpr int commandStripHeight = 28;

    float getContentScale (juce::Rectangle<int> bounds)
    {
        return (float) bounds.getWidth() / (float) designWidth;
    }

    juce::AffineTransform getContentTransform (juce::Rectangle<int> bounds)
    {
        const auto scale = getContentScale (bounds);
        return juce::AffineTransform::scale (scale).translated (0.0f, (float) commandStripHeight);
    }

    juce::String uiTypefaceName()
    {
       #if JUCE_MAC
        return "PingFang SC";
       #elif JUCE_WINDOWS
        return "Microsoft YaHei UI";
       #else
        return "Noto Sans CJK SC";
       #endif
    }

    juce::String monoTypefaceName()
    {
       #if JUCE_MAC
        return "Menlo";
       #elif JUCE_WINDOWS
        return "Consolas";
       #else
        return "Noto Sans Mono CJK SC";
       #endif
    }

    juce::String signatureTypefaceName()
    {
       #if JUCE_MAC
        return "Xingkai SC";
       #elif JUCE_WINDOWS
        return juce::String::fromUTF8 ("\xe5\x8d\x8e\xe6\x96\x87\xe8\xa1\x8c\xe6\xa5\xb7");
       #else
        return uiTypefaceName();
       #endif
    }

    juce::Typeface::Ptr sourceSansRegular()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (BinaryData::OpenSansRegular_ttf, BinaryData::OpenSansRegular_ttfSize);
        return typeface;
    }

    juce::Typeface::Ptr sourceSansLightItalic()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor (BinaryData::OpenSansLightItalic_ttf, BinaryData::OpenSansLightItalic_ttfSize);
        return typeface;
    }

    juce::Font sourceSansFont (float size, int styleFlags = juce::Font::plain)
    {
        auto f = juce::Font (sourceSansRegular());
        f.setHeight (size * 1.2f);
        f.setStyleFlags (styleFlags);
        return f;
    }

    juce::Font sourceSansLightItalicFont (float size)
    {
        auto f = juce::Font (sourceSansLightItalic());
        f.setHeight (size * 1.2f);
        return f;
    }

    juce::FontOptions uiFont (float size, int styleFlags = juce::Font::plain)
    {
        return juce::FontOptions (uiTypefaceName(), size, styleFlags);
    }

    juce::FontOptions monoFont (float size, int styleFlags = juce::Font::plain)
    {
        return juce::FontOptions (monoTypefaceName(), size, styleFlags);
    }

    void styleLabel (juce::Label& label, const juce::String& labelText, juce::Justification justification = juce::Justification::centred)
    {
        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (justification);
        label.setColour (juce::Label::textColourId, muted);
        label.setFont (sourceSansFont (11.3f));
    }

    juce::String formatValue (const juce::RangedAudioParameter& parameter)
    {
        const auto plain = parameter.convertFrom0to1 (parameter.getValue());
        const auto id = parameter.getParameterID();

        if (id == "ratio")
        {
            static const juce::StringArray ratios { "1.5:1", "2:1", "3:1", "4:1", "6:1", "8:1" };
            return ratios[(int) juce::jlimit (0.0f, 5.0f, plain)];
        }

        if (id == "timing")
        {
            static const juce::StringArray timings { "FAST", "MF", "MED", "MS", "SLOW", "AUTO" };
            return timings[(int) juce::jlimit (0.0f, 5.0f, plain)];
        }

        if (id == "threshold")
            return juce::String (plain, 1) + " dBu";

        if (id == "makeupGain")
            return juce::String (plain, 1) + " dB";

        if (id == "sidechainHPF")
            return juce::String (plain, 0) + " Hz";

        if (id == "blend")
            return juce::String (plain, 0) + "%";

        return juce::String (plain, 1);
    }

    juce::Point<float> pointOnCircle (juce::Rectangle<float> bounds, float radius, float angle)
    {
        const auto centre = bounds.getCentre();
        return { centre.x + std::cos (angle) * radius,
                 centre.y + std::sin (angle) * radius };
    }

    juce::String utf8 (const char* source)
    {
        return juce::String::fromUTF8 (source);
    }

    void drawHelpLine (juce::Graphics& g,
                       juce::Rectangle<int>& area,
                       const juce::String& lineText,
                       float size,
                       juce::Colour colour,
                       bool bold = false,
                       int height = 18)
    {
        g.setColour (colour);
        g.setFont (uiFont (size, bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (lineText, area.removeFromTop (height), juce::Justification::centredLeft);
    }

    void drawTimingTable (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        static const std::array<const char*, 7> headers
        {
            "Timing", "1.5:1", "2:1", "3:1", "4:1", "6:1", "8:1"
        };

        static const std::array<std::array<const char*, 7>, 6> rows
        {{
            {{ "Fast", "0.05/250", "0.09/353", "0.17/504", "0.23/619", "0.32/766", "0.45/920" }},
            {{ "MF",   "31/625",   "27/675",   "20.5/775", "13.8/795", "7.8/845",  "5.6/895"  }},
            {{ "Med",  "26/1250",  "26/1250",  "23.5/1250","15.8/1680","11/2025",  "9/2150"   }},
            {{ "MS",   "27/1450",  "27/1450",  "24/1450",  "20/1450",  "16/1450",  "16/1450"  }},
            {{ "Slow", "260/1090", "180/1112", "224/1144", "197/1320", "161/1450", "135/1550" }},
            {{ "Auto", "205/600-2000", "205/600-2000", "205/600-2000", "205/600-2000", "12/600-2000", "6/600-2000" }}
        }};

        const auto headerHeight = 24;
        const auto rowHeight = 23;
        const auto firstColumnWidth = 78;
        const auto dataColumnWidth = (bounds.getWidth() - firstColumnWidth) / 6;
        const auto tableHeight = headerHeight + rowHeight * (int) rows.size();
        bounds = bounds.withHeight (tableHeight);

        g.setColour (juce::Colours::black.withAlpha (0.20f));
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);

        auto rowBounds = bounds.removeFromTop (headerHeight);
        g.setColour (panelDark.withAlpha (0.78f));
        g.fillRect (rowBounds);
        g.setColour (cream);
        g.setFont (uiFont (12.0f, juce::Font::bold));

        auto cell = rowBounds.removeFromLeft (firstColumnWidth);
        g.drawText (headers[0], cell.reduced (6, 0), juce::Justification::centredLeft);
        for (size_t column = 1; column < headers.size(); ++column)
        {
            cell = rowBounds.removeFromLeft (dataColumnWidth);
            g.drawText (headers[column], cell, juce::Justification::centred);
        }

        g.setFont (monoFont (11.5f));
        for (size_t row = 0; row < rows.size(); ++row)
        {
            rowBounds = bounds.removeFromTop (rowHeight);
            g.setColour ((row % 2 == 0 ? panel : panelDark).withAlpha (0.46f));
            g.fillRect (rowBounds);

            g.setColour (text);
            cell = rowBounds.removeFromLeft (firstColumnWidth);
            g.drawText (rows[row][0], cell.reduced (6, 0), juce::Justification::centredLeft);

            for (size_t column = 1; column < rows[row].size(); ++column)
            {
                cell = rowBounds.removeFromLeft (dataColumnWidth);
                g.drawText (rows[row][column], cell, juce::Justification::centred);
            }
        }

        const auto tableBounds = bounds.withY (bounds.getY() - tableHeight).withHeight (tableHeight);
        g.setColour (line.withAlpha (0.38f));
        g.drawRoundedRectangle (tableBounds.toFloat(), 4.0f, 1.0f);

        auto x = tableBounds.getX() + firstColumnWidth;
        for (int i = 0; i < 6; ++i)
        {
            g.drawVerticalLine (x, (float) tableBounds.getY(), (float) tableBounds.getBottom());
            x += dataColumnWidth;
        }

        auto y = tableBounds.getY() + headerHeight;
        for (int i = 0; i < 6; ++i)
        {
            g.drawHorizontalLine (y, (float) tableBounds.getX(), (float) tableBounds.getRight());
            y += rowHeight;
        }
    }
}

DB5035AudioProcessorEditor::HelpOverlay::HelpOverlay()
{
    closeButton.setButtonText ("CLOSE");
    closeButton.setClickingTogglesState (false);
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1d1b));
    closeButton.setColour (juce::TextButton::textColourOffId, cream);
    closeButton.onClick = [this]
    {
        if (onClose)
            onClose();
    };

    helpContent.setSize (900, 600);
    helpViewport.setViewedComponent (&helpContent, false);
    helpViewport.setScrollBarsShown (true, false);
    helpViewport.setOpaque (false);

    addAndMakeVisible (closeButton);
    addAndMakeVisible (helpViewport);
}

void DB5035AudioProcessorEditor::HelpOverlay::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::black.withAlpha (0.56f));
    g.fillAll();

    auto card = getLocalBounds().reduced (70, 38).toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.34f));
    g.fillRoundedRectangle (card.translated (4.0f, 5.0f), 6.0f);

    g.setGradientFill (juce::ColourGradient (panelTop.brighter (0.08f), card.getTopLeft(),
                                             panelDark, card.getBottomLeft(), false));
    g.fillRoundedRectangle (card, 6.0f);

    g.setColour (line.withAlpha (0.55f));
    g.drawRoundedRectangle (card, 6.0f, 1.2f);

    auto content = card.toNearestInt().reduced (24, 18);
    auto header = content.removeFromTop (34);
    g.setColour (cream);
    g.setFont (uiFont (18.0f, juce::Font::bold));
    g.drawText ("DB-5035 Qing Compressor", header, juce::Justification::centredLeft);
}

DB5035AudioProcessorEditor::HelpOverlay::HelpContent::HelpContent()
{
    setInterceptsMouseClicks (false, false);
}

void DB5035AudioProcessorEditor::HelpOverlay::HelpContent::paint (juce::Graphics& g)
{
    auto content = getLocalBounds();
    content.removeFromTop (6);
    drawHelpLine (g, content, utf8 (u8"禁止商用，加Q群692973169交流"), 14.0f, amber, true, 22);

    drawHelpLine (g, content, "ABOUT", 12.5f, cream, true, 18);
    drawHelpLine (g, content, utf8 (u8"DB-5035 Qing Compressor 是一个二极管桥压缩风格的动态处理器。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"它是由混音师顾子青根据他的工作室里的真实硬件，使用 Codex 制作完成的插件。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"动态响应参考了经典 5035 风格压缩模块，并经过正弦波动态测试与听感校准。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"它的重点不是透明数字压缩，而是模拟硬件在不同 Ratio、Timing、Fast 开关状态下的动态包络。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"包括音头、释放曲线、阈值自适应，以及轻微的模拟频响滚降。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"染色部分基于经典二极管桥谐波特征建模，并经过听感校准。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"它在无压缩时保持较高透明度，压缩量越大，谐波质感越明显。"), 12.4f, text, false, 19);

    content.removeFromTop (10);
    drawHelpLine (g, content, "TIMING CHART", 12.5f, cream, true, 18);
    drawHelpLine (g, content, utf8 (u8"近似标称值，实际动态会受输入电平、压缩量、Auto Release 和内部曲线影响。单位：Attack / Release = ms"), 12.0f, muted, false, 19);
    drawTimingTable (g, content.removeFromTop (162));

    content.removeFromTop (12);
    drawHelpLine (g, content, "FAST BUTTON", 12.5f, cream, true, 18);
    drawHelpLine (g, content, utf8 (u8"Fast ON 会在当前 Timing 基础上缩短 Attack / Release。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"Auto 模式下，Fast ON 会继承 Fast OFF 的基础校准，再套用 MS 风格的快速响应逻辑。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"大致为：Attack 约等于 Fast OFF 的 92%，Release 约等于 Fast OFF 的 87%。"), 12.4f, text, false, 19);

    content.removeFromTop (12);
    drawHelpLine (g, content, "CONTROL TIPS", 12.5f, cream, true, 18);
    drawHelpLine (g, content, utf8 (u8"Shift + 鼠标拖动：微调参数。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"双击旋钮或数值：手动输入精确数值。"), 12.4f, text, false, 19);
    drawHelpLine (g, content, utf8 (u8"Alt + 单击：恢复该参数的默认值。"), 12.4f, text, false, 19);
}

void DB5035AudioProcessorEditor::HelpOverlay::resized()
{
    auto card = getLocalBounds().reduced (70, 38);
    auto content = card.reduced (24, 18);
    auto header = content.removeFromTop (38);
    closeButton.setBounds (header.removeFromRight (88).withSizeKeepingCentre (78, 26));
    helpViewport.setBounds (content.reduced (0, 6));
    helpContent.setSize (juce::jmax (860, helpViewport.getWidth() - 16), 600);
}

DB5035AudioProcessorEditor::DB5035AudioProcessorEditor (DB5035AudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      audioProcessor (processor),
      knobParameterIds { "threshold", "ratio", "makeupGain", "timing", "sidechainHPF", "blend" },
      buttonParameterIds { "compIn", "externalSidechain", "fastMode" }
{
    scaledContent.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (scaledContent);

    auto pi = juce::MathConstants<float>::pi;

    configureKnob (knobs[0], knobParameterIds[0], "THRESHOLD", blackKnob, rotaryStart, rotaryEnd, false);
    configureKnob (knobs[1], knobParameterIds[1], "RATIO", blackKnob, pi, pi * 2.0f, true);
    knobs[1].slider.editable = false;
    configureKnob (knobs[2], knobParameterIds[2], "GAIN dB", redKnob, rotaryStart, rotaryEnd, false);
    configureKnob (knobs[3], knobParameterIds[3], "TIMING", creamKnob, pi, pi * 2.0f, true);
    knobs[3].slider.editable = false;
    configureKnob (knobs[4], knobParameterIds[4], "S/C HPF", blackKnob, rotaryStart, rotaryEnd, false);
    configureKnob (knobs[5], knobParameterIds[5], "BLEND %", creamKnob, rotaryStart, rotaryEnd, false);

    knobs[0].scaleLabels = juce::StringArray { "-25", "", "-16", "", "", "-2", "", "", "+11","", "+20" };
    knobs[0].scaleTickCount = 16;
    knobs[1].scaleLabels = juce::StringArray { "1.5:1", "2:1", "3:1", "4:1", "6:1", "8:1" };
    knobs[1].scaleTickCount = 6;
    knobs[2].scaleLabels = juce::StringArray { "-6", "0", "+6","+12", "+20" };
    knobs[2].scaleTickCount = 16;
    knobs[3].scaleLabels = juce::StringArray { "FAST", "MF", "MED", "MS", "SLOW", "AUTO" };
    knobs[3].scaleTickCount = 6;
    knobs[4].scaleLabels = juce::StringArray { "20Hz", "300Hz" };
    knobs[4].scaleTickCount = 16;
    knobs[5].scaleLabels = juce::StringArray { "0", "50", "100" };
    knobs[5].scaleTickCount = 16;

    configureButton (buttons[0], buttonParameterIds[0], "COMP IN");
    buttons[0].button.setColour (juce::TextButton::buttonOnColourId, green);
    configureButton (buttons[1], buttonParameterIds[1], "S/C INSERT");
    buttons[1].button.setColour (juce::TextButton::buttonOnColourId, green);
    configureButton (buttons[2], buttonParameterIds[2], "FAST");
    buttons[2].button.setColour (juce::TextButton::buttonOnColourId, amber);

    configureCommandButton (historyButtons[0], juce::String::fromUTF8 ("↶"));
    configureCommandButton (historyButtons[1], juce::String::fromUTF8 ("↷"));
    configureCommandButton (compareButtons[0], "A");
    configureCommandButton (compareButtons[1], "B");
    configureCommandButton (compareButtons[2], "A>B");
    configureCommandButton (helpButton, "HELP");
    configureCommandButton (oversamplingButton, "OS");
    historyButtons[0].button.onClick = [this] { audioProcessor.getUndoManager().undo(); updateUndoRedoButtons(); };
    historyButtons[1].button.onClick = [this] { audioProcessor.getUndoManager().redo(); updateUndoRedoButtons(); };
    compareButtons[0].button.setButtonText ("A");
    compareButtons[1].button.setButtonText ("B");
    compareButtons[2].button.setButtonText ("A>B");
    compareButtons[0].button.onClick = [this] { audioProcessor.selectCompareSlot (0); updateCompareButtons(); updateValueLabels(); };
    compareButtons[1].button.onClick = [this] { audioProcessor.selectCompareSlot (1); updateCompareButtons(); updateValueLabels(); };
    compareButtons[2].button.onClick = [this] { audioProcessor.copyCompareAToB(); updateCompareButtons(); updateValueLabels(); };
    helpButton.button.setButtonText ("HELP");
    helpButton.button.onClick = [this]
    {
        helpOverlay.setVisible (true);
        helpOverlay.toFront (true);
    };
    helpOverlay.onClose = [this] { helpOverlay.setVisible (false); };
    oversamplingButton.button.setButtonText ("1x");
    oversamplingButton.button.onClick = [this]
    {
        auto& state = audioProcessor.getValueTreeState();

        if (auto* parameter = state.getParameter ("oversampling"))
        {
            const auto current = (int) std::round (parameter->convertFrom0to1 (parameter->getValue()));
            const auto next = (current + 1) % 4;
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float) next));
            parameter->endChangeGesture();
            updateOversamplingButton();
        }
    };

    for (auto& hb : historyButtons)
    {
        hb.button.setLookAndFeel (&flatCommandLookAndFeel);
        hb.name.setVisible (false);
    }
    for (auto& cb : compareButtons)
    {
        cb.button.setLookAndFeel (&flatCommandLookAndFeel);
        cb.name.setVisible (false);
    }
    helpButton.button.setLookAndFeel (&flatCommandLookAndFeel);
    helpButton.name.setVisible (false);
    oversamplingButton.button.setLookAndFeel (&flatCommandLookAndFeel);
    oversamplingButton.name.setVisible (false);

    static const VUMeter::Mode vuModes[] = { VUMeter::Mode::input, VUMeter::Mode::output, VUMeter::Mode::reduction };
    static const juce::String vuLabels[] = { "IN", "OUT", "REDUCTION" };
    const auto initialVuMode = juce::jlimit (0, 2, audioProcessor.getVuMode());
    vuMeter.setMode (vuModes[initialVuMode]);
    vuModeButton.setButtonText (vuLabels[initialVuMode]);
    vuModeButton.setClickingTogglesState (false);
    vuModeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1d1b));
    vuModeButton.setColour (juce::TextButton::buttonOnColourId, cream);
    vuModeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    vuModeButton.setLookAndFeel (&screwLookAndFeel);
    vuModeButton.onClick = [this]
    {
        static const VUMeter::Mode modes[] = { VUMeter::Mode::input, VUMeter::Mode::output, VUMeter::Mode::reduction };
        static const juce::String labels[] = { "IN", "OUT", "REDUCTION" };
        const auto currentMode = vuMeter.getMode();
        int nextIndex = 0;

        if (currentMode == VUMeter::Mode::input)
            nextIndex = 1;
        else if (currentMode == VUMeter::Mode::output)
            nextIndex = 2;

        vuMeter.setMode (modes[nextIndex]);
        vuModeButton.setButtonText (labels[nextIndex]);
        audioProcessor.setVuMode (nextIndex);
    };

    scaledContent.addAndMakeVisible (vuMeter);
    scaledContent.addAndMakeVisible (vuModeButton);

    auto panelImage = juce::ImageFileFormat::loadFrom (BinaryData::panel_png, BinaryData::panel_pngSize);
    if (panelImage.isValid())
    {
        panelOverlay.setImage (panelImage, juce::RectanglePlacement::stretchToFit);
        panelOverlay.setAlpha (1.0f);
        panelOverlay.setInterceptsMouseClicks (false, false);
        scaledContent.addAndMakeVisible (panelOverlay);
        panelOverlay.toFront (false);
    }

    scaledContent.addAndMakeVisible (textOverlay);
    textOverlay.setPaintingIsUnclipped (true);
    vuMeter.onResetPeak = [this]
    {
        gainReductionPeakHoldDb = 0.0f;
        vuMeter.setPeakHold (0.0f);
    };
    addChildComponent (helpOverlay);

    setLookAndFeel (&hardwareLookAndFeel);
    setResizable (true, true);
    setResizeLimits (400, 100, 1920, 1200);
    setConstrainer (&panelConstrainer);
    setSize (designWidth, designHeight + commandStripHeight);
    startTimerHz (30);
    updateValueLabels();
    updateUndoRedoButtons();
    updateCompareButtons();
    updateOversamplingButton();
}

DB5035AudioProcessorEditor::~DB5035AudioProcessorEditor()
{
    for (auto& hb : historyButtons)
        hb.button.setLookAndFeel (nullptr);
    for (auto& cb : compareButtons)
        cb.button.setLookAndFeel (nullptr);
    helpButton.button.setLookAndFeel (nullptr);
    oversamplingButton.button.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void DB5035AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
}

void DB5035AudioProcessorEditor::resized()
{
    scaledContent.setBounds (0, 0, designWidth, designHeight);
    scaledContent.setTransform (getContentTransform (getLocalBounds()));
    layoutContent();
    layoutCommandStrip();
    helpOverlay.setBounds (getLocalBounds());
}

void DB5035AudioProcessorEditor::layoutButton (ButtonControl& control, juce::Rectangle<int> bounds)
{
    const auto btnBounds = bounds.withSizeKeepingCentre (34, 34);
    control.button.setBounds (btnBounds);
    control.name.setBounds (bounds.withHeight (26).withY (bounds.getY()));
}

void DB5035AudioProcessorEditor::layoutContent()
{
    layoutButton (buttons[0], juce::Rectangle<int> (970, 5, 80, 60));
    layoutButton (buttons[1], juce::Rectangle<int> (298, 8, 80, 60));
    buttons[1].button.setBounds (buttons[1].button.getBounds().translated (0, 7));
    layoutButton (buttons[2], juce::Rectangle<int> (478, 8, 80, 60));
    buttons[2].button.setBounds (buttons[2].button.getBounds().translated (0, 7));

    knobs[4].setBounds (170, 13, 155, 120);
    knobs[0].setBounds (260, 90, 155, 120);
    knobs[1].labelYOffset = -13;
    knobs[1].setBounds (350, 26, 155, 120);
    knobs[2].setBounds (440, 90, 155, 120);
    knobs[3].labelYOffset = -13;
    knobs[3].setBounds (530, 26, 155, 120);
    knobs[5].setBounds (620, 90, 155, 120);

    vuMeter.setBounds (774, 24, 190, 84);
    vuModeButton.setBounds (835, 155, 70, 56);
    panelOverlay.setBounds (0, 0, designWidth, designHeight);
    textOverlay.setBounds (0, 0, designWidth, designHeight);

    vuMeter.toBack();
    panelOverlay.toFront (false);
    for (auto& k : knobs) k.toFront (false);
    for (auto& b : buttons) { b.name.toFront (false); b.button.toFront (false); }
    vuModeButton.toFront (false);
    textOverlay.toFront (false);
}

void DB5035AudioProcessorEditor::TextOverlay::paint (juce::Graphics& g)
{
    const int titleX = 26;
    const int titleY = 75;
    const int titleLineH = 28;

    const auto titleFont = sourceSansFont (25.37f);
    const auto subFont = sourceSansLightItalicFont (12.7f);
    const auto titleW = titleFont.getStringWidth ("DB-5035");
    const auto subW = subFont.getStringWidth ("D I O D E  B R I D G E");
    const auto maxW = (float) juce::jmax (titleW, subW);
    const auto cx = (float) titleX + maxW / 2.0f;

    g.setColour (text);
    g.setFont (titleFont);
    g.drawText ("DB-5035", juce::roundToInt (cx - titleW / 2.0f), titleY, titleW + 2, titleLineH, juce::Justification::centredLeft);

    g.setColour (muted);
    g.setFont (subFont);
    g.drawFittedText ("D I O D E  B R I D G E\nC O M P R E S S O R",
                       juce::roundToInt (cx - subW / 2.0f), titleY + titleLineH, subW + 2, titleLineH,
                       juce::Justification::centredLeft, 2);

    DB5035AudioProcessorEditor::drawSignature (g);
}

void DB5035AudioProcessorEditor::layoutCommandStrip()
{
    const auto cmdBtnW = 48;
    const auto cmdBtnH = 22;
    const auto gap = 4;
    const auto startX = 3;
    const auto startY = 3;

    juce::Rectangle<int> r (startX, startY, cmdBtnW, cmdBtnH);
    historyButtons[0].button.setBounds (r); r.setX (r.getRight() + gap);
    historyButtons[1].button.setBounds (r); r.setX (r.getRight() + gap);
    compareButtons[0].button.setBounds (r); r.setX (r.getRight() + gap);
    compareButtons[1].button.setBounds (r); r.setX (r.getRight() + gap);
    compareButtons[2].button.setBounds (r); r.setX (r.getRight() + gap);
    helpButton.button.setBounds (r); r.setX (r.getRight() + gap);
    oversamplingButton.button.setBounds (r);
}

void DB5035AudioProcessorEditor::timerCallback()
{
    const auto meters = audioProcessor.getMeters();
    gainReductionPeakHoldDb = juce::jmax (gainReductionPeakHoldDb, meters.gainReductionDb);

    switch (vuMeter.getMode())
    {
        case VUMeter::Mode::input:
            vuMeter.setValue (meters.inputDb, -24.0f, 3.0f);
            break;
        case VUMeter::Mode::output:
            vuMeter.setValue (meters.outputDb, -24.0f, 3.0f);
            break;
        case VUMeter::Mode::reduction:
            vuMeter.setPeakHold (gainReductionPeakHoldDb);
            vuMeter.setValue (meters.gainReductionDb, 0.0f, 24.0f);
            break;
    }
    updateValueLabels();
    updateUndoRedoButtons();
    updateCompareButtons();
    updateOversamplingButton();
}

void DB5035AudioProcessorEditor::configureKnob (KnobComponent& control,
                                                const juce::String& parameterId,
                                                const juce::String& labelText,
                                                juce::Colour knobColour,
                                                float startAngle,
                                                float endAngle,
                                                bool stepped)
{
    control.knobColour = knobColour;
    control.stepped = stepped;
    control.scaleStartAngle = startAngle;
    control.scaleEndAngle = endAngle;
    styleLabel (control.nameLabel, labelText);
    control.valueLabel.setJustificationType (juce::Justification::centred);
    control.valueLabel.setColour (juce::Label::textColourId, text);
    control.valueLabel.setFont (uiFont (12.0f, juce::Font::plain));
    control.valueLabel.setEditable (false, true, false);
    control.valueLabel.setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xcc11110f));
    control.valueLabel.setColour (juce::Label::textWhenEditingColourId, text);
    control.valueLabel.setColour (juce::Label::outlineWhenEditingColourId, amber);
    control.valueLabel.onEditorHide = [&control]
    {
        control.valueLabel.setVisible (false);
    };

    control.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    control.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    control.slider.setRotaryParameters (startAngle, endAngle, true);
    control.slider.setMouseDragSensitivity (280);
    control.slider.setVelocityModeParameters (0.32, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
    control.slider.setColour (juce::Slider::thumbColourId, knobColour);
    control.slider.setColour (juce::Slider::rotarySliderFillColourId, amber);
    control.slider.setColour (juce::Slider::rotarySliderOutlineColourId, line);
    control.slider.setValueLabel (&control.valueLabel);

    control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getValueTreeState(), parameterId, control.slider);
    control.slider.setDoubleClickReturnValue (false, control.slider.getDoubleClickReturnValue());

    control.valueLabel.onTextChange = [&control]
    {
        if (control.valueLabel.isBeingEdited())
            return;

        control.slider.setValue (control.slider.getValueFromText (control.valueLabel.getText()), juce::sendNotificationSync);
    };

    scaledContent.addAndMakeVisible (control);
}

void DB5035AudioProcessorEditor::configureButton (ButtonControl& control, const juce::String& parameterId, const juce::String& labelText)
{
    styleLabel (control.name, labelText);

    control.button.setButtonText ("");
    control.button.setClickingTogglesState (true);
    control.button.setPaintingIsUnclipped (true);
    control.button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffeaeef4));
    control.button.setColour (juce::TextButton::buttonOnColourId, green);

    control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getValueTreeState(), parameterId, control.button);

    scaledContent.addAndMakeVisible (control.name);
    scaledContent.addAndMakeVisible (control.button);
}

void DB5035AudioProcessorEditor::configureCommandButton (CommandButtonControl& control, const juce::String& labelText)
{
    styleLabel (control.name, labelText);

    control.button.setButtonText (labelText);
    control.button.setClickingTogglesState (false);
    control.button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1d1b));
    control.button.setColour (juce::TextButton::textColourOffId, cream);
    control.button.setColour (juce::TextButton::buttonOnColourId, cream);

    addAndMakeVisible (control.name);
    addAndMakeVisible (control.button);
}

void DB5035AudioProcessorEditor::KnobComponent::paint (juce::Graphics& g)
{
    if (scaleLabels.isEmpty())
        return;

    const auto dial = slider.getBounds().toFloat();
    const auto centre = dial.getCentre();
    const auto radius = juce::jmin (dial.getWidth(), dial.getHeight()) * 0.46f;
    const auto innerArcR = radius + 1.0f;
    const auto tickInner = radius + 1.0f;
    const auto tickOuter = radius + 7.0f;
    const auto outerArcR = tickOuter;

    const auto halfPi = juce::MathConstants<float>::halfPi;

    juce::Path innerArc;
    innerArc.addCentredArc (centre.x, centre.y, innerArcR, innerArcR, 0.0f, scaleStartAngle + halfPi, scaleEndAngle + halfPi, true);
    g.setColour (juce::Colour (0xffffffff).withAlpha (0.8f));
    g.strokePath (innerArc, juce::PathStrokeType (1.0f));

    juce::Path outerArc;
    outerArc.addCentredArc (centre.x, centre.y, outerArcR, outerArcR, 0.0f, scaleStartAngle + halfPi, scaleEndAngle + halfPi, true);
    g.strokePath (outerArc, juce::PathStrokeType (2.0f));

    g.setColour (juce::Colour (0xffffffff));
    bool isStepped = scaleEndAngle - scaleStartAngle < juce::MathConstants<float>::pi * 1.01f;

    for (int i = 0; i < scaleTickCount; ++i)
    {
        auto local_tickOuter = tickOuter;
        if (i == 0 || i == scaleTickCount - 1) {
            local_tickOuter = tickOuter + (!isStepped ? 4.0f : 1.0f); // Make the first and last ticks longer
        }
        const auto t = (float) i / (float) (scaleTickCount - 1);
        const auto angle = scaleStartAngle + t * (scaleEndAngle - scaleStartAngle);
        const auto outer = juce::Point<float> { centre.x + std::cos (angle) * local_tickOuter,
                                                 centre.y + std::sin (angle) * local_tickOuter };
        const auto inner = juce::Point<float> { centre.x + std::cos (angle) * tickInner,
                                                 centre.y + std::sin (angle) * tickInner };
        g.drawLine ({ inner, outer }, 1.0f);
    }

    g.setFont (juce::FontOptions ("Century Gothic", 12.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xffffffff));

    for (int i = 0; i < scaleLabels.size(); ++i)
    {
        const auto t = scaleLabels.size() == 1 ? 0.5f : (float) i / (float) (scaleLabels.size() - 1);
        const auto angle = scaleStartAngle + t * (scaleEndAngle - scaleStartAngle);
        const auto labelR = radius + 18.0f;
        const auto ox = scaleLabels[i] == "SLOW" ? 6.0f : scaleLabels[i] == "AUTO" ? 6.0f : 0.0f;
        const auto p = juce::Point<float> { centre.x + std::cos (angle) * (radius + 18.0f) + ox,
                                             centre.y + std::sin (angle) * (radius + 18.0f) };
        g.drawText (scaleLabels[i], (int) p.x - 20, (int) p.y - 7, 40, 14, juce::Justification::centred);
    }
}

void DB5035AudioProcessorEditor::KnobComponent::resized()
{
    auto bounds = getLocalBounds();
    auto sliderBounds = bounds.reduced (38, 30);
    sliderBounds.translate (0, -16);
    slider.setBounds (sliderBounds);

    const auto sliderBottom = slider.getBounds().getBottom();
    nameLabel.setBounds (0, sliderBottom + 16 + labelYOffset, getWidth(), 20);

    const auto sliderCentre = slider.getBounds().getCentre();
    valueLabel.setBounds (sliderCentre.x - 30, sliderCentre.y - 10, 60, 20);
}

void DB5035AudioProcessorEditor::drawSignature (juce::Graphics& g)
{
    auto signatureArea = juce::Rectangle<int> (1085, 42, 58, 71);

    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (-0.10f,
                                                     (float) signatureArea.getCentreX(),
                                                     (float) signatureArea.getCentreY()));

    const auto signatureText = juce::String::fromUTF8 ("\xe9\x9d\x92");
    auto inkArea = signatureArea.reduced (4, 4).translated (-3, 0);
    const auto signatureFont = juce::FontOptions (signatureTypefaceName(), 52.0f, juce::Font::plain);

    g.setFont (signatureFont);
    g.setColour (panelDark.withAlpha (0.26f));
    g.drawFittedText (signatureText,
                      inkArea.translated (2, 2),
                      juce::Justification::centred,
                      1);

    g.setColour (cream.withAlpha (0.70f));
    g.drawFittedText (signatureText,
                      inkArea,
                      juce::Justification::centred,
                      1);

    g.setColour (cream.withAlpha (0.18f));
    g.drawFittedText (signatureText,
                      inkArea.translated (-1, 0),
                      juce::Justification::centred,
                      1);

    {
        juce::Graphics::ScopedSaveState distressState (g);
        g.reduceClipRegion (inkArea.reduced (2, 8));

        juce::Random random (0x5135);

        for (int i = 0; i < 16; ++i)
        {
            const auto x = (float) inkArea.getX() + 4.0f + random.nextFloat() * (float) inkArea.getWidth();
            const auto y = (float) inkArea.getY() + 10.0f + random.nextFloat() * ((float) inkArea.getHeight() - 20.0f);
            const auto length = 3.0f + random.nextFloat() * 13.0f;
            const auto drift = -1.8f + random.nextFloat() * 3.6f;
            const auto thickness = 0.45f + random.nextFloat() * 0.9f;

            g.setColour ((random.nextBool() ? panel : background).withAlpha (0.30f + random.nextFloat() * 0.20f));
            g.drawLine (x, y, x + length, y + drift, thickness);
        }

        for (int i = 0; i < 46; ++i)
        {
            const auto size = 0.6f + random.nextFloat() * 2.3f;
            const auto x = (float) inkArea.getX() + random.nextFloat() * ((float) inkArea.getWidth() - size);
            const auto y = (float) inkArea.getY() + 8.0f + random.nextFloat() * ((float) inkArea.getHeight() - 18.0f);

            g.setColour ((i % 4 == 0 ? background : panel).withAlpha (0.26f + random.nextFloat() * 0.34f));
            g.fillEllipse (x, y, size * (0.7f + random.nextFloat() * 0.8f), size);
        }

        for (int i = 0; i < 7; ++i)
        {
            const auto x = (float) inkArea.getX() + 6.0f + random.nextFloat() * ((float) inkArea.getWidth() - 12.0f);
            const auto y = (float) inkArea.getY() + 10.0f + random.nextFloat() * ((float) inkArea.getHeight() - 24.0f);

            g.setColour (panel.withAlpha (0.22f));
            g.drawLine (x, y, x + 4.0f + random.nextFloat() * 10.0f, y + 6.0f + random.nextFloat() * 10.0f, 0.8f);
        }

        for (int i = 0; i < 8; ++i)
        {
            const auto size = 1.2f + random.nextFloat() * 3.4f;
            const auto x = (float) inkArea.getX() + random.nextFloat() * ((float) inkArea.getWidth() - size);
            const auto y = (float) inkArea.getY() + random.nextFloat() * ((float) inkArea.getHeight() - size);

            g.setColour (cream.withAlpha (0.05f + random.nextFloat() * 0.08f));
            g.fillEllipse (x, y, size, size * (0.7f + random.nextFloat() * 0.5f));
        }
    }
}

void DB5035AudioProcessorEditor::updateValueLabels()
{
    auto& state = audioProcessor.getValueTreeState();

    for (size_t index = 0; index < knobs.size(); ++index)
    {
        if (!knobs[index].valueLabel.isVisible() || knobs[index].valueLabel.isBeingEdited())
            continue;

        if (auto* parameter = state.getParameter (knobParameterIds[index]))
            knobs[index].valueLabel.setText (formatValue (*parameter), juce::dontSendNotification);
    }
}

void DB5035AudioProcessorEditor::updateUndoRedoButtons()
{
    auto& undoManager = audioProcessor.getUndoManager();
    historyButtons[0].button.setEnabled (undoManager.canUndo());
    historyButtons[1].button.setEnabled (undoManager.canRedo());
}

void DB5035AudioProcessorEditor::updateCompareButtons()
{
    const auto activeSlot = audioProcessor.getActiveCompareSlot();
    compareButtons[0].button.setToggleState (activeSlot == 0, juce::dontSendNotification);
    compareButtons[1].button.setToggleState (activeSlot == 1, juce::dontSendNotification);
    compareButtons[2].button.setToggleState (false, juce::dontSendNotification);
}

void DB5035AudioProcessorEditor::updateOversamplingButton()
{
    static const juce::StringArray labels { "1x", "2x", "4x", "8x" };

    if (auto* value = audioProcessor.getValueTreeState().getRawParameterValue ("oversampling"))
    {
        const auto index = (int) juce::jlimit (0.0f, 3.0f, std::round (value->load (std::memory_order_relaxed)));
        oversamplingButton.button.setButtonText (labels[index]);
    }
}

void DB5035AudioProcessorEditor::ParameterSlider::setValueLabel (juce::Label* labelToUse)
{
    valueLabel = labelToUse;
}

void DB5035AudioProcessorEditor::ParameterSlider::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isAltDown())
    {
        setValue (getDoubleClickReturnValue(), juce::sendNotificationSync);
        return;
    }

    juce::Slider::mouseDown (event);
}

void DB5035AudioProcessorEditor::ParameterSlider::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (!editable || event.mods.isAltDown())
    {
        if (event.mods.isAltDown())
            setValue (getDoubleClickReturnValue(), juce::sendNotificationSync);
        return;
    }

    if (valueLabel != nullptr)
    {
        valueLabel->setText (getTextFromValue (getValue()), juce::dontSendNotification);
        valueLabel->setVisible (true);
        valueLabel->showEditor();
        if (auto* editor = valueLabel->getCurrentTextEditor())
            editor->setJustification (juce::Justification::centred);
    }
}

void DB5035AudioProcessorEditor::HardwareLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                                                        int x,
                                                                        int y,
                                                                        int width,
                                                                        int height,
                                                                        float sliderPos,
                                                                        float rotaryStartAngle,
                                                                        float rotaryEndAngle,
                                                                        juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre (size, size);

    const auto knobColour = slider.findColour (juce::Slider::thumbColourId);
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto radius = size * 0.5f;
    const auto centre = bounds.getCentre();

    for (int i = 3; i >= 0; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.09f * (4 - i)));
        g.fillEllipse (bounds.translated (3.0f, 4.0f).expanded ((float) i));
    }

    g.setGradientFill (juce::ColourGradient (knobColour.brighter (0.28f), bounds.getTopLeft(),
                                             knobColour.darker (0.55f), bounds.getBottomRight(), false));
    g.fillEllipse (bounds);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawEllipse (bounds, 1.0f);

    const auto cap = bounds.reduced (radius * 0.22f);
    g.setColour (knobColour.brighter (0.12f));
    g.drawEllipse (cap, 1.0f);

    const auto pointerStart = pointOnCircle (bounds, radius * 0.26f, angle);
    const auto pointerEnd = pointOnCircle (bounds, radius * 0.82f, angle);
    g.setColour (knobColour == cream ? juce::Colour (0xff292929) : cream);
    g.drawLine ({ pointerStart, pointerEnd }, 5.0f);
}

void DB5035AudioProcessorEditor::HardwareLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                                                           juce::Button& button,
                                                                           const juce::Colour&,
                                                                           bool,
                                                                           bool)
{
    // fullBounds = 按钮完整区域，bounds = 椭圆绘制区域（留出光晕空间）
    const auto fullBounds = button.getLocalBounds().toFloat();
    const auto ellipseInset = 7.0f;   // 椭圆到按钮边缘的距离，越大光晕空间越宽
    auto bounds = fullBounds.reduced (ellipseInset);
    const auto on = button.getToggleState();
    const auto glowColour = button.findColour (juce::TextButton::buttonOnColourId);
    const auto base = juce::Colour (0xffeaeef4);  // 按钮底色 #EAEEF4
    const auto cx = bounds.getCentreX();
    const auto cy = bounds.getCentreY();

    if (!on)
    {
        // 阴影：偏移 (2,3)，3px 虚化
        for (int i = 3; i >= 0; --i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.11f * (4 - i)));
            g.fillEllipse (bounds.translated (2.0f, 3.0f).expanded ((float) i));
        }

        // 按钮主体：左上亮→右下暗的线性渐变
        g.setGradientFill (juce::ColourGradient (base.brighter (0.18f), bounds.getTopLeft(),
                                                 base.darker (0.38f), bounds.getBottomRight(), false));
        g.fillEllipse (bounds);

        // 边框
        g.setColour (juce::Colour (0xaa0c0c0a));
        g.drawEllipse (bounds, 1.5f);
    }
    else
    {
        // ── 第1层：外层弱光晕 ──
        // 径向渐变：中心 glowColour@0.22 → 边缘透明，覆盖 fullBounds
        g.setGradientFill (juce::ColourGradient (glowColour.withAlpha (0.22f), { cx, cy },
                                                 glowColour.withAlpha (0.0f), fullBounds.getCentre().toFloat(), true));
        g.fillEllipse (fullBounds);

        // ── 第2层：内层强光晕 ──
        // 径向渐变：中心 glowColour@0.50 → 边缘透明，椭圆外扩 4px
        const auto innerGlowExpand = 4.0f;  // 内层光晕超出椭圆的距离
        g.setGradientFill (juce::ColourGradient (glowColour.withAlpha (0.70f), { cx, cy },
                                                 glowColour.withAlpha (0.0f), { cx, cy + bounds.getHeight() * 0.5f + innerGlowExpand }, true));
        g.fillEllipse (bounds.expanded (innerGlowExpand));

        // ── 第3层：按钮主体 ──
        // 按下时渲染为灯光颜色，微弱渐变保留立体感
        g.setGradientFill (juce::ColourGradient (glowColour.brighter (0.6f), bounds.getTopLeft(),
                                                 glowColour.darker (0.20f), bounds.getBottomRight(), false));
        g.fillEllipse (bounds);

        // ── 第4层：LED 过曝白点 ──
        // 径向渐变：中心纯白 → 透明，范围由 hotspotShrink 控制
        // hotspotShrink 越大→白点越小，gradientRadius 越大→渐变越平缓
        const auto hotspotShrink = 0.1f;     // 白点缩小比例 (0.15 = 缩到 70% 大小)
        const auto gradientRadius = 0.3f;    // 渐变半径占椭圆高度的比例
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.5f), { cx, cy },
                                                 juce::Colours::white.withAlpha (0.2f), { cx, cy + bounds.getHeight() * gradientRadius }, true));
        g.fillEllipse (bounds.reduced (bounds.getWidth() * hotspotShrink, bounds.getHeight() * hotspotShrink));

        // 边框
        g.setColour (juce::Colour (0x88101016));
        g.drawEllipse (bounds, 1.5f);
    }
}

void DB5035AudioProcessorEditor::HardwareLookAndFeel::drawButtonText (juce::Graphics& g,
                                                                      juce::TextButton& button,
                                                                      bool,
                                                                      bool)
{
    if (button.getButtonText().isEmpty())
        return;

    const auto on = button.getToggleState();
    g.setColour (on ? juce::Colour (0xff1a1a1a) : juce::Colour (0xff3a3a3a));
    g.setFont (uiFont (16.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

void DB5035AudioProcessorEditor::ScrewLookAndFeel::drawButtonBackground (juce::Graphics&,
                                                                          juce::Button&,
                                                                          const juce::Colour&,
                                                                          bool,
                                                                          bool)
{
}

void DB5035AudioProcessorEditor::ScrewLookAndFeel::drawButtonText (juce::Graphics& g,
                                                                     juce::TextButton& button,
                                                                     bool,
                                                                     bool)
{
    if (button.getButtonText().isEmpty())
        return;

    const auto bounds = button.getLocalBounds().toFloat();
    const auto textY = 29.0f;

    g.setColour (juce::Colours::white);
    g.setFont (sourceSansFont (13.3f));
    g.drawText (button.getButtonText(), bounds.withTop ((int) textY).toNearestInt(), juce::Justification::centredTop);
}

void DB5035AudioProcessorEditor::FlatCommandLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                                                               juce::Button& button,
                                                                               const juce::Colour&,
                                                                               bool shouldDrawButtonAsHighlighted,
                                                                               bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();

    if (shouldDrawButtonAsDown)
        g.setColour (juce::Colour (0xff383830));
    else if (shouldDrawButtonAsHighlighted)
        g.setColour (juce::Colour (0xff2e2e28));
    else
        g.setColour (juce::Colour (0xff222220));

    g.fillRect (bounds);
    g.setColour (juce::Colour (0xff444440));
    g.drawRect (bounds, 0.5f);
}

void DB5035AudioProcessorEditor::FlatCommandLookAndFeel::drawButtonText (juce::Graphics& g,
                                                                         juce::TextButton& button,
                                                                         bool,
                                                                         bool)
{
    if (button.getButtonText().isEmpty())
        return;

    g.setColour (button.isEnabled() ? cream : muted.withAlpha (0.45f));
    g.setFont (uiFont (14.0f));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

void DB5035AudioProcessorEditor::VUMeter::setMode (Mode newMode)
{
    mode = newMode;
    repaint();
}

void DB5035AudioProcessorEditor::VUMeter::setValue (float newValueDb, float newMinimumDb, float newMaximumDb)
{
    valueDb = newValueDb;
    minimumDb = newMinimumDb;
    maximumDb = newMaximumDb;

    const auto halfSweep = 77.32f / 2.0f * juce::MathConstants<float>::pi / 180.0f;
    const auto startAngle = juce::MathConstants<float>::pi * 1.5f - halfSweep;
    const auto endAngle = juce::MathConstants<float>::pi * 1.5f + halfSweep;
    const auto sweep = endAngle - startAngle;

    float displayValue = valueDb;

    if (mode == Mode::reduction)
        displayValue = -valueDb;

    const auto normalised = dbToNormalised (displayValue);
    targetAngle = startAngle + normalised * sweep;

    const auto smoothing = 0.18f;
    smoothedAngle += (targetAngle - smoothedAngle) * smoothing;

    repaint();
}

void DB5035AudioProcessorEditor::VUMeter::setPeakHold (float newPeakDb)
{
    heldPeakDb = newPeakDb;
    showPeakHold = true;
    repaint();
}

void DB5035AudioProcessorEditor::VUMeter::mouseDown (const juce::MouseEvent& event)
{
    if (mode == Mode::reduction && event.mods.isLeftButtonDown() && onResetPeak)
        onResetPeak();
}

void DB5035AudioProcessorEditor::VUMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto w = bounds.getWidth();
    const auto meterW = w;
    const auto meterH = w * 0.5f;
    const auto meterBounds = bounds.withSizeKeepingCentre (meterW, meterH);
    const auto centre = juce::Point<float> (meterBounds.getCentreX(), meterBounds.getBottom() + meterH * 0.30f);
    const auto radius = meterH * 1.05f;

    g.setColour (juce::Colour (0xfff5e6a0));
    g.fillRect (meterBounds);

    g.setColour (juce::Colour (0xff2a2520));
    g.drawRect (meterBounds, 2.5f);

    juce::Graphics::ScopedSaveState clipState (g);
    g.reduceClipRegion (meterBounds.toNearestInt());

    const auto halfSweep = 77.32f / 2.0f * juce::MathConstants<float>::pi / 180.0f;
    const auto startAngle = juce::MathConstants<float>::pi * 1.5f - halfSweep;
    const auto endAngle = juce::MathConstants<float>::pi * 1.5f + halfSweep;
    const auto totalSweep = endAngle - startAngle;

    const auto isReduction = (mode == Mode::reduction);

    const auto arcRadius = radius;
    const auto tickOuterR = arcRadius;
    const auto tickInnerR = arcRadius - 7.5f;
    const auto arcLineR = tickInnerR;
    const auto blackArcR = arcLineR + 0.9f;
    const auto labelR = arcRadius + 4.0f;

    {
        const auto zeroNorm = dbToNormalised (0.0f);
        const auto zeroAngle = startAngle + zeroNorm * totalSweep;
        const auto halfPi = juce::MathConstants<float>::halfPi;

        juce::Path blackArc;
        blackArc.addCentredArc (centre.x, centre.y, blackArcR, blackArcR, 0.0f, startAngle + halfPi, zeroAngle + halfPi, true);
        g.setColour (juce::Colour (0xff2a2520));
        g.strokePath (blackArc, juce::PathStrokeType (1.2f));

        juce::Path redArc;
        redArc.addCentredArc (centre.x, centre.y, arcLineR, arcLineR, 0.0f, zeroAngle + halfPi, endAngle + halfPi, true);
        g.setColour (juce::Colour (0xffcc4444));
        g.strokePath (redArc, juce::PathStrokeType (4.0f));

        // “-” 和 “+” 标记
        const auto minusAngle = startAngle;
        const auto minusX = centre.x + std::cos (minusAngle) * tickInnerR;
        const auto minusY = centre.y + std::sin (minusAngle) * tickInnerR;
        g.setColour (juce::Colour (0xff2a2520));
        g.drawLine (minusX - 3.0f, minusY + 8.0f, minusX + 3.0f, minusY + 8.0f, 1.0f);

        const auto plusAngle = endAngle;
        const auto plusX = centre.x + std::cos (plusAngle) * tickInnerR;
        const auto plusY = centre.y + std::sin (plusAngle) * tickInnerR;
        g.setColour (juce::Colour (0xffcc4444));
        g.drawLine (plusX - 3.0f, plusY + 8.0f, plusX + 3.0f, plusY + 8.0f, 1.0f);
        g.drawLine (plusX, plusY + 5.0f, plusX, plusY + 11.0f, 1.0f);
    }

    // -24dB tick
    {
        const auto shortOuterR = tickInnerR + 4.0f;
        const auto oX = centre.x + std::cos (startAngle) * shortOuterR;
        const auto oY = centre.y + std::sin (startAngle) * shortOuterR;
        const auto iX = centre.x + std::cos (startAngle) * (tickInnerR + 0.5f);
        const auto iY = centre.y + std::sin (startAngle) * (tickInnerR + 0.5f);
        g.setColour (juce::Colour (0xff2a2520));
        g.drawLine (iX, iY, oX, oY, 1.2f);
    }

    // short ticks: -6, -4, -0.5, +0.5
    {
        static const float shortDbValues[] = { -6.0f, -4.0f, -0.5f, 0.5f };
        const auto shortInnerR = tickInnerR + 0.5f;
        const auto shortOuterR2 = tickInnerR + 4.0f;
        for (float db : shortDbValues)
        {
            const auto norm = dbToNormalised (db);
            const auto angle = startAngle + norm * totalSweep;
            const auto oX = centre.x + std::cos (angle) * shortOuterR2;
            const auto oY = centre.y + std::sin (angle) * shortOuterR2;
            const auto iX = centre.x + std::cos (angle) * shortInnerR;
            const auto iY = centre.y + std::sin (angle) * shortInnerR;
            const bool inRedZone = norm > dbToNormalised (0.0f);
            g.setColour (inRedZone ? juce::Colour (0xffcc4444) : juce::Colour (0xff2a2520));
            g.drawLine (iX, iY, oX, oY, 1.0f);
        }
    }

    static const int dbValues[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    for (int db : dbValues)
    {
        const auto norm = dbToNormalised ((float) db);
        const auto angle = startAngle + norm * totalSweep;
        const auto isEndTick = (db == 3);
        const auto thisInnerR = isEndTick ? tickInnerR - 2.0f : tickInnerR + 1.0f;
        const auto outerX = centre.x + std::cos (angle) * tickOuterR;
        const auto outerY = centre.y + std::sin (angle) * tickOuterR;
        const auto innerX = centre.x + std::cos (angle) * thisInnerR;
        const auto innerY = centre.y + std::sin (angle) * thisInnerR;

        const bool inRedZone = norm > dbToNormalised (0.0f);
        g.setColour (inRedZone ? juce::Colour (0xffcc4444) : juce::Colour (0xff2a2520));
        g.drawLine (innerX, innerY, outerX, outerY, 1.2f);

        const auto label = (db < 0) ? juce::String (-db) : juce::String (db);
        const auto lx = centre.x + std::cos (angle) * labelR;
        const auto ly = centre.y + std::sin (angle) * labelR;
        const auto textRotation = angle + juce::MathConstants<float>::halfPi;

        juce::Graphics::ScopedSaveState textState (g);
        g.addTransform (juce::AffineTransform::rotation (textRotation, lx, ly));
        g.setFont (juce::FontOptions ("Century Gothic", 9.0f, juce::Font::plain));
        g.setColour (inRedZone ? juce::Colour (0xffcc4444) : juce::Colour (0xff2a2520));
        g.drawText (label,
                     juce::roundToInt (lx) - 16, juce::roundToInt (ly) - 6, 32, 12,
                     juce::Justification::centred);
    }

    g.setFont (uiFont (8.0f));
    g.setColour (juce::Colour (0xff807868));
    g.drawText ("dB",
                 juce::roundToInt (centre.x) - 16, juce::roundToInt (centre.y - radius * 0.16f), 32, 12,
                 juce::Justification::centred);

    {
        const auto vuAngle = startAngle;
        const auto vuX = centre.x + std::cos (vuAngle) * tickOuterR;
        const auto vuY = bounds.getY() + 25.0f;
        g.setFont (sourceSansFont (11.0f));
        g.setColour (juce::Colour (0xff2a2520));
        g.drawText ("VU", juce::roundToInt (vuX) - 12, juce::roundToInt (vuY) - 16, 24, 12, juce::Justification::centred);
    }

    const auto needleAngle = smoothedAngle;
    const auto needleLength = arcRadius - 2.0f;
    const auto needleTipX = centre.x + std::cos (needleAngle) * needleLength;
    const auto needleTipY = centre.y + std::sin (needleAngle) * needleLength;

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (centre.x + 2.0f, centre.y + 2.0f, needleTipX + 2.0f, needleTipY + 2.0f, 1.2f);

    g.setColour (juce::Colour (0xff1a1a1a));
    g.drawLine (centre.x, centre.y, needleTipX, needleTipY, 1.0f);

    g.setFont (juce::FontOptions ("Century Gothic", 12.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff2a2520));
    juce::String valueLabel;
    if (isReduction)
        valueLabel = juce::String (valueDb, 1) + (showPeakHold ? " | " + juce::String (heldPeakDb, 1) : "") + " dB";
    else
        valueLabel = juce::String (valueDb, 1) + " dB";
    g.drawText (valueLabel, juce::roundToInt (meterBounds.getX()), juce::roundToInt (meterBounds.getBottom() - 18.0f), juce::roundToInt (meterBounds.getWidth()), 14,
                juce::Justification::centred);
}
