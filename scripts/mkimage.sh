#!/bin/bash
# Create disk image with GRUB + kernel + optional root FS (fs.img from mkfs)
set -e
cd "$(dirname "$0")/.."
mkdir -p build

if [ ! -f build/kernel.bin ]; then
    echo "Run build.sh first."
    exit 1
fi

# Create ISO (same as Makefile)
mkdir -p build/isodir/boot/grub
cp build/kernel.bin build/isodir/boot/
cp grub.cfg build/isodir/boot/grub/
grub-mkrescue -o build/os.iso build/isodir
echo "Built build/os.iso"

# Optional: create root FS image (for second disk)
if [ -f build/shell.bin ] && [ -f build/hello.bin ]; then
    if command -v gcc >/dev/null 2>&1; then
        gcc -o build/mkfs scripts/mkfs.c
        build/mkfs build/fs.img build/shell.bin build/hello.bin
        echo "Built build/fs.img (attach as second IDE disk to use FS)"
    fi
fi
