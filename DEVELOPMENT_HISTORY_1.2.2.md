# 1.2.2 开发记录 / Development history
Date: 2026-11-17. Status: Stable, explicitly designated by the user.
Author/maintainer: Ziqing-Gu / Qing Audio, with Codex-assisted implementation.
Source: https://github.com/Ziqing-Gu/DB-5035-Compressor

## 问题与修复 / Findings and fixes
- Short-circuit handling left a reset flag unconsumed and reset the next measurement block again. Consume reset flags explicitly.
- Legacy state migration must use the APVTS PARAM child rather than a root property. Add default ON when the child is missing, and discard stale measurements on preset restore.
- OFF preparation is deferred and OFF skips the complete loudness path; ON initializes/reset once. Pending resets prevent stale Match application during rapid switches.
- Append the new parameter after existing parameters, using AU version hint 2 to retain legacy automation ordering.
- Scale narrow command strips so Classic and Vintage controls do not overlap.
- 同步修复：首次测量重复清零、旧状态字段位置、重新载入时过期建议、OFF 延迟准备和快速切换；保留 AU 原参数顺序，并兼容窄窗口。

## 验证 / Validation
Windows Release build and CTest 2/2 passed: BS.1770 self-test and 122 production
processor/UI checks. 24 combinations of 1x/2x/4x/8x oversampling, mono/stereo and
44.1/48/96 kHz produce bit-identical audio with ON/OFF/toggling, with unchanged Gain.
Both skins were checked at widths 400/640/1120/1200/1920. OFF freezes measurement
state/buffers and avoids playhead reads; state round-trips, fresh 400 ms startup and
single-click apply were checked. Steinberg loading/category validation passed.
Windows FileVersion/ProductVersion/moduleinfo and Help are 1.2.2; the installed
bundle matched the verified output. User subsequently designated this version Stable.

中文：Windows 编译、自测及 122 项运行检查通过；24 组音频一致性和五种窗口宽度已检查，
旧状态及预设恢复已检查。Steinberg 加载/分类通过；版本及安装成品一致。
用户随后明确指定 Stable。自动检查不等于所有 DAW 实测。

## 发布过程 / Release process
Plan A is reused, without rebuilding/reinstalling Windows. Plan B was completed
and frozen before source publication. Plan C publishes shared sources and builds
Apple Silicon VST3, Intel VST3 and Universal 2 AU, then delivers a desktop package.
Plan D publishes the same repository's Release and direct README download links.
No repeated backup, no source deletion, and no change to the current license.
Mac CI results and the release link are recorded in the release completion notes.

后续每次修改代码，必须追加需求、原因、实施、验证和遗留事项，不覆盖旧开发经过。
Every future code change must append request, cause, implementation, verification
and outstanding work; do not overwrite historical development records.
