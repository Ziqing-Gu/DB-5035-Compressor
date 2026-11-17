# 构建 / Building DB-5035 Qing Compressor 1.2.2

## 源码与依赖 / Source and dependencies

源代码根目录 / Source root: `source-vst3-aax-projucer-ui-fixes-20260614-194248/vst3-juce`.
JUCE 8.0.13 is fetched by CMake; clap-juce-extensions is vendored in the sibling directory.
Requirements: CMake 3.22+, C++17, Visual Studio 2022 C++ on Windows or Xcode command-line tools on macOS.

## Windows

In a Visual Studio developer shell:
```powershell
cmake -S source-vst3-aax-projucer-ui-fixes-20260614-194248/vst3-juce -B build-win -G "Visual Studio 17 2022" -A x64
cmake --build build-win --config Release --target DB5035Compressor_VST3 DB5035MatchSelfTest DB5035ProcessorTest
ctest --test-dir build-win -C Release --output-on-failure
```

Windows 1.2.2 was already built and verified locally. Release packaging reuses that output;
the Windows workflow is manual-only and is not run again just for cross-platform delivery.
中文：Windows 发布复用已经通过 Plan A 的本地正式成品，默认不重复触发 Windows Actions。

## macOS

```sh
cmake -S source-vst3-aax-projucer-ui-fixes-20260614-194248/vst3-juce -B build-mac -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-mac --target DB5035Compressor_VST3 --parallel 3
```

Use x86_64 for Intel VST3. For Universal 2 AU, pass
`'-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'` and build DB5035Compressor_AU.
COPY_PLUGIN_AFTER_BUILD is FALSE. CI explicitly installs AU in the runner's user
Components folder before `auval -v aufx Db3Q CdxA`.
The manual macOS workflow builds only the three Mac packages, checks architecture
and version, ad-hoc signs/verifies bundles, tests native VST3 shared processing,
and validates AU on the native Apple Silicon runner. Intel AU architecture is
checked; this is not a claim of real Intel Logic testing or Apple notarization.

中文：Mac 最低编译目标为 macOS 11.0；三类构建使用同一提交。VST3 的两个原生架构运行
生产处理器与响度自测；Universal 2 AU 在 Apple Silicon runner 上运行 auval。
签名为 ad-hoc，并非 Developer ID 公证。完整宿主兼容性仍需实际 DAW 测试。

## Source identity / 同源

The public source files, tests, resources, project metadata and vendored dependency
match the Windows 1.2.2 source. The CMake relocations are the sibling path to
clap-juce-extensions and lowercase tests directory, preserving the public layout.
No compressor DSP, loudness algorithm or resource was changed for release.

Windows ZIP SHA-256:
`fcb23b70b3bb03c895b301663931b42299687cb2a4f022f5fb01ddb0372dcc2e`.

The formal source snapshot is a completed frozen historical record; subsequent
publishing never uses it as a mutable workspace or verification source.

CI now uses macos-15 / macos-15-intel, as macos-14 is retired. AU discovery requires
a force refresh of AudioComponentRegistrar on the disposable runner after copying
the component. The 1.2.2 user-directory installation passed native auval after that
refresh; no system-folder fallback was needed. The workflow supports source_ref and
platform selection to recover only missing builds, and retains signed artifacts
if validation fails for diagnosis. Never publish an artifact merely because it exists.
