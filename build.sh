#!/bin/sh
set -eu
: "${ELKS:=${TOPDIR:-}}"
: "${ELKS:?Set ELKS to a configured ELKS source/toolchain}"
cd "$(dirname "$0")"
CROSS=${CROSS:-${CROSSDIR:-$ELKS/cross}}
BUILD=${BUILD:-build}
case "$BUILD" in /*) ;; *) BUILD="$(pwd)/$BUILD";; esac
mkdir -p "$BUILD/host-tools" "$BUILD/obj"
cp toolchain/retain-elf2elks.py "$BUILD/host-tools/elf2elks"
chmod +x "$BUILD/host-tools/elf2elks"
ELX_REAL_ELF2ELKS=$ELKS/elks/tools/bin/elf2elks
PATH=$BUILD/host-tools:$CROSS/bin:$ELKS/elks/tools/bin:$PATH
export PATH ELX_REAL_ELF2ELKS
CC=${CC:-$CROSS/bin/ia16-elf-gcc}
if [ ! -x "$CC" ]; then CC=$(command -v ia16-elf-gcc); fi
SIZE=${SIZE:-$(command -v ia16-elf-size)}
OBJDUMP=${OBJDUMP:-$(command -v ia16-elf-objdump)}
flags="-Os -melks-libc -march=i8086 -mtune=i8086 -mcmodel=small -mno-segment-relocation-stuff -fno-builtin-printf -fno-builtin-fprintf -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fstack-usage"

for name in main capture; do
    "$CC" $flags -Isrc -I"$ELKS/include" -I"$ELKS/libc/include" -I"$ELKS/elks/include" -c "src/$name.c" -o "$BUILD/obj/$name.o"
done
"$CC" $flags -Wl,--gc-sections,-Map,"$BUILD/scrcap.map" -maout-heap=2048 -maout-stack=2048 \
    "$BUILD/obj/main.o" "$BUILD/obj/capture.o" -o "$BUILD/scrcap"
"$SIZE" "$BUILD/scrcap.elf"
"$OBJDUMP" -d -z --insn-width=16 -M intel,i8086 "$BUILD/scrcap.elf" > "$BUILD/scrcap.dis"
python3 toolchain/isa-audit.py "$BUILD/scrcap" "$BUILD/scrcap.elf" "$BUILD/scrcap.dis" --map "$BUILD/scrcap.map" --output "$BUILD/scrcap-audit.json"
"$CC" --version > "$BUILD/compiler-version.txt"
