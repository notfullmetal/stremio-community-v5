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
    $packageArgs['checksum']     = '8e4e4f77afc499814f67b174f6ef37e920900940ae174e5f62ddcd13a580fcd3'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.15/Stremio.5.0.15-x86.exe'
    $packageArgs['checksum']     = '07c3c2f580cb1d62f75d8d41a950a5f043ce40c1bf83e84e669cf65c83e7697f'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
