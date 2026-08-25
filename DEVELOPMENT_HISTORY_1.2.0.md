# DB-5035 Qing Compressor 1.2.0 开发与交接记录

日期：2026-08-26
正式基线：GitHub `origin/main` / `e8e2e21`（1.1.0）
开发分支：`agent/match-gain-candidate`
工作区：本地独立 Git 工作区（公开记录不保存本机绝对路径）

## 版本目标

1. 增加播放期间的自动 Match Gain。
2. 使用 Integrated LUFS 而不是 Peak、RMS 或瞬时响度。
3. 将 Gain 的真实步进和界面显示提高到 0.01 dB。
4. 在不破坏 Vintage 复古皮肤布局的前提下，为 Classic 与 Vintage 提供相同功能。

## 设计过程与关键决定

- Match 遵循 ITU-R BS.1770 / EBU R128：K-weighting、400 ms 测量块、100 ms 步进、-70 LUFS 绝对门限与 -10 LU 相对门限。
- 只在宿主 Transport 播放期间累计；新一次播放、播放位置跳变、旁通或 COMP IN 不可用时重置。
- Dry 为与当前处理处于相同采样率时间域的原始输入。
- Wet 包含动态、染色、瞬态穿透、Timing 内部校准和固定输出带宽，但测量点位于用户 Gain 与 Blend 之前。
- 写回值为 `Dry Integrated LUFS - Wet Integrated LUFS`，并限制在原有 Gain 范围 -6.00 至 +20.00 dB。
- 由于 Gain 只作用于 Wet 支路，因此没有拿 Blend 后的最终 Mix 做简单差值；这避免 Blend 小于 100% 时出现错误目标。
- Match 不进入 Classic 或 Vintage 面板主体，而是放在共用顶部命令条中。
- 按钮状态为 `MATCH`、`M...` 和带符号的结果，例如 `M +3.24`；点击结果后通过正常宿主参数手势写入现有 `makeupGain`。
- 没有新增、删除或重排宿主自动化参数，保持旧工程、自动化和 A/B 快照兼容。

## 版本与构建经过

- 最初功能候选版沿用了 1.1.0，用户指出难以辨认后，CMake、Projucer、Windows 包名与 Help 统一升级到 1.2.0。
- 第一次增量编译后，VST3 `moduleinfo.json` 已显示 1.2.0，但旧构建目录中的 Windows 资源脚本仍保留 1.1.0。
- 为避免旧缓存混入，改用全新的 D 盘临时构建目录重新配置并编译。
- 全新编译后，Windows FileVersion、ProductVersion、VST3 清单、JUCE 编译宏与 Help 显示全部统一为 1.2.0。

## 已完成验证

- 独立 BS.1770 自测：48 kHz、0 dBFS、1 kHz 单声道正弦 = `-3.0036 LUFS`。
- 固定 6 dB Dry/Wet 差值测试 = `+6.00 dB` Match。
- Windows x64 VST3 Release 全新编译通过。
- Steinberg `vst3effectsvalidator.exe` 退出码 `0`，分类识别为 `Fx / Dynamics`。
- 构建件、系统安装件和 D 盘输出件二进制 SHA-256 一致：`FEF7AE53BE66C8BCB9686BBAAE69295F678E1041F6486583390994788BA4D54C`。
- `git diff --check` 通过。
- 用户已完成宿主测试，并于 2026-08-26 明确同意将 1.2.0 设置为稳定基线。

## 稳定发布范围

- 版本：`1.2.0`
- Windows：x64 VST3
- macOS：Apple Silicon VST3、Intel VST3、Universal 2 AU
- 正式 Plan B 备份必须位于用户指定的自动同步源码备份层级。
- Plan C 必须保留 README 的全部旧版本内容，只追加 1.2.0 中英双语介绍和功能截图。
- Plan D 最终包必须放到桌面，根目录只保留中英文安装说明以及 `Win`、`Mac` 文件夹。

## 后续人工回归重点

1. 在 Cubase/Nuendo 中确认 `MATCH` → `M...` → 建议值的完整流程。
2. 点击结果后确认 Gain 写入两位小数，并能正常 Undo/Redo。
3. 分别验证 1x、2x、4x、8x，以及 Classic/Vintage 两种 UI。
4. 验证停止、重新播放、移动播放位置、COMP IN 和宿主 Bypass 后重新累计。
5. 验证不同 Blend 位置不会改变 Match 的“全 Wet Makeup”定义。

## 给后续 AI 的要求

每次修改代码、测量定义、UI 交互、版本号、构建方式或测试结论后，都必须继续更新本记录或创建下一版本记录。必须保留过往开发经过、问题、判断依据和验证结果，不得只留下最终结果。正式发布时还要同步 README、版本号、Plan B 正式备份、GitHub 工作流和跨平台产物。