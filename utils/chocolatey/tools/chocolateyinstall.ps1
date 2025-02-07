$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.17/Stremio.5.0.17-x64.exe'
    $packageArgs['checksum']     = '3135105a2c2e49c8ca8d42a54e24c2b2b983961f3abe06dbdff544cf87823a01'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.17/Stremio.5.0.17-x86.exe'
    $packageArgs['checksum']     = '5ba790af3efff89021862a25db619ffe9fccac65c233fba47cfd934fe4a18328'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
