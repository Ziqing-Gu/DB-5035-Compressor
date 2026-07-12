#DB-5035-Qing-Compressor

## 1.05 更新 / What's new in 1.05

### 中文

- **Vintage VU 表模式记忆：** `IN`、`OUT` 与 `REDUCTION` 的最后一次选择会作为本机偏好保存；新建实例、重开编辑器或重启宿主后，都会默认恢复该选择。
- **状态恢复更可靠：** VU 表显示会在插件状态可用后自动同步。与 UI 模式相同，用户最后一次选择优先于旧 DAW 工程中残留的显示状态。
- **原生 macOS 1.05 包：** GitHub Actions 工作流继续分别构建并校验 Apple Silicon（arm64）和 Intel（x86_64）VST3，产物名称已同步为 1.05。

### English

- **Vintage VU mode memory:** The last selected `IN`, `OUT`, or `REDUCTION` view is saved as a local preference and restored for new instances, editor reopenings, and host restarts.
- **Reliable state restoration:** The editor synchronizes the meter mode once plugin state is available. Like the UI-style preference, the user's latest choice takes precedence over stale display state stored by an older DAW project.
- **Native macOS 1.05 packages:** GitHub Actions continues to build and validate separate Apple Silicon (arm64) and Intel (x86_64) VST3 packages; the artifact names now match version 1.05.

## 1.04 更新 / What's new in 1.04

### 中文

**1.04 汇集了 1.03 与 1.04 的改进；相较 1.02，重点如下：**

- **Classic / Vintage 双 UI：** 可在右上角一键切换；插件会在下次打开时恢复用户上次选择的界面。
- **Classic 边框修正：** 恢复面板外围应有的视觉边框，避免出现多余的黑色内圈。
- **Vintage 参数提示：** 调整 Threshold、Gain、S/C HPF 或 Blend 时，会暂时显示实时数值；松开后自动隐藏。Ratio 与 Timing 保持硬件面板上的直观刻度，不显示冗余浮层。
- **Vintage VU 表优化：** 表针改为从实时增益衰减驱动，以 60 Hz 更新并采用更灵敏、自然的响应；峰值保持读数独立保留。
- **缩放稳定性：** 修正 Vintage 顶部固定工具条与可缩放面板之间的尺寸计算，避免在不同窗口大小下被裁切，或出现大面积无效黑边。
- **保留 1.02 的兼容性措施：** Help 阅读布局、原有旋钮拖动手感和可选 Classic-only 构建方式均继续保留。

<img width="400" height="226" alt="1 03VU Meter表现 00_00_10--00_00_26" src="https://github.com/user-attachments/assets/cbb0a99d-abc6-451d-96ab-36e886081501" />


### English

**1.04 includes the 1.03 and 1.04 refinements. Compared with 1.02, it adds:**

- **Classic / Vintage UI switching:** Use the top-right control to switch styles; the plugin restores the user's last selected UI when it reopens.
- **Classic frame correction:** Restores the intended outer panel framing and removes the unwanted black inset border.
- **Vintage parameter feedback:** Threshold, Gain, S/C HPF, and Blend show a temporary live value while being adjusted, then fade away. Ratio and Timing keep their clear hardware-panel scales without redundant overlays.
- **Vintage VU meter refinement:** The needle is driven by real-time gain reduction, refreshed at 60 Hz with a faster, more natural response. The peak-hold readout remains separate.
- **Reliable resizing:** Corrects the geometry between Vintage's fixed top command strip and its scalable panel, preventing clipping and excessive unused black space at different window sizes.
- **1.02 safeguards retained:** The readable Help layout, original knob-drag feel, and optional Classic-only build remain available.
## 1.02 更新 / What's new in 1.02

### 中文

1.02 是一次以 UI、可用性和跨平台构建为重点的更新。

<img width="1202" height="255" alt="image" src="https://github.com/user-attachments/assets/489f6175-94c9-42a6-a7da-034fa76de2bf" />


- **Classic / Vintage 双 UI：** 右上角可一键切换两套界面；插件会记住上一次选择的 UI 模式。该本地偏好优先于 DAW 工程中旧的实例状态，方便在不同工程之间保持一致的工作方式。
- **保留原有操作手感：** Vintage UI 的视觉设计被整合，但旋钮鼠标拖动速度保持原版；`COMP IN` 在加载工程时继续尊重该工程保存的开关状态，不会被强制打开。
- **窗口与 Help 改进：** 编辑器可正常放大和缩小，并保持界面比例；Vintage UI 打开 Help 时会临时扩展为更大的阅读布局，关闭后恢复正常面板尺寸。
- **可选 Vintage UI：** 维护者可在 CMake 中使用 `-DDB5035_ENABLE_VINTAGE_UI=OFF` 构建仅含 Classic UI 的版本；此模式不会打包 Vintage UI 资源或显示切换按钮，方便在需要时快速回退到纯 Classic 界面。
- **原生 macOS VST3 构建：** 仓库提供手动触发的 **Build macOS VST3** GitHub Actions 工作流，可分别生成 Apple Silicon（arm64）与 Intel（x86_64）原生 VST3 包，并在构建时校验二进制架构。
- **贡献与资源说明：** Vintage UI 的设计和实现基础来自 GitHub 贡献者 [@tangshaozhu](https://github.com/tangshaozhu) 的 PR #3，相关提交历史已保留。Vintage UI 使用的 Open Sans 资源说明见 [`THIRD_PARTY_NOTICES.md`](source-vst3-aax-projucer-ui-fixes-20260614-194248/vst3-juce/THIRD_PARTY_NOTICES.md)。

### English

Version 1.02 focuses on UI flexibility, usability, and cross-platform builds.

- **Classic / Vintage UI switch:** Use the top-right control to switch between the two interfaces. The plugin remembers the most recently selected UI style; this local preference intentionally takes precedence over old instance state restored by a DAW project.
- **Original interaction behavior retained:** The Vintage visual design is integrated without changing the original knob drag sensitivity. `COMP IN` continues to respect the state saved in each project and is not forced on when a project loads.
- **Resizing and Help improvements:** The editor can now grow and shrink while preserving its intended proportions. Opening Help in Vintage mode temporarily expands the editor into a readable layout and restores the normal panel size when closed.
- **Optional Vintage UI:** Maintainers can build a Classic-only fallback with `-DDB5035_ENABLE_VINTAGE_UI=OFF`. That configuration omits Vintage resources and hides the style switch.
- **Native macOS VST3 builds:** The manual **Build macOS VST3** GitHub Actions workflow creates and validates separate native Apple Silicon (arm64) and Intel (x86_64) VST3 packages.
- **Contribution and resources:** The Vintage UI design and implementation foundation comes from GitHub contributor [@tangshaozhu](https://github.com/tangshaozhu) via PR #3; the contributor history is preserved. See [`THIRD_PARTY_NOTICES.md`](source-vst3-aax-projucer-ui-fixes-20260614-194248/vst3-juce/THIRD_PARTY_NOTICES.md) for the Open Sans resource notice.

本项目是一个建模自真实 Rupert Neve 5035 前级话放通道条的音频压缩插件。目前据我了解，市面上似乎没有任何一款音频插件是模仿或建模自这台世界顶尖的设备。

This project is an audio compressor plugin modeled after the compressor section of the real Rupert Neve 5035 Mic Pre / Channel Strip. To the best of my knowledge, there are currently no commercial audio plugins on the market that attempt to emulate or model this world-class piece of hardware.

作为一名从业 15 年的混音师，我在工作的过程中，经常依赖一款经典的 Neve 硬件产品——Rupert Neve 5035 前级话放通道条。这台设备实际上拥有 4 个模块，分别是前级、EQ、压缩和染色模块。我建模的是其中的压缩模块，5035 的压缩模块是一个二极管压缩器。

As a mixing engineer with over 15 years of experience, I have relied heavily on the Rupert Neve 5035 channel strip throughout my work. The hardware consists of four main sections: a microphone preamp, EQ, compressor, and texture/saturation stage. This project specifically models the compressor section, which is based on a diode compressor design.

<img width="4032" height="2272" alt="IMG_20260619_004150" src="https://github.com/user-attachments/assets/5ded82c9-1317-4546-a163-4df52d64fee9" />



与行业内的普遍认知不同，我们通常认为二极管压缩器（例如 33609 等）都具有非常明显的染色特征。而 5035 的二极管压缩却非常特别，它拥有极其通透的听感。如果使用得当，你可以获得一个非常自然且中频突出的人声或乐器声音。它的染色程度会随着压缩量逐渐增加，而在轻微压缩（约 3dB 以内）时，几乎不会产生明显的染色。

Unlike the common perception of diode compressors in the audio industry—such as the famous 33609, which is generally known for its strong coloration—the diode compressor inside the 5035 is quite unique. It sounds remarkably transparent and open. When used properly, it produces a natural sound with a pleasing midrange presence. Its coloration increases progressively with the amount of gain reduction, while at light compression levels (within roughly 3 dB of gain reduction), it remains almost completely transparent.

这个压缩模块非常适合处理人声、贝斯以及打击乐。在录音和混音过程中，我经常使用它的硬件版本。它的原型硬件是我混音工作中的绝对主力设备，尤其是在处理人声时，我几乎一定会使用它的压缩模块。遗憾的是，这台设备价格极其昂贵，我也只拥有一台，因此长期以来只能将它用于单声道素材的处理。

This compressor is exceptionally well suited for vocals, bass, and percussion. I use the original hardware regularly in both recording and mixing sessions. It has become one of the most important tools in my workflow, especially for vocal processing. Unfortunately, the unit is extremely expensive, and I only own a single channel, which limits its use to mono sources.

近年来，随着 AI 技术的兴起，我看到许多同行开始使用 Codex 或 AntiGravity 等工具开发自己的音频插件。这让我对此产生了浓厚兴趣。在与一些有经验的朋友交流之后，我决定使用 Codex 来协助完成这个项目。没错，这个插件完全由 ChatGPT 5.5 在 Codex 环境下完成开发。

In recent years, the rise of AI-assisted development has enabled many audio professionals to build their own plugins using tools such as Codex and AntiGravity. This inspired me to explore plugin development myself. After discussing the process with several experienced colleagues, I decided to use Codex to help create this plugin. Yes—this plugin was developed entirely with the assistance of ChatGPT 5.5 running through Codex.

我初次使用API，由于经验不足，在购买Token时浪费了一些钱。起初用Codex构建一个像模像样的压缩插件，只花了约＄4。但Vibe Coding就像吸毒，起初我只是浅尝一口。。但没想到当你的要求越来越具体、越来越高以后，需要花费的Token就会越来越多，我很后悔没有购买ChatGpt的Pro会员。。在最终完成这个插件时，我一共花费了超过＄220。。

This was my first time using APIs, and due to my lack of experience, I ended up wasting some money buying tokens. At first, building a decent compression plugin with Codex only cost around $4. But Vibe Coding is addictive—at the beginning, I only dabbled in it lightly.but....

I didn’t expect that as my requirements grew more specific and demanding, the token usage would keep skyrocketing. I really regret not subscribing to ChatGPT Pro. By the time I finally finished the plugin, I’d spent over $220 in total.

我的建模思路非常简单：首先让 Codex 根据设备原理构建一个基础版本的插件，然后使用 Plugin Doctor 分析其频率响应、谐波染色和动态特性。接着，我会将插件与硬件之间的测量结果反馈给 Codex，让它不断修正算法，使插件与硬件在 Plugin Doctor 中的测试结果越来越接近。

My modeling approach was relatively straightforward. First, I asked Codex to build an initial implementation based on the known operating principles of the hardware. Then, I analyzed the plugin using Plugin Doctor, measuring frequency response, harmonic coloration, and dynamic behavior. The resulting measurements were repeatedly compared against the hardware, and the differences were fed back into Codex so that the algorithms could be refined iteratively until the plugin's behavior closely matched the original unit.

<img width="1920" height="1032" alt="image" src="https://github.com/user-attachments/assets/34c3a219-e330-42c1-b516-4f9027375fe4" />
<img width="1004" height="785" alt="image" src="https://github.com/user-attachments/assets/7c9b9a2b-bfaa-406c-b783-16536c27e15b" />
<img width="1001" height="782" alt="image" src="https://github.com/user-attachments/assets/2157f389-ecea-40a8-8d90-4dbb22c7b720" />

用 Plugin Doctor 校准，然后让 Codex 不断修正算法的整个过程，前后差不多花了 4 天 3 夜的时间。

The entire process of calibrating the plugin with Plugin Doctor and repeatedly refining the algorithms through Codex took approximately four days and three nights from start to finish.


截至目前，我认为插件与原型硬件的频率响应已经达到了约 99% 的一致性，瞬态表现约为 95%，而染色特征则约达到 50% 的相似度。在建模过程中，我并没有采用能够完全复制硬件染色的方法（因为我的能力有限，无法实现这一点）。相反，我让 Codex 实现了一种经典二极管饱和模型，并通过代码让染色程度随着压缩量增加而逐渐增强，从而近似模拟硬件的工作方式。

At the current stage of development, I estimate that the plugin matches the original hardware's frequency response by approximately 99%, its transient behavior by roughly 95%, and its coloration characteristics by around 50%. During development, I did not have the expertise or resources necessary to perfectly reproduce the hardware's non-linear behavior. Instead, I incorporated a classic diode-style saturation model and programmed the amount of coloration to increase progressively with gain reduction, approximating the behavior of the original unit.

<img width="1240" height="960" alt="image" src="https://github.com/user-attachments/assets/50900477-9e51-48d8-816d-73b5e6755298" />



事实上，我已经在大量音频素材上使用过这个建模插件，并发现它几乎适用于所有场景，包括人声、贝斯、打击乐以及 Mix Bus 等用途。我还将插件分享给了一些同行测试，他们给予了非常积极的反馈。其中甚至有拥有原始硬件的工程师表示，他们几乎无法明显区分插件与硬件之间的声音差异。

In practice, I have used this compressor on a wide variety of audio sources and found it effective in nearly every situation, including vocals, bass, percussion, and mix bus processing. I have also shared the plugin with several professional engineers, who provided overwhelmingly positive feedback. Some of them own the original hardware and reported that they could barely distinguish the plugin from the hardware in blind listening tests.

这台压缩与经典压缩不同的是，它没有 Attack 和 Release 的旋钮，只有一个 Timing 旋钮。这些 Timing 包含 Fast、MF、Med、MS、Slow 以及一个 Auto 模式。用起来有一点像 Fairchild 670/660。不过它同时还提供了一个 Fast 按钮，当你打开它时，Attack 大约会缩短至原来的 92%，而 Release 则大约缩短至原来的 87%。

Unlike most traditional compressors, this unit does not feature separate Attack and Release controls. Instead, it uses a single Timing control, which provides several preset timing modes: Fast, MF, Med, MS, Slow, and Auto. In operation, it is somewhat reminiscent of the classic Fairchild 670/660 approach, where timing characteristics are selected rather than manually adjusted.

In addition, the compressor includes a Fast switch. When engaged, the attack time is reduced to approximately 92% of its original value, while the release time is reduced to approximately 87% of its original value, resulting in a faster and more responsive compression behavior.

<img width="1122" height="478" alt="image" src="https://github.com/user-attachments/assets/13a9f76b-50d3-40ad-8304-1cfb96fb5f09" />


我在 Help 里加入了这台设备不同 Timing 下的真实数值表，供用户自行查询。

I have included a table in the Help section containing the actual timing values of the original hardware under different Timing settings, allowing users to reference them directly.
<img width="1122" height="478" alt="image" src="https://github.com/user-attachments/assets/60428d7c-3642-4852-a66d-bd5b78a6fcd7" />

顺带一提，我最喜欢的人声压缩设置是：Ratio 3:1，Timing 设为 Fast 模式(Fast按钮关闭)。在 Help 文档中，你会发现 Fast Timing 模式下的 Attack 时间快得惊人，仅有 0.17ms。这似乎与传统人声压缩中常见的设置思路并不相同。

As a side note, my favorite vocal compression setting is a 3:1 ratio with the Timing control set to Fast mode(fast switch off). According to the timing chart included in the Help section, the Fast setting has an astonishingly fast attack time of only 0.17 ms. At first glance, this may seem quite different from the conventional compressor settings typically recommended for vocal processing.

不过在得知这些具体数值之前，我一直都是完全依靠耳朵在硬件上寻找最适合的声音，因此才最终形成了这套参数组合。对我而言，这组设置能够将人声压得非常扎实和稳定，同时依然保持自然的听感，并且不会产生那种令人不适的抽吸（pumping）效应。

Interestingly, I arrived at this setting long before I ever knew the actual timing values. My approach was simply to trust my ears and dial in what sounded best on the hardware itself. As a result, this combination became my preferred vocal setting. In my experience, it allows vocals to be compressed very firmly and consistently while still sounding natural and open, without introducing the unpleasant pumping artifacts that are often associated with aggressive compression.



我曾经使用同一段音频素材分别经过硬件和插件处理，并导出波形进行对比分析。结果显示，两者之间的差异非常微小，这一点甚至让我自己都感到惊讶。目前我对这个插件的表现非常满意。虽然我并非职业程序员（尽管过去曾经开发过商业 Kontakt 音色库），但我非常希望更多人能够尝试使用这个插件。

I have also processed identical audio material through both the hardware and the plugin, rendered the outputs, and compared the resulting waveforms. The differences turned out to be surprisingly small. I am genuinely pleased with the current results. Although I am not a professional software developer (despite having written code for commercial Kontakt libraries in the past), I would love for more people to try this plugin and share their experiences.

<img width="1249" height="297" alt="image" src="https://github.com/user-attachments/assets/b2411083-3707-4a1d-8296-b4f552c6ab1d" />


我为插件加入了许多现代商业插件常见的功能，例如 A/B 对比、过采样以及 Undo/Redo 系统。不过最近开始有越来越多用户提出新的功能需求。考虑到个人能力和时间的限制，我认为将项目开源是一个更好的选择，让其他开发者能够基于源代码继续完善和扩展它。

The plugin already includes many features commonly found in modern commercial audio software, including A/B comparison, oversampling, and full Undo/Redo support. Recently, however, users have started requesting additional features and improvements. Given my limited development resources, I believe that open-sourcing the project is the best path forward, allowing other developers to modify, improve, and expand upon the codebase.

如果你对源代码没有兴趣，只想试用一下这个插件，那么你可以直接下载 “DB-5035 Compressor 安装包 1.01 版.zip”。压缩包内包含 Windows 和 macOS 两个平台的安装程序，插件格式为 VST3。此外，macOS 版本还额外提供了 AU（Audio Unit） 格式。下载后直接双击安装即可。

If you're not interested in the source code and simply want to try the plugin, you can download "DB-5035 Compressor 安装包 1.01 版.zip" directly. The package includes installers for both Windows and macOS, with the plugin provided in VST3 format. The macOS version also includes an AU (Audio Unit) build. Simply download the package and run the installer to get started.


Enjoy!

视频介绍(Vedio)：
https://www.bilibili.com/video/BV1hfJL62E7E/?spm_id_from=333.1387.homepage.video_card.click&vd_source=755a32d6ff272ad1739298868f2881c9
