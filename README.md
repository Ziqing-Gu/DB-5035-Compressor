#DB-5035-Qing-Compressor

本项目是一个建模自真实 Rupert Neve 5035 前级话放通道条的音频压缩插件。目前据我了解，市面上似乎没有任何一款音频插件是模仿或建模自这台世界顶尖的设备。

This project is an audio compressor plugin modeled after the compressor section of the real Rupert Neve 5035 Mic Pre / Channel Strip. To the best of my knowledge, there are currently no commercial audio plugins on the market that attempt to emulate or model this world-class piece of hardware.

作为一名从业 15 年的混音师，我在工作的过程中，经常依赖一款经典的 Neve 硬件产品——Rupert Neve 5035 前级话放通道条。这台设备实际上拥有 4 个模块，分别是前级、EQ、压缩和染色模块。我建模的是其中的压缩模块，5035 的压缩模块是一个二极管压缩器。

As a mixing engineer with over 15 years of professional experience, I have relied heavily on the Rupert Neve 5035 channel strip throughout my work. The hardware consists of four main sections: a microphone preamp, EQ, compressor, and texture/saturation stage. This project specifically models the compressor section, which is based on a diode compressor design.

<img width="4032" height="2272" alt="IMG_20260619_004150" src="https://github.com/user-attachments/assets/5ded82c9-1317-4546-a163-4df52d64fee9" />



与行业内的普遍认知不同，我们通常认为二极管压缩器（例如 33609 等）都具有非常明显的染色特征。而 5035 的二极管压缩却非常特别，它拥有极其通透的听感。如果使用得当，你可以获得一个非常自然且中频突出的人声或乐器声音。它的染色程度会随着压缩量逐渐增加，而在轻微压缩（约 3dB 以内）时，几乎不会产生明显的染色。

Unlike the common perception of diode compressors in the audio industry—such as the famous 33609, which is generally known for its strong coloration—the diode compressor inside the 5035 is quite unique. It sounds remarkably transparent and open. When used properly, it produces a natural sound with a pleasing midrange presence. Its coloration increases progressively with the amount of gain reduction, while at light compression levels (within roughly 3 dB of gain reduction), it remains almost completely transparent.

这个压缩模块非常适合处理人声、贝斯以及打击乐。在录音和混音过程中，我经常使用它的硬件版本。它的原型硬件是我混音工作中的绝对主力设备，尤其是在处理人声时，我几乎一定会使用它的压缩模块。遗憾的是，这台设备价格极其昂贵，我也只拥有一台，因此长期以来只能将它用于单声道素材的处理。

This compressor is exceptionally well suited for vocals, bass, and percussion. I use the original hardware regularly in both recording and mixing sessions. It has become one of the most important tools in my workflow, especially for vocal processing. Unfortunately, the unit is extremely expensive, and I only own a single channel, which limits its use to mono sources.

近年来，随着 AI 技术的兴起，我看到许多同行开始使用 Codex 或 AntiGravity 等工具开发自己的音频插件。这让我对此产生了浓厚兴趣。在与一些有经验的朋友交流之后，我决定使用 Codex 来协助完成这个项目。没错，这个插件完全由 ChatGPT 5.5 在 Codex 环境下完成开发。

In recent years, the rise of AI-assisted development has enabled many audio professionals to build their own plugins using tools such as Codex and AntiGravity. This inspired me to explore plugin development myself. After discussing the process with several experienced colleagues, I decided to use Codex to help create this plugin. Yes—this plugin was developed entirely with the assistance of ChatGPT 5.5 running through Codex.

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

<iframe src="//player.bilibili.com/player.html?isOutside=true&aid=116738553289351&bvid=BV1hfJL62E7E&cid=39073025494&p=1" scrolling="no" border="0" frameborder="no" framespacing="0" allowfullscreen="true"></iframe>

