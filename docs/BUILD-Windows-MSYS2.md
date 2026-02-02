# 在 Windows 11 上用 MSYS2 编译并运行 OS-dev

本文档说明如何在 **Windows 11** 下使用 **MSYS2** 完成本项目的编译，并在 QEMU 中启动，**无需 WSL 或 Linux 虚拟机**。

---

## 一、安装 MSYS2

1. 打开 [https://www.msys2.org](https://www.msys2.org)，下载 **MSYS2 安装包**（如 `msys2-x86_64-xxxxxxxx.exe`）。
2. 运行安装程序，按默认选项安装（例如安装到 `C:\msys64`）。
3. 安装完成后，从开始菜单打开 **「MSYS2 UCRT64」**（或 **「MSYS2 MSYS」**），进入终端。

---

## 二、在 MSYS2 中安装基础工具

在 MSYS2 终端中执行（首次建议先执行 `pacman -Syu` 更新）：

```bash
# 更新包数据库（可选）
pacman -Syu

# 安装编译与打包所需工具
pacman -S --noconfirm nasm make xorriso
```

- **nasm**：汇编器  
- **make**：构建  
- **xorriso**：制作 ISO（grub-mkrescue 会用到）

---

## 三、安装 x86_64-elf 交叉工具链（Windows 用）

MSYS2 官方仓库没有 `x86_64-elf-gcc`，需要单独安装。推荐使用预编译的 Windows 工具链。

### 方法 A：使用预编译工具链（推荐）

1. 打开 [https://github.com/trcrsired/windows-hosted-x86_64-elf-toolchains2/releases](https://github.com/trcrsired/windows-hosted-x86_64-elf-toolchains2/releases)，下载最新 **`x86_64-elf-tools-windows.zip`**（或类似名称的压缩包）。
2. 解压到任意目录，例如：  
   `C:\osdev-toolchain`  
   解压后该目录下应存在 `bin` 文件夹，且其中有 `x86_64-elf-gcc.exe`、`x86_64-elf-ld.exe`、`nasm.exe` 等。
3. 将工具链的 `bin` 目录加入系统 **PATH**：  
   - 例如路径为：`C:\osdev-toolchain\bin`  
   - 在 Windows 中：设置 → 系统 → 关于 → 高级系统设置 → 环境变量 → 用户变量中的 Path → 编辑 → 新建 → 填入上述路径并确定。  
   若只在当前终端使用，可在 MSYS2 里执行：  
   `export PATH="/c/osdev-toolchain/bin:$PATH"`  
   （路径按你实际解压位置调整，MSYS2 中 C 盘为 `/c/`。）

### 方法 B：在 MSYS2 里自编译工具链

若希望完全在 MSYS2 内完成，可参考 [OSDev Wiki - GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross_Compiler) 自行编译 x86_64-elf-gcc 与 binutils，再将安装目录的 `bin` 加入 PATH。步骤较多，一般用方法 A 即可。

---

## 四、安装 GRUB（用于生成可启动 ISO）

制作可启动 ISO 需要 **grub-mkrescue**。可选以下两种方式之一。

### 方式 1：GNU 官方 Windows 版 GRUB（推荐）

1. 打开 [https://ftp.gnu.org/gnu/grub/](https://ftp.gnu.org/gnu/grub/)，下载 **`grub-2.06-for-windows.zip`**（或更高版本的 `grub-*-for-windows.zip`）。
2. 解压到例如 `C:\grub2`。  
   解压后应存在 `grub-2.06-for-windows\bin` 或类似路径，其中有 **`grub-mkrescue.exe`**。
3. 将该 **bin** 目录加入 PATH（同工具链，例如 `C:\grub2\grub-2.06-for-windows\bin`）。  
   在 MSYS2 中可临时执行：  
   `export PATH="/c/grub2/grub-2.06-for-windows/bin:$PATH"`  
   （请按实际路径修改。）

### 方式 2：在 MSYS2 中查找 grub 包

部分 MSYS2 镜像可能提供 grub 相关包，可尝试：

```bash
pacman -Ss grub
```

若有 `grub` 或 `grub2` 等包，可安装后使用其中的 `grub-mkrescue`，并将其所在目录加入 PATH。

---

## 五、安装 QEMU（用于在 Windows 下启动）

在 Windows 上运行生成的 ISO 需要 QEMU for Windows。

- **官网**： [https://www.qemu.org/download/#windows](https://www.qemu.org/download/#windows)  
  下载安装包并安装，安装时勾选「Add to PATH」。
- 或使用 **winget**（管理员 PowerShell）：  
  `winget install QEMU.QEMU`  
  安装后若未自动加入 PATH，需在「环境变量」中手动添加 QEMU 的安装目录（如 `C:\Program Files\qemu`）。

安装完成后，在 **MSYS2** 或 **PowerShell** 中执行 `qemu-system-x86_64 --version` 能正常输出版本即可。

---

## 六、在 MSYS2 中编译项目

1. 在 MSYS2 中进入项目根目录，例如：  
   `cd /d/Workspace/OS-dev`  
   （根据你实际路径修改，D 盘为 `/d/`。）
2. 确认 PATH 中包含：  
   - x86_64-elf 的 `bin`  
   - grub-mkrescue 所在目录  
   若未永久加入 PATH，请先执行对应的 `export PATH=...`。
3. 执行：

```bash
make
```

或使用脚本（需 bash）：

```bash
bash scripts/build.sh
```

4. 若成功，会在 **`build/`** 下生成 **`os.iso`**。

若报错「x86_64-elf-gcc 未找到」或「grub-mkrescue 未找到」，请检查对应工具的 `bin` 是否已加入 PATH，并在当前 MSYS2 窗口里重新执行 `export PATH=...` 后再试。

---

## 七、在 Windows 下启动 QEMU 运行 OS

在 **PowerShell** 或 **CMD** 中（无需 MSYS2）执行：

```powershell
cd D:\Workspace\OS-dev
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M -cdrom build\os.iso -no-reboot -no-shutdown -serial stdio -netdev user,id=n0 -device e1000,netdev=n0
```

路径请按你本机项目位置修改。若 QEMU 已加入 PATH，也可直接写：

```powershell
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M -cdrom build\os.iso -no-reboot -no-shutdown -serial stdio
```

看到内核输出和 Shell 提示符即表示在 Windows 上已成功编译并运行。

---

## 八、一键脚本（可选）

若已安装 MSYS2 且 PATH 中已有 x86_64-elf、grub、make，可在项目根目录使用提供的脚本，从 **MSYS2 UCRT64** 或 **MSYS2 MSYS** 中执行：

```bash
bash scripts/build.sh
```

然后用上面的 `qemu-system-x86_64 ...` 命令运行 `build/os.iso`。

若你希望用 **PowerShell 调用 MSYS2 的 bash 再执行 build**，可在项目根目录创建 `build-msys2.ps1`，内容示例：

```powershell
# 请根据本机 MSYS2 安装路径修改
$msys2 = "C:\msys64\usr\bin\bash.exe"
& $msys2 -lc "cd /d/Workspace/OS-dev && export PATH='/c/osdev-toolchain/bin:/c/grub2/grub-2.06-for-windows/bin:$PATH' && make"
```

把路径改成你的实际路径后，在 PowerShell 中执行 `.\build-msys2.ps1` 即可。

---

## 九、小结

| 步骤       | 内容 |
|------------|------|
| 1. MSYS2   | 安装 MSYS2，在终端中安装 `nasm`、`make`、`xorriso`。 |
| 2. 工具链  | 下载并解压 x86_64-elf Windows 预编译包，将其 `bin` 加入 PATH。 |
| 3. GRUB    | 下载 GRUB for Windows，将含 `grub-mkrescue.exe` 的 `bin` 加入 PATH。 |
| 4. QEMU    | 在 Windows 安装 QEMU 并加入 PATH。 |
| 5. 编译    | 在 MSYS2 中 `cd` 到项目根目录，执行 `make` 或 `bash scripts/build.sh`。 |
| 6. 运行    | 在 PowerShell/CMD 中用 `qemu-system-x86_64 ... -cdrom build\os.iso` 启动。 |

按上述步骤即可在 **Windows 11 + MSYS2** 下完成编译并在 QEMU 中启动本系统，无需 WSL 或 Linux 虚拟机。
