#!/bin/bash
# Build kernel and ISO. Requires: x86_64-elf-gcc, nasm, grub-mkrescue, objcopy
set -e
cd "$(dirname "$0")/.."
mkdir -p build build/isodir/boot/grub

CFLAGS="-ffreestanding -fno-pie -fno-stack-protector -mno-red-zone -m64 -Wall -Wextra -O2 -I."
LDFLAGS="-nostdlib -static -no-pie -z max-page-size=0x1000 -T boot/linker.ld"

echo "[*] Building boot.o..."
nasm -f elf64 -o build/boot.o boot/boot.asm
echo "[*] Building isr.o..."
nasm -f elf64 -o build/isr.o kernel/isr.asm
echo "[*] Building switch.o..."
nasm -f elf64 -o build/switch.o kernel/task/switch.asm
echo "[*] Building syscall_entry.o..."
nasm -f elf64 -o build/syscall_entry.o kernel/syscall_entry.asm

echo "[*] Building shell.bin..."
x86_64-elf-gcc $CFLAGS -I user -c -o build/shell.o user/shell.c
x86_64-elf-ld -nostdlib -static -T user/user.ld -o build/shell.elf build/shell.o
objcopy -O binary build/shell.elf build/shell.bin
objcopy -I binary -O elf64-x86-64 -B i386:x86-64 --rename-section .data=.rodata.shell build/shell.bin build/shell_embed.o

echo "[*] Building hello.bin..."
x86_64-elf-gcc $CFLAGS -I user -c -o build/hello.o user/hello.c
x86_64-elf-ld -nostdlib -static -T user/user.ld -o build/hello.elf build/hello.o
objcopy -O binary build/hello.elf build/hello.bin

echo "[*] Building kernel C objects..."
x86_64-elf-gcc $CFLAGS -c -o build/kernel.o kernel/kernel.c
x86_64-elf-gcc $CFLAGS -c -o build/pmm.o kernel/mem/pmm.c
x86_64-elf-gcc $CFLAGS -c -o build/vmm.o kernel/mem/vmm.c
x86_64-elf-gcc $CFLAGS -c -o build/idt.o kernel/idt.c
x86_64-elf-gcc $CFLAGS -c -o build/vga.o kernel/drivers/vga.c
x86_64-elf-gcc $CFLAGS -c -o build/serial.o kernel/drivers/serial.c
x86_64-elf-gcc $CFLAGS -c -o build/pic.o kernel/drivers/pic.c
x86_64-elf-gcc $CFLAGS -c -o build/pit.o kernel/drivers/pit.c
x86_64-elf-gcc $CFLAGS -c -o build/keyboard.o kernel/drivers/keyboard.c
x86_64-elf-gcc $CFLAGS -c -o build/string.o lib/string.c
x86_64-elf-gcc $CFLAGS -c -o build/pcb.o kernel/task/pcb.c
x86_64-elf-gcc $CFLAGS -c -o build/sched.o kernel/task/sched.c
x86_64-elf-gcc $CFLAGS -c -o build/syscall.o kernel/syscall.c
x86_64-elf-gcc $CFLAGS -c -o build/ide.o kernel/drivers/ide.c
x86_64-elf-gcc $CFLAGS -c -o build/pci.o kernel/drivers/pci.c
x86_64-elf-gcc $CFLAGS -c -o build/e1000.o kernel/drivers/e1000.c
x86_64-elf-gcc $CFLAGS -c -o build/fs.o kernel/fs/fs.c

echo "[*] Linking kernel.bin..."
x86_64-elf-ld $LDFLAGS -o build/kernel.bin build/boot.o build/isr.o build/switch.o build/syscall_entry.o \
  build/kernel.o build/pmm.o build/vmm.o build/idt.o \
  build/vga.o build/serial.o build/pic.o build/pit.o build/keyboard.o build/string.o \
  build/pcb.o build/sched.o build/syscall.o build/shell_embed.o build/ide.o build/fs.o build/pci.o build/e1000.o

echo "[*] Creating ISO..."
cp build/kernel.bin build/isodir/boot/
cp grub.cfg build/isodir/boot/grub/
grub-mkrescue -o build/os.iso build/isodir

echo "[+] Done: build/os.iso"
