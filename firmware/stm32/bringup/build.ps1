# Build bringup.hex (STM32F103 · Cortex-M3 · HSI bringup)
# Requires: arm-none-eabi-gcc on PATH, or xpack under %LOCALAPPDATA%\embedded-tools\arm-gcc\

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $here

$xpack = Join-Path $env:LOCALAPPDATA "embedded-tools\arm-gcc\xpack-arm-none-eabi-gcc-14.2.1-1.1\bin"
if (Test-Path (Join-Path $xpack "arm-none-eabi-gcc.exe")) {
  $env:Path = "$xpack;$env:Path"
}

$cc = @(
  "-mcpu=cortex-m3", "-mthumb", "-Os",
  "-ffunction-sections", "-fdata-sections",
  "-ffreestanding", "-fno-builtin", "-nostdlib",
  "-Wall", "-Wextra", "-std=c11"
)

Write-Host "Compiling main.c / startup.s ..."
arm-none-eabi-gcc @cc -c main.c -o main.o
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -c startup.s -o startup.o

Write-Host "Linking bringup.elf ..."
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -T ".\linker.ld" -nostdlib -ffreestanding `
  "-Wl,--gc-sections" "-Wl,-Map=bringup.map" -o bringup.elf startup.o main.o

arm-none-eabi-objcopy -O ihex bringup.elf bringup.hex
arm-none-eabi-objcopy -O binary bringup.elf bringup.bin
arm-none-eabi-size bringup.elf
Write-Host "OK -> $here\bringup.hex"
