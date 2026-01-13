# ParthenonChain Windows Build Script
# Creates NSIS installer for Windows

$ErrorActionPreference = "Stop"

Write-Host "Building ParthenonChain Windows installer..." -ForegroundColor Green

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

# Check if NSIS installer script exists
if (Test-Path ".\parthenon-installer.nsi") {
    Write-Host "Running NSIS installer build..." -ForegroundColor Cyan
    & makensis parthenon-installer.nsi
    
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
