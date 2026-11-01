# Cook + package for Android (ASTC). -Shipping for store builds, -Install to push to a USB-connected device.
param([switch]$Shipping, [switch]$Install)
. "$PSScriptRoot\common.ps1"
$config = if ($Shipping) { 'Shipping' } else { 'Development' }
$out = Join-Path $Root 'Build\Packaged'
$uatArgs = @('BuildCookRun', "-project=$Project", '-platform=Android', '-cookflavor=ASTC', "-clientconfig=$config",
    '-build', '-cook', '-stage', '-pak', '-package', '-archive', "-archivedirectory=$out", '-unattended', '-utf8output')
if ($Install) { $uatArgs += '-deploy' }
& $RunUAT @uatArgs
exit $LASTEXITCODE
