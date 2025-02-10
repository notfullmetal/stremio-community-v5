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
    $packageArgs['checksum']     = '27ca55ded0c74e6e1d7f143f491e4cd35fb0e4f50a1ab6412311de543e6aa257'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.17/Stremio.5.0.17-x86.exe'
    $packageArgs['checksum']     = 'd42cc1b9b8aab6c8f3309741fe40e1e34ae856b4eff9806dde277f96b2384405'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
