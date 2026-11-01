# Regenerates materials from Shaders/*.hlsl (run after editing a shader). Requires a built editor.
. "$PSScriptRoot\common.ps1"
& $EditorCmd $Project -run=DIGenerateAssets -unattended -nosplash -nullrhi "-abslog=$Root\Saved\Logs\generate.log" | Out-Null
$code = $LASTEXITCODE
Select-String -Path "$Root\Saved\Logs\generate.log" -Pattern 'LogDoubleIt|Error:' | Select-Object -First 20 | ForEach-Object { $_.Line }
exit $code
