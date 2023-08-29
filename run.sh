#!/bin/bash


# set doc: https://sc.megabank.tohoku.ac.jp/ph3-doc/howto/linux/script.html
set -xue


# QEMU path
QEMU=/opt/homebrew/bin/qemu-system-riscv32


# clang path
CC=/opt/homebrew/opt/llvm/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -nostdlib -ffreestanding"

OBJCOPY=/opt/homebrew/opt/llvm/bin/llvm-objcopy



# build shell
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf \
	shell.c user.c common.c

# ビルドした実行ファイル(elf形式) を生バイナリ形式(raw binary) に変換する
# 生バイナリとは、ページアドレス(0x1000000) から実際にメモリ上に展開される内容が入ったもの
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin

# 生バイナリ形式のイメージをC言語に埋め込める形式に変換する
$OBJCOPY -Ibinary -Oelf32-littleriscv shell.bin shell.bin.o


# build kernel
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
	kernel.c common.c shell.bin.o


# start QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
	-kernel kernel.elf
