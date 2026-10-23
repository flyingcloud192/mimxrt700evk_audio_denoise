/*
 * pcm_input.h  —  step2 骨架用的合成测试 PCM (内存回环的"音频源")
 *
 * 一段 1024 样本的 int32 时域信号: 低频正弦 + 少量高频"噪声"叠加。
 * 用 int32 Q31-ish 定点(幅度留足余量避免 fft 溢出), 便于直接喂 fft_real32x32。
 * 骨架阶段无需真实音频; 这段信号只为驱动 STFT→NPU→ISTFT 往返并做误差自检。
 *
 * 生成规则 (见 README): s[n] = A*sin(2*pi*f0*n/Fs) + B*sin(2*pi*f1*n/Fs)
 *   Fs=16000, f0=500Hz(主), f1=3500Hz(高频"噪声"), A=0.5*full, B=0.1*full
 * 这里直接内嵌预算好的数组, 不在板上做浮点生成。
 */
#ifndef _PCM_INPUT_H_
#define _PCM_INPUT_H_

#include <stdint.h>

#define PCM_TOTAL_SAMPLES 1024
#define PCM_FULL_SCALE    (1 << 24) /* 幅度上限, 留 7 bit 余量防 fft 溢出 */

/* 运行时按公式填充 (避免手敲 1024 个常量); 由 denoise_pipeline 在 init 时生成。*/
extern int32_t g_pcm_input[PCM_TOTAL_SAMPLES];
extern int32_t g_pcm_output[PCM_TOTAL_SAMPLES];

void PCM_GenerateInput(void); /* 用整数近似正弦填 g_pcm_input */

#endif /* _PCM_INPUT_H_ */
