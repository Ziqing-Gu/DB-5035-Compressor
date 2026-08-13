# DB-5035 Qing Compressor

Current plugin version: 1.1.0

Version 1.1.0 promotes the successfully verified Test C DSP correction to the
official plug-in identity. It is a direct update for version 1.09.

Version 1.1.0 changes:

- slew-limited Fast Timing Transient Pass (0.20 ms rise / 1.0 ms fall);
- maximum Transient Pass reduced from 0.98 to 0.65;
- Release Memory entry smoothed over 2.0 ms;
- all other compressor calibration remains unchanged.

The Help overlay displays `Version 1.1.0`.

## Version 1.09

- Added a host-visible bypass parameter so DAWs can use the plug-in's own
  latency-matched bypass path.
- The bypass path now applies a pure delay equal to the currently reported
  oversampling latency. This keeps DAW bypass aligned in parallel-processing
  workflows and prevents comb filtering caused by a timing jump.
- The latency-matched dry path is continuously primed during normal processing,
  avoiding a silent gap when bypass is engaged.
- Reset all detector/envelope history when `COMP IN` is disabled, preventing
  stale gain reduction when compression is re-enabled.
- The Help overlay reads `JucePlugin_VersionString`, so CMake and Projucer builds
  now display `Version 1.09` to the user.

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
build/DB5035Compressor_artefacts/Release/VST3/DB-5035 Qing Compressor.vst3
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
