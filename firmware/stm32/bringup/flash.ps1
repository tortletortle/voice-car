# Flash bringup.hex via ST-Link + OpenOCD
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$hex = Join-Path $here "bringup.hex"
$ocd = Join-Path $env:LOCALAPPDATA "embedded-tools\openocd\xpack-openocd-0.12.0-7\bin\openocd.exe"
$scripts = Join-Path $env:LOCALAPPDATA "embedded-tools\openocd\xpack-openocd-0.12.0-7\openocd\scripts"

if (-not (Test-Path $hex)) { throw "missing $hex" }
if (-not (Test-Path $ocd)) { throw "missing openocd: $ocd" }

$hexTcl = ($hex -replace '\\', '/')
& $ocd -s $scripts -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program `"$hexTcl`" verify reset exit"
if ($LASTEXITCODE -ne 0) { throw "flash failed: $LASTEXITCODE" }
Write-Host "OK flashed $hex"
