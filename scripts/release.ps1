# Build an unsigned KitForge NSIS installer (Standalone + VST3) for Windows.
#
# Prerequisites:
#   - Visual Studio 2022 (Desktop development with C++)
#   - Ninja on PATH
#   - NSIS (makensis) on PATH — https://nsis.sourceforge.io/
#   - Node.js / npm (or let CMake run npm via KITFORGE_BUILD_UI=ON)
#
# Usage (from "x64 Native Tools Command Prompt for VS 2022" or PowerShell):
#   pwsh -File scripts\release.ps1
#
# Output:
#   build-release\KitForge_artefacts\Release\Standalone\KitForge.exe
#   build-release\KitForge_artefacts\Release\VST3\KitForge.vst3
#   dist\KitForge-<version>-Setup.exe   (unsigned — SmartScreen may warn)

#Requires -Version 5.1
$ErrorActionPreference = "Stop"

$Root = Split-Path $PSScriptRoot -Parent
Set-Location $Root

function Write-Step([string]$Message) {
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Get-ProjectVersion {
    if ($env:KITFORGE_VERSION) { return $env:KITFORGE_VERSION.TrimStart("v") }
    $line = Select-String -Path "CMakeLists.txt" -Pattern '^project\(KitForge VERSION ' | Select-Object -First 1
    if (-not $line) { throw "Could not read project version from CMakeLists.txt" }
    return ($line.Line -replace '^project\(KitForge VERSION\s+', '' -replace '\).*', '').Trim()
}

function Get-NumericVersion([string]$Version) {
    $numeric = ($Version -split '-')[0]
    $parts = $numeric -split '\.'
    while ($parts.Count -lt 4) { $parts += '0' }
    return ($parts[0..3] -join '.')
}

$Version = Get-ProjectVersion
$NumericVersion = Get-NumericVersion $Version
$BuildDir = if ($env:KITFORGE_BUILD_DIR) { $env:KITFORGE_BUILD_DIR } else { "build-release" }
$DistDir = "dist"
$Artefacts = Join-Path $BuildDir "KitForge_artefacts\Release"
$StandaloneDir = Join-Path $Artefacts "Standalone"
$ResourcesDir = Join-Path $Artefacts "Resources"
$Vst3Dir = Join-Path $Artefacts "VST3"
$StageDir = Join-Path $BuildDir "stage"
$InstallerPath = Join-Path $DistDir "KitForge-$Version-Setup.exe"

if (-not (Get-Command makensis -ErrorAction SilentlyContinue)) {
    throw "makensis not on PATH. Install NSIS and add it to PATH."
}

Write-Step "Building KitForge $Version (unsigned Windows installer)"

Write-Step "Configuring ($BuildDir)"
cmake -B $BuildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DKITFORGE_BUILD_UI=ON `
    -DKITFORGE_USE_DEV_SERVER=OFF `
    -DKITFORGE_COPY_PLUGIN_AFTER_BUILD=OFF
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Step "Building Standalone + VST3"
cmake --build $BuildDir --target KitForge_Standalone KitForge_VST3
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ExePath = Join-Path $StandaloneDir "KitForge.exe"
$Vst3Bundle = Join-Path $Vst3Dir "KitForge.vst3"
$UiIndex = Join-Path $ResourcesDir "ui\dist\index.html"

if (-not (Test-Path $ExePath)) { throw "Build did not produce $ExePath" }
if (-not (Test-Path $Vst3Bundle)) { throw "Build did not produce $Vst3Bundle" }
if (-not (Test-Path $UiIndex)) {
    Write-Step "UI bundle missing — building ui/dist"
    Push-Location ui
    npm ci
    npm run build
    Pop-Location
    if (-not (Test-Path $UiIndex)) { throw "UI build did not produce $UiIndex" }
}

Write-Step "Staging installer payload"
if (Test-Path $StageDir) { Remove-Item -Recurse -Force $StageDir }
New-Item -ItemType Directory -Force -Path (Join-Path $StageDir "Standalone") | Out-Null
Copy-Item $ExePath (Join-Path $StageDir "Standalone\KitForge.exe")
Copy-Item -Recurse $ResourcesDir (Join-Path $StageDir "Resources")

Write-Step "Building NSIS installer -> $InstallerPath"
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
$StageAbs = (Resolve-Path $StageDir).Path
$Vst3Abs = (Resolve-Path $Vst3Dir).Path
$OutputAbs = Join-Path (Resolve-Path $DistDir).Path "KitForge-$Version-Setup.exe"

& makensis `
    "/DKITFORGE_VERSION=$Version" `
    "/DKITFORGE_VERSION_NUMERIC=$NumericVersion" `
    "/DKITFORGE_STAGE_DIR=$StageAbs" `
    "/DKITFORGE_VST3_SRC=$Vst3Abs" `
    "/DKITFORGE_OUTPUT=$OutputAbs" `
    (Join-Path $Root "windows\installer.nsi")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Done." -ForegroundColor Green
Write-Host "  $ExePath"
Write-Host "  $Vst3Bundle"
Write-Host "  $OutputAbs"
Write-Host ""
Write-Host "Installer is unsigned. SmartScreen may warn — More info -> Run anyway." -ForegroundColor Yellow
Write-Host "Requires Microsoft Edge WebView2 Runtime (preinstalled on most Windows 10/11)." -ForegroundColor Yellow
