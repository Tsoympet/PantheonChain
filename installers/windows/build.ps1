# ParthenonChain Windows Build Script
# Creates NSIS installer for Windows

$ErrorActionPreference = "Stop"

Write-Host "Building ParthenonChain Windows installer..." -ForegroundColor Green

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

function Resolve-MakensisPath {
    $makensisCandidates = @()

    if ($env:NSIS_HOME) {
        $makensisCandidates += (Join-Path $env:NSIS_HOME "makensis.exe")
    }

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

    return $makensisCandidates |
        Where-Object { $_ -and (Test-Path $_) } |
        Select-Object -First 1
}

function Install-NsisIfAvailable {
    $isCi = $env:CI -eq "true" -or $env:GITHUB_ACTIONS -eq "true"
    if (-not $isCi) {
        return
    }

    $choco = Get-Command choco -ErrorAction SilentlyContinue
    if ($choco) {
        Write-Host "NSIS compiler not found; attempting Chocolatey installation..." -ForegroundColor Yellow
        & $choco.Source install nsis -y --no-progress
        if ($LASTEXITCODE -eq 0) {
            return
        }

        Write-Host "Warning: Chocolatey NSIS installation failed with exit code $LASTEXITCODE." -ForegroundColor Yellow
    }

    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if ($winget) {
        Write-Host "Attempting NSIS installation via winget..." -ForegroundColor Yellow
        & $winget.Source install --id NSIS.NSIS --exact --silent --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -eq 0) {
            return
        }

        Write-Host "Warning: winget NSIS installation failed with exit code $LASTEXITCODE." -ForegroundColor Yellow
    }

    Write-Host "NSIS not found and no supported package manager installation succeeded." -ForegroundColor Yellow
}

# Check if NSIS installer script exists
if (Test-Path ".\parthenon-installer.nsi") {
    $makensisPath = Resolve-MakensisPath

    if (-not $makensisPath) {
        Install-NsisIfAvailable
        $makensisPath = Resolve-MakensisPath
    }

    if (-not $makensisPath) {
        Write-Host "Error: makensis.exe not found. Install NSIS or set NSIS_HOME to the NSIS installation directory." -ForegroundColor Red
        exit 1
    }

    Write-Host "Using NSIS compiler: $makensisPath" -ForegroundColor Cyan
    Write-Host "Running NSIS installer build..." -ForegroundColor Cyan
    & $makensisPath parthenon-installer.nsi

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Windows installer build complete!" -ForegroundColor Green
    }
    else {
        Write-Host "Error: NSIS build failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}
else {
    Write-Host "Error: parthenon-installer.nsi not found" -ForegroundColor Red
    exit 1
}
