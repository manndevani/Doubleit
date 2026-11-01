# Layout check across screen shapes: tall 20:9, 16:9, small phone, 3:4 tablet, unfolded foldable.
# Saves one screenshot per shape and a side-by-side sheet at Saved/Screenshots/sizes.png. Look at the sheet only.
param([int]$Moves = 12, [string]$Extra = '-DITheme=dark -DITutorialStep=3')
. "$PSScriptRoot\common.ps1"
$out = Join-Path $Root 'Saved\Screenshots'
$sizes = @(@(432, 960, '20x9'), @(540, 960, '16x9'), @(360, 640, 'small'), @(600, 800, 'tablet3x4'), @(720, 810, 'fold'))
foreach ($s in $sizes) {
    & "$PSScriptRoot\screenshot.ps1" -Moves $Moves -Width $s[0] -Height $s[1] -Out (Join-Path $out "size_$($s[2]).png") -Extra $Extra | Out-Null
}
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap 1640, 500
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::White)
$x = 0
foreach ($s in $sizes) {
    $im = [System.Drawing.Image]::FromFile((Join-Path $out "size_$($s[2]).png"))
    $w = [int](480 * $im.Width / $im.Height)
    $g.DrawImage($im, $x, 10, $w, 480)
    $g.DrawString($s[2], (New-Object System.Drawing.Font 'Arial', 11), [System.Drawing.Brushes]::Yellow, $x + 4, 14)
    $x += $w + 8
    $im.Dispose()
}
$bmp.Save((Join-Path $out 'sizes.png'))
Write-Host "Sheet: $(Join-Path $out 'sizes.png')"
