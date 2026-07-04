# TinyOS V1 — Build System
#
# Toolchain: clang + lld-link (produces native UEFI PE/COFF)
# No GNU-EFI dependency required — all UEFI types are self-defined.
#
# Targets:
#   make          Build BOOTX64.EFI and copy to ESP
#   make disk     Generate disk0.img
#   make clean    Remove build artifacts
#   make run      Build, generate disk, and launch QEMU
#
# Prerequisites:
#   - clang++ (LLVM/clang)
#   - lld-link (LLVM linker)
#   - python (for disk image generation)
#   - qemu-system-x86_64 (for emulation)
#   - OVMF.fd (UEFI firmware for QEMU)

# ============================================================================
# Toolchain Configuration
# ============================================================================

CXX       := clang++
LD        := lld-link
PYTHON    := python

# UEFI PE/COFF target
TARGET    := x86_64-unknown-windows-msvc

# ============================================================================
# Directories
# ============================================================================

SRCDIR    := src
BUILDDIR  := build
ESPDIR    := esp_temp
TOOLDIR   := tools

# ============================================================================
# Compiler Flags
# ============================================================================

CXXFLAGS  := -target $(TARGET) \
             -ffreestanding \
             -nostdlib \
             -nostdinc \
             -fno-exceptions \
             -fno-rtti \
             -fno-stack-protector \
             -fshort-wchar \
             -mno-red-zone \
             -mno-sse \
             -m64 \
             -O2 \
             -Wall -Wextra \
             -std=c++17 \
             -I$(SRCDIR)

# ============================================================================
# Linker Flags
# ============================================================================

LDFLAGS   := /SUBSYSTEM:EFI_APPLICATION \
             /ENTRY:efi_main \
             /NODEFAULTLIB \
             /DLL \
             /MACHINE:X64 \
             /OPT:REF \
             /OPT:ICF

# ============================================================================
# QEMU Configuration
# ============================================================================

QEMU      := qemu-system-x86_64
OVMF      := D:/qemu/share/edk2-x86_64-code.fd
QEMUFLAGS := -M pc \
             -m 256 \
             -drive if=pflash,format=raw,readonly=on,file="$(OVMF)" \
             -drive file=fat:rw:$(ESPDIR),index=0,media=disk,format=raw \
             -drive file=$(TOOLDIR)/disk0.img,index=1,media=disk,format=raw \
             -serial stdio \
             -display gtk

# ============================================================================
# Source Files
# ============================================================================

SOURCES   := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS   := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.obj,$(SOURCES))

EFI_OUT   := $(ESPDIR)/EFI/BOOT/BOOTX64.EFI
DISK_IMG  := $(TOOLDIR)/disk0.img

# ============================================================================
# Targets
# ============================================================================

.PHONY: all clean disk esp run help

all: esp $(EFI_OUT)
	@echo "Build complete: $(EFI_OUT)"

# --- ESP directory setup ---
esp:
	@if not exist "$(ESPDIR)\EFI\BOOT" mkdir "$(ESPDIR)\EFI\BOOT"

# --- Compile .cpp to .obj ---
$(BUILDDIR)/%.obj: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Link .obj to .EFI ---
$(EFI_OUT): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) /OUT:$@

# --- Build directory ---
$(BUILDDIR):
	@if not exist "$(BUILDDIR)" mkdir "$(BUILDDIR)"

# --- Disk Image ---
disk: $(DISK_IMG)

$(DISK_IMG): $(TOOLDIR)/create_disk.py
	$(PYTHON) $(TOOLDIR)/create_disk.py $@

# --- Clean ---
clean:
	@if exist "$(BUILDDIR)" rmdir /S /Q "$(BUILDDIR)"
	@if exist "$(DISK_IMG)" del "$(DISK_IMG)"
	@echo "Cleaned."

# --- Run ---
run: all disk
	$(QEMU) $(QEMUFLAGS)

# --- Help ---
help:
	@echo "TinyOS V1 Build System"
	@echo "======================"
	@echo "  make          Build BOOTX64.EFI"
	@echo "  make disk     Generate disk0.img"
	@echo "  make clean    Remove build artifacts"
	@echo "  make run      Build, generate disk, and launch QEMU"
	@echo ""
	@echo "Prerequisites:"
	@echo "  clang++      LLVM C++ compiler"
	@echo "  lld-link     LLVM linker"
	@echo "  python       For disk image generation"
	@echo "  qemu-system-x86_64  For emulation"
	@echo "  OVMF.fd      UEFI firmware (place in project root)"
