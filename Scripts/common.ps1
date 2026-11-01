$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Root 'Doubleit.uproject'
$Engine = if ($env:UE_ROOT) { $env:UE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }
$BuildBat = Join-Path $Engine 'Engine\Build\BatchFiles\Build.bat'
$RunUAT = Join-Path $Engine 'Engine\Build\BatchFiles\RunUAT.bat'
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
