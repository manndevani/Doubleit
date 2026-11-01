# One-time: accept SDK licences and install the SDK/NDK packages UE 5.8 expects.
. "$PSScriptRoot\common.ps1"
$studio = (Get-ItemProperty 'HKLM:\SOFTWARE\Android Studio' -ErrorAction SilentlyContinue).Path
if (-not $studio) { $studio = 'C:\Program Files\Android\Android Studio' }
$env:JAVA_HOME = Join-Path $studio 'jbr'
$sdk = Join-Path $env:LOCALAPPDATA 'Android\Sdk'
$env:ANDROID_HOME = $sdk

$yesFile = Join-Path $env:TEMP 'doubleit_yes.txt'
Set-Content -Path $yesFile -Value ((1..30 | ForEach-Object { 'y' }) -join "`r`n") -Encoding ascii
cmd /c "`"$sdk\cmdline-tools\latest\bin\sdkmanager.bat`" --licenses < `"$yesFile`"" | Out-Null
& (Join-Path $Engine 'Engine\Extras\Android\SetupAndroid.bat') android-36 36.1.0 3.22.1 27.2.12479018 -noninteractive
exit $LASTEXITCODE
