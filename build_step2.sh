#!/usr/bin/env bash
# =============================================================================
# build_step2.sh  —  SAI + HiFi4(STFT/ISTFT) + NPU 端到端骨架 一键构建
#                    (独立项目版: 只操作本项目内的本地源树, 不借其他工程)
#
# 本地树:
#   hifi4_xtensa/   HiFi4 侧 (TFLM+Neutron+NatureDSP+denoise/, main.cpp=denoise 入口)
#   cm33_armgcc/    CM33 侧 (启动 HiFi4 + 内嵌镜像)
#
# 流程: HiFi4 自编译(xt-clang) → 切 4 段 bin → 覆盖本地 cm33 release bin → 重编 M33 → md5 自校验
#
# 用法:
#   bash build_step2.sh            # 完整构建
#   bash build_step2.sh --restore  # 还原本地 cm33 原始 release bin 基线
# =============================================================================
set -u

SELF="$(cd "$(dirname "$0")" && pwd)"
RT="$SELF/hifi4_xtensa"
CM33="$SELF/cm33_armgcc"
LOG="$SELF/logs"
BACKUP="$SELF/backup_orig_release_bins"
OUT="$SELF/_build"
OBJ="$OUT/obj"
BIN="$OUT/binary"

XT="C:/usr/xtensa/XtDevTools/install/tools/RI-2023.11-win32/XtensaTools/bin"
export XTENSA_SYSTEM="C:/usr/xtensa/XtDevTools/install/tools/RI-2023.11-win32/XtensaTools/config"
export XTENSA_CORE=rt700_hifi4_RI23_11_nlib

CM33_BIN="$CM33/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4/binary"
SEGS="data literal text"

mkdir -p "$OBJ" "$BIN" "$LOG" "$BACKUP"
die() { echo ""; echo "!! 失败: $*"; exit 1; }

# ---- --restore --------------------------------------------------------------
if [ "${1:-}" = "--restore" ]; then
  echo "== 还原本地 cm33 原始 release bin 基线 =="
  n=0
  for s in $SEGS; do
    [ -f "$BACKUP/dsp_${s}_release.bin" ] && {
      cp -f "$BACKUP/dsp_${s}_release.bin" "$CM33_BIN/dsp_${s}_release.bin" && echo "  恢复 dsp_${s}_release.bin" && n=$((n+1)); }
  done
  [ $n -eq 0 ] && echo "  (备份目录为空)"
  echo "完成。随后 'cd $CM33 && cmake -G Ninja -DCMAKE_BUILD_TYPE=debug . && ninja' 重建基线。"
  exit 0
fi

echo "============================================================"
echo " step2 独立项目一键构建  —  $(date '+%Y-%m-%d %H:%M:%S')"
echo "============================================================"

ND="$RT/middleware/cadence/naturedsp"
INC="-I$RT/devices/RT/RT700/MIMXRT798S/drivers -I$RT/devices/RT/RT700/periph -I$RT/devices/RT/RT700/MIMXRT798S \
-I$RT/examples/_boards/mimxrt700evk -I$RT/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4 \
-I$RT/drivers/common -I$RT/drivers/gpio -I$RT/drivers/lpflexcomm -I$RT/drivers/lpflexcomm/lpuart \
-I$RT/components/assert -I$RT/components/debug_console_lite -I$RT/components/str -I$RT/components/uart \
-I$RT/middleware/eiq/neutron/common/include -I$RT/middleware/eiq/neutron/driver/include \
-I$RT/middleware/eiq/tensorflow-lite \
-I$RT/middleware/eiq/tensorflow-lite/third_party/flatbuffers/include \
-I$RT/middleware/eiq/tensorflow-lite/third_party/gemmlowp \
-I$RT/middleware/eiq/tensorflow-lite/third_party/kissfft \
-I$RT/middleware/eiq/tensorflow-lite/third_party/ruy \
-I$RT/middleware/eiq/tensorflow-lite/third_party/cmsis_nn \
-I$RT/middleware/eiq/tensorflow-lite/third_party/cmsis_nn/Include \
-I$RT/examples/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4 \
-I$RT/examples/eiq_examples/tflm_cifar10_hifi4_neutron/hifi4/denoise \
-I$RT/examples/eiq_examples/common -I$RT/examples/eiq_examples/common/image \
-I$RT/examples/eiq_examples/common/tflm/model \
-I$ND/hifi4/include"
DEF="-DCPU_MIMXRT798SGFOB_hifi4 -DDEBUG -DMCUX_META_BUILD -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DTF_LITE_STATIC_MEMORY -DKERNELS_OPTIMIZED_FOR_SPEED -DTENSORARENA_NONCACHE=1"
FINC="-include $RT/mcux_config.h -include $RT/mcuxsdk_version.h"
COMMON="--xtensa-core=$XTENSA_CORE -O0 -g3 -mlongcalls -Wall -fsigned-char -fmessage-length=0 -Wno-unused -Wno-missing-braces"

# denoise/ 已在 hifi4 树内, main.cpp 已是 denoise 入口 → 全量编译, 无需排除/注入
CSRC=$(find "$RT" -type f -name "*.c"   ! -path "*/third_party/*" ! -path "*/tensorflow-lite/lib/*" ! -path "*/naturedsp/*" | sort)
CPPSRC=$(find "$RT" -type f \( -name "*.cpp" -o -name "*.cc" \) ! -path "*/third_party/*" ! -path "*/tensorflow-lite/lib/*" ! -path "*/naturedsp/*" ! -name 'main.cpp.orig' | sort)

FAIL=0; OK=0
compile() { # $1=compiler $2=std $3=src $4=extra
  local base; base=$(echo "$3" | md5sum | cut -c1-8)_$(basename "$3")
  if "$1" $COMMON -std=$2 ${4:-} $DEF $FINC $INC -c "$3" -o "$OBJ/$base.o" 2>"$OBJ/$base.err"; then OK=$((OK+1));
  else FAIL=$((FAIL+1)); echo "  FAIL: $(basename "$3")"; grep -E "error:" "$OBJ/$base.err" | head -5; fi
}

echo ""
echo "[1/4] HiFi4 自编译 (TFLM+Neutron+NatureDSP+denoise) ..."
for f in $CSRC;   do compile "$XT/xt-clang.exe"   c99   "$f"; done
for f in $CPPSRC; do compile "$XT/xt-clang++.exe" c++14 "$f" "-stdlib=libc++"; done
echo "      编译结果: OK=$OK FAIL=$FAIL"
[ $FAIL -ne 0 ] && die "编译 (见 $OBJ/*.err)"

echo "      链接 (本地库: libtflm/libNeutron*/libnaturedsp) ..."
LSP="$RT/examples/_boards/mimxrt700evk/eiq_examples/tflm_cifar10_hifi4_neutron/linker/hifi4/gdbio"
ELF="$OUT/step2_denoise_skeleton.elf"
"$XT/xt-clang++.exe" --xtensa-core=$XTENSA_CORE -mlsp="$LSP" $OBJ/*.o \
  -Wl,--orphan-handling=place -Wl,--start-group \
  "$RT/middleware/eiq/neutron/rt700/hifi4/libNeutronDriver.a" \
  "$RT/middleware/eiq/neutron/rt700/hifi4/libNeutronFirmware.a" \
  "$RT/middleware/eiq/tensorflow-lite/lib/hifi4/xcc/rt798s/libtflm.a" \
  "$ND/hifi4_rt700/libnaturedsp.a" \
  -Wl,--end-group -lxos \
  -o "$ELF" 2>"$OUT/link.err"
[ $? -ne 0 ] && { head -40 "$OUT/link.err"; die "链接 (见 $OUT/link.err)"; }
echo "      LINK OK → $ELF ($(stat -c%s "$ELF") bytes)"
echo "      naturedsp 符号核验:"
"$XT/xt-nm.exe" "$ELF" | grep -wE 'fft_real32x32|ifft_real32x32' | sed 's/^/        /' || echo "        ⚠️ 未见 FFT 符号"

echo ""
echo "[2/4] 切 4 段 bin ..."
"$XT/xt-objcopy.exe" -O binary "$ELF" "$BIN/dsp_literal_debug.bin" \
  --only-section=.Level3InterruptVector.literal --only-section=.DebugExceptionVector.literal \
  --only-section=.NMIExceptionVector.literal --only-section=.UserExceptionVector.literal
"$XT/xt-objcopy.exe" -O binary "$ELF" "$BIN/dsp_text_debug.bin" \
  --only-section=.ResetVector.text --only-section=.WindowVectors.text \
  --only-section=.Level2InterruptVector.text --only-section=.Level3InterruptVector.text \
  --only-section=.DebugExceptionVector.text --only-section=.NMIExceptionVector.text \
  --only-section=.KernelExceptionVector.text --only-section=.UserExceptionVector.text \
  --only-section=.DoubleExceptionVector.text
"$XT/xt-objcopy.exe" -O binary "$ELF" "$BIN/dsp_data_debug.bin" \
  --only-section=.clib.rodata --only-section=.rtos.rodata --only-section=.rodata \
  --only-section=.text --only-section=.clib.data --only-section=.rtos.percpu.data \
  --only-section=.data --only-section=.bss
"$XT/xt-objcopy.exe" -O binary "$ELF" "$BIN/dsp_ncache_debug.bin" --only-section=NonCacheable
for s in literal text data ncache; do printf "      dsp_%-8s %8d bytes\n" "${s}" "$(stat -c%s "$BIN/dsp_${s}_debug.bin")"; done

echo ""
echo "[3/4] 覆盖本地 cm33 release bin (⚠️ _release 非 _debug) ..."
for s in $SEGS; do
  dst="$CM33_BIN/dsp_${s}_release.bin"
  [ -f "$dst" ] || die "cm33 目标不存在: $dst"
  [ -f "$BACKUP/dsp_${s}_release.bin" ] || { cp -f "$dst" "$BACKUP/dsp_${s}_release.bin"; echo "      备份原始 dsp_${s}_release.bin"; }
  cp -f "$BIN/dsp_${s}_debug.bin" "$dst" && echo "      覆盖 dsp_${s}_release.bin"
done

echo ""
echo "[4/4] 重编 M33 + 验证 ..."
(
  cd "$CM33" || exit 1
  rm -rf CMakeFiles Makefile build.ninja cmake_install.cmake CMakeCache.txt 2>/dev/null
  cmake -G "Ninja" -DCMAKE_BUILD_TYPE=debug . && ninja
) > "$LOG/cm33_build.log" 2>&1 || { tail -30 "$LOG/cm33_build.log"; die "M33 重编 (见 $LOG/cm33_build.log)"; }
M33_ELF=$(find "$CM33" -maxdepth 2 -name "*.elf" | head -1)
echo "      M33 OK → $M33_ELF ($(stat -c%s "$M33_ELF") bytes)"

ANM=$(command -v arm-none-eabi-nm 2>/dev/null); AOC=$(command -v arm-none-eabi-objcopy 2>/dev/null)
if [ -z "$ANM" ]; then
  for d in "/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi"/*/bin; do
    [ -x "$d/arm-none-eabi-nm.exe" ] && { ANM="$d/arm-none-eabi-nm.exe"; AOC="$d/arm-none-eabi-objcopy.exe"; break; }
  done
fi
if [ -n "$ANM" ]; then
  L=$("$ANM" "$M33_ELF" | grep -w dsp_literal_image_start | awk '{print $1}')
  D=$("$ANM" "$M33_ELF" | grep -w dsp_data_image_start | awk '{print $1}')
  E=$("$ANM" "$M33_ELF" | grep -w dsp_data_image_end | awk '{print $1}')
  if [ -n "$L" ] && [ -n "$D" ] && [ -n "$E" ]; then
    off=$((0x$D - 0x$L)); cnt=$((0x$E - 0x$D))
    "$AOC" -O binary "$M33_ELF" "$LOG/_dspcode.bin" --only-section=.dspcode 2>/dev/null
    dd if="$LOG/_dspcode.bin" of="$LOG/_emb_data.bin" bs=1 skip=$off count=$cnt 2>/dev/null
    a=$(md5sum "$LOG/_emb_data.bin" | awk '{print $1}'); b=$(md5sum "$BIN/dsp_data_debug.bin" | awk '{print $1}')
    echo "      内嵌 data md5 : $a"
    echo "      自编译 data md5: $b"
    [ "$a" = "$b" ] && echo "      ✅ 字节一致 — step2 镜像进了 M33 固件" || echo "      ⚠️ md5 不一致"
  fi
fi

echo ""
echo "============================================================"
echo " step2 独立项目构建完成。烧录后串口应打印 '[step2] ... skeleton'"
echo " + [NPU] 形状 + Invoke OK/us + 往返误差。还原: bash build_step2.sh --restore"
echo "============================================================"
