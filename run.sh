#!/bin/bash

# set doc: https://sc.megabank.tohoku.ac.jp/ph3-doc/howto/linux/script.html
set -xue

# QEMU path
QEMU=/opt/homebrew/bin/qemu-system-riscv32

# clang path
CC=/opt/homebrew/opt/llvm/bin/clang

CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -nostdlib -ffreestanding"

# build kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
	kernel.c common.c

# start QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
	-kernel kernel.elf
