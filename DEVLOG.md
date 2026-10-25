# DEVLOG — RT700 Audio Denoise 开发进展记录

> **本文档是什么**：本项目的"活"开发日志，记录每一个开发阶段做了什么、遇到什么问题、
> 怎么解决的、核心代码改动在哪。面向**所有读者**——无论是人，还是任何 AI 编码助手
> （Kimi Code、Claude Code、Codex 等）——都能靠这一份文档无上下文地接手项目。
>
> **怎么维护**：每完成一个有意义的开发步（新功能、新验证、踩坑修复），就在
> **"开发日志"**一节追加一条记录（模板见该节末尾）。改动核心代码时，同时更新
> **"核心代码改动记录"**一节。文档保持中英混合、以中文为主，与代码注释风格一致。
>
> 阅读顺序建议：先看 `README.md` 了解项目全貌，再看本文档了解"怎么一步步走到现在"。

---

## 1. 项目目标与定位

基于 **NXP RT700（MIMXRT798S / mimxrt700evk）** 的语音降噪系统，三核协同：

| 核 | 工具链 | 角色 |
|----|--------|------|
| **CM33** (Cortex-M33) | armgcc (arm-none-eabi) | 启动核：上电把 HiFi4 镜像从自己固件搬到 DSP 并放核；（真机时）SAI 音频采集/回放 |
| **HiFi4** (Cadence DSP) | Xtensa (xt-clang) | **主控**：STFT/ISTFT（NatureDSP 定点 FFT）、调用 NPU、overlap-add |
| **NPU** (Neutron-C) | 预编译固件 | 推理加速器：跑 mask 预测模型，HiFi4 通过 TFLM 裸机直调 |

分工原因：**NPU 没有 FFT 指令**，做不了 STFT；HiFi4 有强 DSP 指令集 + NatureDSP
定点 FFT 库。所以 HiFi4 当主控握住时频变换，只把矩阵乘为主的模型推理甩给 NPU。

### 分阶段路线图

| 阶段 | 名称 | 状态 | 说明 |
|------|------|------|------|
| step0 | standalone 剥离 | ✅ 完成（外部目录） | 从 NXP SDK 用 `west build -t standalone_project` 剥出 SAI/NPU/DSP 三例程 |
| step1 | HiFi4 自编译脚手架 | ✅ 完成（外部目录） | 证明 HiFi4 命令行自编译 + 换镜像重烧闭环可行 |
| **step2** | **三核协同骨架** | **✅ 完成（本仓）** | **本仓内容：内存 PCM → STFT → NPU(占位) → ISTFT 全链路打通** |
| step3 | 真降噪模型 | ⬜ 未开始 | 训练 mask 模型 → neutron-converter 转换 → 替换 `model_data.h` |
| step4 | 真 mask 解析 | ⬜ 未开始 | `npu_mask.c` 把模型输出解成 per-bin 增益 |
| step5 | 特征对齐 | ⬜ 未开始 | STFT 帧长/窗/特征与训练侧一致（可能要 log-mel） |
| step6 | 真 SAI 上板 | ⬜ 未开始 | 内存回环换成 SAI + EDMA 实时录放 |

> **step2 的诚实边界**（务必记住）：模型是 cifar10 占位不是真降噪；音频是内存合成
> 无 codec；STFT 特征未与训练侧对齐。本仓只证明"链路成立"，不是能听的产品。

---

## 2. 环境需求速查

要在任意一台新机器上构建本项目，需要以下工具链。**这是机器无关的权威清单**，
遇到问题先对照本节排查。

### 2.1 必需工具

| 工具 | 用途 | 版本要求 | 如何确认 |
|------|------|----------|----------|
| **ARM GCC** (arm-none-eabi) | 编 CM33 固件 | ≥ 13.2（开发机用 14.2，13.2 实测兼容） | `arm-none-eabi-gcc --version` |
| **CMake** | CM33 构建系统 | ≥ 3.10（实测 3.30） | `cmake --version` |
| **Ninja** | CM33 构建执行 | ≥ 1.10（实测 1.12.1） | `ninja --version` |
| **Xtensa 工具链** | 编 HiFi4 固件 | RI-2023.11-win32 + core `rt700_hifi4_RI23_11_nlib` | `xt-clang --version`（需 license） |
| **Git Bash**（Windows） | 跑 `build_step2.sh` | 任意 | — |

### 2.2 环境变量

构建脚本/CMake 靠以下环境变量定位工具链：

| 变量 | 指向 | 何时需要 | 说明 |
|------|------|----------|------|
| `ARMGCC_DIR` | ARM GCC 安装根目录（**含 `bin/`** 的那一级） | CM33 cmake 配置 | ⚠️ **只编 CM33 也必须设**，`armgcc.cmake` 靠它拼编译器路径 |
| `XTENSA_SYSTEM` | `.../XtensaTools/config` | HiFi4 编译 | `build_step2.sh` 内部已设 |
| `XTENSA_CORE` | `rt700_hifi4_RI23_11_nlib` | HiFi4 编译 | `build_step2.sh` 内部已设 |

> **路径约定因机器而异**：`build_step2.sh` 顶部的 `XT=` 写死了 Xtensa 路径，
> 换机器后需按实际安装位置修改。ARM GCC 路径不写死在脚本里，靠 `ARMGCC_DIR`。

### 2.3 只编 CM33 的最小步骤（不需要 Xtensa）

HiFi4 镜像已随仓入库（`cm33_armgcc/.../hifi4/binary/dsp_*_release.bin`），
所以只编 CM33 时**不需要 Xtensa 环境**：

```bash
cd cm33_armgcc
export ARMGCC_DIR="你的ARM_GCC安装根目录"   # 例如 "C:/Users/xxx/.mcuxpressotools/arm-gnu-toolchain-13.2.Rel1-..."
rm -rf CMakeFiles Makefile build.ninja cmake_install.cmake CMakeCache.txt
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=debug .
ninja
# 产物: tflm_cifar10_hifi4_neutron_cm33_core0.elf / .bin
```

也可用现成脚本：`bash cm33_armgcc/build_debug.sh`（Windows 批处理版 `build_debug.bat`），
但脚本不设 `ARMGCC_DIR`，仍需你预先 export。

---

## 3. 仓库结构与构建产物归属

```
mimxrt700evk_audio_denoise/            ← git 仓库根（独立仓）
├── README.md                          ← 项目全貌（先读这个）
├── DEVLOG.md                          ← 本文件
├── build_step2.sh                     ← 一键完整构建（CM33+HiFi4 全量）
├── step2_denoise_skeleton.code-workspace
│
├── cm33_armgcc/                       ← CM33 侧完整 standalone 树
│   ├── CMakeLists.txt / flags.cmake / config.cmake
│   ├── build_debug.sh / build_debug.bat / clean.sh / ...   ← CM33 单独构建脚本
│   ├── examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/
│   │   └── hifi4/binary/              ← ★ HiFi4 内嵌镜像（git 入库，见下）
│   │       dsp_{data,literal,text}_{debug,release}.bin
│   └── (构建产物) tflm_cifar10_hifi4_neutron_cm33_core0.elf / .bin
│
└── hifi4_xtensa/                      ← HiFi4 侧完整 standalone 树
    ├── middleware/cadence/naturedsp/hifi4_rt700/libnaturedsp.a   ← FFT 库（入库）
    ├── middleware/eiq/tensorflow-lite/lib/hifi4/xcc/rt798s/libtflm.a
    ├── middleware/eiq/neutron/rt700/hifi4/libNeutron{Driver,Firmware}.a
    └── examples/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4/
        ├── main.cpp                   ← denoise 入口（原 cifar10 备份为 main.cpp.orig）
        └── denoise/                   ← ★ 本工程新增的降噪源码
            ├── stft.c / stft.h
            ├── npu_mask.c / npu_mask.h
            ├── denoise_pipeline.c / .h
            └── pcm_input.h
```

### 3.1 `.gitignore` 与"哪些产物入库"决策

这是一个**有意为之**的设计，新读者容易困惑，特此说明：

| 文件类型 | 是否入库 | 原因 |
|----------|----------|------|
| `*.a`（libtflm/libNeutron*/libnaturedsp） | ✅ 入库 | clone 后无需外部 SDK 即可构建 |
| HiFi4 镜像 `dsp_*_release.bin` / `dsp_*_debug.bin` | ✅ 入库 | CM33 固件的输入，入库才能独立构建 |
| `cm33_armgcc/*.bin`（CM33 自己的二进制） | ✅ 入库 | 最终可烧录固件，保留一份可用基线 |
| `*.elf` | ❌ 忽略（`*.elf` 规则） | 体积大，可由源码重建 |
| `_build/`、`logs/`、`backup_orig_release_bins/` | ❌ 忽略 | 纯构建产物 |
| `CMakeFiles/`、`*.o`、`build.ninja` 等 | ❌ 忽略 | CMake 中间产物 |

> **⚠️ 常见困惑**：`cm33_armgcc/*.bin` 入了库，但每次 `ninja` 都会覆写它，所以
> `git status` 会显示它 `M`（modified）。这是**正常现象**（README §5 有写）。
> 想回到干净基线：`git checkout -- <该文件>`，或 `bash build_step2.sh --restore`。

---

## 4. 开发日志

> 按时间倒序记录（新的在前）。每条包含：日期、做了什么、结果、遇到的问题与解法。

### [2026-10-25] step3 选型：开源 MCU 端侧降噪模型调研（GTCRN 为首选）

**做了什么**：为 step3"真降噪模型"做选型调研（GitHub REST API / Gitee 实测：GitHub API 与
raw 可用、网页版不可达；Gitee 仅有 rnnoise / deepfilternet 官方镜像，其余无）。
候选：GTCRN / RNNoise / DTLN / Ambiq NNSE / NSNet2 / DeepFilterNet / UL-UNAS 等。

**结论（推荐 GTCRN，理由如下）**：

| 模型 | 仓库 | ★ | 参数量 | 算量 | 许可证 | MCU/int8 先例 |
|------|------|---|--------|------|--------|---------------|
| **GTCRN**(ICASSP'24) | github.com/Xiaobin-Rong/gtcrn | ~760 | **48.2K** | **33 MMAC/s** | MIT | sherpa-onnx 流式集成；无官方 int8 |
| RNNoise | github.com/xiph/rnnoise | ~5.9k | ~60K | ~40 MMAC/s | BSD-3 | 多（NNSE 即其衍生） |
| DTLN(Interspeech'20) | github.com/breizhn/DTLN | ~750 | <1M | 较高 | MIT | 官方自带量化 tflite |
| Ambiq NNSE | github.com/AmbiqAI/nnse | 44 | ~100KB 模型 | — | BSD-3 | TFLM int8 @ Cortex-M4 无 NPU |
| DeepFilterNet | github.com/Rikorose/DeepFilterNet | ~4.9k | 1.8M | 350 MMAC/s | MIT/Apache | 非 int8/TFLM，移植重 |

- **GTCRN 最匹配 Neutron-C**：参数量/算量比其余小 1~2 个数量级，纯 mask 预测，
  结构 = Conv2d encoder + ERB 固定矩阵映射 + DPGRNN(GRU) + decoder，矩阵乘友好；
  有训练代码与流式实现，二开基础好。代价：需自行 int8 量化（建议 QAT）+ 按本项目
  256/128 STFT 参数重训对齐。
- RNNoise 注意点：**不是 per-bin mask**（输出 22 个 Bark 带增益 + VAD），48kHz/帧 960，
  与本管线差异最大，作备选/参照。
- DTLN 是"今天就有量化 tflite"的捷径，但 512 单元 LSTM×4 偏重。
- Ambiq NNSE 是"TFLM int8 全流程上 MCU"的现成模板，其 训练→int8→TFLM C 库 流程与
  本项目 step3 路线一一对应，值得照抄流程（不必照抄模型）。

**本地核查的两个关键事实**（决定 step3 工程方案）：
1. **整图单算子转换**：`model_cifarnet_ops_npu.cpp` 显示当前模型被 neutron-converter
   整图转成一个 `NEUTRON_GRAPH` 自定义算子（resolver 只注册 `AddDequantize` + `AddCustom`）。
   → 新模型里**任何不被 Neutron 支持的算子都会导致整图转换失败**，GRU/LSTM 能否上 NPU
   必须先用 neutron-converter 实测。
2. **HiFi4 兜底可用**：`strings libtflm.a` 确认内含完整
   `UnidirectionalSequenceLstmEval/Prepare`（含 int8/int16 路径）及 HiFi4 优化符号
   `xa_nn_lstm_cell_state_update_16`。→ 若 GRU/LSTM 不能上 NPU，可把模型**拆两段**：
   Conv/FC 部分转 NPU，RNN 部分在 HiFi4 跑 TFLM 原生 kernel（HiFi4 SIMD 扛几十 K MAC/帧
   没问题，但需计入时延预算）。GTCRN 的 GRU 是逐 cell 的少量 matmul+sigmoid/tanh，
   也可改写成纯 FC 序列便于整图上 NPU。

**下一步**：拉取 GTCRN 仓库 → 按 16kHz/帧长256/hop128 改训练配置 → 重训 → PTQ/QAT 量化
int8 → neutron-converter 转换实测（重点验证 GRU 展开算子支持度）→ 替换 `model_data.h`。

---

### [2025-10-25] 修复新机器上 VSCode/clangd 无法代码跳转

**现象**：项目用 VSCode + clangd 插件编辑 C 代码，在原机器上 `F12`/Ctrl+点击可正常跳转，
但 clone 到新机器（`D:\AI\mimxrt700evk_audio_denoise`）后**完全无法跳转**。

**根因（两个叠加）**：
1. **`compile_commands.json` 缺失（主因）**：clangd 跳转依赖编译数据库告诉它每个文件的
   头文件路径与宏定义。该文件在 `.gitignore` 里（不随仓走），且 cmake 默认不生成 →
   新机器上一个 `compile_commands.json` 都没有，clangd 建不起索引。
2. **工具链路径写死成原机器（次因）**：workspace 与 `cm33_armgcc/.clangd` 里的 ARM GCC
   路径是原机器的 `C:/Program Files (x86)/.../14.2 rel1`，而新机器 ARM GCC 在
   `C:/Users/flyingbird/.mcuxpressotools/arm-gnu-toolchain-13.2.Rel1-...`（13.2.1），
   原路径在新机器上不存在。

**解法**：
1. 重新配置 cmake 时加 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`，生成
   `cm33_armgcc/compile_commands.json`（34 个编译单元齐全，编译器路径正确指向本机 13.2）。
2. `cm33_armgcc/.clangd`：三条 `-isystem` 头文件路径从 14.2 改为本机 13.2 的
   `lib/gcc/arm-none-eabi/13.2.1/{include,include-fixed}` + `arm-none-eabi/include`
   （三个目录都验证过存在）。
3. `step2_denoise_skeleton.code-workspace` 的 `--query-driver` 第一条 ARM GCC glob
   同步改为本机路径。

**验证**：用本机 clangd 对核心源文件做 `--check`：
`clangd --check=.../cm/main_cm33.c --query-driver=<本机ARM_GCC>` →
**"All checks completed, 0 errors"**，编译数据库被正确加载，索引可建立。

**如何让修复生效（用户操作）**：VSCode 里 `Ctrl+Shift+P` → `clangd: Restart language server`，
等后台索引（`--background-index`）建完即可跳转。

**遗留局限**：
- **HiFi4（xtensa）侧跳转暂不可用**：本机无 Xtensa 工具链，且 `hifi4_xtensa/` 本无
  `compile_commands.json`（它原是 Xplorer 工程，原机器是手工生成的本地 cdb）。等装了
  Xtensa 或需要 DSP 侧跳转时再生成那份 cdb。
- 本机 PATH 上的 clangd 是 WitmemStudio 自带的 14.0.3（较旧）。VSCode clangd 扩展（0.6.0）
  可提示下载官方较新版本，建议接受；不过 14.0.3 实测 `--check` 也是 0 errors，不下载也能用。

**经验（供其他机器/协作者参考）**：clone 本仓后若 clangd 不能跳转，**先检查有没有
`compile_commands.json`**——这是最常见的遗漏。生成方法见 §2.3（cmake 加
`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`），并确认 `.clangd`/workspace 里的工具链路径
与本机实际安装位置一致。

---

### [2025-10-25] step2 骨架在新机器上完成 CM33 编译验证

**做了什么**：把项目 clone 到一台新机器（Windows，路径 `D:\AI\mimxrt700evk_audio_denoise`），
验证"只编 CM33 部分"是否可行（该机器没有 Xtensa 工具链，HiFi4 编不了——但 HiFi4
镜像已随仓入库，理论上不需要重编）。

**环境**：ARM GCC 13.2.Rel1（`C:\Users\flyingbird\.mcuxpressotools\arm-gnu-toolchain-13.2.Rel1-mingw-w64-i686-arm-none-eabi`）、
cmake 3.30.0、ninja 1.12.1，均已在 PATH。无 Xtensa。

**步骤与结果**：
1. 检查 HiFi4 镜像：`cm33_armgcc/.../hifi4/binary/` 下 6 个 bin 齐全 ✅
2. `export ARMGCC_DIR=...` → `cmake -G Ninja -DCMAKE_BUILD_TYPE=debug .` → 配置成功
3. `ninja` → **34/34 全过，0 错误 0 警告**
4. 产物 `tflm_cifar10_hifi4_neutron_cm33_core0.elf` = 971 080 字节
   （原开发机 14.2 编出 970 292 字节，差异是编译器版本导致，正常）
5. **镜像嵌入验证**：用 `arm-none-eabi-nm`/`objcopy` 从 M33 ELF 的 `.dspcode` 段抽回
   data 子段，md5 = `d1be9055a758539c6b61a5131fbb12df`，与仓内 `dsp_data_release.bin`
   的 md5 **完全一致** ✅（也与原开发机记录的 NXP 基线 md5 吻合，证明镜像是干净原版）

**结论**：**新机器上 CM33 侧环境完全满足需求**。无需 Xtensa 即可编出含 HiFi4 镜像的
完整 CM33 固件。

**踩坑与解法**：
- `ARMGCC_DIR` 必须指向 ARM GCC 安装根目录（含 `bin/` 那级），否则 cmake 配置阶段报
  `***Please set ARMGCC_DIR in environment variables***`。
- 本机 ARM GCC 路径与原开发机不同（13.2 vs 14.2），但 `armgcc.cmake` 只吃
  `ARMGCC_DIR`，不写死路径，所以只要设对变量即可，无需改任何工程文件。

---

### [2025-10-24] step2 骨架升级为独立自洽项目（原开发机）

**做了什么**：把原本依赖外部 SDK 目录的 step2 工程，升级为**完整独立自洽的 git 仓**
（对标 NXP `dsp_mu_polling` 的组织方式）。本仓由此诞生。

**关键决策**：
- 自带 `cm33_armgcc/`(~17MB) + `hifi4_xtensa/`(~23MB) 两套 standalone 全树
- 预编译库（`*.a`）与 HiFi4 内嵌镜像（`*.bin`）**入库** → clone 即可构建
- denoise 源码并入工程树 `hifi4_xtensa/.../hifi4/denoise/`
- VSCode workspace：2 folder + clangd 多核索引（query-driver 放行 arm-none-eabi + xt-*）

**实测验证**（命令行自编译，非 IDE）：29 源编出 2.87MB HiFi4 ELF；
`xt-nm` 见 `fft_real32x32`/`ifft_real32x32` 真符号；切 4 段 bin → 覆盖 cm33 release bin →
重编 M33 → `.dspcode` 段抽回 data md5 `0ba29b80...` 字节一致。

**git**：首个 commit `326d5ce`（骨架本体），随后 `59e726a`（README 详写）。

---

### [更早] step1：HiFi4 自编译链路证明（外部目录 `sdk-build/rt700_denoise/step1_hifi4_scaffold`）

证明 HiFi4 工程能不依赖 Xplorer IDE、用命令行自编译，且自编译镜像能进 M33 固件。
核心产出：`chain_rebuild.sh` 五阶段一键链路（自编译→切段→覆盖→重编→md5 校验）。
**关键坑**（已带入 step2 的 README §6）：
- `incbin.S` 的 `#ifdef DEBUG` 是假：armgcc 的 ASM 编译规则不传 `-DDEBUG`，
  所以即便 debug 构建，实际嵌的是 `_release` 组 bin。**替换必须改 `_release`**。
- 验证 M33 侧镜像必须用 **arm-none-eabi** 工具链（M33 是 ARM 目标），
  且镜像在 `.dspcode` 段（不是 `.rodata`）。

---

### 日志条目模板（追加新记录时复制此模板）

```markdown
### [YYYY-MM-DD] 一句话标题

**做了什么**：
**环境**（如与标准环境不同）：
**结果**：
**踩坑与解法**（如有）：
**核心代码改动**（如有，同步到 §5）：
```

---

## 5. 核心代码改动记录

> 记录**本工程新增/修改**的关键代码及其设计意图。NXP SDK 原生文件不在此列。

### 5.1 `hifi4_xtensa/.../hifi4/main.cpp` — denoise 入口

**改动**：整体替换原 cifar10 的 `main.cpp`（原文件备份为 `main.cpp.orig`）。

**内容**：`BOARD_Init → TIMER_Init → DEMO_PrintInfo → DENOISE_Init → DENOISE_RunOnce`。
用 `extern "C"` 包 `denoise_pipeline.h`。跑完打印 `[step2] idle.` 后空转。

### 5.2 `denoise/stft.c / stft.h` — STFT/ISTFT

**职责**：加 Hann 窗 + 正/逆 STFT + 谱幅计算，基于 NatureDSP `fft_real32x32`/`ifft_real32x32`。

**关键设计**：
- 参数：帧长 `N=256`，`HOP=128`（50% 重叠），实数谱 `BINS=129`，打包谱 `SPEC_LEN=258`（Re/Im 交错）
- Hann 窗 Q31 定点，初始化时预计算
- **NatureDSP 约束**（必须遵守）：FFT 是 in-place 破坏输入，`x`/`y` 必须 8 字节对齐且不重叠。
  所以所有工作缓冲带 `__attribute__((aligned(8)))`，输入帧内部拷贝到 `s_frame` 再变换
- `scalingOpt=2`（32-bit dynamic scaling，RT700 例程取值）
- c99 下没有 `M_PI`，文件内自带 `#ifndef M_PI #define ...`
- `STFT_Magnitude` 用 `|Re|+|Im|` 近似（省 sqrt），`>>1` 防溢出
- `STFT_Inverse`：谱 × mask（Q15，`0x8000`=1.0）→ IFFT → 再加一次合成窗

### 5.3 `denoise/npu_mask.c / npu_mask.h` — NPU 调用桥（骨架占位）

**职责**：把谱幅喂进 NPU 模型、跑推理、取输出当 mask。

**骨架阶段的占位语义**（重要，真降噪时要重写）：
- 复用 `MODEL_*` extern "C" 接口（`MODEL_Init`/`MODEL_GetInputTensorData`/`MODEL_RunInference`）
- 现有 cifar10 模型输入是 `[1,32,32,3]` int8 = 3072 元素；谱幅只有 129 个，
  按"填前 K 个、其余置零"塞进去——**无语义，只为驱动 Invoke 证明调用链成立**
- **mask 取恒等 `0x8000`**（Q15 的 1.0）：保证 STFT→ISTFT 往返可做误差自检。
  真降噪时这里要从 `MODEL_GetOutputTensorData()` 解析 per-bin 增益
- 每次调用打印 Invoke 状态/耗时 + 输出张量前几个值，证明推理真跑了

### 5.4 `denoise/denoise_pipeline.c / .h` — 流水线 + PCM 生成 + 自检

**职责**：串起整条数据流，并提供内存回环的"音频源"。

**内容**：
- `PCM_GenerateInput`：整数三角波近似正弦（避免板上浮点），合成 500Hz 主音 + 3500Hz
  高频"噪声"，1024 样本，`PCM_FULL_SCALE = 1<<24`（留 7 bit 余量防 FFT 溢出）
- `DENOISE_Init` = STFT_Init + PCM 生成 + NPU_MaskInit
- `DENOISE_RunOnce`：50% overlap 分帧循环（STFT→谱幅→NPU→ISTFT→overlap-add），
  最后在**稳态区间**（去掉首尾各一帧，避开 overlap 不全处）算往返误差
  `|out-in|/|in| × 1000` 自检。mask 恒等时此值应很小，证明链路数值正确。

### 5.5 `build_step2.sh` — 一键构建脚本

**职责**：HiFi4 自编译 → 切 4 段 bin → 覆盖本地 cm33 release bin → 重编 M33 → md5 自校验。

**关键设计**：
- **只操作本地树**（`SELF=脚本目录`），不借用其他例程
- 29 源：21 个 C 用 `-std=c99`，5 个 C++ 用 `-std=c++14 -stdlib=libc++`（两个都是坑点，
  不给会编译失败），外加 denoise 源
- 链接本地 `libtflm.a` + `libNeutron*.a` + `libnaturedsp.a`，`-Wl,--start-group...--end-group -lxos`
- 切 4 段 bin（literal/text/data/**ncache**），各段 `--only-section` 清单照抄 `post_build.bat`
- **覆盖 `_release` 而非 `_debug`**（ASM-no-DEBUG 坑，见 step1 日志），覆盖前备份到
  `backup_orig_release_bins/`
- 验证用 **arm-none-eabi-nm/objcopy**（不是 xt-*）抽 M33 的 `.dspcode` 段，md5 对比
- `--restore` 还原始 bin 基线

### 5.6 `cm33_armgcc/CMakeLists.txt` 等 — NXP 原生（未改）

CM33 侧工程文件为 NXP SDK 原生，未做修改。构建系统靠 `ARMGCC_DIR` 环境变量定位工具链。

---

## 6. 已知坑点速查表

| 坑 | 现象 | 解法 |
|----|------|------|
| `ARMGCC_DIR` 未设 | cmake 报 `Please set ARMGCC_DIR` | export 指向 ARM GCC 安装根（含 `bin/`） |
| `incbin.S` ASM-no-DEBUG | 改了 `_debug` bin 但没生效 | 必须改 `_release` 组（ASM 规则不传 `-DDEBUG`） |
| 用 xt-nm 验 M33 ELF | 符号读不到/报错 | M33 是 ARM 目标，用 `arm-none-eabi-nm/objcopy` |
| 镜像段找错 | 在 `.rodata` 里找不到 | 镜像在 M33 的 `.dspcode` 段 |
| `git status` 显示 `.bin` 被改 | 以为仓库脏了 | 正常：构建会覆写入库的 `.bin`，`--restore` 还原 |
| C++ 编译报 flatbuffers 错 | "需要 C++11" | 必须 `-std=c++14 -stdlib=libc++` |
| FFT 结果错乱/崩溃 | 输入被破坏 | NatureDSP FFT in-place，x/y 8 字节对齐不重叠 |
| c99 下 `M_PI` 未定义 | 编译错 | `stft.c` 已自带 `#ifndef M_PI` 兜底 |
| `XCC_DIR` 带 `/bin` | cmake 找不到工具 | 指向 `XtensaTools` 本身，cmake 自己接 `/bin` |
| clangd 无法跳转（新机器） | F12/Ctrl+点击无反应 | ①缺 `compile_commands.json`（在 .gitignore 不随仓走，cmake 加 `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` 生成）②`.clangd`/workspace 里工具链路径写死成别的机器，改成本机实际路径 |

---

## 7. 下一步行动项（详细版）

按优先级排序，每项给出接入点：

1. **真降噪模型**（最大改动）：训练 mask 预测模型 → neutron-converter 转换 →
   替换 `model_data.h`（数组 + `MODEL_NAME` + `kTensorArenaSize`）+ 算子注册 cpp。
   构建链路复用 `build_step2.sh`（step1 已固化）。
2. **真 mask 解析**：改 `denoise/npu_mask.c` 的 `NPU_MaskRun`，
   从 `MODEL_GetOutputTensorData()` 解析 per-bin 增益填入 `mask_q15`，去掉恒等占位。
3. **特征对齐**：`stft.c` 的帧长/窗/特征与训练侧 Python 完全一致（可能需 log-mel，
   NatureDSP 有 `logmel32x32`）。⚠️ 不一致会导致降噪效果崩。
4. **真 SAI 上板**：把内存回环换成 SAI + EDMA 录放（参考 SDK 例程
   `driver_examples/sai/edma_record_playback`），CM33 负责采集/回放，HiFi4 处理。
