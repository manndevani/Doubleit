# Compiles the Win64 editor target. Exit code != 0 on compile errors.
# -DisableAdaptiveUnity: edited files are compiled inside the unity blobs too, so name clashes between files show up
# now instead of on the first build after committing.
. "$PSScriptRoot\common.ps1"
& $BuildBat DoubleItEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload -DisableAdaptiveUnity
exit $LASTEXITCODE
