$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.16/Stremio.5.0.16-x64.exe'
    $packageArgs['checksum']     = '2b60d91e07b0a1c9d174e5a3e66fc040b5e648cb60a5ea28ecac95895ed16fcf'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.16/Stremio.5.0.16-x86.exe'
    $packageArgs['checksum']     = 'a5809e560ce0a291c9727b02be1b2fd7fb0e5d85463b01f96edb5fe0294010a8'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
