param(
    [Parameter(Mandatory = $true)]
    [string]$SourceFile,

    [Parameter(Mandatory = $true)]
    [string]$ReceivedFile
)

if (!(Test-Path -LiteralPath $SourceFile)) {
    Write-Error "Source file does not exist: $SourceFile"
    exit 1
}

if (!(Test-Path -LiteralPath $ReceivedFile)) {
    Write-Error "Received file does not exist: $ReceivedFile"
    exit 1
}

$src = Get-FileHash -LiteralPath $SourceFile -Algorithm SHA256
$dst = Get-FileHash -LiteralPath $ReceivedFile -Algorithm SHA256

Write-Host "Source   : $($src.Path)"
Write-Host "SHA256   : $($src.Hash)"
Write-Host ""
Write-Host "Received : $($dst.Path)"
Write-Host "SHA256   : $($dst.Hash)"
Write-Host ""

if ($src.Hash -eq $dst.Hash) {
    Write-Host "Result   : PASS - files are identical"
    exit 0
}

Write-Host "Result   : FAIL - files differ"
exit 2
