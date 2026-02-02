# OS-dev: x86_64 kernel for QEMU/VMware/Hyper-V
# Requires: x86_64-elf-gcc, nasm, grub-mkrescue

CC   = x86_64-elf-gcc
LD   = x86_64-elf-ld
ASM  = nasm
GRUB = grub-mkrescue

CFLAGS  = -ffreestanding -fno-pie -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -m64 -Wall -Wextra -O2 -I.
LDFLAGS = -nostdlib -static -no-pie -z max-page-size=0x1000 -T boot/linker.ld
ASMFLAGS = -f elf64

KERNEL_BIN = build/kernel.bin
ISO        = build/os.iso

BOOT_OBJ   = build/boot.o
ISR_OBJ    = build/isr.o
SWITCH_OBJ = build/switch.o
SYSCALL_ASM = build/syscall_entry.o
KERNEL_OBJS = build/kernel.o build/pmm.o build/vmm.o build/idt.o \
	build/vga.o build/serial.o build/pic.o build/pit.o build/keyboard.o build/string.o \
	build/pcb.o build/sched.o build/syscall.o build/shell_embed.o build/ide.o build/fs.o build/pci.o build/e1000.o

OBJS = $(BOOT_OBJ) $(ISR_OBJ) $(SWITCH_OBJ) $(SYSCALL_ASM) $(KERNEL_OBJS)

all: $(ISO)

$(ISO): $(KERNEL_BIN) grub.cfg
	@mkdir -p build/isodir/boot/grub
	cp $(KERNEL_BIN) build/isodir/boot/
	cp grub.cfg build/isodir/boot/grub/
	$(GRUB) -o $(ISO) build/isodir
	@echo "Built $(ISO)"

build/shell.bin: user/shell.c user/syscall.h user/user.ld
	$(CC) $(CFLAGS) -I user -c -o build/shell.o user/shell.c
	$(LD) -nostdlib -static -T user/user.ld -o build/shell.elf build/shell.o
	objcopy -O binary build/shell.elf build/shell.bin

build/shell_embed.o: build/shell.bin
	@cd build && objcopy -I binary -O elf64-x86-64 -B i386:x86-64 --rename-section .data=.rodata.shell shell.bin shell_embed.o

build/hello.bin: user/hello.c user/syscall.h user/user.ld
	$(CC) $(CFLAGS) -I user -c -o build/hello.o user/hello.c
	$(LD) -nostdlib -static -T user/user.ld -o build/hello.elf build/hello.o
	objcopy -O binary build/hello.elf build/hello.bin

build/ide.o: kernel/drivers/ide.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/pci.o: kernel/drivers/pci.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/e1000.o: kernel/drivers/e1000.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/fs.o: kernel/fs/fs.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

build/boot.o: boot/boot.asm
	@mkdir -p build
	$(ASM) $(ASMFLAGS) -o $@ $<

build/isr.o: kernel/isr.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

build/switch.o: kernel/task/switch.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

build/syscall_entry.o: kernel/syscall_entry.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

build/pcb.o: kernel/task/pcb.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/sched.o: kernel/task/sched.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/syscall.o: kernel/syscall.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/pmm.o: kernel/mem/pmm.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/vmm.o: kernel/mem/vmm.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/idt.o: kernel/idt.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/vga.o: kernel/drivers/vga.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/serial.o: kernel/drivers/serial.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/pic.o: kernel/drivers/pic.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/pit.o: kernel/drivers/pit.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/keyboard.o: kernel/drivers/keyboard.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/string.o: lib/string.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build

.PHONY: all clean
