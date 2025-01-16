$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.8/Stremio.5.0.8-x64.exe'
    $packageArgs['checksum']     = 'a8228526cbbbffa1265b437109f6d570f31705bae71eb2be11192b02a0bff1ba'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.8/Stremio.5.0.8-x86.exe'
    $packageArgs['checksum']     = '84cca8fcc6635792e273af3220db505024c7f36aef4d8ae588ef895a5ff9fc3d'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
