#!/bin/bash

SCRIPT_DIR=$(dirname "$0")
cd "$SCRIPT_DIR"

# 编译 C 文件为目标文件 (.o)
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
                -c \
                -g \
                hello.c \
                -o \
                hello.o

# 链接目标文件为 ELF 文件 (.elf) 并生成 map 文件
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
                -T hello.ld \
                -nostdlib -ffreestanding \
                hello.o \
                -o hello.elf \
                -Wl,-Map=hello.map

# 将 ELF 文件转换为二进制文件 (.bin)
arm-none-eabi-objcopy -O binary hello.elf hello.bin

# 生成反汇编文件
arm-none-eabi-objdump -d -l -C hello.elf > hello.dis

echo "Compilation and conversion complete."

mkdir -p $SCRIPT_DIR/../build
cp hello.bin $SCRIPT_DIR/../build/hello.bin