<#
  Batman Asylum VR uninstaller. The installer copies it into the game folder (next to ShippingPC-BmGame.exe).
  Removes the mod and the shortcuts, puts back a d3d9.dll / xinput1_3.dll from another mod if there was one, and deletes
  the mod's own VR settings, logs and pictures (%LOCALAPPDATA%\Batman_Asylum). The game's own graphics settings are
  only changed while a VR session runs, and put back when it ends; an interrupted session is put back here.
  Optional: -GameDir "<folder with ShippingPC-BmGame.exe>" -Yes (no questions) -KeepSettings (keep the mod's VR
  settings, e.g. to reinstall) -DataDir "<mod data folder>" -KeepShortcuts -KeepSelf (practice runs)
#>
param([string]$GameDir, [string]$DataDir, [switch]$Yes, [switch]$KeepSettings, [switch]$KeepShortcuts, [switch]$KeepSelf)

$ErrorActionPreference = 'Stop'
function Say([string]$text, [string]$color = 'Gray') { Write-Host $text -ForegroundColor $color }
function Fail([string]$text) { Write-Host ''; Write-Host "  STOPPED: $text" -ForegroundColor Red; Write-Host ''; exit 1 }
$Here = [IO.Path]::GetDirectoryName($MyInvocation.MyCommand.Path)
$Marker = '[batman_asylum-proxy]'
$Exe = 'ShippingPC-BmGame.exe'

Write-Host ''
Write-Host '  Batman Asylum VR - uninstaller' -ForegroundColor White
Write-Host '  ------------------------------'
if (-not $GameDir) {
    if (Test-Path (Join-Path $Here $Exe)) { $GameDir = $Here }
    else {
        try {
            $root = ((Get-ItemProperty 'HKCU:\Software\Valve\Steam' -ErrorAction Stop).SteamPath) -replace '/', '\'
            $libs = @($root)
            $vdf = Join-Path $root 'steamapps\libraryfolders.vdf'
            if (Test-Path $vdf) { foreach ($m in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) { $libs += ($m.Groups[1].Value -replace '\\\\', '\') } }
            foreach ($l in $libs) {
                $d = Join-Path $l 'steamapps\common\Batman Arkham Asylum GOTY\Binaries'
                if (Test-Path (Join-Path $d $Exe)) { $GameDir = $d; break }
            }
        } catch { }
    }
}
if (-not $GameDir -or -not (Test-Path (Join-Path $GameDir $Exe))) { Fail 'the game was not found. Run this from the game folder (next to ShippingPC-BmGame.exe).' }
if (Get-Process -Name 'ShippingPC-BmGame' -ErrorAction SilentlyContinue) { Fail 'Batman: Arkham Asylum is running. Close it first.' }
$GameDir = [IO.Path]::GetFullPath($GameDir)
$GameRoot = [IO.Path]::GetFullPath((Join-Path $GameDir '..'))
$bk = Join-Path $GameRoot 'BatmanAsylumVR_backup'
if (-not $DataDir) { $DataDir = Join-Path $env:LOCALAPPDATA 'Batman_Asylum' }
Say "  Game: $GameDir"
if (-not $Yes) {
    $a = Read-Host '  Remove Batman Asylum VR? (y/n)'
    if ($a -notmatch '^[yY]') { Say '  Nothing changed.'; exit 0 }
}

# ---- a VR launch that was cut short: its temporary graphics settings go first ------------------------------
$journal = Join-Path $DataDir 'profile-active.json'
if (Test-Path -LiteralPath $journal) {
    # Skip files already equal to their original (a write Windows refused never changed them, WRITEDENIED). If one
    # really cannot be put back, stop BEFORE the data folder (which holds the originals) is deleted.
    $prof = Get-Content -LiteralPath $journal -Raw | ConvertFrom-Json
    foreach ($file in $prof.files) {
        if ((Test-Path -LiteralPath $file.path) -and (Get-FileHash -LiteralPath $file.path).Hash -eq $file.sha256) { continue }
        if ((Test-Path -LiteralPath $file.backup) -and (Get-FileHash -LiteralPath $file.backup).Hash -eq $file.sha256) {
            try { [IO.File]::WriteAllBytes($file.path, [IO.File]::ReadAllBytes($file.backup)) }
            catch { Fail ("Windows did not allow your graphics settings file $($file.path) to be put back. Your original is " +
                          "saved at $($file.backup). Copy it back by hand, or allow PowerShell in Windows Security > Ransomware " +
                          'protection > Controlled folder access, then run the uninstaller again. Nothing was removed.') }
        }
    }
    Remove-Item -LiteralPath $journal -Force
    # PROFILECLEAN: their backup copies go too (matters with -KeepSettings; otherwise the data folder goes below).
    $profiles = Join-Path $DataDir 'profiles'
    if (Test-Path -LiteralPath $profiles) {
        Get-ChildItem -LiteralPath $profiles -Directory | Where-Object Name -Match '^\d{8}_\d{6}_\d{3}$' |
            ForEach-Object { Remove-Item -LiteralPath $_.FullName -Recurse -Force -ErrorAction SilentlyContinue }
    }
    Say '  Your graphics settings from an interrupted VR launch are back.' Green
}

# ---- the mod's files ------------------------------------------------------------------------------
foreach ($name in 'd3d9.dll', 'xinput1_3.dll') {
    $p = Join-Path $GameDir $name
    # Ours = the marker text, or (builds before 0.1.0) the name of the export our d3d9.dll calls in batman_asylum.dll.
    if ((Test-Path $p) -and ([IO.File]::ReadAllText($p, [Text.Encoding]::GetEncoding(28591)) | ForEach-Object { $_.Contains($Marker) -or $_.Contains('_AA_CreateD3D9@4') })) { Remove-Item $p -Force }
}
foreach ($f in 'batman_asylum.dll', 'aa_xr_config32.exe', 'asylumvr_manifest.json', 'Batman Asylum VR.bat',
               'Batman Asylum VR (SteamVR).bat', 'Batman Asylum VR (Previous Bridge).bat') {
    $p = Join-Path $GameDir $f; if (Test-Path $p) { Remove-Item $p -Force }
}
$p = Join-Path $GameDir 'AsylumVR'; if (Test-Path $p) { Remove-Item $p -Recurse -Force }
foreach ($name in 'd3d9.dll', 'xinput1_3.dll') {
    $saved = Join-Path $bk "binaries\$name"
    if (Test-Path $saved) { Copy-Item $saved (Join-Path $GameDir $name) -Force; Say "  The $name that was there before is back." }
}
Say '  Mod files removed.' Green

# ---- shortcuts ----------------------------------------------------------------------------------------
if (-not $KeepShortcuts) {
    $desk = [Environment]::GetFolderPath('Desktop')
    foreach ($n in 'Batman Asylum VR.lnk', 'Batman Asylum VR (SteamVR).lnk') { $p = Join-Path $desk $n; if (Test-Path $p) { Remove-Item $p -Force } }
    $start = Join-Path ([Environment]::GetFolderPath('Programs')) 'Batman Asylum VR'
    if (Test-Path $start) { Remove-Item $start -Recurse -Force }
}

# ---- the mod's own VR settings, logs and pictures (aa_rendering.ini, launch backups, captures) -------
if ($KeepSettings) {
    Say "  Your VR settings are kept in $DataDir (-KeepSettings)."
} elseif (Test-Path $DataDir) {
    Remove-Item $DataDir -Recurse -Force
    Say '  The mod''s VR settings, logs and pictures are deleted.' Green
}
if (Test-Path $bk) { Remove-Item $bk -Recurse -Force }

Write-Host ''
Write-Host '  Batman Asylum VR is removed. Starting the game from Steam plays the normal game.' -ForegroundColor White
Write-Host ''
# Remove ourselves when run from the game folder (the .bat deletes itself after this script is gone).
if (-not $KeepSelf -and $Here -eq $GameDir) { Remove-Item $MyInvocation.MyCommand.Path -Force -ErrorAction SilentlyContinue }
