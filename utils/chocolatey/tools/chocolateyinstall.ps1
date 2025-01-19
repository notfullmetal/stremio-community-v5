$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.13/Stremio.5.0.13-x64.exe'
    $packageArgs['checksum']     = 'eec67b8979528ad78784012c35725d9930d88802cefb6a5338911aaa7658e5a5'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.13/Stremio.5.0.13-x86.exe'
    $packageArgs['checksum']     = 'a0263a7238dc9a7b8868c30467fea0190e6855a397b3f8b72bc4b675cd496e65'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
