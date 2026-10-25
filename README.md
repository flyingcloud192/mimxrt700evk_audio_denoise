# step2_denoise_skeleton

> **开发进展与踩坑记录**：见 [`DEVLOG.md`](DEVLOG.md)（面向所有读者/AI 工具的详细开发日志，
> 含环境需求、核心代码改动、已知坑点、下一步行动项。新机器/新协作者请先读它）。

**RT700 (MIMXRT798S / mimxrt700evk) 语音降噪 —— CPU + DSP + NPU 三核协同端到端骨架**

一个**完整、独立、自洽**的工程（结构对标 NXP `dsp_mu_polling`）：自带 CM33 与 HiFi4 的全部源码与预编译库，
用 git 单独管理，用 VSCode workspace 打开，clone 下来即可一键构建。本工程属于 denoise 项目的**第二步**
（第一步 `step1_hifi4_scaffold` 证明了 HiFi4 命令行自编译 + 换镜像重烧的闭环）。

> **定位说明（务必先读）**：这是一个**骨架 / skeleton**——目的是**打通 CPU↔DSP↔NPU 三方的完整数据通路**，
> 证明"HiFi4 当主控、串起 STFT/NPU/ISTFT、镜像能进 M33 固件"这条链路成立。
> 它**不是一个能听的真降噪器**：用的是现成 cifar10 占位模型、恒等 mask、内存合成 PCM。诚实边界见文末。

---

## 1. 这个项目在做什么

语音降噪的经典做法是"频域掩蔽"：把时域音频切帧做 STFT 变到频域，用一个神经网络预测每个频点的
增益（mask，0=全抑制，1=全保留），把谱乘上 mask，再 ISTFT 变回时域。本工程把这条链路**按 RT700 的三类算力分工**落地：

| 核 | 角色 | 本工程里具体干什么 |
|----|------|-------------------|
| **CM33 (Cortex-M33, armgcc)** | CPU / 启动核 | 上电启动，把 HiFi4 镜像从自己固件里搬到 DSP 并放行；（真机时）负责 SAI 采集/回放音频 |
| **HiFi4 (Cadence DSP, xtensa)** | **主控** | 整条数据流的大脑：加窗、STFT(`fft_real32x32`)、算谱幅、调 NPU、乘 mask、ISTFT(`ifft_real32x32`)、overlap-add |
| **NPU (Neutron)** | 推理加速器 | 被 HiFi4 通过 TFLM 直接驱动（Neutron-C 裸机直调），跑模型 `Invoke()` 产出 mask |

**为什么这样分工**：NPU 没有 FFT 指令，做不了 STFT；HiFi4 有强 DSP 指令集 + NatureDSP 定点 FFT 库，
天生适合做频域变换，所以让 HiFi4 当主控把时频变换握在手里，只把"矩阵乘为主的模型推理"这一段甩给 NPU。

### 数据流

```
                        ┌──────────────────────── HiFi4 (DSP, 主控) ────────────────────────┐
  内存 PCM(int32)       │                                                                    │      内存 PCM(int32)
  ──────────────►  [取一帧 256]──►[加 Hann 窗]──►[fft_real32x32]──►[谱幅 |Re|+|Im|]          │  ◄──────────────
   (真机=SAI 采集)      │              │                                 │                    │   (真机=SAI 回放)
                        │              │                                 ▼                    │
                        │              │                        [压 int8 喂模型]             │
                        │              │                                 │                    │
                        │              │                          ╔══════▼══════╗            │
                        │              │                          ║  NPU Invoke ║  ◄── Neutron-C 裸机直调
                        │              │                          ║ (cifar10占位)║            │
                        │              │                          ╚══════╤══════╝            │
                        │              │                                 │ (输出→mask, 骨架取恒等)
                        │              ▼                                 ▼                    │
                        │   [谱 × mask]──►[ifft_real32x32]──►[加窗]──►[overlap-add 累加]──────┘
                        └────────────────────────────────────────────────────────────────────┘
```

参数：帧长 `N=256`，`HOP=128`（50% 重叠），实数谱 `BINS=129`，打包谱长 `SPEC_LEN=258`（Re/Im 交错）；
Hann 窗 Q31；mask Q15（`0x8000`=1.0，骨架用恒等）；NatureDSP scalingOpt=2（32-bit dynamic）。

---

## 2. 目录结构（独立自洽）

```
step2_denoise_skeleton/                    ← git 仓库根（独立仓, commit 从 326d5ce 起）
├── README.md                              ← 本文件
├── .gitignore                             ← 排构建产物; 保留 *.a / *.bin（库入库, clone 即可构建）
├── step2_denoise_skeleton.code-workspace  ← VSCode workspace: 2 folder + clangd 多核索引
├── build_step2.sh                         ← 一键构建脚本（只操作本地树）
│
├── cm33_armgcc/      (~17 MB)             ← CM33 侧完整 standalone 树（armgcc 工具链）
│   ├── drivers/ components/ devices/ ...
│   ├── examples/_boards/mimxrt700evk/.../hifi4/binary/
│   │        dsp_{data,literal,text,ncache}_{debug,release}.bin   ← HiFi4 内嵌镜像（入库）
│   │        incbin.S                       ← 把上面的 bin 嵌进 .dspcode 段
│   └── CMakeLists.txt ...                   ← ninja 构建, 产出 cm33_core0.elf
│
└── hifi4_xtensa/     (~23 MB)             ← HiFi4 侧完整 standalone 树（Xtensa 工具链）
    ├── middleware/
    │   ├── cadence/naturedsp/
    │   │        hifi4_rt700/libnaturedsp.a          ← 定点 FFT 库（入库）
    │   │        hifi4/include/NatureDSP_Signal*.h
    │   └── eiq/
    │       ├── tensorflow-lite/lib/hifi4/xcc/rt798s/libtflm.a   ← TFLM 库（入库）
    │       └── neutron/rt700/hifi4/libNeutron{Driver,Firmware}.a ← NPU 库（入库）
    └── examples/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4/
        ├── main.cpp                        ← denoise 入口（Init → RunOnce）
        ├── main.cpp.orig                   ← 原 cifar10 main.cpp 备份
        └── denoise/                        ← ★本工程新增的 denoise 源码
            ├── stft.c / stft.h             ← 加窗 + 正/逆 STFT + 谱幅
            ├── npu_mask.c / npu_mask.h     ← NPU 调用面（MODEL_Init / 喂输入 / Invoke / 读出）
            ├── denoise_pipeline.c / .h     ← 串起整条流水线 + PCM 生成 + 往返误差自检
            └── pcm_input.h                 ← 内存 PCM 缓冲参数
```

两棵树都是 **standalone**（各自带全套 drivers/components/devices/linker），构建只依赖本仓内文件
加 Xtensa / armgcc 两个工具链，不引用 SDK 其他任何例程。

### denoise 源码逐文件

| 文件 | 职责 |
|------|------|
| `stft.c/.h` | `STFT_Init` 建 Hann Q31 窗表；`STFT_Forward` 加窗后调 `fft_real32x32`（句柄 `rfft32_256`）；`STFT_Magnitude` 取 `|Re|+|Im|`；`STFT_Inverse` 谱×mask 后调 `ifft_real32x32`（`rifft32_256`）再加窗。缓冲 8 字节对齐（FFT 要求），输入内部拷贝以免被 in-place 破坏。 |
| `npu_mask.c/.h` | `NPU_MaskInit` 调 `MODEL_Init()` 并打印 I/O 张量形状；`NPU_MaskRun` 把谱幅压成 int8 塞进模型输入前 K 个（占位映射，仅为驱动 NPU），真跑 `MODEL_RunInference()`（内部 `Invoke`），计时并打印状态/耗时/输出，mask 取恒等 `0x8000`。 |
| `denoise_pipeline.c/.h` | `PCM_GenerateInput` 合成测试信号（500 Hz 主音 + 3500 Hz 高频"噪声"）；`DENOISE_Init` = STFT_Init + 生成 PCM + NPU_MaskInit；`DENOISE_RunOnce` 跑帧循环（STFT→谱幅→NPU→ISTFT→overlap-add）并在稳态区间算往返误差 `|out-in|/|in|×1000` 自检。 |
| `pcm_input.h` | `PCM_TOTAL_SAMPLES=1024`、`PCM_FULL_SCALE=1<<24`（留 7 bit 余量防 FFT 溢出）。 |
| `main.cpp` | `BOARD_Init → TIMER_Init → banner → DENOISE_Init → DENOISE_RunOnce`，用 `extern "C"` 包 `denoise_pipeline.h`。 |

---

## 3. 构建

### 3.1 一键脚本

```bash
cd step2_denoise_skeleton
bash build_step2.sh            # 完整构建（见下 4 阶段）
bash build_step2.sh --restore  # 还原本地 cm33 原始 release bin 基线
```

脚本 4 个阶段：

1. **HiFi4 自编译**（`xt-clang` / `xt-clang++`）：29 源（21 C `-std=c99` + 5 C++ `-std=c++14 -stdlib=libc++` + denoise），
   链接本地 `libtflm.a` + `libNeutron*.a` + `libnaturedsp.a`，用 `xt-nm` 核验 `fft_real32x32`/`ifft_real32x32` 真符号。
2. **切 4 段 bin**（`xt-objcopy`）：从 HiFi4 ELF 切出 `dsp_{literal,text,data,ncache}.bin`。
3. **覆盖本地 cm33 release bin**：⚠️ 目标是 `dsp_*_release.bin`（**不是** `_debug`——armgcc 的 ASM 规则不传 `-DDEBUG`，
   `incbin.S` 的 `#ifdef DEBUG` 为假，固件实际嵌的是 `_release` 组）。覆盖前先备份到 `backup_orig_release_bins/`。
4. **重编 M33 + 验证**（cmake -G Ninja + ninja）：用 **arm-none-eabi** 工具链从 M33 ELF 的 **`.dspcode`** 段
   抽回 data 子段，md5 与自编译的 `dsp_data.bin` 对比，字节一致即证明镜像进了固件。

### 3.2 工具链与环境

- **Xtensa**：`XtDevTools/.../RI-2023.11-win32`；`XTENSA_CORE=rt700_hifi4_RI23_11_nlib`；`XTENSA_SYSTEM` 指向 `XtensaTools/config`。
- **ARM**：`arm-none-eabi-*`（gcc + nm/objcopy），CMake + Ninja。
- 两者都在脚本里写死路径，按本机实际安装位置改 `build_step2.sh` 顶部的 `XT=`。

### 3.3 VSCode

打开 `step2_denoise_skeleton.code-workspace`，含 2 个 folder（CM33 armgcc / HiFi4 xtensa），
clangd 多核配置（`--query-driver` 同时放行 arm-none-eabi 与 xt-*，关掉内置 intelliSenseEngine 避免冲突）。

---

## 4. 已实测（命令行独立构建，只用本仓本地树）

| 项 | 结果 |
|----|------|
| HiFi4 编译链接 | **29 源 OK=29 FAIL=0** → `step2_denoise_skeleton.elf` **2 867 112 字节** |
| NatureDSP 真链进来 | `xt-nm` 见 `fft_real32x32 @2045e210`、`ifft_real32x32 @2045e520` 真符号 |
| 切段 | literal 12 / text 1604 / **data 407 716** / ncache 262 144 字节 |
| M33 重编 | `cm33_core0.elf` **970 292 字节** |
| 镜像进固件校验 | M33 `.dspcode` 抽回 data md5 = 自编译 data md5 = **`0ba29b80393c29be591bef60f6f9273d`**，**字节一致 ✅** |

烧录后串口应打印：`[step2] SAI(memloop) + HiFi4 STFT/ISTFT + NPU skeleton` → NPU 形状 →
`Invoke OK / us` → 往返误差 → `[step2] idle.`。

---

## 5. git 与自洽性

- 本目录是独立 git 仓，首个 commit `326d5ce`。
- **库入库决策**：`*.a`（libtflm/libNeutron*/libnaturedsp）与内嵌镜像 `*.bin` 都提交进仓 →
  clone 后无需任何外部 SDK 即可直接 `bash build_step2.sh`。
- **构建产物不入库**（`.gitignore` 排除）：`_build/`、`logs/`、`backup_orig_release_bins/`、
  `CMakeFiles/`、`*.o`/`*.elf`/`*.map`、`compile_commands.json` 等。
- ⚠️ 跑一次 `build_step2.sh` 会覆写仓内的 `dsp_*_release.bin` 和 `cm33_core0.bin`（这是构建的正常产物）；
  需要回到干净基线用 `bash build_step2.sh --restore`，或 `git checkout -- .`。

---

## 6. 坑点（沿用 step1 的经验）

- 覆盖的是 cm33 侧 **`dsp_*_release.bin`**（非 `_debug`，原因见 §3.1 第 3 步）。
- 验证 M33 侧镜像**必须用 arm-none-eabi 工具链**（M33 是 ARM 目标，不是 xt-*），
  且镜像住在 **`.dspcode`** 段（不是 `.rodata`）。
- `fft_real32x32` / `ifft_real32x32` 都是 **in-place 破坏输入**，`x`/`y` 须 8 字节对齐且不重叠。
- c99 下没有 `M_PI`，`stft.c` 自带 `#ifndef M_PI #define M_PI ... #endif`。

---

## 7. ⚠️ 诚实边界（这不是真降噪器）

1. **模型是 cifar10 占位，不是真降噪 mask**。谱幅→模型输入是无语义的占位映射，只为驱动 NPU 调用链；输出未解析成 per-bin 增益，mask 恒等。
2. **无 codec，实时 SAI 录放未上板**。音频源是内存合成 PCM，HiFi4 直接读写内存缓冲（回环）。
3. **STFT 特征未与训练侧对齐**（帧长/窗/缩放按 NatureDSP 例程取值，真模型需与训练一致）。
4. 往返误差是粗判（`|Re|+|Im|` 幅度近似 + 定点缩放），不是精确重建质量指标。

---

## 8. 下一步（走向真降噪）

1. **真降噪模型**：训练一个 mask 预测模型 → neutron-converter 转 → 替换 `model_data.h`（走 step1 固化的换镜像链路）。
2. **真 mask 解析**：`npu_mask.c` 把 `MODEL_GetOutputTensorData()` 解成每个频点的 mask（而非恒等）。
3. **特征对齐**：`stft.c` 的帧长/窗/特征与训练侧一致（可能需 log-mel，NatureDSP 有 `logmel32x32`）。
4. **真 SAI**：上板接 codec，把内存回环换成 SAI + EDMA 录放（参考 `sai_edma_record_playback`）。
