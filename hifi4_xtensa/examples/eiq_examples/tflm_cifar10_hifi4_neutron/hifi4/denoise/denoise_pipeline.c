/*
 * denoise_pipeline.c  —  SAI(内存回环) → HiFi4 STFT → NPU → HiFi4 ISTFT → 写回
 *
 * 骨架: 证明三方交互成立 + STFT/ISTFT 往返正确 (mask=恒等) + NPU 调用链成立。
 * 音频源是内存里的合成 PCM (pcm_input.h), 非真实 SAI 采集; 实时录放是上板项。
 */
#include "denoise_pipeline.h"
#include "pcm_input.h"
#include "stft.h"
#include "npu_mask.h"
#include "fsl_debug_console.h"
#include <string.h>

int32_t g_pcm_input[PCM_TOTAL_SAMPLES]  __attribute__((aligned(8)));
int32_t g_pcm_output[PCM_TOTAL_SAMPLES] __attribute__((aligned(8)));

/* 整数近似正弦: 用 16 点查表线性插值, 避免板上浮点。幅度 ~0.5*FULL + 0.1*FULL 高频。*/
static int32_t isin_q(int idx, int period)
{
    /* 粗糙正弦: 三角波近似足够驱动链路 (骨架不追求频谱纯度)。*/
    int p = ((idx % period) * 4) / period; /* 0..3 象限 */
    int frac = (idx % period) - (p * period) / 4;
    int quarter = period / 4;
    int32_t tri;
    switch (p)
    {
        case 0: tri =  (int32_t)(((int64_t)frac * PCM_FULL_SCALE) / quarter); break;
        case 1: tri =  (int32_t)(PCM_FULL_SCALE - ((int64_t)frac * PCM_FULL_SCALE) / quarter); break;
        case 2: tri = -(int32_t)(((int64_t)frac * PCM_FULL_SCALE) / quarter); break;
        default:tri = -(int32_t)(PCM_FULL_SCALE - ((int64_t)frac * PCM_FULL_SCALE) / quarter); break;
    }
    return tri;
}

void PCM_GenerateInput(void)
{
    /* Fs=16000: f0=500Hz → period=32 样本; f1=3500Hz → period≈4.57, 用 period=5 近似。*/
    for (int n = 0; n < PCM_TOTAL_SAMPLES; n++)
    {
        int32_t lo = isin_q(n, 32) / 2;       /* 主信号 ~0.5 */
        int32_t hi = isin_q(n, 5)  / 10;      /* 高频"噪声" ~0.1 */
        g_pcm_input[n] = lo + hi;
    }
}

int DENOISE_Init(void)
{
    STFT_Init();
    PCM_GenerateInput();
    if (NPU_MaskInit() != 0)
        return -1;
    return 0;
}

void DENOISE_RunOnce(void)
{
    static int32_t frame[STFT_N]  __attribute__((aligned(8)));
    static int32_t spec[STFT_SPEC_LEN + 2] __attribute__((aligned(8)));
    static int32_t mag[STFT_BINS];
    static int32_t mask[STFT_BINS];
    static int32_t rec[STFT_N]    __attribute__((aligned(8)));

    memset(g_pcm_output, 0, sizeof(g_pcm_output));

    int nframes = 0;
    long npu_us = 0;

    /* 50% overlap 分帧; overlap-add 重建到 g_pcm_output。*/
    for (int start = 0; start + STFT_N <= PCM_TOTAL_SAMPLES; start += STFT_HOP)
    {
        for (int n = 0; n < STFT_N; n++)
            frame[n] = g_pcm_input[start + n];

        STFT_Forward(frame, spec);        /* 加窗 + FFT */
        STFT_Magnitude(spec, mag);        /* 谱幅 */
        npu_us += NPU_MaskRun(mag, mask); /* NPU 推理 → mask (骨架恒等) */
        STFT_Inverse(spec, mask, rec);    /* 谱×mask + IFFT + 窗 */

        for (int n = 0; n < STFT_N; n++)  /* overlap-add */
            g_pcm_output[start + n] += rec[n];

        nframes++;
    }

    /* --- 往返重建误差自检 (mask=恒等): 取中间稳态区间, 避开首尾 overlap 不全处 ---
     * Hann 50% overlap 下合成窗平方和为常数, 波形应按比例重建; 这里只看形状相关性的粗判。*/
    long num = 0, den = 0;
    int lo = STFT_N, hi = PCM_TOTAL_SAMPLES - STFT_N; /* 稳态区间 */
    for (int n = lo; n < hi; n++)
    {
        int32_t d = g_pcm_output[n] - g_pcm_input[n];
        num += (long)(d < 0 ? -d : d);
        den += (long)(g_pcm_input[n] < 0 ? -g_pcm_input[n] : g_pcm_input[n]);
    }

    PRINTF("\r\n==== Denoise skeleton done ====\r\n");
    PRINTF("frames processed : %d\r\n", nframes);
    PRINTF("NPU total time   : %ld us (%ld us/frame)\r\n", npu_us, nframes ? npu_us / nframes : 0);
    PRINTF("roundtrip |err|/|in| (steady, x1000): %ld\r\n", den ? (num * 1000 / den) : -1);
    PRINTF("in [%d..]  : %d %d %d %d\r\n", lo,
           g_pcm_input[lo], g_pcm_input[lo+1], g_pcm_input[lo+2], g_pcm_input[lo+3]);
    PRINTF("out[%d..]  : %d %d %d %d\r\n", lo,
           g_pcm_output[lo], g_pcm_output[lo+1], g_pcm_output[lo+2], g_pcm_output[lo+3]);
    PRINTF("(mask=identity → out 应与 in 形状相关; 比值反映定点缩放/窗增益, 非0即链路通)\r\n");
}
