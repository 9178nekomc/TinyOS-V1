<p align="center">
  <img src="icon.jpg" alt="TinyOS Logo" width="128" height="128">
</p>

<h1 align="center">TinyOS V1</h1>

<p align="center">
  <strong>一个从零构建的裸机 x86_64 单内核操作系统 · 纯 UEFI 引导</strong>
  <br>
  <em>A bare-metal x86_64 monolithic kernel built from scratch · Pure UEFI boot</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/arch-x86__64-blue" alt="Arch">
  <img src="https://img.shields.io/badge/platform-UEFI-green" alt="Platform">
  <img src="https://img.shields.io/badge/language-C++17%20%2B%20ASM-orange" alt="Language">
  <img src="https://img.shields.io/badge/license-GPL%20v3-red" alt="License">
</p>

---

## 简介 | Introduction

**TinyOS V1** 是一个完全独立于 Linux 的裸机操作系统内核，使用 C++17 和汇编编写，通过现代 UEFI 固件引导。项目实现了完整的图形启动界面、系统安装向导、持久文件存储以及交互式命令行 Shell。

**TinyOS V1** is a bare-metal OS kernel completely independent of Linux, written in C++17 and assembly, booted via modern UEFI firmware. It features a graphical boot screen, system installation wizard, persistent file storage, and an interactive command-line shell.

### 特性 | Features

- 🖥️ **图形启动** — GOP 帧缓冲渲染，1024×768 星环壁纸
- 💾 **真持久存储** — PFS 扇区级文件系统，重启数据不丢失
- 📦 **系统安装向导** — 选盘、格式化、安装一气呵成
- 🐚 **交互式 Shell** — 15 个内置命令，支持 ls/cd/mkdir/pfls/mkfile/pfwrite/pfcat/pfrm 等
- 🔌 **串口调试** — COM1 115200 8N1，启动第一时间可用
- 📀 **ATA PIO 驱动** — 双总线支持（Primary + Secondary），28-bit LBA 读写
- ⚡ **纯 UEFI** — 无 GNU-EFI 依赖，自包含类型定义

---

## 快速开始 | Quick Start

### 依赖 | Prerequisites

| 工具 | 用途 | 安装 |
|------|------|------|
| LLVM/clang 21+ | 编译 | [github.com/llvm/llvm-project](https://github.com/llvm/llvm-project/releases) |
| QEMU 11+ | 模拟 | [qemu.org](https://www.qemu.org/download/) |
| Python 3.11+ | 磁盘镜像 | [python.org](https://www.python.org/downloads/) |
| OVMF (EDK2) | UEFI 固件 | QEMU 自带或 [github.com/retrage/edk2-nightly](https://github.com/retrage/edk2-nightly) |

### 构建与运行 | Build & Run

```batch
# 1. 构建内核
build.bat

# 2. 启动
run.bat
```

或者 Makefile：

```bash
make          # 构建
make disk     # 生成磁盘镜像
make run      # 构建并启动 QEMU
```

### 磁盘布局 | Disk Layout

```
Primary Master   (index=0) → ESP (FAT, BOOTX64.EFI)
Primary Slave    (index=1) → hd0.img (64 MB raw)
Secondary Master (index=2) → hd1.img (64 MB raw)
```

---

## 架构 | Architecture

```
efi_main()
  ├─ serial_init()          ← 串口第一行初始化（断生死线）
  ├─ gop_init()             ← GOP 图形协议
  ├─ 壁纸展示 (3 秒)         ← LBA 1000 加载 BGR24 像素矩阵
  ├─ 黑屏图形终端             ← 8×16 嵌入字体，128×48 字符
  ├─ sysdisk_check()        ← 检测系统盘标记
  │   ├─ 已安装 → 挂载 PFS → Shell
  │   └─ 未安装 → 安装向导
  └─ shell_run()            ← 交互式 REPL
```

### 源代码结构 | Source Layout

```
src/
├── uefi/types.h      # UEFI 类型定义（自包含，无 GNU-EFI 依赖）
├── main.cpp          # efi_main 入口，系统编排
├── serial.h/cpp      # COM1 串口驱动
├── gop.h/cpp         # GOP 图形协议 + 帧缓冲渲染
├── ata.h/cpp         # ATA PIO 双总线读写驱动
├── gfxterm.h/cpp     # 图形终端引擎
├── font8x16.h/cpp    # 嵌入 8×16 VGA 位图字体
├── alloc.h/cpp       # UEFI 内存分配器封装
├── diskimg.h/cpp     # 磁盘镜像解析 + 壁纸加载
├── fat.h/cpp         # EFI_FILE_PROTOCOL 文件系统 (ESP)
├── pfs.h/cpp         # 持久文件存储 (系统盘, ~498 KB)
├── sysdisk.h/cpp     # 系统盘标记检测与写入
├── install.h/cpp     # 系统安装向导
├── shell.h/cpp       # 命令行 REPL
├── commands.h/cpp    # 15 个内置命令
└── console.h/cpp     # 键盘 + 串口输入（非阻塞）
```

---

## 命令参考 | Command Reference

| 命令 | 说明 | Description |
|------|------|-------------|
| `ls [path]` | 列出 ESP 目录 | List ESP directory |
| `cd <path>` | 切换 ESP 目录 | Change ESP directory |
| `mkdir <name>` | 创建 ESP 目录 | Create ESP directory |
| `pfls` | 列出系统盘文件 | List persistent files |
| `mkfile <name>` | 创建系统盘文件 | Create file on system disk |
| `pfcat <name>` | 读取系统盘文件 | Read file from system disk |
| `pfwrite <n> <t>` | 写入文本到文件 | Write text to file |
| `pfrm <name>` | 删除系统盘文件 | Delete file from system disk |
| `install` | 运行安装向导 | Run install wizard |
| `shutdown` | 关机/重启 | Shutdown / reboot |
| `help` | 显示帮助 | Show help |
| `clear` | 清屏 | Clear screen |
| `info` | 系统信息 | System info |
| `read <lba> [n]` | 读取磁盘扇区 | Read disk sectors |
| `wallpaper` | 重新渲染壁纸 | Re-render wallpaper |

---

## 技术约束 | Technical Constraints

本项目严格遵守以下裸机开发原则：

1. **串口最优先** — COM1 必须在 efi_main 第一行初始化，任何早期崩溃都能被记录
2. **UEFI 合规** — 使用 EFIAPI 调用约定，通过 BS->LocateProtocol 获取 GOP，BS->AllocatePool 分配大缓冲区
3. **栈安全** — 2.25 MB 壁纸缓冲区通过堆分配，目录/输入缓冲区在 .bss 段，绝不在栈上分配大型对象
4. **ATA 从设备寻址** — 驱动器/磁头寄存器写入 0xF0（非 0xE0）以定位正确设备
5. **编译约束** — `-fno-exceptions -fno-rtti -ffreestanding`，PE/COFF 输出格式

---

## 许可证 | License

GNU General Public License v3.0 or later.

```
TinyOS V1 — A bare-metal x86_64 monolithic kernel
Copyright (C) 2026  Coffee

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
```

详见 [LICENSE](LICENSE) 文件。

---

<p align="center">
  <sub>Made with ❤️ for bare-metal by Coffee · 2026</sub>
</p>
