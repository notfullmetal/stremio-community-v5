$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.10/Stremio.5.0.10-x64.exe'
    $packageArgs['checksum']     = '480ada3bf18966176f6b1027110cb3e712e4b341a3dbceaf0409f40e4b415e37'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.10/Stremio.5.0.10-x86.exe'
    $packageArgs['checksum']     = '1584c883fd2eaacc4c2c354738a8e8656c90d3e353fbd37b169eb748faa1cad3'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
