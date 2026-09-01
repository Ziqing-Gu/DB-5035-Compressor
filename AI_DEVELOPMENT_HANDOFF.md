# DB-5035 Qing Compressor — AI Development Handoff

本文件是项目唯一的权威 AI 交接入口。详细旧过程保留在各版本开发记录中；后续只允许累积追加，不得删除或覆盖历史。

## 项目状态

- 项目名称：DB-5035 Qing Compressor
- 当前稳定版本：1.2.0
- 当前候选版本：无
- 基于版本：1.1.0
- 当前回滚基线：1.1.0
- 厂商：Qing Audio
- 主要平台：Windows x64、macOS Apple Silicon、macOS Intel
- 插件格式：VST3、Audio Unit（macOS）
- 主要开发环境：JUCE、CMake、Visual Studio 2022 / Xcode
- 主要测试环境：Windows x64 VST3；DAW 由用户自行测试

## 当前已完成

- 1.2.0 新增基于 ITU-R BS.1770 / EBU R128 Integrated LUFS 的 Match Gain。
- Match 测量 Dry 与用户 Gain/Blend 之前的全 Wet 信号，结果写入现有 `makeupGain`。
- Gain 参数步进和显示精度提高到 0.01 dB。
- Classic 与 Vintage 通过顶部命令条共用紧凑 Match 控件。
- CMake、Projucer、Help、Windows 文件版本、VST3 清单与包名已统一为 1.2.0。
- Windows Release 构建、BS.1770 自测、Steinberg validator、系统安装哈希和用户宿主测试均已完成；用户明确确认 1.2.0 为 Stable。

## 当前已知问题

- macOS Apple Silicon VST3、Intel VST3 与 Universal 2 AU 已在 Plan D 从同一公开 commit 构建，并完成架构、bundle、版本、哈希和 AU 验证；仍建议用户在真实 macOS 宿主中继续反馈兼容性。
- 项目许可证已由作者明确选择为 LicenseRef-Qing-Audio-NC-Source-Share-1.0；仓库根目录 `LICENSE` 保存未经改写的 Qing Audio NC Source-Share License 1.0 正文，README 保存当前项目声明；本许可证没有 “or later” 选项。

## 当前发布状态

- Plan A：Windows 本地构建、自测、validator、系统安装和用户宿主测试已完成。
- Plan B：正式源码快照已写入用户指定备份层级并通过逐文件 SHA-256 校验。
- Plan C：PR、main、v1.2.0 标签、README、CHANGELOG、许可证和远端内容核对已完成。
- Plan D：同一公开提交的四类成品、架构/版本/AU 验证、安装教程和桌面交付已完成。
---

## v1.2.0 — Integrated LUFS Match Gain

**日期：** 2026-08-26
**状态：** Stable
**基于：** v1.1.0

### 用户需求

增加与 QQ ChainScope、QQ Super Compressor 思路一致的播放期间 LUFS 响度匹配，并把 Gain 精度提高到小数点后两位；Vintage 面板空间有限，Match 不能破坏现有复古布局。

### 问题表现

- 原版没有自动响度匹配，用户需要手动补偿压缩前后的响度差。
- Gain 只有 0.1 dB 步进，难以与其它 LUFS Match 工具精确对齐。
- Vintage 面板没有预留大型新组件位置。

### 根因

原参数和 DSP 仅提供手动 makeup Gain，没有独立的 BS.1770 Integrated LUFS 测量器和匹配状态机。

### 尝试过的方案

- 讨论过匹配最终 Blend 后 Mix；因 Gain 只作用于 Wet 支路，Blend 小于 100% 时会形成错误目标，未采用。
- 初次增量构建沿用旧资源缓存，VST3 清单为 1.2.0、Windows 文件属性仍为 1.1.0；改用全新构建目录后解决。
- 最初候选仍标 1.1.0，用户指出难以区分后统一升级为 1.2.0。

### 修改内容

- 新增 BS.1770 / EBU R128 Integrated LUFS 测量：K-weighting、400 ms block、100 ms hop、绝对和相对门限。
- 播放期间同步累计 Dry 与全 Wet；播放重启、跳转、旁通或 COMP IN 不可用时重置。
- Match 结果通过正常宿主参数手势写入现有 `makeupGain`。
- 顶部命令条提供 `MATCH`、`M...`、`M ±x.xx` 三态 UI。
- Gain 步进和 Classic/Vintage 数值显示提高到 0.01 dB。

### 为什么这样修改

采用全 Wet、Gain/Blend 之前的测量点，可使结果与实际 Makeup Gain 作用点一致，并保持不同 Blend 设置下的定义稳定；共享顶部控件避免重画 Vintage 面板主体。

### 保持不变

- 原有参数 ID、顺序和范围未改变。
- 原工程、自动化、A/B 快照、Ratio、Timing、Dynamic Sag、染色和过采样工作流继续兼容。
- Match 不改变 Blend 的语义，也不是最终 Mix 自动目标。

### 验证

- [x] 静态检查
- [x] Windows 编译
- [x] validator / BS.1770 self-test
- [ ] macOS 编译（Plan D 待执行）
- [x] DAW 实测（由用户执行）
- [x] 用户确认

### 已知问题

macOS 三类成品和 AU `auval` 结果必须等待 Plan D 的 GitHub Actions；不能用 Windows 结论替代。

### 回滚

若 1.2.0 在后续平台验证中出现不可接受问题，回滚到用户此前确认的 1.1.0 稳定基线。

### 后续建议

完成 Plan C/D 后，把公开 commit/tag、workflow run、四类 artifact、内部验证目录和桌面交付路径追加到本文件；不得删除本条的候选过程和失败原因。
---

## v1.2.0 Plan C / Plan D 发布证据

### 公开源码与版本

- GitHub PR：#15 — `DB-5035 1.2.0: Integrated LUFS Match Gain`
- PR 合并提交：`ac94688b1900ad870c51f96709b7faa8cbafd8ce`
- 稳定标签：`v1.2.0`，解引用后指向同一合并提交。
- Plan B 源码树与合并时 `main` 源码树一致；发布后仅追加本节交接证据，不改变插件源码或已构建二进制。
- README 在线核对通过：1.2.0 中英双语说明与 Match Gain 截图存在，1.02 至 1.1.0 的历史更新内容均保留。
- 根目录 `LICENSE` 为 Qing Audio Non-Commercial Source-Share License 1.0 完整正文（不是 GNU/GPL 许可证）；README 明确项目采用 LicenseRef-Qing-Audio-NC-Source-Share-1.0。

### GitHub Actions

- Windows run `32907819815`：Windows x64 VST3 构建、打包和上传成功。
- macOS run `32907822471`：Apple Silicon VST3、Intel VST3、Universal 2 AU 全部成功。
- 四类 job 均检出 `v1.2.0` / `ac94688b1900ad870c51f96709b7faa8cbafd8ce`。
- Apple Silicon VST3：`arm64`；Intel VST3：`x86_64`；AU：`x86_64 + arm64`。
- AU `auval -v aufx Db3Q CdxA` 返回 `AU VALIDATION SUCCEEDED.`。

### 成品验收与交付

- Windows PE Machine 为 `0x8664`，FileVersion 与 ProductVersion 均为 1.2.0。
- 三个 macOS bundle 的 CFBundleShortVersionString 与 CFBundleVersion 均为 1.2.0。
- 四个用户 ZIP 均通过非空、bundle 根目录、结构、版本、架构与 SHA-256 检查。
- 最终用户包按根目录双语安装教程、`Win`、`Mac` 的精简结构交付；内部日志和证明文件不混入用户桌面包。

### 后续 AI 必须遵守

- 新功能必须先追加到本权威交接文件，再更新版本文档；不得删除候选过程、失败原因或历史证据。
- 任何源码或发布文档变更若发生在 Plan B 之后，必须刷新 Plan B，不能沿用旧 manifest。
- Plan D 成品必须来自同一确定公开 commit/tag，且 Windows、Apple Silicon、Intel、AU 四类结果分别验证和报告。
