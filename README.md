# OS-dev

x86_64 操作系统，可在 **QEMU**、**VMware Workstation**、**Hyper-V** 上运行。使用 Legacy BIOS + Multiboot2 引导，包含内存管理、IDT、多任务与系统调用、简易文件系统、IDE 磁盘与 e1000 网络驱动，以及用户态 Shell。

## 工具链

- **x86_64-elf-gcc**（交叉编译器）
- **nasm**
- **grub-mkrescue**（生成 ISO）
- **objcopy**（GNU binutils）

在 Linux 上可安装：

```bash
# Debian/Ubuntu
sudo apt install gcc-multilib nasm grub-pc-bin

# 如需 x86_64-elf 交叉工具链
# 从 https://wiki.osdev.org/GCC_Cross-Compiler 自编译，或使用包管理器提供的 cross toolchain
```

**在 Windows 11 上（MSYS2）**：可使用 MSYS2 完成编译与运行，无需 WSL。详见 **[docs/BUILD-Windows-MSYS2.md](docs/BUILD-Windows-MSYS2.md)**。简要步骤：

1. 安装 [MSYS2](https://www.msys2.org)，在 MSYS2 终端执行：`pacman -S nasm make xorriso`
2. 安装 x86_64-elf 交叉工具链：下载 [Windows 预编译工具链](https://github.com/trcrsired/windows-hosted-x86_64-elf-toolchains2/releases) 并解压，将其 `bin` 目录加入 PATH
3. 安装 GRUB for Windows：下载 [GNU GRUB for Windows](https://ftp.gnu.org/gnu/grub/)，解压后将含 `grub-mkrescue.exe` 的 `bin` 加入 PATH
4. 在 MSYS2 中进入项目目录，执行 `make`
5. 在 Windows 上安装 [QEMU](https://www.qemu.org/download/#windows)，用 `qemu-system-x86_64 -cdrom build\os.iso ...` 启动

**在 WSL2 Ubuntu 上**：可在 WSL2 内完成编译与运行。详见 **[docs/BUILD-WSL2-Ubuntu.md](docs/BUILD-WSL2-Ubuntu.md)**。简要步骤：

1. 在 Ubuntu 中执行：`sudo apt install build-essential nasm grub-pc-bin xorriso qemu-system-x86`
2. 安装 x86_64-elf 工具链（自编译或使用预编译包），并将其 `bin` 加入 PATH
3. 进入项目目录（如 `cd /mnt/d/Workspace/OS-dev`），执行 `make`
4. 执行 `qemu-system-x86_64 -cdrom build/os.iso -serial stdio ...` 或 `bash scripts/run-qemu.sh`

也可使用其它 Linux 发行版或虚拟机进行编译。

## 构建

```bash
# 使用 Makefile（需 make）
make
# 生成 build/os.iso

# 或使用脚本（无需 make）
./scripts/build.sh
```

生成文件：

- `build/os.iso`：可启动 ISO（用于 CD 或挂载为光驱）
- `build/kernel.bin`：内核二进制
- `build/shell.bin`、`build/hello.bin`：用户程序（已嵌入内核）

## 运行

### QEMU

```bash
# 从 ISO 启动（推荐）
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M -cdrom build/os.iso -no-reboot -no-shutdown -serial stdio

# 启用 e1000 网卡（用户态网络测试）
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M -cdrom build/os.iso \
  -netdev user,id=n0 -device e1000,netdev=n0 -serial stdio
```

或使用脚本（Linux/MSYS2）：

```bash
./scripts/run-qemu.sh
```

**Windows（PowerShell）**：若已安装 QEMU 并加入 PATH，可在项目根目录执行：

```powershell
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M -cdrom build\os.iso -no-reboot -no-shutdown -serial stdio -netdev user,id=n0 -device e1000,netdev=n0
```

### VMware Workstation

1. 新建虚拟机 → 典型 → 稍后安装操作系统 → 其他。
2. 固件选择 **BIOS**（不要选 UEFI）。
3. 磁盘：使用 ISO 或 SCSI/IDE，若用 ISO 则选择「使用 ISO 镜像」并指向 `build/os.iso`。
4. 网络：选择「NAT」或「桥接」，适配器类型选 **e1000**（若列表中有）。
5. 启动虚拟机，从 CD/ISO 启动即可。

同一 `build/os.iso` 可直接用于多台 VMware 虚拟机。

### Hyper-V

1. 新建虚拟机 → 第一代（Gen 1，**Legacy BIOS**）。
2. 内存建议 ≥ 128MB。
3. 网络：先选「未连接」或默认交换机；若需网络，可后续添加「旧版网络适配器」或支持 e1000 的适配器（视 Hyper-V 版本而定）。
4. 虚拟硬盘：可先建一个小的，或选择「从 CD/DVD 启动」。
5. 安装选项：选择「从可启动的 CD/DVD-ROM 映像文件」并指定 `build/os.iso`。
6. 完成向导后启动 VM，从 CD 启动。

**注意**：Gen 2 虚拟机仅支持 UEFI，本内核使用 Legacy BIOS + Multiboot2，请使用 **Gen 1**。

## 验证

启动后应看到：

- 第一行：`OS-dev: x86_64 kernel (QEMU/VMware/Hyper-V)`
- 随后：`Shell ready. Type 'hello' or other.`
- 若挂载了根 FS：`FS mounted.`

在 Shell 中：

- 输入 `hello` 并回车，会输出 `Hello from shell!`
- 任意键盘输入会回显；其他命令会显示 `unknown command`

同一 `build/os.iso` 在 QEMU、VMware、Hyper-V（Gen 1）上应都能进入上述 Shell 界面。

## 文档索引

| 环境 | 文档 |
|------|------|
| Windows 11 + MSYS2 | [docs/BUILD-Windows-MSYS2.md](docs/BUILD-Windows-MSYS2.md) |
| WSL2 + Ubuntu 24.04 | [docs/BUILD-WSL2-Ubuntu.md](docs/BUILD-WSL2-Ubuntu.md) |

## 项目结构

```
OS-dev/
├── boot/          引导（Multiboot2、实模式→长模式）
├── kernel/        内核（内存、IDT、驱动、FS、任务、系统调用）
├── user/          用户程序（Shell、hello、syscall 封装）
├── lib/           基础库（string 等）
├── scripts/       构建与运行脚本
├── Makefile
├── grub.cfg
└── README.md
```

## 许可

可自由用于学习与修改。
