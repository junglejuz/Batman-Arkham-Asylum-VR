<#
  Batman Asylum VR installer (Batman: Arkham Asylum Game of the Year Edition in PC VR).

  Double-click Install-BatmanAsylumVR.bat. It finds the game, copies the mod into the game's Binaries folder, copies
  the tested VR settings (first install only) and makes the "Batman Asylum VR" shortcuts. The game's own graphics
  settings are NOT changed here: each VR start switches them to VR for that one launch and puts your own back when the
  game closes (AsylumVR\Launch-VR.ps1). Uninstall-BatmanAsylumVR.bat (copied into the game folder) undoes everything.

  Optional: -GameDir "<folder with ShippingPC-BmGame.exe>" -DataDir "<mod data folder>" (practice runs; default
            %LOCALAPPDATA%\Batman_Asylum) -Yes -NoShortcuts
#>
param([string]$GameDir, [string]$DataDir, [switch]$Yes, [switch]$NoShortcuts)

$Mod = @{
    Name        = 'Batman Asylum VR'
    Exe         = 'ShippingPC-BmGame.exe'
    ExeSha256   = '4dac1f5e2ac6710b7378fdce74601f616f4753e3756cb5fda63c7519cc2eb028'   # Steam GOTY, the tested build
    ExeSha256Joker = 'ed65ce3c52baca7adaea00cb0e5def3d8a78bd05f7ceacd68a68462e92d8dab2' # same build after the Joker DLC Patcher (Godoy301)
    SteamFolder = 'Batman Arkham Asylum GOTY\Binaries'                                  # under steamapps\common
    ProxyMarker = '[batman_asylum-proxy]'                                               # text inside our d3d9 / xinput1_3
    Backup      = 'BatmanAsylumVR_backup'                                               # in the game's top folder
}

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
function Say([string]$text, [string]$color = 'Gray') { Write-Host $text -ForegroundColor $color }
function Step([string]$text) { Write-Host ''; Write-Host "  $text" -ForegroundColor Cyan }
function Fail([string]$text) { Write-Host ''; Write-Host "  STOPPED: $text" -ForegroundColor Red; Write-Host ''; exit 1 }
$Here = [IO.Path]::GetDirectoryName($MyInvocation.MyCommand.Path)
# Ours = the marker text, or (builds before 0.1.0) the name of the export our d3d9.dll calls in batman_asylum.dll.
function Is-Ours([string]$path) { if (-not (Test-Path -LiteralPath $path)) { return $false }; $t = [IO.File]::ReadAllText($path, [Text.Encoding]::GetEncoding(28591)); $t.Contains($Mod.ProxyMarker) -or $t.Contains('_AA_CreateD3D9@4') }

# ---- finding things -------------------------------------------------------------------------------
function Find-SteamGameDir {
    $roots = @()
    foreach ($key in 'HKCU:\Software\Valve\Steam', 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam', 'HKLM:\SOFTWARE\Valve\Steam') {
        try {
            $p = Get-ItemProperty $key -ErrorAction Stop
            if ($p.SteamPath) { $roots += ($p.SteamPath -replace '/', '\') }
            if ($p.InstallPath) { $roots += $p.InstallPath }
        } catch { }
    }
    $libs = @()
    foreach ($r in ($roots | Select-Object -Unique)) {
        $libs += $r
        $vdf = Join-Path $r 'steamapps\libraryfolders.vdf'
        if (Test-Path $vdf) {
            foreach ($m in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) { $libs += ($m.Groups[1].Value -replace '\\\\', '\') }
        }
    }
    foreach ($lib in ($libs | Select-Object -Unique)) {
        $dir = [IO.Path]::Combine($lib, 'steamapps\common\' + $Mod.SteamFolder)
        if (Test-Path -LiteralPath ([IO.Path]::Combine($dir, $Mod.Exe))) { return $dir }
    }
    return $null
}
function Select-Folder([string]$description) {
    $f = New-Object System.Windows.Forms.Form; $f.TopMost = $true
    $d = New-Object System.Windows.Forms.FolderBrowserDialog; $d.Description = $description
    if ($d.ShowDialog($f) -eq [System.Windows.Forms.DialogResult]::OK) { return $d.SelectedPath }
    return $null
}
function Resolve-GameDir([string]$picked) {
    if (-not $picked -or -not (Test-Path -LiteralPath $picked -PathType Container)) { return @() }
    if (Test-Path -LiteralPath (Join-Path $picked $Mod.Exe)) { return @($picked) }
    return @(Get-ChildItem -LiteralPath $picked -Filter $Mod.Exe -Recurse -Depth 3 -File -ErrorAction SilentlyContinue |
        ForEach-Object { $_.DirectoryName } | Select-Object -Unique)
}

# =================================================================================================
Write-Host ''
Write-Host '  Batman Asylum VR - installer' -ForegroundColor White
Write-Host '  ----------------------------'

# ---- 1. the game ---------------------------------------------------------------------------------
Step '1. Finding Batman: Arkham Asylum'
if (-not $GameDir) { $GameDir = Find-SteamGameDir }
while (-not $GameDir -or -not (Test-Path (Join-Path $GameDir $Mod.Exe))) {
    if ($Yes) { Fail 'the game was not found. Run the installer again with -GameDir "<folder with ShippingPC-BmGame.exe>".' }
    Say '  Could not find the game by itself. Please pick its folder (for example ...\steamapps\common\Batman Arkham Asylum GOTY).' Yellow
    $pick = Select-Folder 'Pick the Batman: Arkham Asylum GOTY folder'
    if (-not $pick) { Fail 'no game folder was picked.' }
    $found = @(Resolve-GameDir $pick)
    if ($found.Count -eq 1) { $GameDir = $found[0] } else { Say '  ShippingPC-BmGame.exe is not in that folder. Try again.' Yellow }
}
if (Get-Process -Name 'ShippingPC-BmGame' -ErrorAction SilentlyContinue) { Fail 'Batman: Arkham Asylum is running. Close it, then run the installer again.' }
$GameDir = [IO.Path]::GetFullPath($GameDir)
$GameRoot = [IO.Path]::GetFullPath((Join-Path $GameDir '..'))   # ...\Batman Arkham Asylum GOTY
Say "  Game: $GameDir"
if ((Get-FileHash -LiteralPath (Join-Path $GameDir $Mod.Exe)).Hash -notin @($Mod.ExeSha256, $Mod.ExeSha256Joker)) {
    Fail 'this copy of the game is a different version from the one the mod was made for (Steam, Game of the Year Edition, unpatched or with the Joker DLC Patcher). Nothing was changed.'
}
if (-not (Test-Path (Join-Path $GameRoot 'BmGame\Config\BmEngine.ini'))) {
    Fail ('the game has not made its settings yet. Start Batman: Arkham Asylum once from Steam as normal, wait for the ' +
          'main menu, quit, then run this installer again.')
}
if (-not $DataDir) { $DataDir = Join-Path $env:LOCALAPPDATA 'Batman_Asylum' }

# A VR launch that was cut short leaves its temporary graphics settings in place: put the player's back first.
$journal = Join-Path $DataDir 'profile-active.json'
if (Test-Path -LiteralPath $journal) {
    # A file already equal to its original is skipped: a Documents copy Windows would not let us write (Controlled
    # folder access, permissions) was never changed, and writing it again here stopped the reinstall (WRITEDENIED).
    $prof = Get-Content -LiteralPath $journal -Raw | ConvertFrom-Json
    $left = @()
    foreach ($file in $prof.files) {
        if ((Test-Path -LiteralPath $file.path) -and (Get-FileHash -LiteralPath $file.path).Hash -eq $file.sha256) { continue }
        if ((Test-Path -LiteralPath $file.backup) -and (Get-FileHash -LiteralPath $file.backup).Hash -eq $file.sha256) {
            try { [IO.File]::WriteAllBytes($file.path, [IO.File]::ReadAllBytes($file.backup)) }
            catch { $left += $file.path; Say "  Windows did not allow $($file.path) to be put back. Your original is saved at $($file.backup)." Yellow }
        }
    }
    if ($left.Count) { Say '  The VR launcher will try again and tell you what to do.' Yellow }
    else {
        Remove-Item -LiteralPath $journal -Force
        # PROFILECLEAN: the originals are back, so the launch's backup copies (and older builds' leftovers) can go.
        $profiles = Join-Path $DataDir 'profiles'
        if (Test-Path -LiteralPath $profiles) {
            Get-ChildItem -LiteralPath $profiles -Directory | Where-Object Name -Match '^\d{8}_\d{6}_\d{3}$' |
                ForEach-Object { Remove-Item -LiteralPath $_.FullName -Recurse -Force -ErrorAction SilentlyContinue }
        }
        Say '  Settings from an interrupted VR launch were put back first.' Yellow
    }
}

# ---- 2. someone else's d3d9.dll / xinput1_3.dll (another mod): keep it, the uninstaller puts it back ------------
Step '2. Checking the game folder'
$bk = Join-Path $GameRoot $Mod.Backup
foreach ($name in 'd3d9.dll', 'xinput1_3.dll') {
    $p = Join-Path $GameDir $name
    if ((Test-Path $p) -and -not (Is-Ours $p)) {
        New-Item -ItemType Directory -Force (Join-Path $bk 'binaries') | Out-Null
        if (-not (Test-Path (Join-Path $bk "binaries\$name"))) { Copy-Item $p (Join-Path $bk "binaries\$name") }
        Say "  Another $name was in the game folder (another mod?). It is saved and comes back on uninstall." Yellow
    }
}
Say '  Ready.' Green

# ---- 3. the mod ---------------------------------------------------------------------------------
Step '3. Copying the mod into the game folder'
$files = @('d3d9.dll', 'xinput1_3.dll', 'batman_asylum.dll', 'aa_xr_config32.exe', 'Batman Asylum VR.bat',
           'Batman Asylum VR (SteamVR).bat', 'Uninstall-BatmanAsylumVR.bat', 'Uninstall-BatmanAsylumVR.ps1')
foreach ($f in $files) {
    $src = Join-Path $Here $f
    if (-not (Test-Path $src)) { Fail "$f is missing from the download. Unpack the whole zip and run the installer from there." }
}
if (-not (Test-Path (Join-Path $Here 'files\AsylumVR'))) { Fail 'files\AsylumVR is missing from the download. Unpack the whole zip and run the installer from there.' }
foreach ($f in $files) { Copy-Item (Join-Path $Here $f) (Join-Path $GameDir $f) -Force }
Copy-Item (Join-Path $Here 'files\AsylumVR') $GameDir -Recurse -Force
# Left over from development builds: their own uninstaller (refuses the new files) and a test launcher.
foreach ($old in 'AsylumVR\Uninstall-VR.ps1', 'Batman Asylum VR (Previous Bridge).bat') { $p = Join-Path $GameDir $old; if (Test-Path $p) { Remove-Item $p -Force } }
# The launcher checks the installed files against this list before every VR start.
$records = @()
foreach ($f in @('d3d9.dll', 'xinput1_3.dll', 'batman_asylum.dll', 'aa_xr_config32.exe') +
              @(Get-ChildItem (Join-Path $GameDir 'AsylumVR') -File -Recurse | ForEach-Object { $_.FullName.Substring($GameDir.Length + 1) -replace '\\', '/' })) {
    $p = Join-Path $GameDir $f
    $records += @{ name = $f; sha256 = (Get-FileHash -LiteralPath $p).Hash; bytes = (Get-Item -LiteralPath $p).Length }
}
$manifest = @{ version = (Get-Content (Join-Path $Here 'files\version.txt') -Raw).Trim(); game = $GameDir;
               installed = (Get-Date -Format o); files = $records } | ConvertTo-Json -Depth 5
[IO.File]::WriteAllText((Join-Path $GameDir 'asylumvr_manifest.json'), $manifest, (New-Object Text.UTF8Encoding($false)))
Say '  Mod files copied.' Green

# ---- 4. the mod's tested settings (first install only: never over the player's own) --------------------------
New-Item -ItemType Directory -Force $DataDir | Out-Null
if (-not (Test-Path (Join-Path $DataDir 'aa_rendering.ini'))) {
    Copy-Item (Join-Path $Here 'files\aa_rendering.ini') (Join-Path $DataDir 'aa_rendering.ini')
    Say '  Tested VR settings copied (change them later in the in-game VR menu).' Green
} else {
    Say '  Your VR settings from an earlier install are kept.'
}

# ---- 5. shortcuts -----------------------------------------------------------------------------------
if (-not $NoShortcuts) {
    Step '4. Making the shortcuts'
    $ws = New-Object -ComObject WScript.Shell
    $places = @([Environment]::GetFolderPath('Desktop'), (Join-Path ([Environment]::GetFolderPath('Programs')) 'Batman Asylum VR'))
    New-Item -ItemType Directory -Force $places[1] | Out-Null
    foreach ($p in $places) {
        foreach ($s in @(@('Batman Asylum VR', 'Batman Asylum VR.bat'), @('Batman Asylum VR (SteamVR)', 'Batman Asylum VR (SteamVR).bat'),
                         @('Uninstall Batman Asylum VR', 'Uninstall-BatmanAsylumVR.bat'))) {
            if ($p -eq $places[0] -and $s[0] -like 'Uninstall*') { continue }   # the desktop gets the two launchers only
            $lnk = $ws.CreateShortcut((Join-Path $p ($s[0] + '.lnk')))
            $lnk.TargetPath = Join-Path $GameDir $s[1]
            $lnk.WorkingDirectory = $GameDir
            $lnk.IconLocation = (Join-Path $GameDir $Mod.Exe) + ',0'
            $lnk.Save()
        }
    }
    Say '  "Batman Asylum VR" is on your desktop and in the Start menu.' Green
}

Write-Host ''
Write-Host '  Done. Start your PC VR software, then start the game with the "Batman Asylum VR" shortcut.' -ForegroundColor White
Write-Host '  In the game, hold both thumbsticks down (or press F10) for the VR menu.'
Write-Host ''
