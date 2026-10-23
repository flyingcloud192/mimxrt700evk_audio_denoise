/*
 * npu_mask.c  —  NPU 模型调用桥 (骨架占位)
 *
 * 复用 tflm_neutron 的 MODEL_* API (common/tflm/model.cpp, extern "C")。
 * 骨架阶段证明的是"调用路径": MODEL_Init 一次, 每帧填输入→Invoke→读输出。
 * 现有 cifar10 输入是 [1,32,32,3] int8 = 3072 元素; 谱幅只有 STFT_BINS(=129) 个,
 * 按"填前 K 个、其余置零"的占位映射塞进去, 只为驱动 Invoke, 不代表任何语义。
 */
#include "npu_mask.h"
#include "model.h"
#include "timer.h"
#include "fsl_debug_console.h"
#include <string.h>

static uint8_t      *s_in_data;
static tensor_dims_t s_in_dims;
static tensor_type_t s_in_type;
static uint8_t      *s_out_data;
static tensor_dims_t s_out_dims;
static tensor_type_t s_out_type;
static int           s_in_elems;

static int tensor_elems(const tensor_dims_t *d)
{
    int n = 1;
    for (uint32_t i = 0; i < d->size; i++)
        n *= (int)d->data[i];
    return n;
}

int NPU_MaskInit(void)
{
    if (MODEL_Init() != kStatus_Success)
    {
        PRINTF("[NPU] MODEL_Init failed\r\n");
        return -1;
    }

    s_in_data  = MODEL_GetInputTensorData(&s_in_dims, &s_in_type);
    s_out_data = MODEL_GetOutputTensorData(&s_out_dims, &s_out_type);
    s_in_elems = tensor_elems(&s_in_dims);

    PRINTF("[NPU] model = %s\r\n", MODEL_GetModelName());
    PRINTF("[NPU] input  dims=%u [%u,%u,%u,%u] type=%d elems=%d\r\n",
           s_in_dims.size, s_in_dims.data[0], s_in_dims.data[1],
           s_in_dims.data[2], s_in_dims.data[3], (int)s_in_type, s_in_elems);
    PRINTF("[NPU] output dims=%u [%u,%u,%u,%u] type=%d\r\n",
           s_out_dims.size, s_out_dims.data[0], s_out_dims.data[1],
           s_out_dims.data[2], s_out_dims.data[3], (int)s_out_type);
    return 0;
}

int NPU_MaskRun(const int32_t *mag, int32_t *mask_q15)
{
    /* --- 占位映射: 谱幅 → 模型输入张量 (填前 K 个, 其余置零) --- */
    int k = STFT_BINS < s_in_elems ? STFT_BINS : s_in_elems;
    memset(s_in_data, 0, (size_t)s_in_elems); /* int8/uint8 张量按字节清零 */
    for (int i = 0; i < k; i++)
    {
        /* 把 int32 谱幅压到 int8 范围 (右移取高位), 仅为给 NPU 一个非零输入。 */
        int32_t v = mag[i] >> 24;
        if (v > 127) v = 127;
        if (v < -128) v = -128;
        ((int8_t *)s_in_data)[i] = (int8_t)v;
    }

    /* --- 真跑一次 NPU 推理, 计时 --- */
    int t0 = TIMER_GetTimeInUS();
    status_t st = MODEL_RunInference();
    int t1 = TIMER_GetTimeInUS();

    if (st != kStatus_Success)
        PRINTF("[NPU] Invoke FAILED\r\n");

    /* --- 骨架: mask 取恒等 (0x8000 = Q15 的 1.0), 保证 ISTFT 后波形≈输入 ---
     * 真 mask 下一步从 s_out_data 解析; 这里读一下输出前几个值证明张量被写了。 */
    for (int i = 0; i < STFT_BINS; i++)
        mask_q15[i] = 0x8000;

    PRINTF("[NPU] Invoke %s, %d us, out[0..3]=%d,%d,%d,%d\r\n",
           st == kStatus_Success ? "OK" : "FAIL", t1 - t0,
           ((int8_t *)s_out_data)[0], ((int8_t *)s_out_data)[1],
           ((int8_t *)s_out_data)[2], ((int8_t *)s_out_data)[3]);
    return t1 - t0;
}
