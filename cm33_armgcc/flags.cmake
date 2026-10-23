IF(NOT DEFINED FPU)  
    SET(FPU "-mfloat-abi=hard -mfpu=fpv5-sp-d16")  
ENDIF()  

IF(NOT DEFINED SPECS)  
    SET(SPECS "--specs=nosys.specs")  
ENDIF()  

IF(NOT DEFINED DEBUG_CONSOLE_CONFIG)  
    SET(DEBUG_CONSOLE_CONFIG "-DSDK_DEBUGCONSOLE=1")  
ENDIF()  

SET(CMAKE_ASM_FLAGS_DEBUG " \
    ${CMAKE_ASM_FLAGS_DEBUG} \
    -DCPU_MIMXRT798SGFOB_cm33_core0 \
    -DMIMXRT798S_cm33_core0_SERIES \
    -D__STARTUP_CLEAR_BSS \
    -DMCUXPRESSO_SDK \
    -D__STARTUP_INITIALIZE_NONCACHEDATA \
    -DDSP_IMAGE_COPY_TO_RAM=1 \
    -include \
    \"${ProjDirPath}/mcux_config.h\" \
    -include \
    \"${ProjDirPath}/mcuxsdk_version.h\" \
    -mcpu=cortex-m33 \
    -g \
    -mthumb \
    -fmacro-prefix-map=\"${ProjDirPath}/\"=./ \
    ${FPU} \
")
SET(CMAKE_C_FLAGS_DEBUG " \
    ${CMAKE_C_FLAGS_DEBUG} \
    -DCPU_MIMXRT798SGFOB_cm33_core0 \
    -DMIMXRT798S_cm33_core0_SERIES \
    -D__STARTUP_CLEAR_BSS \
    -DDEBUG \
    -DMCUX_META_BUILD \
    -DMCUXPRESSO_SDK \
    -DBOOT_HEADER_ENABLE=1 \
    -D__STARTUP_INITIALIZE_NONCACHEDATA \
    -DSDK_I2C_BASED_COMPONENT_USED=1 \
    -DDSP_IMAGE_COPY_TO_RAM=1 \
    -DEIQ_EXAMPLE_HSRUN_CLOCK \
    -include \
    \"${ProjDirPath}/mcux_config.h\" \
    -include \
    \"${ProjDirPath}/mcuxsdk_version.h\" \
    -mcpu=cortex-m33 \
    -g \
    -O0 \
    --specs=nano.specs \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -std=gnu99 \
    -Werror \
    -fstack-usage \
    -Wno-error=attributes \
    -fmacro-prefix-map=\"${ProjDirPath}/\"=./ \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG " \
    ${CMAKE_EXE_LINKER_FLAGS_DEBUG} \
    -mcpu=cortex-m33 \
    -g \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mthumb \
    -mapcs \
    -Wl,--gc-sections \
    -Wl,-static \
    -Wl,--print-memory-usage \
    -Xlinker \
    -Map=output.map \
    -Wl,--no-warn-rwx-segments \
    ${FPU} \
    ${SPECS} \
    -T\"${ProjDirPath}/devices/RT/RT700/MIMXRT798S/gcc/MIMXRT798Sxxxx_cm33_core0_ram.ld\" -static \
")
