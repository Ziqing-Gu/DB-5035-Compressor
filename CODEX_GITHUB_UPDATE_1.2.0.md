# DB-5035 Qing Compressor 1.2.0 — Codex / GitHub 更新说明

## 中文结论

1.2.0 在 1.1.0 稳定基线上加入符合 BS.1770 / EBU R128 的 Integrated LUFS Match Gain，并把 Gain 精度提高到 0.01 dB。用户已完成 Windows 宿主测试并确认将其设为稳定基线。

## English summary

Version 1.2.0 adds an Integrated LUFS Match Gain workflow based on BS.1770 / EBU R128 and increases Gain precision to 0.01 dB. The user completed Windows host testing and approved this build as the new stable baseline.

## 主要修改 / Main changes

- 新增 `vst3-juce/Source/BS1770LoudnessMatch.h`。
- Dry 与全 Wet 在播放期间分别累计 Integrated LUFS；Wet 测量点位于用户 Gain 与 Blend 之前。
- Match 结果写入原有 `makeupGain`，不增加或重排宿主参数。
- 顶部命令条新增紧凑的 `MATCH` / `M...` / `M ±x.xx` 三态按钮，Classic 与 Vintage 共用。
- Gain 步进和显示精度提高到 0.01 dB。
- CMake、Projucer、Help、Windows 元数据和工作流包名统一为 1.2.0。
- Added `vst3-juce/Source/BS1770LoudnessMatch.h`.
- Dry and fully processed Wet Integrated LUFS are accumulated during playback; Wet is measured before user Gain and Blend.
- Match writes to the existing `makeupGain` without adding or reordering host parameters.
- A compact shared `MATCH` / `M...` / `M ±x.xx` control is available in Classic and Vintage.
- Gain step and display precision are increased to 0.01 dB.
- CMake, Projucer, Help, Windows metadata, and workflow package names are synchronized to 1.2.0.

## 验证 / Verification

- BS.1770 1 kHz 自测：`-3.0036 LUFS`。
- 固定 6 dB 差值自测：`+6.00 dB`。
- Windows x64 Release 编译通过。
- Steinberg VST3 validator 退出码 `0`，分类为 `Fx / Dynamics`。
- Windows 构建件、安装件和输出件 SHA-256 相同。
- Help、FileVersion、ProductVersion 与 VST3 清单均为 `1.2.0`。

## README 要求 / README requirement

README 的旧版本内容必须全部保留。只在顶部追加 1.2.0 中英双语章节，并使用 `docs/images/db5035-match-gain-1.2.0.png` 介绍新功能。

All previous README release notes must be preserved. Add the bilingual 1.2.0 section near the top and use `docs/images/db5035-match-gain-1.2.0.png` to introduce Match Gain.

## 正式插件身份——不得改变

- Product Name：`DB-5035 Qing Compressor`
- Plugin Code：`Db3Q`
- Manufacturer Code：`CdxA`
- Bundle ID：`com.codexaudio.db5035qingcompressor`
- CLAP ID：`com.codexaudio.qing-compressor`

这些身份值必须继续保持与 1.1.0 一致，确保 1.2.0 是直接升级而不是新的并存插件。

## 后续 AI 交接要求

每次更新代码后都必须继续记录过往开发经过、问题与验证结果，不得覆盖或删除旧记录。涉及正式发布时，要同步版本、README、Plan B 备份、GitHub 工作流以及 Plan D 跨平台交付物。