# DB-5035 Qing Compressor 1.1.0 — Codex / GitHub 更新说明

## 结论

这是正式版 1.09 的直接升级版。已验证成功的 Test C 瞬态修复已合并到正式插件身份，版本号统一升级为 `1.1.0`。

## 应提交的主要修改

- `vst3-juce/Source/DiodeBridgeCompressor.h`
  - Fast Timing Transient Pass 最大值 `0.98` → `0.65`
  - Transient Pass 约 `0.20 ms` 平滑进入、`1.0 ms` 平滑退出
  - Release Memory 约 `2.0 ms` 平滑进入
- `vst3-juce/CMakeLists.txt`
  - 项目版本更新为 `1.1.0`
  - 恢复正式 Product Name、Plugin Code、Bundle ID 与 CLAP ID
- `vst3-juce/DB-5035 Qing Compressor.jucer`
  - 版本和正式插件身份同步
- `vst3-juce/Source/PluginEditor.cpp`
  - Help 显示正式名称与 `Version 1.1.0`
- `build-vst3.ps1`
  - 仅构建 Windows VST3 目标并生成 1.1.0 zip
- `.github/workflows/`
  - Windows 与 macOS VST3 产物名称同步为 1.1.0

## 正式插件身份——不要改成测试身份

- Product Name：`DB-5035 Qing Compressor`
- Plugin Code：`Db3Q`
- Manufacturer Code：`CdxA`
- Bundle ID：`com.codexaudio.db5035qingcompressor`
- CLAP ID：`com.codexaudio.qing-compressor`
- Help：`Version 1.1.0`

这些值必须保持与正式版 1.09 一致，确保 1.1.0 是升级而不是一个新的并存插件。

## GitHub 构建与检查

1. 提交完整源码，不要提交本地 build 目录。
2. 手动运行 Windows VST3 工作流。
3. 下载 `DB-5035-Qing-Compressor-1.1.0-Windows-x64` 产物。
4. 确认包内插件名为 `DB-5035 Qing Compressor.vst3`。
5. 在 Cubase 中删除或移走 Test 版，然后重新扫描插件。
6. 打开 Help，确认显示 `Version 1.1.0`。
7. 打开已有 1.09 工程，确认插件实例和参数正常恢复。
8. 使用已暴露问题的 Vocal 片段复测 Fast Timing。
9. 在 1x、2x、4x、8x 过采样下复测 DAW Bypass 时间对齐。

## 不要顺手修改

- 不要更换过采样滤波器。
- 不要重校准其他 Timing、Ratio、Dynamic Sag 或染色。
- 不要改变已有参数 ID 和参数顺序。
- 不要把 `Db3T` 或测试 Bundle ID 带回正式构建。
