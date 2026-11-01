# Builds, then runs automation tests headless. Prints a one-line summary plus failures only.
param([string]$Filter = 'DoubleIt', [switch]$NoBuild)
. "$PSScriptRoot\common.ps1"

if (-not $NoBuild) {
    $buildLog = & $BuildBat DoubleItEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload 2>&1
    if ($LASTEXITCODE -ne 0) {
        $buildLog | Select-String -Pattern 'error|warning C' | Select-Object -First 30 | ForEach-Object { $_.Line }
        Write-Host "BUILD FAILED"
        exit 1
    }
}

$report = Join-Path $Root 'Saved\TestReport'
if (Test-Path $report) { Remove-Item -Recurse -Force $report }
& $EditorCmd $Project "-ExecCmds=Automation RunTests $Filter; Quit" -unattended -nullrhi -nosplash -nosound -NoP4 `
    "-ReportExportPath=$report" "-abslog=$Root\Saved\Logs\tests.log" | Out-Null

$json = Join-Path $report 'index.json'
if (-not (Test-Path $json)) { Write-Host "NO REPORT - see Saved\Logs\tests.log"; exit 2 }
$r = Get-Content $json -Raw -Encoding UTF8 | ConvertFrom-Json
foreach ($t in $r.tests) {
    if ($t.state -ne 'Success') {
        Write-Host "FAIL $($t.fullTestPath)"
        $t.entries | Where-Object { $_.event.type -eq 'Error' } | ForEach-Object { Write-Host "   $($_.event.message)" }
    }
}
Write-Host "Tests: $($r.succeeded) passed, $($r.failed) failed"
if ($r.failed -gt 0 -or $r.succeeded -eq 0) { exit 1 }
exit 0
