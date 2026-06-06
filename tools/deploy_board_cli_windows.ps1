$projectRoot = Split-Path -Parent $PSScriptRoot
$qtBin = "D:\Linux\QTcreater\App\6.11.0\mingw_64\bin"
$mingwBin = "D:\Linux\QTcreater\App\Tools\mingw1310_64\bin"
$releaseExe = Join-Path $projectRoot "build-board-cli\release\board_cli.exe"

if (!(Test-Path -LiteralPath $releaseExe)) {
    Write-Error "Release board_cli executable was not found: $releaseExe"
    exit 1
}

$env:Path = "$qtBin;$mingwBin;$env:Path"

Write-Host "Deploying Qt runtime for $releaseExe"
& (Join-Path $qtBin "windeployqt.exe") $releaseExe
