param(
    [string[]]$BoardCliArgs
)

$projectRoot = Split-Path -Parent $PSScriptRoot
$qtBin = "D:\Linux\QTcreater\App\6.11.0\mingw_64\bin"
$mingwBin = "D:\Linux\QTcreater\App\Tools\mingw1310_64\bin"

$debugExe = Join-Path $projectRoot "board_cli\build\Desktop_Qt_6_11_0_MinGW_64_bit-Debug\debug\board_cli.exe"
$releaseExe = Join-Path $projectRoot "build-board-cli\release\board_cli.exe"

if (Test-Path -LiteralPath $debugExe) {
    $exePath = $debugExe
} elseif (Test-Path -LiteralPath $releaseExe) {
    $exePath = $releaseExe
} else {
    Write-Error "board_cli executable was not found. Please build board_cli first in Qt Creator or with qmake/mingw32-make."
    exit 1
}

$env:Path = "$qtBin;$mingwBin;$env:Path"

Write-Host "Running: $exePath"
if ($BoardCliArgs -and $BoardCliArgs.Count -gt 0) {
    & $exePath @BoardCliArgs
} else {
    & $exePath
}
