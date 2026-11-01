# Full-run check: a bot plays one Classic game through real screen taps, then restarts.
# Prints the metrics line, builds Saved/Playthrough/sheet.png from the captured frames, exits 0 on PASS.
param([int]$Seed = 12345, [int]$Width = 540, [int]$Height = 960, [string]$Extra = '-DITheme=dark')
. "$PSScriptRoot\common.ps1"
$dir = Join-Path $Root 'Saved\Playthrough'
if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
New-Item -ItemType Directory -Force $dir | Out-Null
$log = Join-Path $Root 'Saved\Logs\playthrough.log'
$editor = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$p = Start-Process $editor -ArgumentList "`"$Project`" -game -windowed -ResX=$Width -ResY=$Height -nosplash -unattended -nosound $Extra -DIPlaythrough=$Seed `"-DIShotDir=$dir`" `"-abslog=$log`"" -PassThru
if (-not $p.WaitForExit(900000)) { $p.Kill(); Write-Host 'TIMEOUT'; exit 1 }

Select-String -Path $log -Pattern 'Playthrough: (RESULT|tap|bot)|Error:' | Select-Object -First 15 | ForEach-Object { ($_.Line -split 'LogDoubleIt: ')[-1] }

$shots = Get-ChildItem $dir -Filter 'pt_*.png' | Sort-Object Name
if ($shots) {
    Add-Type -AssemblyName System.Drawing
    $cols = 4; $tw = 270; $th = 480; $rows = [math]::Ceiling($shots.Count / $cols)
    $bmp = New-Object System.Drawing.Bitmap ($cols * $tw), ($rows * $th)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $font = New-Object System.Drawing.Font 'Arial', 11
    for ($i = 0; $i -lt $shots.Count; $i++) {
        $im = [System.Drawing.Image]::FromFile($shots[$i].FullName)
        $x = ($i % $cols) * $tw; $y = [math]::Floor($i / $cols) * $th
        $g.DrawImage($im, $x, $y, $tw, $th)
        $g.DrawString(($shots[$i].BaseName -replace '^pt_\d+_', ''), $font, [System.Drawing.Brushes]::Yellow, $x + 4, $y + 4)
        $im.Dispose()
    }
    $bmp.Save((Join-Path $dir 'sheet.png'))
    Write-Host "Sheet: $(Join-Path $dir 'sheet.png') ($($shots.Count) frames)"
}
if (Select-String -Path $log -Pattern 'RESULT=PASS' -Quiet) { exit 0 } else { exit 1 }
