from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROCESSOR = (ROOT / "Source" / "PluginProcessor.cpp").read_text(encoding="utf-8")
EDITOR = (ROOT / "Source" / "PluginEditor.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
JUCER = (ROOT / "DB-5035 Qing Compressor.jucer").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


require("project(DB5035Compressor VERSION 1.2.2)" in CMAKE,
        "CMake product version is not 1.2.2")
require('version="1.2.2"' in JUCER,
        "Projucer product version is not 1.2.2")

host_bypass = PROCESSOR.index("ParamID::hostBypass, 1")
measurement = PROCESSOR.index("ParamID::loudnessMeasurement, 2")
require(measurement > host_bypass,
        "The new parameter must remain appended after every 1.2.0 parameter")
require('ParamID::loudnessMeasurement, 2 }, "Loudness Measurement", true' in PROCESSOR,
        "The loudness measurement parameter must default to ON")

require('getChildWithProperty ("id", ParamID::loudnessMeasurement)' in PROCESSOR
        and 'measurementState.setProperty ("value", 1.0f' in PROCESSOR,
        "Old states without the field must restore with measurement ON")
require("loudnessMeasurementEnabled && ! hostBypassed && currentParameters.compIn" in PROCESSOR,
        "Measurement eligibility is not gated by the new toggle")
enabled_start = PROCESSOR.index("if (loudnessMeasurementEnabled)\n    {")
enabled_end = PROCESSOR.index("    auto transportDiscontinuity", enabled_start)
enabled_block = PROCESSOR[enabled_start:enabled_end]
require("if (auto* hostPlayHead = getPlayHead())" in enabled_block,
        "Host playhead polling must be bypassed while measurement is OFF")
require("if (matchNeedsPrepare)" in enabled_block and "prepareMatchMeasurement();" in enabled_block,
        "Deferred measurement preparation must be gated by ON")
require("resetForMeasurementStateChange = loudnessMeasurementEnabled\n                                             && matchResetRequested.exchange" in PROCESSOR,
        "The BS.1770 accumulator must not be touched while measurement is OFF")
require("if (loudnessMeasurementEnabled\n        && (resetForMeasurementStateChange" in PROCESSOR,
        "All measurement resets must remain bypassed while measurement is OFF")
require("if (measureMatch)\n        {\n            copyAudioBuffer" in PROCESSOR,
        "Dry/Wet measurement buffers must remain gated by measureMatch")
require("if (measureMatch)\n        updateMatchResult();" in PROCESSOR,
        "Repeated loudness result recomputation must remain gated by measureMatch")

require('ButtonAttachment> (\n        audioProcessor.getValueTreeState(), "loudnessMeasurement"' in EDITOR,
        "The toggle must be persisted through APVTS project/preset state")
require('enabled ? "LUFS ON" : "LUFS OFF"' in EDITOR,
        "The UI must clearly show the current ON/OFF state")
require("measurementEnabled && ready" in EDITOR,
        "Match must be disabled while measurement is OFF")
require('juce::String ("Version ") + JucePlugin_VersionString' in EDITOR,
        "Help must display the synchronized product version")
require("完全停止响度滤波" in EDITOR and "重新打开会从零开始一次新测量" in EDITOR,
        "Help must document the OFF bypass and fresh ON measurement behavior")

print("PASS: DB-5035 1.2.2 loudness measurement toggle contract")
