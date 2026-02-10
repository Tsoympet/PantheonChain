# ParthenonChain Windows Build Script
# Creates NSIS installer for Windows

$ErrorActionPreference = "Stop"

Write-Host "Building ParthenonChain Windows installer..." -ForegroundColor Green

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

# Check if NSIS installer script exists
if (Test-Path ".\parthenon-installer.nsi") {
    $makensisCandidates = @()

    try {
        $makensisCommand = Get-Command makensis -ErrorAction Stop
        $makensisCandidates += $makensisCommand.Source
    }
    catch {
        # Fallback to known default install locations below.
    }

    $makensisCandidates += @(
        "C:\Program Files (x86)\NSIS\makensis.exe",
        "C:\Program Files\NSIS\makensis.exe"
    )

    $makensisPath = $makensisCandidates |
        Where-Object { $_ -and (Test-Path $_) } |
        Select-Object -First 1

    if (-not $makensisPath) {
        Write-Host "Error: makensis.exe not found. Ensure NSIS is installed and available in PATH." -ForegroundColor Red
        exit 1
    }

    Write-Host "Using NSIS compiler: $makensisPath" -ForegroundColor Cyan
    Write-Host "Running NSIS installer build..." -ForegroundColor Cyan
    & $makensisPath parthenon-installer.nsi
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Windows installer build complete!" -ForegroundColor Green
    } else {
        Write-Host "Error: NSIS build failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
} else {
    Write-Host "Error: parthenon-installer.nsi not found" -ForegroundColor Red
    exit 1
}
