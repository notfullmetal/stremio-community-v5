$packageName = 'stremio-desktop-v5'
$toolsDir    = Split-Path $MyInvocation.MyCommand.Definition

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  silentArgs     = '/S'
  validExitCodes = @(0)
}



if ([Environment]::Is64BitOperatingSystem) {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.9/Stremio.5.0.9-x64.exe'
    $packageArgs['checksum']     = 'd2720ccc2f9ad293035ac76de435101de55bc09cab425e091c864b56597ccbf8'
    $packageArgs['checksumType'] = 'sha256'
} else {
    $packageArgs['url']          = 'https://github.com/Zaarrg/stremio-desktop-v5/releases/download/5.0.0-beta.9/Stremio.5.0.9-x86.exe'
    $packageArgs['checksum']     = 'df76fe7f8e846f3968b9acc9dd2394159cd86de79a029974e5ddb33e291ea583'
    $packageArgs['checksumType'] = 'sha256'
}

Install-ChocolateyPackage @packageArgs
