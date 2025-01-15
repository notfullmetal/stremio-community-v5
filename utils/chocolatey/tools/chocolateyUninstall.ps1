$packageName = 'stremio-desktop-v5'
$uninstallPath = "$env:LOCALAPPDATA\Programs\LNV\Stremio-5\Uninstall.exe"

If (Test-Path $uninstallPath) {
    Start-Process -FilePath $uninstallPath -ArgumentList '/S' -Wait
}
