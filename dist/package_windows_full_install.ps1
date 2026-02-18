param(
  [string]$BuildDir = "build_win_vst3",
  [string]$Version = "1.0.0-beta"
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Vst3Path = Join-Path $RepoRoot "$BuildDir\plugins\aifr3d_vst3\aifr3d_vst3_artefacts\Release\VST3\AIFR3D.vst3"
$StagingRoot = Join-Path $RepoRoot "dist\windows\staging"
$OutRoot = Join-Path $RepoRoot "dist\windows\out"
$InstallerExe = Join-Path $OutRoot "AIFR3D_Installer_v$Version.exe"
$ZipPath = Join-Path $OutRoot "AIFR3D_v$Version`_Windows_FullInstall.zip"

New-Item -ItemType Directory -Force -Path $StagingRoot | Out-Null
New-Item -ItemType Directory -Force -Path $OutRoot | Out-Null

if (-not (Test-Path $Vst3Path)) {
  throw "VST3 bundle not found at: $Vst3Path"
}

Write-Host "Staging VST3 from: $Vst3Path"
Copy-Item -Path $Vst3Path -Destination (Join-Path $StagingRoot "AIFR3D.vst3") -Recurse -Force

$iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
  throw "Inno Setup compiler not found: $iscc"
}

Push-Location $RepoRoot
try {
  & $iscc "packaging\windows\AIFR3D.iss"
} finally {
  Pop-Location
}

if (-not (Test-Path $InstallerExe)) {
  throw "Installer EXE not found after ISCC run: $InstallerExe"
}

Copy-Item -Path (Join-Path $RepoRoot "dist\README_install.md") -Destination $OutRoot -Force
Copy-Item -Path (Join-Path $RepoRoot "VERSION") -Destination $OutRoot -Force

if (Test-Path $ZipPath) {
  Remove-Item $ZipPath -Force
}

Compress-Archive -Path @(
  $InstallerExe,
  (Join-Path $OutRoot "README_install.md"),
  (Join-Path $OutRoot "VERSION")
) -DestinationPath $ZipPath -CompressionLevel Optimal

Write-Host "Created installer: $InstallerExe"
Write-Host "Created zip: $ZipPath"
