# Changelog / 版本记录

## 2026-11-17 发布区间 / Publication range

Last remote / 上次远端: 1.2.0 (main 0b324f861e2399af45c54f5b7c6c39011b63fcf1, Release v1.2.0).
Target / 本次目标: 1.2.2. Actual versions / 实际版本: 1.2.1, 1.2.2.
README, development records and this changelog cover both; missing: none / 遗漏：无。

### 1.2.2 — Stable：响度开关完善 / Loudness-toggle refinements
**日期 / Date:** 2026-11-17. **状态 / Status:** Stable，用户明确指定 / explicitly designated by the user.
- 中文：修复首次测量的重复清零；旧工程缺少开关时明确恢复为 ON；重新载入预设及快速 OFF→ON 后丢弃旧测量，不会误用旧建议。新增参数位于原参数之后，并使用独立 AU version hint，保持原参数自动化顺序。小窗口命令条避免重叠；Help、工程及插件版本统一为 1.2.2。
- English: Fixed a repeated reset at measurement start; legacy sessions explicitly default to ON; preset reload and rapid OFF→ON transitions discard stale measurements. The new parameter is appended with its own AU version hint to retain existing automation order. The command strip avoids overlap in narrow windows; Help, project and plug-in metadata report 1.2.2.
- 中文：OFF 完全跳过播放头查询、测量副本、K-weighting、LUFS 统计和 Match 计算。Match 灰显且不可点击，但已应用的 Gain 不会自动撤回，压缩、Blend 和 VU 保持原行为。ON 重新累计播放期间的 Dry / 全 Wet 响度，至少一个 400 ms 测量块后可得到建议；点击 Match 写入 Gain。
- English: OFF bypasses playhead polling, measurement copies, K-weighting, LUFS statistics and Match calculations. Match is greyed out and disabled; previously applied Gain is retained, with compression, Blend and VU unchanged. ON starts fresh Dry/full-Wet playback measurement; at least one 400 ms block is needed before a recommendation can be applied by clicking Match.
- 中文：Windows 已完成 BS.1770 自测、122 项生产处理器/界面检查、24 组逐样本音频一致性检查及 Steinberg 加载/分类检查，已交付并由用户指定 Stable。Mac 三类成品由同源 CI 构建验证；具体结果随本次发布记录补充，不将自动检查等同于全部宿主实测。
- English: Windows passed BS.1770 self-tests, 122 processor/UI checks, 24 bit-identical audio comparisons and Steinberg loading/category checks, and was delivered and designated Stable. Three Mac artifacts are built from the same source in CI; release records state the actual results. Automated validation is not comprehensive DAW testing.
- 中文：DSP、响度算法、资源、Gain 精度及原有插件标识均未更改。新开关随工程/预设保存，默认 ON；兼容 1.2.0 及更早状态。Mac 尚未经过 Apple Developer ID 公证。
- English: DSP, loudness algorithm, resources, Gain precision and plug-in identity are unchanged. The new toggle is saved in sessions/presets and defaults to ON, including legacy states. Mac builds are not Apple Developer ID notarized.

### 1.2.1 — 响度测量开关初版 / Initial loudness measurement toggle
**日期 / Date:** 2026-11-17. **状态 / Status:** 已交付的开发版本，未单独发布 Release，已由 1.2.2 取代 / delivered development version, not separately released, superseded by 1.2.2.
- 中文：Classic/Vintage 共用顶部新增 LUFS ON/OFF；OFF 禁用 Match 并停止响度测量/统计/计算；重新 ON 从零测量。开关随工程预设保存，缺失字段的旧工程使用 ON；Gain、Blend、VU、A/B 与压缩声音不受切换影响。CMake、Projucer、Help 和插件元数据为 1.2.1。
- English: Added a shared Classic/Vintage LUFS ON/OFF control. OFF disables Match and stops loudness measurement/statistics/calculation; ON restarts measurement. The toggle is stored in sessions/presets, with ON for missing legacy fields; Gain, Blend, VU, A/B and compression audio are unaffected. CMake, Projucer, Help and plug-in metadata were 1.2.1.
- 中文：实现位于 Windows/macOS VST3/AU 共用源码；当时仅交付 Windows 测试，恢复状态与测量启动细节随后在 1.2.2 中完善。
- English: Implemented in the shared Windows/macOS VST3/AU sources; only a Windows test build was delivered then. State restoration and measurement-start details were subsequently refined in 1.2.2.


## 以前的发布记录 / Earlier publication records


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
