# 1.2.1 开发记录 / Development history
Date: 2026-11-17. Status: delivered Windows development build, superseded by 1.2.2.
Author/maintainer: Ziqing-Gu / Qing Audio, with Codex-assisted implementation.
Source: https://github.com/Ziqing-Gu/DB-5035-Compressor

## 需求与实现 / Request and implementation
Classic/Vintage 顶部新增 LUFS ON/OFF；关闭时 Match 灰显，不查询播放头、不复制测量信号、
不执行 K-weighting、LUFS 统计或匹配计算；声音、已有 Gain 与 Blend 不变。
Added a shared LUFS ON/OFF control. OFF disables Match and bypasses playhead reads,
measurement copies, K-weighting, LUFS statistics and match calculation, without
changing audio, existing Gain or Blend.

开关保存到状态，重新开启丢弃旧测量。实现使用 Windows/macOS VST3/AU 共用源码。
The state is persisted and re-enabling starts a fresh measurement in shared
Windows/macOS VST3/AU code.

## 后续 / Follow-up
此版曾交付 Windows，不抹去历史或改名冒充新版；测量启动/旧状态恢复细节在 1.2.2 完善。
This Windows version was delivered and remains real history. Measurement startup
and legacy state restoration were refined in 1.2.2. It did not have a separate Release.

每次更新代码必须追加开发经过和实际验证，不覆盖旧记录。
Every code update must append its development process and actual validation, preserving old records.
