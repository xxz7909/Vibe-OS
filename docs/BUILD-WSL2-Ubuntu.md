# 在 WSL2 Ubuntu 24.04 上编译并运行 OS-dev

本文档说明如何在 **WSL2** 下的 **Ubuntu 24.04 LTS** 中完成本项目的编译，并用 QEMU 在终端中运行，无需额外虚拟机。

---

## 一、前置条件

- 已安装 **WSL2**，并已安装 **Ubuntu 24.04 LTS**。
- 在 Windows 中打开 Ubuntu：开始菜单 → **Ubuntu 24.04**，或 PowerShell 中执行 `wsl -d Ubuntu-24.04`。

---

## 二、安装依赖（一次性）

在 Ubuntu 终端中执行：

```bash
sudo apt update
sudo apt install -y build-essential nasm grub-pc-bin xorriso qemu-system-x86
```

说明：

| 包名 | 用途 |
|------|------|
| build-essential | gcc、make、ld 等基础编译工具 |
| nasm | 汇编器 |
| grub-pc-bin | 提供 **grub-mkrescue**，用于生成可启动 ISO |
| xorriso | 制作 ISO（grub-mkrescue 会调用） |
| qemu-system-x86 | 在 WSL 内用 QEMU 运行生成的 ISO |

若提示找不到 `grub-pc-bin`，可改用：

```bash
sudo apt install -y grub-common xorriso
```

（部分环境里 `grub-mkrescue` 由 `grub-common` 提供。）

---

## 三、安装 x86_64-elf 交叉工具链

Ubuntu 官方仓库没有 `x86_64-elf-gcc`，需要自编译或使用第三方脚本。任选其一即可。

### 方法 A：使用预编译 Linux 工具链（推荐，免编译）

部分项目提供适用于 Linux/WSL 的 x86_64-elf 预编译包，解压后加入 PATH 即可，无需自编译。

1. 打开 [lordmilko/i686-elf-tools Releases](https://github.com/lordmilko/i686-elf-tools/releases) 或 [alessandromrc/i686-elf-tools Releases](https://github.com/alessandromrc/i686-elf-tools/releases)，下载适用于 **Linux x86_64** 的压缩包（如 `x86_64-elf-tools-linux.tar.xz` 或类似名称）。
2. 解压到本地目录并加入 PATH，例如：

```bash
cd ~
tar -xf ~/Downloads/x86_64-elf-tools-linux-*.tar.xz   # 按实际文件名修改
mkdir -p ~/opt
mv x86_64-elf-tools ~/opt/   # 或按压缩包内实际目录名调整
echo 'export PATH="$HOME/opt/x86_64-elf-tools/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

3. 验证：`x86_64-elf-gcc --version`。

若找不到现成的 Linux x86_64-elf 预编译包，请用下面的 **方法 B** 自编译。

### 方法 B：自编译 x86_64-elf-gcc（通用做法）

1. 安装依赖：

```bash
sudo apt install -y build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo
```

2. 按 [OSDev Wiki - GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross_Compiler) 执行，或使用以下精简步骤：

```bash
export PREFIX="$HOME/opt/cross"
export TARGET=x86_64-elf
mkdir -p $PREFIX
export PATH="$PREFIX/bin:$PATH"

# 1. 编译 binutils
cd /tmp
curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz
tar xf binutils-2.42.tar.xz
mkdir build-binutils && cd build-binutils
../binutils-2.42/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..

# 2. 编译 GCC（仅 freestanding：无 libc）
curl -O https://ftp.gnu.org/gnu/gcc/gcc-14.2.0/gcc-14.2.0.tar.xz
tar xf gcc-14.2.0.tar.xz
mkdir build-gcc && cd build-gcc
../gcc-14.2.0/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc install-target-libgcc
cd ..
```

3. 将工具链加入 PATH（建议写进 `~/.bashrc`）：

```bash
echo 'export PATH="$HOME/opt/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

4. 验证：

```bash
x86_64-elf-gcc --version
```

### 方法 C：其它预编译或自建工具链

若你从其它渠道获得适用于 Linux 的 **x86_64-elf** 预编译包，解压后将其中的 `bin` 目录加入 PATH 即可，例如：

```bash
echo 'export PATH="/path/to/x86_64-elf/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

---

## 四、进入项目目录

项目若在 Windows 磁盘上，在 WSL 中路径一般为 `/mnt/<盘符>/<路径>`。例如项目在 `D:\Workspace\OS-dev` 时：

```bash
cd /mnt/d/Workspace/OS-dev
```

若项目在 WSL 自己的家目录下（例如 `~/OS-dev`），则：

```bash
cd ~/OS-dev
```

---

## 五、编译

在项目根目录执行其一即可：

```bash
make
```

或（无需 make）：

```bash
bash scripts/build.sh
```

成功后会生成：

- `build/os.iso` — 可启动 ISO
- `build/kernel.bin` — 内核二进制

若报错 **x86_64-elf-gcc: command not found**，说明 PATH 中还没有交叉工具链，请回到「三」完成安装并 `source ~/.bashrc` 后重试。

---

## 六、在 WSL2 中运行 QEMU

在**同一项目目录**下执行：

```bash
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M -boot d -cdrom build/os.iso -no-reboot -no-shutdown -serial stdio -netdev user,id=n0 -device e1000,netdev=n0
```

- **-serial stdio**：把串口输出打到当前终端，Shell 和键盘输入都在这个窗口里，无需图形界面即可操作。
- 若希望同时看到“虚拟机窗口”（VGA 画面），可去掉 `-serial stdio` 或保留；Ubuntu 24.04 的 WSL2 一般已启用 WSLg，图形窗口会正常弹出。

使用项目自带脚本（效果同上）：

```bash
bash scripts/run-qemu.sh
```

退出 QEMU：在 QEMU 窗口获得焦点时按 **Ctrl+C** 或关闭窗口；若只用串口无窗口，在终端中 **Ctrl+C** 即可。

---

## 七、验证是否成功

启动后应看到类似输出：

- 第一行：`OS-dev: x86_64 kernel (QEMU/VMware/Hyper-V)`
- 随后：`Shell ready. Type 'hello' or other.`

在 Shell 中输入 **hello** 并回车，应出现：`Hello from shell!`。说明在 WSL2 Ubuntu 24.04 上已成功编译并运行本系统。

---

## 八、常见问题

### 1. `grub-mkrescue: command not found`

安装 GRUB 相关包之一：

```bash
sudo apt install -y grub-pc-bin
# 或
sudo apt install -y grub-common
```

然后执行 `which grub-mkrescue` 确认。

### 2. `xorriso` 相关报错

安装 xorriso：

```bash
sudo apt install -y xorriso
```

### 3. 找不到 `build/os.iso`

确保在**项目根目录**（包含 `Makefile`、`boot/`、`kernel/` 的目录）下执行 `make` 或 `bash scripts/build.sh`。

### 4. QEMU 无图形窗口

- 确认 Windows 已安装并更新 **WSL2** 与 **WSLg**（Windows 11 22H2 及以上通常已包含）。
- 若只需使用 Shell，可一直使用 `-serial stdio`，在终端中操作即可。

### 5. 项目在 `/mnt/d/...` 下编译很慢

WSL2 访问 Windows 盘符下的文件会较慢。可把项目复制到 WSL 家目录再编译，例如：

```bash
cp -r /mnt/d/Workspace/OS-dev ~/OS-dev
cd ~/OS-dev
make
```

---

## 九、步骤小结

| 步骤 | 操作 |
|------|------|
| 1 | 安装依赖：`sudo apt install build-essential nasm grub-pc-bin xorriso qemu-system-x86` |
| 2 | 安装 x86_64-elf 工具链（方法 A/B/C 之一），并把 `bin` 加入 PATH |
| 3 | `cd` 到项目根目录（如 `cd /mnt/d/Workspace/OS-dev`） |
| 4 | 执行 `make` 或 `bash scripts/build.sh` |
| 5 | 执行 `qemu-system-x86_64 ... -cdrom build/os.iso -serial stdio` 或 `bash scripts/run-qemu.sh` |

按上述步骤即可在 **WSL2 Ubuntu 24.04 LTS** 上完成编译并运行本系统。
