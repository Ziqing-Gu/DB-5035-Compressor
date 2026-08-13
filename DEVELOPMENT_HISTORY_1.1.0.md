# DB-5035 Qing Compressor 1.1.0 — 开发记录

## 本次升级目的

修复部分大 Vocal 瞬态在 Fast Timing 下压缩时偶发的轻微爆裂、短促沙哑、“啪”声、高频毛刺和类似瞬间过载的失真。

## 问题定位

问题来自 Fast Timing 中的 `Transient Pass`：原逻辑可能在一个采样点内把干声放行比例提升到接近 `0.98`，造成压缩信号与瞬态干声之间过快、幅度过大的切换。Release Memory 入口也存在单采样点写入额外衰减的可能。

其他 Timing 的 Transient Pass 数值原本均为 `0.0`，所以本次 Transient Pass 修复只实际作用于 Fast Timing。

## Test C 验证阶段

正式发布前曾建立可与 1.09 并存的 Test C：

- Product Name：`DB-5035 Qing Compressor Test`
- Plugin Code：`Db3T`
- Bundle ID：`com.codexaudio.db5035qingcompressortest`
- Help：`Version 1.09 TEST C`

用户已使用出现过异常的 Vocal 素材完成 A/B，确认问题得到完美解决，因此将同一套 DSP 修改正式提升为 1.1.0。

## 1.1.0 DSP 修改

### Transient Pass

- Fast Timing 最大放行量：`0.98` → `0.65`
- 平滑进入时间：约 `0.20 ms`
- 平滑退出时间：约 `1.0 ms`
- 消除单采样点内接近全量的干湿切换

### Release Memory

- 额外衰减改为约 `2.0 ms` 平滑进入
- 保留原有程序相关慢速衰减

### 未改动部分

- 其他 Timing 的 Transient Pass 仍为 `0.0`
- Ratio 与 Threshold 校准
- 其他 Timing 和 Fast 按钮校准
- Dynamic Sag、染色、Sidechain HPF 与过采样算法
- 1.09 的 DAW Bypass 延迟匹配修复

## 正式版身份

1.1.0 恢复与 1.09 相同的正式插件身份，用于正常覆盖升级：

- Product Name：`DB-5035 Qing Compressor`
- Plugin Code：`Db3Q`
- Bundle ID：`com.codexaudio.db5035qingcompressor`
- CLAP ID：`com.codexaudio.qing-compressor`
- CMake / Projucer：`1.1.0`
- Help：`Version 1.1.0`

测试版使用的 `Db3T` 与测试 Bundle ID 不再出现在正式构建元数据中。用户可自行删除 `DB-5035 Qing Compressor Test.vst3`。

## 验证记录

- DSP 核心已完成 C++ 编译与运行检查。
- 44.1、48、96、192 kHz 的单声道与双声道高瞬态测试全部通过。
- 未发现 NaN、无穷值或异常输出幅度。
- Projucer XML 与 GitHub Actions YAML 已完成语法检查。
- 产品名称、版本、Plugin Code、Bundle ID 和 Help 显示已做一致性检查。
- 用户已通过真实 Vocal A/B 确认 Test C 的异响问题完美解决。
- 当前 Linux 开发容器没有 Windows/Visual Studio 编译器，Windows VST3 需通过仓库脚本或 GitHub Actions 最终构建。

## 后续 AI 接力规则

后续 AI 每次修改代码，都必须继续更新本开发记录或新增相应版本记录，写明基线、修改原因、具体参数、插件身份、测试结果和下一步。开发记录、静态检查报告与 ZIP 校验文件应一并放入最终源码压缩包，避免遗漏开发上下文。
