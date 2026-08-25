# DB-5035 Match Gain 候选版开发记录

日期：2026-08-26
基线：GitHub `origin/main` / `e8e2e21`（正式版 1.1.0）
工作分支：`agent/match-gain-candidate`
工作区：本地独立 Git 工作区（公开记录不保存本机绝对路径）

## 本轮目标

1. 增加播放期间的自动 Match Gain。
2. Match 使用 Integrated LUFS，而不是 Peak、RMS 或瞬时响度。
3. Gain 参数的真实步进和界面显示都提高到 0.01 dB。
4. Vintage 皮肤没有预留 Match 组件区域，因此不能破坏复古面板主体。

## Match Gain 的测量定义

- 算法：ITU-R BS.1770 / EBU R128 Integrated LUFS。
- K-weighting：高频搁架 + RLB 高通。
- 门限：-70 LUFS 绝对门限，再应用低于绝对门限结果 10 LU 的相对门限。
- 分块：400 ms 测量块、100 ms 步进（75% 重叠）。
- 只在宿主 Transport 播放期间累计。
- 新一次播放、播放位置跳变、宿主旁通状态切换或 COMP IN 不可用时重置累计。
- Dry：与当前压缩处理处于相同采样率时间域的原始输入。
- Wet：经过动态、染色、瞬态穿透、Timing 内部校准和固定输出带宽之后，但位于用户 Gain 与 Blend 之前的全 Wet 信号。
- 写回值：`Dry Integrated LUFS - Wet Integrated LUFS`，最后限制在现有 Gain 范围 -6.00 至 +20.00 dB。

这个定义与 QQ Super Compressor 的 Makeup Match 思路一致。因为 DB-5035 的 Gain 只作用于 Wet 支路，不能直接拿 Blend 后的最终输出做简单差值，否则 Blend 小于 100% 时目标会失真。

## UI 决策

- Match 不进入 Classic 或 Vintage 面板主体。
- 两套皮肤共用顶部命令条中的 70 px 小按钮，位置紧邻 Oversampling。
- `MATCH`：没有可用结果。
- `M...`：正在累计播放期间的 LUFS。
- `M +3.24`：结果已准备好；点击后写入 Gain。
- Gain 的 Classic 常驻数值和 Vintage 拖动临时数值统一显示两位小数。

## 兼容性

- 没有增加新的宿主自动化参数，也没有改变现有参数 ID、顺序或范围。
- Gain 仍使用 `makeupGain`，仅把步进从 0.1 dB 改为 0.01 dB。
- 原项目、自动化和 A/B 快照继续使用同一参数；Match 写入也会通过正常宿主参数手势和 UndoManager 完成。
- 2026-08-26：用户指出候选版继续显示 1.1.0 难以辨认，因此将 CMake、Projucer 和打包名称统一更新为 1.2.0。
- 1.2.0 目前仍是本地候选版；不要在用户完成宿主测试前擅自发布或推送 GitHub。

## 已完成验证

- 独立 BS.1770 自测：48 kHz、0 dBFS、1 kHz 单声道正弦 = -3.0036 LUFS。
- 固定 6 dB Dry/Wet 差值 = +6.00 dB Match。
- Windows x64 VST3 Release 完整编译通过。
- Steinberg `vst3effectsvalidator.exe` 退出码 0，并识别为 `Fx` / `Dynamics`。
- `git diff --check` 通过。

## 下一步人工测试

1. 在 Cubase/Nuendo 中播放至少 0.4 秒，确认按钮由 `M...` 变成带两位小数的目标值。
2. 点击结果，确认 Gain 写入两位小数且能 Undo/Redo。
3. 分别验证 1x、2x、4x、8x。
4. 分别验证 Classic 与 Vintage；重点确认顶部小按钮不遮挡原 UI。
5. 验证停止、重新播放、移动播放光标、COMP IN 和宿主 Bypass 后会重新累计。
6. 验证 Blend 不同位置时，Match 仍按全 Wet Makeup 定义工作。

## 给后续 AI 的要求

每次修改代码、改变测量定义、UI 交互、版本号、构建方式或测试结论后，都必须继续更新本开发记录，保留之前的经过与决定，不要只留下最终结果。正式发布时再同步 README、版本号、Plan B 备份、GitHub 和跨平台产物。
