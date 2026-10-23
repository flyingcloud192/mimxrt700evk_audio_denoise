/*
 * denoise_pipeline.h  —  端到端骨架一帧/整段处理 + 内存回环驱动
 */
#ifndef _DENOISE_PIPELINE_H_
#define _DENOISE_PIPELINE_H_

/* 初始化 STFT 窗 + NPU 模型 + 合成 PCM。返回 0 成功。 */
int DENOISE_Init(void);

/* 跑一遍完整数据流: 读内存 PCM → 逐帧 STFT → NPU → ISTFT → overlap-add → 写回 g_pcm_output。
 * 跑完打印: 处理帧数、NPU 累计耗时、STFT/ISTFT 往返重建误差 (mask=恒等时应很小)。 */
void DENOISE_RunOnce(void);

#endif /* _DENOISE_PIPELINE_H_ */
