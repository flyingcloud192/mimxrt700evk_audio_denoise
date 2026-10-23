/*
 * stft.h  —  N=256 实数定点 STFT / ISTFT (基于 NatureDSP fft_real32x32)
 *
 * 分帧 + Hann 窗 + fft_real32x32 → 谱; 谱 × mask → ifft_real32x32 + 窗 + overlap-add 重建。
 * 50% overlap (HOP = N/2)。数据为 int32 定点。
 *
 * NatureDSP 实数 FFT 的谱布局 (已核对头文件):
 *   fft_real32x32: 输入 x[N], 输出 y[(N/2+1)*2], Re/Im 交错 [Re0,Im0, Re1,Im1, ..., Re(N/2),Im(N/2)]
 *   ifft_real32x32: 输入 x[(N/2+1)*2] 同布局, 输出 y[N]
 *   ⚠️ 两者都 in-place 破坏输入, 且 x/y 必须 8 字节对齐、不重叠。
 */
#ifndef _STFT_H_
#define _STFT_H_

#include <stdint.h>

#define STFT_N        256                 /* 帧长 (对应 rfft32_256 / rifft32_256) */
#define STFT_HOP      (STFT_N / 2)         /* 50% overlap */
#define STFT_BINS     (STFT_N / 2 + 1)     /* 实数谱独立 bin 数 (0..N/2) = 129 */
#define STFT_SPEC_LEN (STFT_BINS * 2)      /* 打包谱长度 (Re/Im 交错) = 258 */

#define STFT_SCALING  2                    /* NatureDSP 32-bit dynamic scaling (RT700 例程取值) */

/* 初始化: 预计算 Hann 窗表 (Q31 定点)。 */
void STFT_Init(void);

/* 加窗 + 正变换: frame_in[STFT_N] → freq_out[STFT_SPEC_LEN]。frame_in 不被破坏(内部拷贝)。 */
void STFT_Forward(const int32_t *frame_in, int32_t *freq_out);

/* 取谱幅度近似 |Re|+|Im| 到 mag[STFT_BINS] (供喂 NPU 占位输入)。 */
void STFT_Magnitude(const int32_t *freq, int32_t *mag);

/* 谱 × mask(mask_q15[STFT_BINS], 0x8000=1.0) 后逆变换 + 加窗 → frame_out[STFT_N]。freq 不被破坏(内部拷贝)。 */
void STFT_Inverse(const int32_t *freq, const int32_t *mask_q15, int32_t *frame_out);

#endif /* _STFT_H_ */
