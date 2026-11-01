# Play the game on PC in a phone-shaped window, with sound. Mouse: hover to aim, click to place. Esc pauses.
param([int]$Width = 540, [int]$Height = 1170)
. "$PSScriptRoot\common.ps1"
$editor = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
Start-Process $editor -ArgumentList "`"$Project`" -game -windowed -ResX=$Width -ResY=$Height -nosplash"
