$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$serverExe = Join-Path $repoRoot "cmake-build-debug\server\StalkerServer.exe"
$runtimeBin = "C:\msys64\ucrt64\bin"

if (-not (Test-Path -Path $runtimeBin -PathType Container))
{
    throw "Runtime directory not found: $runtimeBin"
}

if (-not (Test-Path -Path $serverExe -PathType Leaf))
{
    throw "Server executable not found: $serverExe"
}

$env:PATH = "$runtimeBin;$env:PATH"

& $serverExe
exit $LASTEXITCODE
