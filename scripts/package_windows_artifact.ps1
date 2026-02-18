param(
  [string]$BuildDir = "build_win_vst3",
  [string]$Version = "1.0.0-beta",
  [string]$ReadmePath = "dist/README_install.md"
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildRoot = Join-Path $RepoRoot $BuildDir
$ReadmeFullPath = Join-Path $RepoRoot $ReadmePath
$OutRoot = Join-Path $RepoRoot "dist\windows\out"
$StagingRoot = Join-Path $RepoRoot "dist\windows\staging_vst3"

$ExpectedVst3Path = Join-Path $BuildRoot "plugins\aifr3d_vst3\aifr3d_vst3_artefacts\Release\VST3\AIFR3D.vst3"

Write-Host "[package] repo root: $RepoRoot"
Write-Host "[package] build root: $BuildRoot"
Write-Host "[package] expected VST3 path: $ExpectedVst3Path"

if (-not (Test-Path $ReadmeFullPath)) {
  throw "README_install.md not found at: $ReadmeFullPath"
}

$Vst3Path = $ExpectedVst3Path
if (-not (Test-Path $Vst3Path)) {
  Write-Host "[package] expected path missing; searching build tree for AIFR3D.vst3..."
  $found = Get-ChildItem -Path $BuildRoot -Filter "AIFR3D.vst3" -Directory -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
  if (-not $found) {
    throw "AIFR3D.vst3 not found. Checked expected path and recursive search under: $BuildRoot"
  }
  $Vst3Path = $found.FullName
}

Write-Host "[package] using VST3 bundle path: $Vst3Path"

New-Item -ItemType Directory -Force -Path $OutRoot | Out-Null
if (Test-Path $StagingRoot) {
  Remove-Item -Recurse -Force $StagingRoot
}
New-Item -ItemType Directory -Force -Path $StagingRoot | Out-Null

$StagedVst3 = Join-Path $StagingRoot "AIFR3D.vst3"
$StagedReadme = Join-Path $StagingRoot "README_install.md"

Copy-Item -Path $Vst3Path -Destination $StagedVst3 -Recurse -Force
Copy-Item -Path $ReadmeFullPath -Destination $StagedReadme -Force

$ZipPath = Join-Path $OutRoot "AIFR3D_v$Version`_Windows.zip"
if (Test-Path $ZipPath) {
  Remove-Item $ZipPath -Force
}

Compress-Archive -Path @(
  $StagedVst3,
  $StagedReadme
) -DestinationPath $ZipPath -CompressionLevel Optimal

Write-Host "[package] created zip: $ZipPath"
Write-Host "[package] contents:"
Get-ChildItem -Path $StagingRoot | Select-Object Name, FullName | Format-Table -AutoSize | Out-String | Write-Host
