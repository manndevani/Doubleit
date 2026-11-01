# Compiles the Win64 editor target. Exit code != 0 on compile errors.
. "$PSScriptRoot\common.ps1"
& $BuildBat DoubleItEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload
exit $LASTEXITCODE
