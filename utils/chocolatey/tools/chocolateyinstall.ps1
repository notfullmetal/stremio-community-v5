$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition
$packageArgs = @{
  packageName   = $packageName
  fileType      = 'exe'
  silentArgs    = '/S'
  validExitCodes= @(0)
}

if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url'] = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.7/Stremio.5.0.7-x64.exe'''
} else {
    $packageArgs['url'] = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.7/Stremio.5.0.7-x86.exe'''
}

Install-ChocolateyPackage @packageArgs
