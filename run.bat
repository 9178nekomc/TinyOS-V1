@echo off
setlocal
set QEMU=D:\qemu\qemu-system-x86_64.exe
set OVMF=D:\qemu\share\edk2-x86_64-code.fd
set ESPDIR=D:\Users\Coffee\Desktop\OS\esp_temp
set HD0=D:\Users\Coffee\Desktop\OS\tools\hd0.img
set HD1=D:\Users\Coffee\Desktop\OS\tools\hd1.img

if not exist "%HD0%" (
    echo Creating empty hd0.img...
    powershell -Command "[IO.File]::WriteAllBytes('%HD0%',(New-Object byte[] 67108864))"
)
if not exist "%HD1%" (
    echo Creating empty hd1.img...
    powershell -Command "[IO.File]::WriteAllBytes('%HD1%',(New-Object byte[] 67108864))"
)

echo TinyOS V1 - Booting...
"%QEMU%" -M pc -m 256 -drive "if=pflash,format=raw,readonly=on,file=%OVMF%" -drive "file=fat:rw:%ESPDIR%,index=0,media=disk,format=raw" -drive "file=%HD0%,index=1,media=disk,format=raw" -drive "file=%HD1%,index=2,media=disk,format=raw" -serial stdio -display gtk
echo QEMU exited.
