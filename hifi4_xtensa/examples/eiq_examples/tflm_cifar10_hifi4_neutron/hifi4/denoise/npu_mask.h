/*
 * npu_mask.h  —  把谱幅喂进 NPU 模型、跑推理、取输出当 mask (骨架占位)
 *
 * 骨架阶段: 用现有 cifar10 模型证明"HiFi4 → NPU 调用路径"成立。
 * 谱幅塞进模型输入张量前若干元素, 跑 MODEL_RunInference(), 读回输出。
 * mask 骨架阶段取恒等(0x8000), 保证 STFT→ISTFT 往返可做误差自检;
 * 同时打印 NPU Invoke 状态/耗时 + 输出张量前几个值, 证明推理真的跑了。
 */
#ifndef _NPU_MASK_H_
#define _NPU_MASK_H_

#include <stdint.h>
#include "stft.h"

/* 初始化 NPU 模型 (封装 MODEL_Init + 打印输入/输出张量形状)。返回 0 成功。 */
int NPU_MaskInit(void);

/* 输入谱幅 mag[STFT_BINS], 跑一次 NPU 推理, 产出 mask_q15[STFT_BINS]。
 * 骨架: mask 取恒等; 内部仍真跑 Invoke 并打印状态, 证明调用链。返回 Invoke 耗时(us)。 */
int NPU_MaskRun(const int32_t *mag, int32_t *mask_q15);

#endif /* _NPU_MASK_H_ */
