# ParthenonChain Windows Build Script
# Creates NSIS installer for Windows

$ErrorActionPreference = "Stop"

Write-Host "Building ParthenonChain Windows installer..." -ForegroundColor Green

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

function Resolve-MakensisPath {
    $makensisCandidates = [System.Collections.Generic.List[string]]::new()

    function Add-Candidate([string]$candidate) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and -not $makensisCandidates.Contains($candidate)) {
            $makensisCandidates.Add($candidate)
        }
    }

    if ($env:NSIS_HOME) {
        Add-Candidate (Join-Path $env:NSIS_HOME "makensis.exe")
    }

    try {
        $makensisCommand = Get-Command makensis -ErrorAction Stop
        Add-Candidate $makensisCommand.Source
    }
    catch {
        # Fallback to known default install locations below.
    }

    if ($env:ChocolateyInstall) {
        Add-Candidate (Join-Path $env:ChocolateyInstall "bin\makensis.exe")
        Add-Candidate (Join-Path $env:ChocolateyInstall "lib\nsis\tools\makensis.exe")
    }


    $choco = Get-Command choco -ErrorAction SilentlyContinue
    if ($choco) {
        $chocoBinDir = Split-Path -Parent $choco.Source
        Add-Candidate (Join-Path $chocoBinDir "makensis.exe")

        $chocoRoot = Split-Path -Parent $chocoBinDir
        Add-Candidate (Join-Path $chocoRoot "lib\nsis\tools\makensis.exe")
    }

    @(
        "C:\ProgramData\chocolatey\bin\makensis.exe",
        "C:\ProgramData\chocolatey\lib\nsis\tools\makensis.exe",
        "C:\Program Files (x86)\NSIS\makensis.exe",
        "C:\Program Files\NSIS\makensis.exe"
    ) | ForEach-Object { Add-Candidate $_ }

    $script:MakensisSearchPaths = @($makensisCandidates)

    return $makensisCandidates |
        Where-Object { Test-Path $_ } |
        Select-Object -First 1
}

function Refresh-ProcessPathFromSystem {
    $pathSegments = [System.Collections.Generic.List[string]]::new()

    foreach ($scope in @("Process", "Machine", "User")) {
        $scopePath = [System.Environment]::GetEnvironmentVariable("Path", $scope)
        if (-not $scopePath) {
            continue
        }

        foreach ($entry in ($scopePath -split ";")) {
            if (-not [string]::IsNullOrWhiteSpace($entry) -and -not $pathSegments.Contains($entry)) {
                $pathSegments.Add($entry)
            }
        }
    }

    if ($pathSegments.Count -gt 0) {
        $env:Path = $pathSegments -join ";"
    }
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
            Refresh-ProcessPathFromSystem
            return
        }

        Write-Host "Warning: Chocolatey NSIS installation failed with exit code $LASTEXITCODE." -ForegroundColor Yellow
    }

    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if ($winget) {
        Write-Host "Attempting NSIS installation via winget..." -ForegroundColor Yellow
        & $winget.Source install --id NSIS.NSIS --exact --silent --accept-package-agreements --accept-source-agreements
        if ($LASTEXITCODE -eq 0) {
            Refresh-ProcessPathFromSystem
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
        if ($script:MakensisSearchPaths) {
            Write-Host "Searched paths:" -ForegroundColor Yellow
            $script:MakensisSearchPaths | ForEach-Object { Write-Host " - $_" -ForegroundColor Yellow }
        }
        exit 1
    }

    Write-Host "Using NSIS compiler: $makensisPath" -ForegroundColor Cyan
    Write-Host "Running NSIS installer build..." -ForegroundColor Cyan

    $buildConfig = if ([string]::IsNullOrWhiteSpace($env:BUILD_TYPE)) { "Release" } else { $env:BUILD_TYPE }
    Write-Host "NSIS BUILD_CONFIG: $buildConfig" -ForegroundColor Cyan
    & $makensisPath "/DBUILD_CONFIG=$buildConfig" parthenon-installer.nsi

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
