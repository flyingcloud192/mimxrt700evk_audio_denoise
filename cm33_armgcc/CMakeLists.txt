# CROSS COMPILER SETTING
set(CMAKE_SYSTEM_NAME Generic)
cmake_minimum_required(VERSION 3.10.0)

if (NOT DEFINED SdkRootDirPath)
    SET(SdkRootDirPath ${CMAKE_CURRENT_LIST_DIR})
endif()

include(${CMAKE_CURRENT_LIST_DIR}/config.cmake)
include(${SdkRootDirPath}/cmake/toolchain/toolchain.cmake)

# THE VERSION NUMBER
SET (MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION 2)
SET (MCUXPRESSO_CMAKE_FORMAT_MINOR_VERSION 0)

include(ide_overrides.cmake OPTIONAL)

if(CMAKE_SCRIPT_MODE_FILE)
  message("${MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION}")
  return()
endif()


set(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
set(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
set(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

set(EXECUTABLE_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})


project(tflm_cifar10_hifi4_neutron_cm33_core0)

enable_language(ASM)

set(MCUX_BUILD_TYPES debug)

set(EXECUTABLE_OUTPUT_PATH ${ProjDirPath})
set(LIBRARY_OUTPUT_PATH ${ProjDirPath})
set(MCUX_SDK_PROJECT_NAME tflm_cifar10_hifi4_neutron_cm33_core0.elf)

include(${ProjDirPath}/flags.cmake)

add_executable(${MCUX_SDK_PROJECT_NAME} 
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_clock.c"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_dsp.c"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_power.c"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_reset.c"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/system_MIMXRT798S_cm33_core0.c"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/startup_MIMXRT798S_cm33_core0.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/flash_config/flash_config.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/pmic_support.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/board.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/common/clock/cm33_core0/clock_config.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/cm33_core0/pin_mux.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/cm33_core0/hardware_init.c"
"${ProjDirPath}/drivers/common/fsl_common.c"
"${ProjDirPath}/drivers/common/fsl_common_arm.c"
"${ProjDirPath}/drivers/glikey/fsl_glikey.c"
"${ProjDirPath}/drivers/gpio/fsl_gpio.c"
"${ProjDirPath}/drivers/lpflexcomm/fsl_lpflexcomm.c"
"${ProjDirPath}/drivers/xspi/fsl_xspi.c"
"${ProjDirPath}/drivers/cache/xcache/fsl_cache.c"
"${ProjDirPath}/drivers/lpflexcomm/lpi2c/fsl_lpi2c.c"
"${ProjDirPath}/drivers/lpflexcomm/lpuart/fsl_lpuart.c"
"${ProjDirPath}/components/assert/fsl_assert.c"
"${ProjDirPath}/components/debug_console_lite/fsl_debug_console.c"
"${ProjDirPath}/components/misc_utilities/fsl_syscall_stub.c"
"${ProjDirPath}/components/misc_utilities/fsl_sbrk.c"
"${ProjDirPath}/components/misc_utilities/fsl_memcpy.S"
"${ProjDirPath}/components/misc_utilities/fsl_memset.S"
"${ProjDirPath}/components/pmic/pca9422/fsl_pca9422.c"
"${ProjDirPath}/components/str/fsl_str.c"
"${ProjDirPath}/components/uart/fsl_adapter_lpuart.c"
"${ProjDirPath}/examples/eiq_examples/tflm_cifar10_hifi4_neutron/cm/main_cm33.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/dsp_support.c"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/incbin.S"
"${ProjDirPath}/mcux_config.h"
"${ProjDirPath}/mcuxsdk_version.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_clock.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_dsp.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_iopctl.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_power.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers/fsl_reset.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/fsl_device_registers.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/MIMXRT798S_cm33_core0.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/MIMXRT798S_cm33_core0_COMMON.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/MIMXRT798S_cm33_core0_features.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_ADC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_AHBSC0.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_AHBSC3.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_AHBSC4.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CACHE64_CTRL.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CACHE64_POLSEL.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CDOG.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CLKCTL0.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CLKCTL1.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CLKCTL2.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CLKCTL3.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CLKCTL4.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CMP.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CRC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_CTIMER.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_DEBUG_MAILBOX.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_DMA.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_ELS.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_FLEXIO.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_FREQME.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_FRO.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_GDET.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_GLIKEY.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_GPIO.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_I2S.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_I3C.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_INPUTMUX0.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_INPUTMUX1.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_IOPCTL0.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_IOPCTL1.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_IOPCTL2.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_ITRC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_JPEGDEC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_JPGDECWRP.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_LCDIF.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_LPI2C.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_LPSPI.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_LPUART.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_LP_FLEXCOMM.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_MIPI_DSI_HOST.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_MMU.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_MRT.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_MU.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_NIC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_OCOTP.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_OSC32KNP.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_OSCCA.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_OSTIMER.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_PDM.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_PINT.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_PKC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_PMC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_PNGDEC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_PUF.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_PVTS.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_RSTCTL0.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_RSTCTL1.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_RSTCTL2.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_RSTCTL3.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_RSTCTL4.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_RTC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SCT.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SDADC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SEMA42.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SLEEPCON0.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SLEEPCON1.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SYSCON0.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SYSCON1.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SYSCON2.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SYSCON3.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SYSCON4.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_SYSPM.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_TRNG.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_USB.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_USBHSDCD.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_USBNC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_USBPHY.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_USDHC.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_UTICK.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_WWDT.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_XCACHE.h"
"${ProjDirPath}/devices/RT/RT700/periph/PERI_XSPI.h"
"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/system_MIMXRT798S_cm33_core0.h"
"${ProjDirPath}/arch/arm/CMSIS/Core/Include/cmsis_compiler.h"
"${ProjDirPath}/arch/arm/CMSIS/Core/Include/cmsis_version.h"
"${ProjDirPath}/arch/arm/CMSIS/Core/Include/cmsis_gcc.h"
"${ProjDirPath}/arch/arm/CMSIS/Core/Include/m-profile/cmsis_gcc_m.h"
"${ProjDirPath}/arch/arm/CMSIS/Core/Include/core_cm33.h"
"${ProjDirPath}/arch/arm/CMSIS/Core/Include/m-profile/armv8m_mpu.h"
"${ProjDirPath}/arch/arm/CMSIS/Core/Include/tz_context.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/flash_config/flash_config.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/pmic_support.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/board.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/common/clock/cm33_core0/clock_config.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/cm33_core0/pin_mux.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/cm33_core0/app.h"
"${ProjDirPath}/drivers/common/fsl_common.h"
"${ProjDirPath}/drivers/common/fsl_common_arm.h"
"${ProjDirPath}/drivers/glikey/fsl_glikey.h"
"${ProjDirPath}/drivers/gpio/fsl_gpio.h"
"${ProjDirPath}/drivers/lpflexcomm/fsl_lpflexcomm.h"
"${ProjDirPath}/drivers/xspi/fsl_xspi.h"
"${ProjDirPath}/drivers/cache/xcache/fsl_cache.h"
"${ProjDirPath}/drivers/lpflexcomm/lpi2c/fsl_lpi2c.h"
"${ProjDirPath}/drivers/lpflexcomm/lpuart/fsl_lpuart.h"
"${ProjDirPath}/components/assert/fsl_assert.h"
"${ProjDirPath}/components/debug_console_lite/fsl_debug_console.h"
"${ProjDirPath}/components/pmic/pca9422/fsl_pca9422.h"
"${ProjDirPath}/components/str/fsl_str.h"
"${ProjDirPath}/components/uart/fsl_adapter_uart.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/dsp_config.h"
"${ProjDirPath}/examples/eiq_examples/common/board_init.h"
"${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/dsp_support.h"
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    "${ProjDirPath}/devices/RT/RT700/MIMXRT798S/drivers"
    "${ProjDirPath}/devices/RT/RT700/MIMXRT798S"
    "${ProjDirPath}/devices/RT/RT700/periph"
    "${ProjDirPath}/arch/arm/CMSIS/Core/Include"
    "${ProjDirPath}/arch/arm/CMSIS/Core/Include/m-profile"
    "${ProjDirPath}/examples/_boards/mimxrt700evk/flash_config"
    "${ProjDirPath}/examples/_boards/mimxrt700evk"
    "${ProjDirPath}/examples/_boards/mimxrt700evk/common/clock/cm33_core0"
    "${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/cm33_core0"
    "${ProjDirPath}/drivers/common"
    "${ProjDirPath}/drivers/glikey"
    "${ProjDirPath}/drivers/gpio"
    "${ProjDirPath}/drivers/lpflexcomm"
    "${ProjDirPath}/drivers/xspi"
    "${ProjDirPath}/drivers/cache/xcache"
    "${ProjDirPath}/drivers/lpflexcomm/lpi2c"
    "${ProjDirPath}/drivers/lpflexcomm/lpuart"
    "${ProjDirPath}/components/assert"
    "${ProjDirPath}/components/debug_console_lite"
    "${ProjDirPath}/components/pmic/pca9422"
    "${ProjDirPath}/components/str"
    "${ProjDirPath}/components/uart"
    "${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron"
    "${ProjDirPath}/examples/eiq_examples/common"
    "${ProjDirPath}/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4/binary"
    "${ProjDirPath}/../../tflm_cifar10_hifi4_neutron/armgcc"
)


IF(NOT DEFINED TARGET_LINK_SYSTEM_LIBRARIES)  
    SET(TARGET_LINK_SYSTEM_LIBRARIES "-lm -lc -lgcc -lnosys")  
ENDIF()  

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE ${TARGET_LINK_SYSTEM_LIBRARIES})

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

ADD_CUSTOM_COMMAND(TARGET ${MCUX_SDK_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY}
-Obinary ${EXECUTABLE_OUTPUT_PATH}/${MCUX_SDK_PROJECT_NAME} ${EXECUTABLE_OUTPUT_PATH}/tflm_cifar10_hifi4_neutron_cm33_core0.bin)

set_target_properties(${MCUX_SDK_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES "output.map;${EXECUTABLE_OUTPUT_PATH}/tflm_cifar10_hifi4_neutron_cm33_core0.bin")

