#include "PluginEditor.h"

#include <cmath>
#include <cstdlib>

namespace
{
    const auto background = juce::Colour (0xff12120f);
    const auto panel = juce::Colour (0xff30312d);
    const auto panelTop = juce::Colour (0xff41423d);
    const auto panelDark = juce::Colour (0xff20211d);
    const auto line = juce::Colour (0xffcbc7bb);
    const auto text = juce::Colour (0xffeee8d8);
    const auto muted = juce::Colour (0xffd6d0bf);
    const auto cream = juce::Colour (0xffe6dfc5);
    const auto blackKnob = juce::Colour (0xff171817);
    const auto redKnob = juce::Colour (0xffc74743);
    const auto amber = juce::Colour (0xffd4aa55);
    const auto green = juce::Colour (0xff58a07f);
    const auto red = juce::Colour (0xffd65245);

    constexpr auto rotaryStart = juce::MathConstants<float>::pi * 0.78f;
    constexpr auto rotaryEnd = juce::MathConstants<float>::pi * 2.22f;
    constexpr int designWidth = 1120;
    constexpr int designHeight = 450;

    float getContentScale (juce::Rectangle<int> bounds)
    {
        return juce::jmin ((float) bounds.getWidth() / (float) designWidth,
                           (float) bounds.getHeight() / (float) designHeight);
    }

    juce::AffineTransform getContentTransform (juce::Rectangle<int> bounds)
    {
        const auto scale = getContentScale (bounds);
        const auto offsetX = ((float) bounds.getWidth() - (float) designWidth * scale) * 0.5f;
        const auto offsetY = ((float) bounds.getHeight() - (float) designHeight * scale) * 0.5f;
        return juce::AffineTransform::scale (scale).translated (offsetX, offsetY);
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
        label.setFont (uiFont (13.0f, juce::Font::bold));
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

    configureKnob (knobs[0], knobParameterIds[0], "THRESHOLD", blackKnob, false);
    configureKnob (knobs[1], knobParameterIds[1], "RATIO", blackKnob, true);
    configureKnob (knobs[2], knobParameterIds[2], "GAIN dB", redKnob, false);
    configureKnob (knobs[3], knobParameterIds[3], "TIMING", cream, true);
    configureKnob (knobs[4], knobParameterIds[4], "S/C HPF", cream, false);
    configureKnob (knobs[5], knobParameterIds[5], "BLEND %", cream, false);

    configureButton (buttons[0], buttonParameterIds[0], "COMP IN");
    configureButton (buttons[1], buttonParameterIds[1], "EXT S/C");
    configureButton (buttons[2], buttonParameterIds[2], "FAST");

    configureCommandButton (historyButtons[0], "UNDO");
    configureCommandButton (historyButtons[1], "REDO");
    configureCommandButton (compareButtons[0], "A");
    configureCommandButton (compareButtons[1], "B");
    configureCommandButton (compareButtons[2], "A>B");
    configureCommandButton (helpButton, "HELP");
    configureCommandButton (oversamplingButton, "OS");
    historyButtons[0].button.onClick = [this] { audioProcessor.getUndoManager().undo(); updateUndoRedoButtons(); };
    historyButtons[1].button.onClick = [this] { audioProcessor.getUndoManager().redo(); updateUndoRedoButtons(); };
    compareButtons[0].button.setButtonText ("A");
    compareButtons[1].button.setButtonText ("B");
    compareButtons[2].button.setButtonText (">");
    compareButtons[0].button.onClick = [this] { audioProcessor.selectCompareSlot (0); updateCompareButtons(); updateValueLabels(); };
    compareButtons[1].button.onClick = [this] { audioProcessor.selectCompareSlot (1); updateCompareButtons(); updateValueLabels(); };
    compareButtons[2].button.onClick = [this] { audioProcessor.copyCompareAToB(); updateCompareButtons(); updateValueLabels(); };
    helpButton.button.setButtonText ("?");
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

    vuModeButton.setButtonText ("OUT");
    vuModeButton.setClickingTogglesState (false);
    vuModeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1d1b));
    vuModeButton.setColour (juce::TextButton::buttonOnColourId, cream);
    vuModeButton.setColour (juce::TextButton::textColourOffId, cream);
    vuModeButton.onClick = [this]
    {
        const auto currentMode = vuMeter.getMode();
        VUMeter::Mode nextMode;
        juce::String nextLabel;

        if (currentMode == VUMeter::Mode::input)
        {
            nextMode = VUMeter::Mode::output;
            nextLabel = "OUT";
        }
        else if (currentMode == VUMeter::Mode::output)
        {
            nextMode = VUMeter::Mode::reduction;
            nextLabel = "RED";
        }
        else
        {
            nextMode = VUMeter::Mode::input;
            nextLabel = "IN";
        }

        vuMeter.setMode (nextMode);
        vuModeButton.setButtonText (nextLabel);
    };

    scaledContent.addAndMakeVisible (vuMeter);
    scaledContent.addAndMakeVisible (vuModeButton);
    vuMeter.onResetPeak = [this]
    {
        gainReductionPeakHoldDb = 0.0f;
        vuMeter.setPeakHold (0.0f);
    };
    addChildComponent (helpOverlay);

    setLookAndFeel (&hardwareLookAndFeel);
    setResizable (true, true);
    setResizeLimits (896, 360, 1680, 675);
    if (auto* editorConstrainer = getConstrainer())
        editorConstrainer->setFixedAspectRatio ((double) designWidth / (double) designHeight);
    setSize (designWidth, designHeight);
    startTimerHz (30);
    updateValueLabels();
    updateUndoRedoButtons();
    updateCompareButtons();
    updateOversamplingButton();
}

DB5035AudioProcessorEditor::~DB5035AudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void DB5035AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (getContentTransform (getLocalBounds()));

    const auto designBounds = juce::Rectangle<int> (0, 0, designWidth, designHeight);
    drawHardwareFrame (g, designBounds.reduced (14));

    auto module = designBounds.reduced (30);
    module.removeFromTop (36);
    auto controls = module;
    const auto meterWidth = 150;
    controls.removeFromRight (meterWidth + 18);

    g.setColour (line.withAlpha (0.95f));
    g.fillRect (controls.getRight() + 4, controls.getY() + 16, 3, controls.getHeight() - 26);
    g.fillRect (controls.getX() + 122, controls.getY() + 16, 3, controls.getHeight() - 26);

    g.setColour (text);
    g.setFont (uiFont (28.0f, juce::Font::bold));
    g.drawText ("DB-5035", module.removeFromTop (30), juce::Justification::centredLeft);

    g.setColour (muted);
    g.setFont (uiFont (12.0f, juce::Font::bold));
    g.drawText ("DIODE BRIDGE COMPRESSOR", 36, 58, 260, 20, juce::Justification::centredLeft);
    drawSignature (g, designBounds.reduced (30));

    auto knobBand = designBounds.reduced (30);
    knobBand.removeFromTop (84);
    knobBand.removeFromRight (meterWidth + 18);
    knobBand.removeFromLeft (124);
    knobBand.removeFromTop (58);
    knobBand = knobBand.withHeight (250);
    const auto knobWidth = knobBand.getWidth() / 6;

    drawKnobScale (g, knobBand.removeFromLeft (knobWidth).reduced (10, 0),
                   juce::StringArray { "-25", "0", "+20" }, rotaryStart, rotaryEnd, false);
    drawKnobScale (g, knobBand.removeFromLeft (knobWidth).reduced (10, 0),
                   juce::StringArray { "1.5", "3", "6", "8" }, rotaryStart, rotaryEnd, true);
    drawKnobScale (g, knobBand.removeFromLeft (knobWidth).reduced (10, 0),
                   juce::StringArray { "-6", "0", "+12", "+20" }, rotaryStart, rotaryEnd, false);
    drawKnobScale (g, knobBand.removeFromLeft (knobWidth).reduced (10, 0),
                   juce::StringArray { "FAST", "MED", "SLOW", "AUTO" }, rotaryStart, rotaryEnd, true);
    drawKnobScale (g, knobBand.removeFromLeft (knobWidth).reduced (10, 0),
                   juce::StringArray { "20", "90", "300" }, rotaryStart, rotaryEnd, false);
    drawKnobScale (g, knobBand.removeFromLeft (knobWidth).reduced (10, 0),
                   juce::StringArray { "0", "50", "100" }, rotaryStart, rotaryEnd, false);
}

void DB5035AudioProcessorEditor::resized()
{
    scaledContent.setBounds (0, 0, designWidth, designHeight);
    scaledContent.setTransform (getContentTransform (getLocalBounds()));
    layoutContent();
    helpOverlay.setBounds (getLocalBounds());
}

void DB5035AudioProcessorEditor::layoutContent()
{
    auto bounds = juce::Rectangle<int> (0, 0, designWidth, designHeight).reduced (30);
    bounds.removeFromTop (84);

    const auto meterWidth = 150;
    auto rightArea = bounds.removeFromRight (meterWidth).reduced (8, 16);
    rightArea.removeFromTop (50);

    vuMeter.setBounds (rightArea.removeFromTop (200));
    rightArea.removeFromTop (4);
    vuModeButton.setBounds (rightArea.removeFromTop (24).withSizeKeepingCentre (60, 22));

    bounds.removeFromRight (18);
    auto leftInset = bounds.removeFromLeft (124);
    layoutButton (buttons[0], leftInset.withTrimmedTop (22).withHeight (72).reduced (4, 0));

    auto topButtons = bounds.withHeight (62);
    layoutCommandButton (historyButtons[0], topButtons.removeFromLeft (50).reduced (4, 0));
    layoutCommandButton (historyButtons[1], topButtons.removeFromLeft (50).reduced (4, 0));
    topButtons.removeFromLeft (8);
    layoutCommandButton (compareButtons[0], topButtons.removeFromLeft (48).reduced (4, 0));
    layoutCommandButton (compareButtons[1], topButtons.removeFromLeft (48).reduced (4, 0));
    layoutCommandButton (compareButtons[2], topButtons.removeFromLeft (56).reduced (4, 0));
    topButtons.removeFromLeft (8);
    layoutCommandButton (helpButton, topButtons.removeFromLeft (50).reduced (4, 0));
    layoutCommandButton (oversamplingButton, topButtons.removeFromLeft (50).reduced (4, 0));
    topButtons.removeFromLeft (18);
    layoutButton (buttons[1], topButtons.removeFromLeft (124).reduced (8, 0));
    topButtons.removeFromLeft (168);
    layoutButton (buttons[2], topButtons.removeFromLeft (96).reduced (8, 0));

    bounds.removeFromTop (58);
    auto knobArea = bounds.withHeight (250);
    const auto knobWidth = knobArea.getWidth() / 6;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto cell = knobArea.removeFromLeft (knobWidth).reduced (10, 0);
        layoutKnob (knobs[i], cell);
    }
}

void DB5035AudioProcessorEditor::timerCallback()
{
    const auto meters = audioProcessor.getMeters();
    gainReductionPeakHoldDb = juce::jmax (gainReductionPeakHoldDb, meters.gainReductionDb);

    switch (vuMeter.getMode())
    {
        case VUMeter::Mode::input:
            vuMeter.setValue (meters.inputDb, -20.0f, 3.0f);
            break;
        case VUMeter::Mode::output:
            vuMeter.setValue (meters.outputDb, -20.0f, 3.0f);
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

void DB5035AudioProcessorEditor::configureKnob (KnobControl& control,
                                                const juce::String& parameterId,
                                                const juce::String& labelText,
                                                juce::Colour knobColour,
                                                bool stepped)
{
    control.knobColour = knobColour;
    control.stepped = stepped;
    styleLabel (control.name, labelText);
    control.value.setJustificationType (juce::Justification::centred);
    control.value.setColour (juce::Label::textColourId, text);
    control.value.setFont (uiFont (12.0f, juce::Font::bold));
    control.value.setEditable (false, true, false);
    control.value.setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff11110f));
    control.value.setColour (juce::Label::textWhenEditingColourId, text);
    control.value.setColour (juce::Label::outlineWhenEditingColourId, amber);

    control.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    control.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    control.slider.setRotaryParameters (rotaryStart, rotaryEnd, true);
    control.slider.setMouseDragSensitivity (520);
    control.slider.setVelocityModeParameters (0.32, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
    control.slider.setColour (juce::Slider::thumbColourId, knobColour);
    control.slider.setColour (juce::Slider::rotarySliderFillColourId, amber);
    control.slider.setColour (juce::Slider::rotarySliderOutlineColourId, line);
    control.slider.setValueLabel (&control.value);

    control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getValueTreeState(), parameterId, control.slider);
    control.slider.setDoubleClickReturnValue (false, control.slider.getDoubleClickReturnValue());

    control.value.onTextChange = [&control]
    {
        if (control.value.isBeingEdited())
            return;

        control.slider.setValue (control.slider.getValueFromText (control.value.getText()), juce::sendNotificationSync);
    };

    scaledContent.addAndMakeVisible (control.name);
    scaledContent.addAndMakeVisible (control.slider);
    scaledContent.addAndMakeVisible (control.value);
}

void DB5035AudioProcessorEditor::configureButton (ButtonControl& control, const juce::String& parameterId, const juce::String& labelText)
{
    styleLabel (control.name, labelText);

    control.button.setButtonText ("");
    control.button.setClickingTogglesState (true);
    control.button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff252622));
    control.button.setColour (juce::TextButton::buttonOnColourId, cream);

    control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getValueTreeState(), parameterId, control.button);

    scaledContent.addAndMakeVisible (control.name);
    scaledContent.addAndMakeVisible (control.button);
}

void DB5035AudioProcessorEditor::configureCommandButton (CommandButtonControl& control, const juce::String& labelText)
{
    styleLabel (control.name, labelText);
    if (labelText == "UNDO")
        control.button.setButtonText ("<");
    else if (labelText == "REDO")
        control.button.setButtonText (">");
    else
        control.button.setButtonText (labelText);

    control.button.setClickingTogglesState (false);
    control.button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1d1b));
    control.button.setColour (juce::TextButton::textColourOffId, cream);
    control.button.setColour (juce::TextButton::buttonOnColourId, cream);

    scaledContent.addAndMakeVisible (control.name);
    scaledContent.addAndMakeVisible (control.button);
}

void DB5035AudioProcessorEditor::layoutKnob (KnobControl& control, juce::Rectangle<int> bounds)
{
    control.name.setBounds (bounds.removeFromBottom (28));
    control.value.setBounds (bounds.removeFromBottom (24));
    auto sliderBounds = bounds.reduced (18, 12);
    sliderBounds.translate (0, -18);
    control.slider.setBounds (sliderBounds);
}

void DB5035AudioProcessorEditor::layoutButton (ButtonControl& control, juce::Rectangle<int> bounds)
{
    control.name.setBounds (bounds.removeFromTop (26));
    const auto diameter = juce::jmin (36, bounds.getHeight());
    control.button.setBounds (bounds.withSizeKeepingCentre (diameter, diameter));
}

void DB5035AudioProcessorEditor::layoutCommandButton (CommandButtonControl& control, juce::Rectangle<int> bounds)
{
    control.name.setBounds (bounds.removeFromTop (22));
    control.button.setBounds (bounds.withSizeKeepingCentre (42, 28));
}

void DB5035AudioProcessorEditor::drawHardwareFrame (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setGradientFill (juce::ColourGradient (panelTop, bounds.getTopLeft().toFloat(),
                                             panel, bounds.getBottomLeft().toFloat(), false));
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);

    g.setColour (juce::Colour (0xff565650));
    g.drawRoundedRectangle (bounds.toFloat(), 4.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.32f));
    g.fillRect (bounds.removeFromBottom (16));
}

void DB5035AudioProcessorEditor::drawSignature (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto meterWidth = 150;
    auto signatureArea = bounds.removeFromRight (meterWidth).reduced (4, 0);
    signatureArea = signatureArea.withTrimmedTop (30).withHeight (86);

    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (-0.10f,
                                                     (float) signatureArea.getCentreX(),
                                                     (float) signatureArea.getCentreY()));

    const auto signatureText = juce::String::fromUTF8 ("\xe9\x9d\x92");
    auto inkArea = signatureArea.reduced (36, 4).translated (-4, 0);
    const auto signatureFont = juce::FontOptions (signatureTypefaceName(), 62.0f, juce::Font::plain);

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

void DB5035AudioProcessorEditor::drawKnobScale (juce::Graphics& g,
                                                juce::Rectangle<int> bounds,
                                                const juce::StringArray& labels,
                                                float startAngle,
                                                float endAngle,
                                                bool majorLabels)
{
    bounds.removeFromBottom (28);
    bounds.removeFromBottom (24);
    auto dialBounds = bounds.reduced (18, 12);
    dialBounds.translate (0, -18);
    const auto dial = dialBounds.toFloat();
    const auto centre = dial.getCentre();
    const auto radius = juce::jmin (dial.getWidth(), dial.getHeight()) * 0.48f;
    const auto tickOuter = radius + 9.0f;
    const auto tickInner = radius + 2.0f;

    g.setColour (line);

    const auto tickCount = 17;
    for (int i = 0; i < tickCount; ++i)
    {
        const auto t = (float) i / (float) (tickCount - 1);
        const auto angle = startAngle + t * (endAngle - startAngle);
        const auto isMajor = i % 5 == 0 || (majorLabels && labels.size() == 6 && i % 4 == 0);
        const auto outer = pointOnCircle (dial, tickOuter, angle);
        const auto inner = pointOnCircle (dial, isMajor ? tickInner - 4.0f : tickInner, angle);
        g.drawLine ({ inner, outer }, isMajor ? 2.0f : 1.0f);
    }

    g.setFont (uiFont (majorLabels ? 13.0f : 12.0f, juce::Font::bold));
    g.setColour (text);

    for (int i = 0; i < labels.size(); ++i)
    {
        const auto t = labels.size() == 1 ? 0.5f : (float) i / (float) (labels.size() - 1);
        const auto angle = startAngle + t * (endAngle - startAngle);
        const auto p = juce::Point<float> { centre.x + std::cos (angle) * (radius + 24.0f),
                                            centre.y + std::sin (angle) * (radius + 24.0f) };
        g.drawText (labels[i], (int) p.x - 24, (int) p.y - 8, 48, 16, juce::Justification::centred);
    }
}

void DB5035AudioProcessorEditor::updateValueLabels()
{
    auto& state = audioProcessor.getValueTreeState();

    for (size_t index = 0; index < knobs.size(); ++index)
    {
        if (knobs[index].value.isBeingEdited())
            continue;

        if (auto* parameter = state.getParameter (knobParameterIds[index]))
            knobs[index].value.setText (formatValue (*parameter), juce::dontSendNotification);
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
    if (event.mods.isAltDown())
    {
        setValue (getDoubleClickReturnValue(), juce::sendNotificationSync);
        return;
    }

    if (valueLabel != nullptr)
    {
        valueLabel->setText (getTextFromValue (getValue()), juce::dontSendNotification);
        valueLabel->showEditor();
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

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (bounds.translated (3.0f, 4.0f));

    g.setGradientFill (juce::ColourGradient (knobColour.brighter (0.28f), bounds.getTopLeft(),
                                             knobColour.darker (0.55f), bounds.getBottomRight(), false));
    g.fillEllipse (bounds);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawEllipse (bounds, 2.0f);

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
                                                                           bool shouldDrawButtonAsHighlighted,
                                                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const auto on = button.getToggleState();
    const auto base = on ? cream : juce::Colour (0xff1c1d1b);

    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.fillEllipse (bounds.translated (2.0f, 3.0f));

    g.setGradientFill (juce::ColourGradient (base.brighter (on ? 0.08f : 0.18f), bounds.getTopLeft(),
                                             base.darker (on ? 0.15f : 0.38f), bounds.getBottomRight(), false));
    g.fillEllipse (bounds);

    g.setColour (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted ? amber : juce::Colour (0xff0c0c0a));
    g.drawEllipse (bounds, 1.5f);
}

void DB5035AudioProcessorEditor::HardwareLookAndFeel::drawButtonText (juce::Graphics& g,
                                                                      juce::TextButton& button,
                                                                      bool,
                                                                      bool)
{
    if (button.getButtonText().isEmpty())
        return;

    g.setColour (button.isEnabled() ? cream : muted.withAlpha (0.45f));
    g.setFont (uiFont (16.0f, juce::Font::bold));
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

    const auto startAngle = juce::MathConstants<float>::pi * 7.0f / 6.0f;
    const auto endAngle = juce::MathConstants<float>::pi * 11.0f / 6.0f;
    const auto sweep = endAngle - startAngle;

    const auto meterMin = -20.0f;
    const auto meterMax = 3.0f;

    float displayValue = valueDb;

    if (mode == Mode::reduction)
        displayValue = -valueDb;

    const auto normalised = juce::jlimit (0.0f, 1.0f, (displayValue - meterMin) / (meterMax - meterMin));
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
    const auto h = bounds.getHeight();

    const auto meterSize = juce::jmin (w, h * 1.3f);
    const auto meterBounds = bounds.withSizeKeepingCentre (meterSize, meterSize * 0.77f);
    const auto centre = juce::Point<float> (meterBounds.getCentreX(), meterBounds.getBottom() - meterSize * 0.06f);
    const auto radius = meterSize * 0.38f;

    g.setColour (juce::Colour (0xff1a1a16));
    g.fillRoundedRectangle (meterBounds, 6.0f);

    auto innerBg = meterBounds.reduced (3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2a24), innerBg.getCentre().toFloat(),
                                              juce::Colour (0xff1e1e1a), innerBg.getBottomLeft().toFloat(), false));
    g.fillRoundedRectangle (innerBg, 5.0f);

    g.setColour (juce::Colour (0xff3a3a34));
    g.drawRoundedRectangle (meterBounds, 6.0f, 1.2f);

    const auto startAngle = juce::MathConstants<float>::pi * 7.0f / 6.0f;
    const auto endAngle = juce::MathConstants<float>::pi * 11.0f / 6.0f;
    const auto totalSweep = endAngle - startAngle;

    const auto isReduction = (mode == Mode::reduction);
    const auto isInput = (mode == Mode::input);

    const auto arcRadius = radius;
    const auto tickOuterR = arcRadius;
    const auto tickInnerMajorR = arcRadius - 12.0f;
    const auto tickInnerMinorR = arcRadius - 7.0f;
    const auto labelR = arcRadius - 20.0f;

    struct TickMark
    {
        float normalised;
        juce::String label;
        bool isMajor;
    };

    juce::Array<TickMark> ticks;

    static const int dbValues[] = { -20, -15, -10, -7, -5, -3, -1, 0, 1, 2, 3 };
    for (int db : dbValues)
    {
        const auto norm = (float) (db - (-20)) / (3.0f - (-20.0f));
        ticks.add ({ norm, juce::String (db), (db % 5 == 0 || db == 0) });
    }

    for (auto& tick : ticks)
    {
        const auto angle = startAngle + tick.normalised * totalSweep;
        const auto outerX = centre.x + std::cos (angle) * tickOuterR;
        const auto outerY = centre.y + std::sin (angle) * tickOuterR;
        const auto innerR = tick.isMajor ? tickInnerMajorR : tickInnerMinorR;
        const auto innerX = centre.x + std::cos (angle) * innerR;
        const auto innerY = centre.y + std::sin (angle) * innerR;

        const bool inRedZone = tick.normalised > 0.8696f;
        g.setColour (inRedZone ? juce::Colour (0xffcc4444) : juce::Colour (0xffd6cfaa));
        g.drawLine (innerX, innerY, outerX, outerY, tick.isMajor ? 1.8f : 0.8f);

        if (tick.isMajor && tick.label.isNotEmpty())
        {
            const auto lx = centre.x + std::cos (angle) * labelR;
            const auto ly = centre.y + std::sin (angle) * labelR;
            g.setFont (uiFont (8.5f, juce::Font::bold));
            g.setColour (inRedZone ? juce::Colour (0xffcc4444) : juce::Colour (0xffc8c0a8));
            g.drawText (tick.label,
                        juce::roundToInt (lx) - 16, juce::roundToInt (ly) - 6, 32, 12,
                        juce::Justification::centred);
        }
    }

    {
        const auto zeroNorm = (0.0f - (-20.0f)) / 23.0f;
        const auto redStart = startAngle + zeroNorm * totalSweep;
        juce::Path redArc;
        redArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, redStart, endAngle, true);
        g.setColour (juce::Colour (0xffcc4444).withAlpha (0.30f));
        g.strokePath (redArc, juce::PathStrokeType (3.0f));
    }

    g.setFont (uiFont (10.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffa09880));
    const auto modeLabel = isInput ? "INPUT" : (isReduction ? "REDUCTION" : "OUTPUT");
    g.drawText (modeLabel, juce::roundToInt (centre.x) - 40, juce::roundToInt (centre.y - radius * 0.35f), 80, 14, juce::Justification::centred);

    g.setFont (uiFont (8.0f));
    g.setColour (juce::Colour (0xff807868));
    g.drawText ("dB",
                juce::roundToInt (centre.x) - 16, juce::roundToInt (centre.y - radius * 0.16f), 32, 12,
                juce::Justification::centred);

    const auto needleAngle = smoothedAngle;
    const auto needleLength = arcRadius - 2.0f;
    const auto needleTip = juce::Point<float> (centre.x + std::cos (needleAngle) * needleLength,
                                                centre.y + std::sin (needleAngle) * needleLength);
    const auto needleTail = juce::Point<float> (centre.x - std::cos (needleAngle) * (radius * 0.15f),
                                                 centre.y - std::sin (needleAngle) * (radius * 0.15f));

    const auto perpX = -std::sin (needleAngle);
    const auto perpY = std::cos (needleAngle);
    const auto halfBase = 1.6f;

    juce::Path needlePath;
    needlePath.startNewSubPath (needleTip);
    needlePath.lineTo (centre.x + perpX * halfBase, centre.y + perpY * halfBase);
    needlePath.lineTo (needleTail);
    needlePath.lineTo (centre.x - perpX * halfBase, centre.y - perpY * halfBase);
    needlePath.closeSubPath();

    g.setColour (juce::Colour (0xff111111));
    g.fillPath (needlePath, juce::AffineTransform::translation (1.0f, 1.5f));

    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillPath (needlePath);

    const auto pivotRadius = 4.5f;
    g.setColour (juce::Colour (0xff2a2a2a));
    g.fillEllipse (centre.x - pivotRadius, centre.y - pivotRadius, pivotRadius * 2.0f, pivotRadius * 2.0f);
    g.setColour (juce::Colour (0xff444440));
    g.drawEllipse (centre.x - pivotRadius, centre.y - pivotRadius, pivotRadius * 2.0f, pivotRadius * 2.0f, 0.8f);

    g.setFont (monoFont (10.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffc8c0a8));
    juce::String valueLabel;
    if (isReduction)
        valueLabel = juce::String (valueDb, 1) + (showPeakHold ? " | " + juce::String (heldPeakDb, 1) : "") + " dB";
    else
        valueLabel = juce::String (valueDb, 1) + " dB";
    g.drawText (valueLabel, meterBounds.getBottomLeft().getX(), juce::roundToInt (meterBounds.getBottom() - 16), juce::roundToInt (meterBounds.getWidth()), 14,
                juce::Justification::centred);
}
