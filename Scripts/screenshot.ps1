# Launches the game windowed, auto-plays N moves, saves a PNG and quits. For visual checks without clicking.
# Extra flags: -DITheme=dark|light, -DITutorial, -DIPaused
param([int]$Moves = 12, [string]$Out = "$PSScriptRoot\..\Saved\Screenshots\board.png", [int]$Width = 540, [int]$Height = 960, [string]$Extra = '')
. "$PSScriptRoot\common.ps1"
$Out = [IO.Path]::GetFullPath($Out)
New-Item -ItemType Directory -Force (Split-Path $Out) | Out-Null
if (Test-Path $Out) { Remove-Item $Out }
$editor = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$p = Start-Process $editor -ArgumentList "`"$Project`" -game -windowed -ResX=$Width -ResY=$Height -nosplash -unattended -nosound -DIAutoplay=$Moves $Extra `"-DIShot=$Out`" `"-abslog=$Root\Saved\Logs\shot.log`"" -PassThru
if (-not $p.WaitForExit(900000)) { $p.Kill(); Write-Host "TIMEOUT"; exit 1 }
if (Test-Path $Out) { Write-Host "Screenshot: $Out"; exit 0 }
Select-String -Path "$Root\Saved\Logs\shot.log" -Pattern 'Error|LogDoubleIt' | Select-Object -First 15 | ForEach-Object { $_.Line }
exit 1
