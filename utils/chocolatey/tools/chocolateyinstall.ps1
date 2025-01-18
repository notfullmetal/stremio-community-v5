$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.12/Stremio.5.0.12-x64.exe'
    $packageArgs['checksum']     = 'e7c045800d502874b8e083427aa81d316625c658fa804e67156dddd4f03ea6ed'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.12/Stremio.5.0.12-x86.exe'
    $packageArgs['checksum']     = '715ec5def305a019849b3a5e402398b53b5ed242f065266384f4428ed4c39818'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
