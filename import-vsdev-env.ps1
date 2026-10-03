[CmdletBinding()]
param(
    [ValidateSet("x64", "x86", "arm64")]
    [string] $Architecture = "x64",

    [ValidateSet("x64", "x86", "arm64")]
    [string] $HostArchitecture = "x64"
)

$ErrorActionPreference = "Stop"

$vswhereCandidates = @(
    (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"),
    (Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe")
)

$vswhere = $vswhereCandidates |
    Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
    Select-Object -First 1

if ($vswhere) {
    $installationPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
} else {
    $installationPath = @(
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\Community"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\Professional"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\Enterprise"),
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2022\BuildTools")
    ) |
        Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
        Select-Object -First 1
}

if (-not $installationPath) {
    throw "Visual Studio with C++ tools was not found."
}

$vsdevcmd = Join-Path $installationPath "Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path -LiteralPath $vsdevcmd)) {
    throw "VsDevCmd.bat was not found at '$vsdevcmd'."
}

$setupCommand = 'call "{0}" -arch={1} -host_arch={2} && set' -f `
    $vsdevcmd, $Architecture, $HostArchitecture
$environmentLines = & $env:ComSpec /d /s /c $setupCommand
if ($LASTEXITCODE -ne 0) {
    throw "Visual Studio environment setup failed with exit code $LASTEXITCODE."
}

foreach ($line in $environmentLines) {
    if ($line -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable(
            $Matches[1],
            $Matches[2],
            [EnvironmentVariableTarget]::Process
        )
    }
}

Write-Host "Visual Studio $Architecture tools imported into this PowerShell process."
Write-Host "Compiler: $env:VCToolsInstallDir"
Write-Host "Run 'cl' to verify the compiler is available."
