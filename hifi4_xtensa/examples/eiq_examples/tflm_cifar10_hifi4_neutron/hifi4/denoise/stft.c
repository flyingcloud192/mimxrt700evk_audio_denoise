/*
 * stft.c  —  实数定点 STFT / ISTFT 实现 (NatureDSP fft_real32x32 / ifft_real32x32)
 *
 * 骨架目标: 跑通 加窗→FFT→(谱操作)→IFFT→加窗→overlap-add 的往返链路, 可做误差自检。
 * 不追求数值精度与真降噪; 定点缩放、窗、幅度近似都按"够驱动链路"选取。
 */
#include "stft.h"
#include <string.h>
#include <math.h>

#include "NatureDSP_Signal.h"
#include "NatureDSP_Signal_fft.h"
#include "NatureDSP_types.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 8 字节对齐的工作缓冲 (NatureDSP 要求 x/y 8-byte aligned)。 */
static int32_t s_win_q31[STFT_N] __attribute__((aligned(8)));     /* Hann 窗表 Q31 */
static int32_t s_frame[STFT_N]   __attribute__((aligned(8)));     /* 加窗后时域帧 (fft 输入, 会被破坏) */
static int32_t s_spec[STFT_SPEC_LEN + 2] __attribute__((aligned(8))); /* 谱缓冲 (+余量) */

void STFT_Init(void)
{
    /* Hann 窗: w[n] = 0.5 - 0.5*cos(2*pi*n/(N-1)), 存成 Q31。 */
    for (int n = 0; n < STFT_N; n++)
    {
        double w = 0.5 - 0.5 * cos(2.0 * M_PI * (double)n / (double)(STFT_N - 1));
        if (w < 0.0) w = 0.0;
        if (w > 0.999999) w = 0.999999;
        s_win_q31[n] = (int32_t)(w * 2147483647.0);
    }
}

/* Q31 乘法: (a * w_q31) >> 31, 带饱和省略 (输入已留余量)。 */
static inline int32_t mul_q31(int32_t a, int32_t w)
{
    return (int32_t)(((int64_t)a * (int64_t)w) >> 31);
}

void STFT_Forward(const int32_t *frame_in, int32_t *freq_out)
{
    for (int n = 0; n < STFT_N; n++)
        s_frame[n] = mul_q31(frame_in[n], s_win_q31[n]);

    /* in-place 会破坏 s_frame; 输出写到 freq_out[(N/2+1)*2]。 */
    (void)fft_real32x32(freq_out, s_frame, rfft32_256, STFT_SCALING);
}

void STFT_Magnitude(const int32_t *freq, int32_t *mag)
{
    /* 幅度近似 |Re|+|Im| (省 sqrt, 够当 NPU 占位输入)。 */
    for (int k = 0; k < STFT_BINS; k++)
    {
        int32_t re = freq[2 * k];
        int32_t im = freq[2 * k + 1];
        int32_t are = re < 0 ? -re : re;
        int32_t aim = im < 0 ? -im : im;
        mag[k] = (int32_t)(((int64_t)are + (int64_t)aim) >> 1); /* >>1 防溢出 */
    }
}

void STFT_Inverse(const int32_t *freq, const int32_t *mask_q15, int32_t *frame_out)
{
    /* 拷谱到工作缓冲 (ifft in-place 会破坏输入), 同时按 bin 施加 mask。 */
    for (int k = 0; k < STFT_BINS; k++)
    {
        int32_t m = mask_q15[k];               /* Q15, 0x8000 = 1.0 */
        int32_t re = freq[2 * k];
        int32_t im = freq[2 * k + 1];
        s_spec[2 * k]     = (int32_t)(((int64_t)re * m) >> 15);
        s_spec[2 * k + 1] = (int32_t)(((int64_t)im * m) >> 15);
    }

    (void)ifft_real32x32(s_frame, s_spec, rifft32_256, STFT_SCALING);

    /* 逆变换后再加一次窗 (STFT 标准合成窗), overlap-add 由上层累加。 */
    for (int n = 0; n < STFT_N; n++)
        frame_out[n] = mul_q31(s_frame[n], s_win_q31[n]);
}
