# config to select component, the format is CONFIG_USE_${component}
# Please refer to cmake files below to get available components:
#  ${ProjDirPath}/devices/MIMXRT798S/all_lib_device.cmake

set(CONFIG_COMPILER gcc)
set(CONFIG_TOOLCHAIN armgcc)
set(CONFIG_USE_COMPONENT_CONFIGURATION false)
set(CONFIG_CORE cm33)
set(CONFIG_DEVICE MIMXRT798S)
set(CONFIG_BOARD mimxrt700evk)
set(CONFIG_KIT mimxrt700evk)
set(CONFIG_DEVICE_ID MIMXRT798S)
set(CONFIG_FPU SP_FPU)
set(CONFIG_DSP '')
set(CONFIG_CORE_ID cm33_core0)
