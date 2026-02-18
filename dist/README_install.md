# AIFR3D v1.0.0-beta — Windows Install (FL Studio)

This beta is packaged for one-step installation on Windows.

## Download
Download `AIFR3D_v1.0.0-beta_Windows_FullInstall.zip` from the GitHub release assets.

## One-step install
1. Extract the zip.
2. Run `AIFR3D_Installer_v1.0.0-beta.exe`.
3. Keep default path unless you use a custom VST3 path:
   - `%COMMONPROGRAMFILES%\VST3\AIFR3D.vst3`

## FL Studio scan steps
1. Open **Options → Manage plugins**.
2. Ensure your VST3 path includes:
   - `C:\Program Files\Common Files\VST3`
3. Click **Find installed plugins**.
4. Insert **AIFR3D** in a mixer effect slot.

## Uninstall
Use Windows Apps/Installed Apps and remove **AIFR3D v1.0.0-beta**.

## Notes
- Installer is generated in CI on Windows using Inno Setup.
- If SmartScreen appears, choose “More info” → “Run anyway” for unsigned beta builds.
