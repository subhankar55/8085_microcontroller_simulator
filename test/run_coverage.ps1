param(
    [string]$BuildDirectory = "",
    [string]$CoverageDirectory = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $projectRoot "build-coverage"
}
if ([string]::IsNullOrWhiteSpace($CoverageDirectory)) {
    $CoverageDirectory = Join-Path $projectRoot "coverage"
}

$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$CoverageDirectory = [System.IO.Path]::GetFullPath($CoverageDirectory)

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
$compiler = Get-Command g++ -ErrorAction SilentlyContinue
$gcovrCommand = Get-Command gcovr -ErrorAction SilentlyContinue
$localGcovr = Join-Path $projectRoot "cpu\Scripts\gcovr.exe"

if ($null -eq $cmake) {
    throw "CMake was not found on PATH. Install CMake and reopen PowerShell."
}
if ($null -eq $compiler) {
    throw "g++ was not found on PATH. Install a GCC toolchain with gcov support."
}

if ($env:GCOVR_EXECUTABLE) {
    $gcovr = $env:GCOVR_EXECUTABLE
} elseif ($null -ne $gcovrCommand) {
    $gcovr = $gcovrCommand.Source
} elseif (Test-Path -LiteralPath $localGcovr) {
    $gcovr = $localGcovr
} else {
    throw "gcovr was not found. Install it or set GCOVR_EXECUTABLE to its executable path."
}

function Invoke-CheckedCommand {
    param(
        [Parameter(Mandatory = $true)][string]$Executable,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Executable failed with exit code $LASTEXITCODE."
    }
}

Invoke-CheckedCommand $cmake.Source @(
    "-S", $projectRoot,
    "-B", $BuildDirectory,
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_CXX_COMPILER=$($compiler.Source)",
    "-DCMAKE_CXX_FLAGS=--coverage -O0 -g",
    "-DCMAKE_EXE_LINKER_FLAGS=--coverage",
    "-DCMAKE_SHARED_LINKER_FLAGS=--coverage"
)

Get-ChildItem -LiteralPath $BuildDirectory -Filter "*.gcda" -File -Recurse |
    Remove-Item -Force

Invoke-CheckedCommand $cmake.Source @("--build", $BuildDirectory, "--parallel")

Invoke-CheckedCommand "ctest" @("--test-dir", $BuildDirectory, "--output-on-failure")

New-Item -ItemType Directory -Path $CoverageDirectory -Force | Out-Null
$coreFilter = ((Join-Path $projectRoot "Core_engine").Replace("\", "/")) + "/"
Invoke-CheckedCommand $gcovr @(
    "--root", $projectRoot,
    "--filter", $coreFilter,
    "--html-details", (Join-Path $CoverageDirectory "index.html"),
    "--print-summary",
    "--exclude-unreachable-branches",
    "--exclude-throw-branches",
    "--fail-under-line", "100",
    "--fail-under-function", "100",
    $BuildDirectory
)

Write-Host "Coverage report: $(Join-Path $CoverageDirectory 'index.html')"
