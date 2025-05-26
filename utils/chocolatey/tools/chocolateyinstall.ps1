$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.18/Stremio.5.0.18-x64.exe'
    $packageArgs['checksum']     = '9a04f81d5ef6e76207e2af1d8fa567b1f6c5edd696b8c49fb5b94261ebf04ea9'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.18/Stremio.5.0.18-x86.exe'
    $packageArgs['checksum']     = '3633ad3eda25c26e4b213f5a93f845ed226a9fb5186df59293e8576fc2de2d32'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
