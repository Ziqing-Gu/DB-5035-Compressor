# DB-5035 Qing Compressor 1.1.0 — Mac Build Notes

This project includes both CMake and Projucer setup.

## Projucer workflow

1. Install Xcode.
2. Install JUCE/Projucer. Use JUCE 8.x if possible.
3. Open `DB-5035 Qing Compressor.jucer` in Projucer.
4. If Projucer reports missing modules, set the module path to your local JUCE `modules` folder.
5. Save the project to generate `Builds/MacOSX`.
6. Open the generated Xcode project.
7. Build the `DB-5035 Qing Compressor - VST3` target in Release.

The project retains its official VST3, AU, AAX, Standalone and CLAP format configuration.

## CMake workflow

If you prefer CMake:

```sh
cmake -S . -B build-mac -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-mac --config Release
```

The VST3 output is `DB-5035 Qing Compressor.vst3` and keeps the official identity used by version 1.09.
