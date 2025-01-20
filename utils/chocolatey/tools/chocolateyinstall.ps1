$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.14/Stremio.5.0.14-x64.exe'
    $packageArgs['checksum']     = '2d206f6e5d7ce22be968211ef445fa5febdeaf4cae8223945d7602296f03effe'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.14/Stremio.5.0.14-x86.exe'
    $packageArgs['checksum']     = '2a8caede355650a6f018c53ab3b3fcd25c2fb264aa06c190ee007f0bf7d90f80'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
