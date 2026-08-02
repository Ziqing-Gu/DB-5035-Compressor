# DB-5035 Compressor VST3

Current plugin version: 1.08

Manufacturer: Qing Audio

VST3 category: Fx / Dynamics

This is a JUCE/CMake VST3 project for a diode-bridge-inspired compressor.

It is not affiliated with, endorsed by, or an exact model of Shelford, Neve, or any related trademarked product.

## Requirements

- Visual Studio 2022 with the C++ desktop workload
- CMake 3.22 or newer
- JUCE 8.0.13 or newer, either downloaded by CMake or available locally

JUCE is GPL/commercial dual licensed. Check the license before distributing a closed-source plugin.

## Build

From this folder:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The VST3 target is copied after build by JUCE. The build output is normally under:

```text
build/DB5035Compressor_artefacts/Release/VST3/DB-5035 Compressor.vst3
```

To use a local JUCE checkout instead of downloading JUCE:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDB5035_FETCH_JUCE=OFF -DDB5035_JUCE_PATH=C:\path\to\JUCE
cmake --build build --config Release
```

## DSP

The processing core is in:

```text
Source/DiodeBridgeCompressor.h
```

It includes:

- sidechain high-pass detection;
- soft-knee feed-forward compression;
- diode-style tanh drive with slight asymmetry;
- program-dependent sag;
- wet/dry mix, output trim, and host-automatable parameters.
