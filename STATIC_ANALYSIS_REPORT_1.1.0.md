# DB-5035 Qing Compressor 1.1.0 — 静态检查报告

检查日期：2026-08-13

## 检查结论

本次 1.1.0 正式化检查通过，未发现会阻止交付的代码或元数据问题。

## 已执行检查

### DSP 编译与运行

使用 JUCE Core、Audio Basics、Audio Formats 与 DSP 模块编译 `DiodeBridgeCompressor.h` 测试程序，并使用大幅度短瞬态运行：

| 采样率 | 通道 | 输出峰值 | 结果 |
| --- | ---: | ---: | --- |
| 44.1 kHz | Mono | 0.358504 | 通过 |
| 44.1 kHz | Stereo | 0.357733 | 通过 |
| 48 kHz | Mono | 0.369001 | 通过 |
| 48 kHz | Stereo | 0.368136 | 通过 |
| 96 kHz | Mono | 0.488256 | 通过 |
| 96 kHz | Stereo | 0.486144 | 通过 |
| 192 kHz | Mono | 0.574671 | 通过 |
| 192 kHz | Stereo | 0.572558 | 通过 |

所有输出均为有限值，未出现 NaN、无穷值或异常幅度。

### DSP 一致性

正式 1.1.0 的 `DiodeBridgeCompressor.h` 与用户确认解决问题的 Test C 相比，仅将代码注释中的 `TEST C` 改为 `1.1.0`；可执行 DSP 数值和逻辑完全相同。

### 插件身份与版本

- CMake version：`1.1.0`
- Projucer version：`1.1.0`
- Product Name：`DB-5035 Qing Compressor`
- Plugin Code：`Db3Q`
- Manufacturer Code：`CdxA`
- Bundle ID：`com.codexaudio.db5035qingcompressor`
- CLAP ID：`com.codexaudio.qing-compressor`
- Help：通过 `JucePlugin_VersionString` 显示 `Version 1.1.0`

正式构建元数据中未发现测试版的 `Db3T`、测试 Bundle ID 或 `TEST C` 版本后缀。

### 项目文件

- Projucer XML：解析通过
- GitHub Actions YAML：解析通过
- 版本及身份断言：通过
- 修改内容 whitespace 检查：通过

## 已知验证边界

当前开发容器没有 Windows/Visual Studio 编译器，因此未在这里生成或加载 Windows VST3，也未运行 Cubase 插件扫描。仓库已提供 Windows PowerShell 构建脚本与 GitHub Actions 工作流，最终发布前仍应完成：

1. Windows x64 Release VST3 构建。
2. Cubase 扫描并确认 Help 显示 `Version 1.1.0`。
3. 打开 1.09 工程确认实例与参数正常恢复。
4. 用原问题 Vocal 复测 Fast Timing。
5. 复测 1x、2x、4x、8x 过采样下的 DAW Bypass 时间对齐。

真实 Vocal 的 Test C A/B 已由用户完成，并确认问题完美解决。
