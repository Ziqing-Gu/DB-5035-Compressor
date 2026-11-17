# 1.2.2 发布状态 / Release status

- Stable: explicitly designated by the user on 2026-11-17.
- Plan A: prior Windows 1.2.2 build, tests and installation reused; no repeat Windows CI.
- Plan B: completed and frozen before public synchronization; never accessed in later stages.
- Plan C: completed on 2026-11-17. Public shared source, all four platform ZIPs,
  bilingual installation guides and a verified desktop package have been delivered.
- Plan D: v1.2.2 is published as the latest, non-prerelease Release in this same
  repository. All 6 uploaded asset byte counts and SHA-256 digests match the
  verified desktop package; README and Release provide their API-returned URLs.
- Release: https://github.com/Ziqing-Gu/DB-5035-Compressor/releases/tag/v1.2.2
- Windows: BS.1770 self-test, 122 processor/UI checks, 24 audio-equivalence cases,
  Steinberg loading/category validation, consistent version and installation hashes.
- Native Apple Silicon and Intel VST3: both passed CTest 2/2 including 122 processor/UI checks.
- Universal 2 AU: arm64 + x86_64 verified, native Apple Silicon auval passed
  (AU VALIDATION SUCCEEDED). This is not manual Intel Logic/DAW validation.
- All Mac packages: version 1.2.2, deployment target 11.0, ad-hoc signature verification,
  ZIP CRC/root, executable permission, Mach-O architectures and SHA-256 checked.
- Desktop package: 6 actual files, 21,437,382 bytes, copied hashes 6/6 match.
  Internal proof files are not included in the user package.

## Source and CI identity / 同源与构建

Product source: e5cec7534b3e19970ed7147eaa72c4a95e34ee82.
Product source tree: f8fcbc2cf4c252c11e23c3ecf22a0136a14f026e.
Later commits adjust only CI/documentation; the release tag retains this product tree.

- Intel VST3: https://github.com/Ziqing-Gu/DB-5035-Compressor/actions/runs/44086945542 (Intel job succeeded; unused retired-runner jobs were cancelled).
- Apple Silicon VST3: https://github.com/Ziqing-Gu/DB-5035-Compressor/actions/runs/44088099056 (Apple Silicon job succeeded; AU registration failed in this attempt).
- Universal 2 AU: https://github.com/Ziqing-Gu/DB-5035-Compressor/actions/runs/44090410893 (succeeded).
- All three artifacts explicitly build the same e5cec75 product commit; successful jobs are reused.

macos-14 retirement initially prevented two jobs starting. CI now uses macos-15 /
macos-15-intel with source_ref and per-platform recovery. AU ordinary Registrar
termination did not discover the newly copied component. A force refresh of the
runner user's AudioComponentRegistrar registered the user-folder copy and auval
passed. The system-folder fallback was not needed. No product code was changed.

## Version coverage / 版本区间

Last fully documented remote version and latest earlier Release: 1.2.0.
Evidence: main 0b324f861e2399af45c54f5b7c6c39011b63fcf1 and Release v1.2.0.
Actual intervening versions: 1.2.1, 1.2.2. Both are directly documented in README,
CHANGELOG and individual development records; missing versions: none.

中文：用户指定 1.2.2 Stable；A 复用、B 已冻结、C 完成，D 已发布同仓库正式 Release。
Windows、Apple Silicon VST3、Intel VST3、Universal 2 AU 与双语说明共 6 文件已交付桌面，
总计 21,437,382 字节，哈希一致。AU 在 Apple Silicon runner 上通过 auval；
不冒充完整 Mac DAW 人工实测。许可保持 Qing Audio 非商业源码共享许可证 1.0。
