@echo off
REM TinyOS V1 — Windows Build Script
REM Uses clang + lld-link to produce BOOTX64.EFI
REM No GNU-EFI dependency required.

setlocal enabledelayedexpansion

REM Ensure LLVM is in PATH (also supports manual overrides)
set "PATH=D:\LLVM\bin;%PATH%"

set "CXX=clang++"
set "LD=lld-link"

set "SRCDIR=src"
set "BUILDDIR=build"
set "ESPDIR=esp_temp"
set "EFIOUT=%ESPDIR%\EFI\BOOT\BOOTX64.EFI"

set "CXXFLAGS=-target x86_64-unknown-windows-msvc -ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti -fno-stack-protector -fshort-wchar -mno-red-zone -mno-sse -m64 -O2 -Wall -Wextra -std=c++17 -I%SRCDIR%"
set "LDFLAGS=/SUBSYSTEM:EFI_APPLICATION /ENTRY:efi_main /NODEFAULTLIB /DLL /MACHINE:X64 /OPT:REF /OPT:ICF"

echo ========================================
echo   TinyOS V1 Build
echo ========================================
echo.

REM --- Check toolchain ---
where %CXX% >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] clang++ not found in PATH.
    echo Please install LLVM/clang: https://github.com/llvm/llvm-project/releases
    exit /b 1
)

where %LD% >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] lld-link not found in PATH.
    echo lld-link is included with LLVM. Ensure it's in your PATH.
    exit /b 1
)

REM --- Create directories ---
if not exist "%BUILDDIR%" mkdir "%BUILDDIR%"
if not exist "%ESPDIR%\EFI\BOOT" mkdir "%ESPDIR%\EFI\BOOT"

REM --- Compile ---
echo [1/2] Compiling sources...
set "OBJFILES="

for %%f in (%SRCDIR%\*.cpp) do (
    set "OBJ=%BUILDDIR%\%%~nf.obj"
    set "OBJFILES=!OBJFILES! !OBJ!"
    echo   %CXX% %CXXFLAGS% -c %%f -o !OBJ!
    %CXX% %CXXFLAGS% -c "%%f" -o "!OBJ!"
    if !ERRORLEVEL! NEQ 0 (
        echo [ERROR] Compilation failed for %%f
        exit /b 1
    )
)

REM --- Link ---
echo.
echo [2/2] Linking BOOTX64.EFI...
echo   %LD% %LDFLAGS% %OBJFILES% /OUT:%EFIOUT%
%LD% %LDFLAGS% %OBJFILES% /OUT:"%EFIOUT%"

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Linking failed.
    exit /b 1
)

echo.
echo ========================================
echo   Build successful!
echo   Output: %EFIOUT%
echo ========================================

REM --- Generate disk image ---
echo.
echo Generating disk image...
python tools\create_disk.py tools\disk0.img
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] Disk image generation failed.
    echo You can run it manually: python tools\create_disk.py tools\disk0.img
)

echo.
echo Ready to run: launch QEMU with run.bat
