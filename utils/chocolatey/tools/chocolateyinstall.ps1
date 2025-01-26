$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.15/Stremio.5.0.15-x64.exe'
    $packageArgs['checksum']     = '2ae3e585b34d2f0c54e8290c27d48a291e7c2c54ea2c95f1d0c2a4652b970c00'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.15/Stremio.5.0.15-x86.exe'
    $packageArgs['checksum']     = '5cb7ca75d694d1c13d8a6ce2bc38f4a4f3c9121b50f5462cea8878d875512964'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
