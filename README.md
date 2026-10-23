# step2_denoise_skeleton — SAI + HiFi4(STFT/ISTFT) + NPU 端到端骨架（独立项目）

RT700 denoise 项目的**第二步**，现已升级为**完整独立的 VSCode workspace 项目**（结构对标 `dsp_mu_polling`）：
自带 CM33 + HiFi4(DSP+NPU) 的全部源码与库，用 git 单独管理，脱离其他例程即可独立打开/构建。

三方数据流（骨架，非真降噪）：

```
内存 PCM ──► [HiFi4] 加窗+STFT ──► 谱幅 ──► [NPU] Invoke ──► (mask) ──► [HiFi4] 谱×mask + ISTFT + overlap-add ──► 写回 PCM
  (SAI 占位)        fft_real32x32              MODEL_RunInference      恒等占位      ifft_real32x32
```

---

## 项目结构（独立自洽）

```
step2_denoise_skeleton/            ← git 仓库根 (独立仓)
├── .gitignore                     ← 排构建产物; 保留 *.a / *.bin (库入库, clone 后可直接构建)
├── README.md
├── step2_denoise_skeleton.code-workspace   ← 2 folder: CM33 armgcc / HiFi4 xtensa
├── build_step2.sh                 ← 一键构建 (只操作本地树)
├── cm33_armgcc/                   ← CM33 侧全树 (启动 HiFi4 + 内嵌镜像 + SAI 占位)
└── hifi4_xtensa/                  ← HiFi4 侧全树 (主控 + STFT/ISTFT + NPU)
    ├── middleware/eiq/...                       libtflm.a / libNeutron*.a (入库)
    ├── middleware/cadence/naturedsp/            libnaturedsp.a + hifi4/include (入库)
    └── examples/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4/
        ├── denoise/               ← denoise 源码 (并入工程树)
        │   stft.c/.h  npu_mask.c/.h  denoise_pipeline.c/.h  pcm_input.h
        ├── main.cpp               ← denoise 入口 (Init → RunOnce)
        └── main.cpp.orig          ← 原 cifar10 main.cpp 备份
```

两者都是 standalone 树（自带 drivers/components/devices/linker），构建只依赖本项目内文件 + Xtensa/armgcc 工具链。

---

## 一键用法

```bash
cd step2_denoise_skeleton
bash build_step2.sh            # HiFi4 自编译(含naturedsp) → 切段 → 覆盖本地cm33 → 重编M33 → md5自校验
bash build_step2.sh --restore  # 还原本地 cm33 原始 release bin 基线
```

VSCode：打开 `step2_denoise_skeleton.code-workspace`，2 folder（CM33 / HiFi4），clangd 索引两套工具链。

---

## 已实测（命令行独立构建，只用本地树）

- **29 源编译链接通过** → 2.87 MB HiFi4 ELF。
- **NatureDSP 真链进来**：`xt-nm` 见 `fft_real32x32` / `ifft_real32x32` 真符号。
- **本地 cm33 重编**：切 4 段 → 覆盖本地 `dsp_*_release.bin` → 重编 M33 →
  从 M33 elf `.dspcode` 段抽回 data 段 **md5 字节一致**（`0ba29b...`），证明镜像进了固件。

---

## 数据流细节（N=256, HOP=128, 50% overlap）

1. 取一帧 256 样本 → 加 Hann 窗 → `fft_real32x32` → 谱 `[(N/2+1)*2]` int32（Re/Im 交错）。
2. `STFT_Magnitude` 取 `|Re|+|Im|` 幅度（129 bin）。
3. `NPU_MaskRun`：谱幅压 int8 塞模型输入前 K 个（占位映射，仅驱动 Invoke），真跑 `Invoke()`，打印状态/耗时/输出。mask 恒等 `0x8000`。
4. `STFT_Inverse`：谱×mask → `ifft_real32x32` → 加窗。
5. overlap-add 累加到输出缓冲；稳态区间算 `|out-in|/|in|` 往返误差自检。

---

## ⚠️ 诚实边界

1. **模型是 cifar10 占位，不是真降噪 mask**。谱幅→模型输入是无语义占位映射，只为证明 NPU 调用链。
2. **无 codec，实时 SAI 录放未上板**。音频源是内存合成 PCM，HiFi4 直接读写内存缓冲（回环）。
3. **STFT 特征未与训练侧对齐**（帧长/窗/缩放按 naturedsp 例程取值）。
4. 往返误差是粗判（`|Re|+|Im|` 近似 + 定点缩放），非精确重建质量指标。

---

## 坑点（沿用 step1）

- 覆盖的是 cm33 侧 **`dsp_*_release.bin`**（非 `_debug`，因 armgcc ASM 规则不传 `-DDEBUG`）。
- 验证用 **arm-none-eabi** 工具链抽 M33 侧 **`.dspcode`** 段。
- 首次覆盖前备份原始 bin 到 `backup_orig_release_bins/`（此目录是运行时产物，不入库），`--restore` 还原。

---

## git

本目录是独立 git 仓库。库（*.a）与内嵌镜像（*.bin）入库 → clone 后可直接 `bash build_step2.sh`。
构建产物（`_build/`、`logs/`、CMakeFiles、*.o/*.elf/*.map 等）被 `.gitignore` 排除。

---

## 下一步

1. **真降噪模型**：训练 mask 模型 → neutron-converter 转 → 替换 `model_data.h`。
2. **真 mask 解析**：`npu_mask.c` 把 `MODEL_GetOutputTensorData()` 解成 per-bin mask（而非恒等）。
3. **特征对齐**：`stft.c` 帧长/窗/特征与训练侧一致（可能需 log-mel，naturedsp 有 `logmel32x32`）。
4. **真 SAI**：上板接 codec，把内存回环换成 SAI EDMA 录放（参考 `sai_edma_record_playback`）。
