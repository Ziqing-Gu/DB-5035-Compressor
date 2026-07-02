cmake_minimum_required(VERSION 3.22)

project(DB5035Compressor VERSION 0.1.0)

option(DB5035_FETCH_JUCE "Download JUCE with CMake FetchContent" ON)
set(DB5035_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout when DB5035_FETCH_JUCE is OFF")

if(DB5035_FETCH_JUCE)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.13
    )
    FetchContent_MakeAvailable(JUCE)
else()
    if(NOT DB5035_JUCE_PATH)
        message(FATAL_ERROR "Set DB5035_JUCE_PATH to a local JUCE checkout, or enable DB5035_FETCH_JUCE.")
    endif()
    add_subdirectory("${DB5035_JUCE_PATH}" JUCE)
endif()

add_subdirectory(../clap-juce-extensions clap-juce-extensions)

juce_add_plugin(DB5035Compressor
    COMPANY_NAME "Codex Audio"
    BUNDLE_ID "com.codexaudio.db5035qingcompressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE CdxA
    PLUGIN_CODE Db3Q
    FORMATS VST3 AAX Standalone CLAP
    AAX_CATEGORY AAX_ePlugInCategory_Dynamics
    PRODUCT_NAME "DB-5035 Qing Compressor"
)

clap_juce_extensions_plugin(
    TARGET DB5035Compressor
    CLAP_ID "com.codexaudio.qing-compressor"
    CLAP_FEATURES audio-effect stereo
)

juce_generate_juce_header(DB5035Compressor)

target_sources(DB5035Compressor
    PRIVATE
        Source/DiodeBridgeCompressor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
)

target_compile_features(DB5035Compressor PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(DB5035Compressor PRIVATE /utf-8)
endif()

target_compile_definitions(DB5035Compressor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DB5035Compressor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        clap_juce_extensions
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
