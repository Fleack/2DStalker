param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$ServerExe,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ServerArgs = @()
)

$ErrorActionPreference = "Stop"

$runtimeBin = "C:\msys64\ucrt64\bin"

if (-not (Test-Path -LiteralPath $runtimeBin -PathType Container))
{
    throw "Runtime directory not found: $runtimeBin"
}

if (-not (Test-Path -LiteralPath $ServerExe -PathType Leaf))
{
    throw "Server executable not found: $ServerExe"
}

$serverExePath = Resolve-Path -LiteralPath $ServerExe

$env:PATH = "$runtimeBin;$env:PATH"

& $serverExePath.ProviderPath @ServerArgs
exit $LASTEXITCODE