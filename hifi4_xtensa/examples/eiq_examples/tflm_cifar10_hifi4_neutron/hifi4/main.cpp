/*
 * denoise_main.cpp  —  step2 骨架入口 (替代 tflm_cifar10 的 main.cpp)
 *
 * 流程: BOARD_Init → TIMER_Init → banner → DENOISE_Init (STFT+NPU+PCM) → DENOISE_RunOnce。
 * CM33 侧负责启动本 HiFi4 镜像 (与 tflm_neutron 一致); 此处 HiFi4 当主控跑整条数据流。
 */
#include "board_init.h"
#include "demo_info.h"
#include "fsl_debug_console.h"
#include "timer.h"

extern "C" {
#include "denoise_pipeline.h"
}

int main(void)
{
    BOARD_Init();
    TIMER_Init();

    DEMO_PrintInfo();
    PRINTF("\r\n[step2] SAI(memloop) + HiFi4 STFT/ISTFT + NPU skeleton\r\n");

    if (DENOISE_Init() != 0)
    {
        PRINTF("DENOISE_Init failed\r\n");
        for (;;) {}
    }

    DENOISE_RunOnce();

    PRINTF("[step2] idle.\r\n");
    for (;;) {}
}
