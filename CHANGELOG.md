# Changelog / 版本记录

本文件记录公开源码版本变化。README 保留直接可读的中英双语概要；本文件提供更详细的行为与验证记录。

## 发布区间核对 / Release-range audit

- 远端上次完整记录版本 / Last fully documented remote version: `1.1.0`
- 本次目标版本 / Target version: `1.2.0`
- 证据 / Evidence: public `main` commit `e8e2e21`, remote README, local Git history, and 1.2.0 stable handoff
- 区间内真实版本 / Real versions in range: `1.2.0`
- README 已记录 / Documented in README: `1.2.0`
- CHANGELOG 已记录 / Documented in CHANGELOG: `1.2.0`
- 遗漏版本 / Missing versions: 无 / None
- 开发状态补充 / Development-state note: the Match Gain work existed briefly as an unversioned candidate based on 1.1.0, then was explicitly promoted to 1.2.0 and approved as Stable. It was not a separate numbered release.

## 1.2.0 — Integrated LUFS Match Gain

**发布日期 / Release date:** 2026-08-26
**状态 / Status:** Stable
**基于 / Based on:** 1.1.0

### 中文

#### 新增功能

- 新增播放期间的自动 Match Gain，采用 ITU-R BS.1770 / EBU R128 Integrated LUFS。
- 新增 Classic/Vintage 共用的顶部三态按钮：`MATCH`、`M...`、`M ±x.xx`。

#### Bug 修复

- 修复最初增量构建中 Windows 文件属性仍显示 1.1.0 的资源缓存问题；正式构建改用全新目录后，Help、FileVersion、ProductVersion 与 VST3 清单统一为 1.2.0。

#### 行为变化

- Match 比较 Dry 与用户 Gain/Blend 之前的全 Wet 信号，并把建议值写入现有 `makeupGain`。
- Gain 真实步进和 UI 显示提高到 0.01 dB。
- 播放重启、播放位置跳变、旁通或 COMP IN 不可用时重新开始累计。

#### 性能优化

- LUFS 测量使用固定 400 ms block 与 100 ms hop；没有改变既有压缩核心、过采样、Ratio 或 Timing 校准。

#### 兼容性变化

- 没有新增、删除或重排宿主参数；旧项目、自动化和 A/B 快照继续兼容。

#### 已知问题与验证

- Windows x64 已完成 Release 编译、BS.1770 自测、Steinberg validator、宿主实测和用户确认。
- macOS Apple Silicon VST3、Intel VST3 和 Universal 2 AU 需要在 Plan D 从同一公开 commit 构建并验证。

#### 升级注意事项

- 升级前完全关闭 DAW，只保留一个 `DB-5035 Qing Compressor.vst3` 副本，覆盖后重新扫描插件。

### English

#### Added

- Added automatic Match Gain during transport playback using ITU-R BS.1770 / EBU R128 Integrated LUFS.
- Added a compact shared Classic/Vintage three-state control: `MATCH`, `M...`, and `M ±x.xx`.

#### Fixed

- Fixed stale Windows 1.1.0 file metadata from the first incremental candidate build. A clean build now aligns Help, FileVersion, ProductVersion, and the VST3 manifest at 1.2.0.

#### Changed behavior

- Match compares Dry with fully processed Wet before user Gain and Blend, then writes the recommendation to the existing `makeupGain`.
- Gain parameter step and UI display precision are now 0.01 dB.
- Measurement restarts after new playback, a playhead discontinuity, bypass, or unavailable COMP IN processing.

#### Performance

- Loudness measurement uses fixed 400 ms blocks and a 100 ms hop. Existing compression, oversampling, Ratio, and Timing calibration are unchanged.

#### Compatibility

- No host parameter was added, deleted, or reordered. Existing projects, automation, and A/B snapshots remain compatible.

#### Known limitations and validation

- Windows x64 passed a Release build, BS.1770 self-test, Steinberg validator, host testing, and user acceptance.
- macOS Apple Silicon VST3, Intel VST3, and Universal 2 AU must still be built and validated from the same public commit during Plan D.

#### Upgrade notes

- Fully quit the DAW before upgrading, keep only one `DB-5035 Qing Compressor.vst3` copy, and rescan plug-ins after replacement.
