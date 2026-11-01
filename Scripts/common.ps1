$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Root 'Doubleit.uproject'
$Engine = if ($env:UE_ROOT) { $env:UE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }
$BuildBat = Join-Path $Engine 'Engine\Build\BatchFiles\Build.bat'
$RunUAT = Join-Path $Engine 'Engine\Build\BatchFiles\RunUAT.bat'
$EditorCmd = Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

# Android builds: UE 5.8's Gradle 8.13 cannot run on Android Studio's bundled Java 25, so use JDK 21
# (winget install Microsoft.OpenJDK.21). Fills in only what the environment does not already set.
function Use-AndroidToolchain {
    if (-not $env:ANDROID_HOME) { $env:ANDROID_HOME = Join-Path $env:LOCALAPPDATA 'Android\Sdk' }
    $jdk = Get-ChildItem 'C:\Program Files\Microsoft' -Directory -Filter 'jdk-21*' -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending | Select-Object -First 1
    if ($jdk) { $env:JAVA_HOME = $jdk.FullName }
    elseif (-not $env:JAVA_HOME) { throw 'JDK 21 not found: winget install Microsoft.OpenJDK.21' }
}
