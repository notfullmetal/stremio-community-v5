$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.11/Stremio.5.0.11-x64.exe'
    $packageArgs['checksum']     = '9333f379ce0ce04a9743f033b39ce6c4a00a4bba231009ebba0454532c7c10aa'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.11/Stremio.5.0.11-x86.exe'
    $packageArgs['checksum']     = '12535d930dc8c7ec260ab4ab83e189862bae5b848d6d68d9dc00167e4c434013'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
