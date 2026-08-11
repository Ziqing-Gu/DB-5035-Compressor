# DB-5035 Qing Compressor 开发历史与 AI 交接记录

> 本文件让后续开发者或 AI 快速理解项目做过什么、为什么这样做，以及哪些行为不能被无意改坏。

## 后续 AI 必须遵守的记录规则

1. 修改前先阅读本文件、根目录 README 和当前版本的 CODEX_GITHUB_UPDATE 文档。
2. 每次更新代码后，必须在本文件末尾追加开发记录，不得删除、覆盖或改写既有历史。
3. 每条记录至少包含：日期与版本、用户目标、问题原因、设计决定、修改文件、构建/测试结果、仍需人工验证的项目。
4. 修改版本、元数据或用户可见功能时，同步更新 CMake、Projucer、README、Help/版本显示及发布说明。
5. 宿主测试发现新问题时单独记录，不要顺手重写已经校准的 DSP。
6. 交付源码时确保本文件与源码处于同一个根目录。

## 当前结构与关键约束

- 主工程：source-vst3-aax-projucer-ui-fixes-20260614-194248/vst3-juce
- Windows 基线：JUCE 8.0.13、Visual Studio 2022、x64 Release。
- 厂商 Qing Audio；VST3 分类 Fx / Dynamics。
- Classic 与 Vintage 都是正式 UI；最后选择的 UI 和 Vintage VU 模式需要持久化。
- 不要修改既有参数 ID 或 1.08 及更早参数顺序；1.09 的 hostBypass 必须在原参数列表末尾。
- 不要重新校准压缩曲线、Ratio、Timing、Fast、瞬态、染色或现有 IIR 过采样滤波器，除非用户明确要求并提供硬件对比依据。
- Vintage UI 基于贡献者 @tangshaozhu 的设计，但旋钮拖动手感保持项目原设置。
- A/B、缩放、Help、VU 指针和宿主状态恢复均做过专项修复，相关改动必须回归检查。

## 版本与开发过程

### 1.0：基础模型

- 建立二极管桥式压缩器 DSP，完成核心压缩、侧链、Stereo Link、COMP IN 与 Makeup Gain。
- 加入 A/B、Undo/Redo 和过采样。
- 使用 Plugin Doctor 与真实硬件的频响、谐波和动态行为反复对照校准。

### 1.02：双 UI、窗口与 Help

- 吸收外部贡献者的 Vintage UI，同时保留 Classic UI并增加一键切换。
- 记住上次 UI；保留原旋钮拖动速度，没有采用外部 PR 的速度修改。
- 修复只能缩小不能放大；Vintage Help 打开时扩展为可读尺寸。
- 增加 Classic-only 构建开关，作为视觉或授权风险下的回退方案。

### 1.03–1.04：参数反馈、VU 与缩放

- 修复 Classic 外围黑圈。
- Vintage 调整 Threshold、Gain、S/C HPF、Blend 时临时显示实时数值；Ratio 与 Timing 不显示冗余浮层。
- VU 改为 60 Hz、更灵敏自然的响应并保留峰值读数。
- 修正固定工具条和可缩放面板的尺寸计算，减少裁切、黑边和跳变。

### 1.05：VU 模式记忆

- 保存并恢复 Vintage VU 的 IN、OUT、REDUCTION 最后选择，本机偏好优先于旧工程残留显示状态。

### 1.06：短瞬态表针与跨平台

- 音频线程保存两次 UI 刷新间的峰值，避免短音头被后续安静块覆盖。
- 保留此前针对硬件校准的表针动态，不再叠加额外慢速平滑。
- Help 增加版本号；建立 Apple Silicon、Intel VST3 与 Logic AU 构建验证。

### 1.07：双向 A/B

- 支持 A>B 和 B>A；复制后目标槽位成为当前槽位。
- Classic 与 Vintage 都高亮当前 A/B。
- 调整 Help 版本号位置，避免被 Close 遮挡。

### 1.08：品牌与 Dynamics 分类

- 厂商由 Codex Audio 统一为 Qing Audio，同步 CMake、JUCER、README 与 Help。
- VST3 声明 Fx / Dynamics，解决 Cubase/Nuendo 归入 Other 的问题。
- 保持已校准 DSP 不变。

### 1.09：过采样下宿主 Bypass 延迟匹配（2026-08-12）

- 目标：修复 2x/4x/8x 过采样时 DAW Bypass 导致的时间跳变、平行混合梳状滤波和相位失真。
- 原因：1.08 无宿主专用 Bypass 参数，也没有与已报告延迟一致的干声旁通路径。
- 设计：
  - 在旧参数末尾追加 hostBypass，保护旧索引和自动化。
  - 实现 getBypassParameter 与 processBlockBypassed。
  - 正常时持续预热循环干声延迟线；宿主 Bypass 时输出延迟匹配的纯干声。
  - 进入 Bypass 时清理压缩器状态；旁通时 Input/Output 表继续更新，Gain Reduction 为 0 dB。
  - 关闭 COMP IN 时补清主包络、通道包络及 Sidechain HPF 历史。
- 主要修改：PluginProcessor.h/.cpp、DiodeBridgeCompressor.h、CMake、JUCER、README。
- Plan A 构建与验证结果（2026-08-12）：严格使用用户提供的 1.09 C++/DSP/UI 源码，未改动代码；采用 JUCE 8.0.13、Visual Studio 2022、x64 Release 编译成功并覆盖系统 VST3。已安装 moduleinfo 确认为 Version 1.09、Qing Audio、Dynamics；构建与安装二进制 SHA-256 均为 EC3B1C0ADBE117473F13637C1946E27486EFDE929C48803452EDE8632DAD6E49。编译无错误，仅有原有未使用 ratio 参数和 JUCE Font 构造函数弃用警告。
- 仍需人工验证：Cubase/Nuendo 原生 Bypass、1x/2x/4x/8x 延迟读数、平行轨脉冲对齐、播放中反复旁通、旧 1.08 工程状态恢复。
