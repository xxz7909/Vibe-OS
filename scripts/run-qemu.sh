#!/bin/sh
# Run OS-dev in QEMU (Legacy BIOS, CD boot, e1000)
cd "$(dirname "$0")/.."
[ -f build/os.iso ] || { echo "Run 'make' or ./scripts/build.sh first."; exit 1; }
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M -boot d -cdrom build/os.iso -no-reboot -no-shutdown -serial stdio -netdev user,id=n0 -device e1000,netdev=n0
