# DB-5035 Qing Compressor 1.09 — GitHub 更新说明

> 这是正式版 1.09 的基线历史，不是 Test C 的发布说明。Test C 的当前修改与接力要求请以根目录 `TEST_C_DEVELOPMENT_NOTE.md` 为准。

## 本次任务

将插件从 1.08 更新到 1.09，修复开启过采样后使用 DAW Bypass 时发生时间偏移的问题。该问题在平行处理工作流中会导致梳状滤波和相位失真。

## 问题原因

1. 1.08 没有向宿主提供专用的 Bypass 参数，DAW 只能使用宿主侧的通用旁通行为。
2. 开启 2x、4x 或 8x 过采样后，插件会向宿主报告过采样延迟，但原代码没有为 Bypass 建立一条保持相同延迟的干声路径。
3. 因此插件正常处理与 DAW Bypass 的信号可能不在同一时间位置。两条信号用于平行混合时会出现相位抵消。

## 已完成的代码修改

### 1. 宿主可识别的 Bypass

- 新增隐藏参数 `hostBypass`，显示名称为 `Bypass`。
- `DB5035AudioProcessor::getBypassParameter()` 将该参数返回给 JUCE/宿主。
- 同时重写 `processBlockBypassed()`，兼容直接调用旁通回调的宿主。
- 新参数被追加在原有参数列表末尾，避免改变 1.08 参数的索引顺序，尽量保护旧工程和自动化数据的兼容性。

### 2. 延迟匹配的干声旁通路径

- 新增循环延迟缓冲区。
- 延迟采样数直接使用插件当前通过 `setLatencySamples()` 向 DAW 报告的数值。
- 正常处理时，干声持续写入延迟缓冲区但不输出，用于保持缓冲区处于预热状态。
- DAW Bypass 时，输出延迟后的纯干声，不经过压缩、染色和输出带宽处理。
- 这样 Bypass 前后保持相同的插件延迟，同时不改变原有压缩器和 IIR 过采样算法的声音校准。

### 3. Bypass 状态与电平表

- 进入宿主 Bypass 时清空压缩器内部动态状态，防止解除 Bypass 后恢复旧的增益衰减。
- Bypass 状态下 Input/Output 表继续更新，Gain Reduction 显示为 0 dB。

### 4. `COMP IN` 状态复位补充

1.08 在关闭 `COMP IN` 时没有完整清除主包络、各通道包络以及 Sidechain HPF 历史。本次补充清理：

- `linkedEnvelope`
- `linkedSlowPreviewEnvelope`
- `channelEnvelopes`
- `channelSlowPreviewEnvelopes`
- `sidechainState`
- `lastSidechain`

这可以避免重新开启 `COMP IN` 后短暂继承关闭前的旧压缩状态。

## 版本号修改

- `vst3-juce/CMakeLists.txt`：`1.08` → `1.09`
- `vst3-juce/DB-5035 Qing Compressor.jucer`：`1.08` → `1.09`
- `vst3-juce/README.md`：当前版本更新为 `1.09`
- 根目录 `README.md`：新增 1.09 中英文更新日志
- Help 界面原本通过 `JucePlugin_VersionString` 显示版本，因此 CMake 或 Projucer 生成的 1.09 构建会直接显示 `Version 1.09`。

## 主要修改文件

- `vst3-juce/Source/PluginProcessor.h`
- `vst3-juce/Source/PluginProcessor.cpp`
- `vst3-juce/Source/DiodeBridgeCompressor.h`
- `vst3-juce/CMakeLists.txt`
- `vst3-juce/DB-5035 Qing Compressor.jucer`
- `vst3-juce/README.md`
- 根目录 `README.md`

## Codex 更新 GitHub 时应执行的检查

1. 使用项目指定的 JUCE 8.0.13 进行 Windows Release 构建。
2. 确认 Cubase 将插件识别为具有原生 Bypass 参数的 VST3。
3. 分别在 1x、2x、4x、8x 下检查宿主显示的插件延迟。
4. 建立两条完全相同且同步的音轨，其中一条插入插件；在插件轨上按 DAW Bypass，确认与原始轨平行叠加时不再出现时间偏移和梳状滤波。
5. 使用脉冲或短促鼓声进行离线渲染，比较 Bypass 轨与原始轨的起始采样位置。
6. 在播放过程中反复切换 DAW Bypass，确认没有静音缺口、爆音或旧压缩包络恢复。
7. 打开 Help，确认显示 `Version 1.09`。
8. 打开旧版 1.08 工程，确认 Threshold、Ratio、Timing、Fast、Blend、Oversampling 和 A/B 状态能够正常恢复。

## 不要改动的部分

- 不要更换现有 IIR 过采样滤波器。
- 不要重新校准压缩曲线、Timing、Ratio、瞬态或染色算法。
- 不要调整现有参数 ID 或原有参数顺序。
- 如果构建或宿主测试暴露新的问题，应单独记录，不要顺手重写已经校准的 DSP。
