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


function Resolve-BuildArtifactPath {
    param(
        [Parameter(Mandatory=$true)][string]$configuredPath,
        [Parameter(Mandatory=$true)][string]$flatPath,
        [Parameter(Mandatory=$true)][string]$binaryName,
        [Parameter(Mandatory=$true)][string]$label
    )

    foreach ($candidate in @($configuredPath, $flatPath)) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $buildRoot = "..\..\build"
    if (Test-Path $buildRoot) {
        $discoveredCandidate = Get-ChildItem -Path $buildRoot -Filter $binaryName -Recurse -File -ErrorAction SilentlyContinue |
            Select-Object -First 1

        if ($discoveredCandidate) {
            Write-Host "Warning: using discovered fallback artifact for ${label}: $($discoveredCandidate.FullName)" -ForegroundColor Yellow
            return $discoveredCandidate.FullName
        }
    }

    Write-Host "Error: Missing required artifact for $label." -ForegroundColor Red
    Write-Host "Expected one of:" -ForegroundColor Yellow
    Write-Host " - $configuredPath" -ForegroundColor Yellow
    Write-Host " - $flatPath" -ForegroundColor Yellow
    if (Test-Path $buildRoot) {
        Write-Host "Searched recursively under: $buildRoot for $binaryName" -ForegroundColor Yellow
    }
    exit 1
}

function Stage-ArtifactForNsis {
    param(
        [Parameter(Mandatory=$true)][string]$sourcePath,
        [Parameter(Mandatory=$true)][string]$outputFileName,
        [Parameter(Mandatory=$true)][string]$stagingDirectory
    )

    New-Item -ItemType Directory -Path $stagingDirectory -Force | Out-Null
    $destinationPath = Join-Path $stagingDirectory $outputFileName
    Copy-Item -Path $sourcePath -Destination $destinationPath -Force
    return (Resolve-Path $destinationPath).Path
}

    exit 1
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

    $daemonBinaryPath = Resolve-BuildArtifactPath -configuredPath "..\..\build\clients\core-daemon\$buildConfig\parthenond.exe" -flatPath "..\..\build\clients\core-daemon\parthenond.exe" -binaryName "parthenond.exe" -label "parthenond"
    $cliBinaryPath = Resolve-BuildArtifactPath -configuredPath "..\..\build\clients\cli\$buildConfig\parthenon-cli.exe" -flatPath "..\..\build\clients\cli\parthenon-cli.exe" -binaryName "parthenon-cli.exe" -label "parthenon-cli"
    $desktopBinaryPath = Resolve-BuildArtifactPath -configuredPath "..\..\build\clients\desktop\$buildConfig\parthenon-qt.exe" -flatPath "..\..\build\clients\desktop\parthenon-qt.exe" -binaryName "parthenon-qt.exe" -label "parthenon-qt"

    $nsisStagingDirectory = ".\.nsis-input"
    $stagedDaemonBinaryPath = Stage-ArtifactForNsis -sourcePath $daemonBinaryPath -outputFileName "parthenond.exe" -stagingDirectory $nsisStagingDirectory
    $stagedCliBinaryPath = Stage-ArtifactForNsis -sourcePath $cliBinaryPath -outputFileName "parthenon-cli.exe" -stagingDirectory $nsisStagingDirectory
    $stagedDesktopBinaryPath = Stage-ArtifactForNsis -sourcePath $desktopBinaryPath -outputFileName "parthenon-qt.exe" -stagingDirectory $nsisStagingDirectory
    $daemonBinaryPath = Resolve-BuildArtifactPath -configuredPath "..\..\build\clients\core-daemon\$buildConfig\parthenond.exe" -flatPath "..\..\build\clients\core-daemon\parthenond.exe" -label "parthenond"
    $cliBinaryPath = Resolve-BuildArtifactPath -configuredPath "..\..\build\clients\cli\$buildConfig\parthenon-cli.exe" -flatPath "..\..\build\clients\cli\parthenon-cli.exe" -label "parthenon-cli"
    $desktopBinaryPath = Resolve-BuildArtifactPath -configuredPath "..\..\build\clients\desktop\$buildConfig\parthenon-qt.exe" -flatPath "..\..\build\clients\desktop\parthenon-qt.exe" -label "parthenon-qt"

    Write-Host "NSIS BUILD_CONFIG: $buildConfig" -ForegroundColor Cyan
    Write-Host "Resolved parthenond binary: $daemonBinaryPath" -ForegroundColor Cyan
    Write-Host "Resolved parthenon-cli binary: $cliBinaryPath" -ForegroundColor Cyan
    Write-Host "Resolved parthenon-qt binary: $desktopBinaryPath" -ForegroundColor Cyan
    Write-Host "Staged parthenond binary: $stagedDaemonBinaryPath" -ForegroundColor Cyan
    Write-Host "Staged parthenon-cli binary: $stagedCliBinaryPath" -ForegroundColor Cyan
    Write-Host "Staged parthenon-qt binary: $stagedDesktopBinaryPath" -ForegroundColor Cyan

    Write-Host "Enumerating build tree executables for diagnostics..." -ForegroundColor Cyan
    if (Test-Path "..\..\build") {
        Get-ChildItem -Path "..\..\build" -Recurse -File -Filter "*.exe" -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty FullName |
            ForEach-Object { Write-Host " - $_" -ForegroundColor DarkGray }
    }
    else {
        Write-Host "Warning: build directory not found at ..\..\build" -ForegroundColor Yellow
    }

    & $makensisPath "/DBUILD_CONFIG=$buildConfig" "/DDAEMON_BINARY_PATH=$stagedDaemonBinaryPath" "/DCLI_BINARY_PATH=$stagedCliBinaryPath" "/DDESKTOP_BINARY_PATH=$stagedDesktopBinaryPath" parthenon-installer.nsi
    & $makensisPath "/DBUILD_CONFIG=$buildConfig" "/DDAEMON_BINARY_PATH=$stagedDaemonBinaryPath" "/DCLI_BINARY_PATH=$stagedCliBinaryPath" "/DDESKTOP_BINARY_PATH=$stagedDesktopBinaryPath" parthenon-installer.nsi

    & $makensisPath "/DBUILD_CONFIG=$buildConfig" "/DDAEMON_BINARY_PATH=$daemonBinaryPath" "/DCLI_BINARY_PATH=$cliBinaryPath" "/DDESKTOP_BINARY_PATH=$desktopBinaryPath" parthenon-installer.nsi

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
