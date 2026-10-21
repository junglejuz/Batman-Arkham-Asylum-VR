param([switch]$SteamVR,[switch]$LegacyBridge)
$ErrorActionPreference='Stop'
$env:AA_GRAPHICS_BRIDGE=if($LegacyBridge){'9on12'}else{'native9ex'}
$bin=Split-Path $PSScriptRoot -Parent
$game=Split-Path $bin -Parent
$exe=Join-Path $bin 'ShippingPC-BmGame.exe'
$data=Join-Path $env:LOCALAPPDATA 'Batman_Asylum'
$journal=Join-Path $data 'profile-active.json'
$manifest=Get-Content -LiteralPath (Join-Path $bin 'asylumvr_manifest.json') -Raw | ConvertFrom-Json
foreach($record in $manifest.files) {
 if((Get-FileHash -LiteralPath (Join-Path $bin $record.name)).Hash -ne $record.sha256) {throw "A mod file in the game folder is missing or changed ($($record.name)). Run Install-BatmanAsylumVR.bat again."}
}
# Steam GOTY, and the same build after the Joker DLC Patcher (two bytes in its DLC flag routine; Godoy301 JOKERCOMPAT).
if((Get-FileHash -LiteralPath $exe).Hash -notin @('4dac1f5e2ac6710b7378fdce74601f616f4753e3756cb5fda63c7519cc2eb028','ed65ce3c52baca7adaea00cb0e5def3d8a78bd05f7ceacd68a68462e92d8dab2')) {throw 'This copy of the game is a different version from the one the mod was made for (Steam, Game of the Year Edition, unpatched or with the Joker DLC Patcher).'}
$games=@(Get-Process -ErrorAction SilentlyContinue | Where-Object ProcessName -Match '^(ShippingPC-BmGame|BatmanOrigins|BatmanAC|BatmanAK|ACUnity|sekiro|DarkSoulsIII|armoredcore6|starwarsbattlefrontii|BioShockInfinite|BioshockHD)$')
if($games.Count) {throw "Close the running game first: $($games.ProcessName -join ', ')"}
New-Item -ItemType Directory -Path $data -Force | Out-Null
. (Join-Path $PSScriptRoot 'Graphics-Profile.ps1')
. (Join-Path $PSScriptRoot 'Render-Size.ps1')
if(Test-Path -LiteralPath $journal) {Restore-AsylumProfile (Get-Content -LiteralPath $journal -Raw | ConvertFrom-Json);Remove-Item -LiteralPath $journal}
Remove-AsylumProfileBackups $data
function Find-SteamVR32 {
 $steam=(Get-ItemProperty 'HKCU:/SOFTWARE/Valve/Steam' -ErrorAction SilentlyContinue).SteamPath
 $candidates=@()
 # City SVRACTIVE: first the SteamVR install SteamVR itself treats as active (a player with two SteamVR installs).
 try {$active=(Get-Content -Raw (Join-Path $env:LOCALAPPDATA 'openvr/openvrpaths.vrpath') | ConvertFrom-Json).runtime[0]
      if($active) {$candidates+=Join-Path $active 'steamxr_win32.json'}} catch {}
 $candidates+=Join-Path (Split-Path $game -Parent) 'SteamVR/steamxr_win32.json'
 if($steam) {$candidates+=(Join-Path $steam 'steamapps/common/SteamVR/steamxr_win32.json')}
 $candidates | Where-Object {Test-Path -LiteralPath $_} | Select-Object -First 1
}
if($SteamVR) {
 $runtime=Find-SteamVR32
 if(!$runtime) {throw 'SteamVR was not found. Install SteamVR from Steam and start it once, then try again.'}
 $env:XR_RUNTIME_JSON=$runtime
}
$recommend=& (Join-Path $bin 'aa_xr_config32.exe')
# Exit 4 = the PC's VR software has no 32-bit runtime (Pimax): use SteamVR's 32-bit one for this launch only.
if($LASTEXITCODE -eq 4 -and !$SteamVR) {
 $runtime=Find-SteamVR32
 if($runtime) {
  Write-Host 'Your VR software has no 32-bit OpenXR runtime - using SteamVR''s for this launch.'
  $env:XR_RUNTIME_JSON=$runtime
  $recommend=& (Join-Path $bin 'aa_xr_config32.exe')
 }
}
if($LASTEXITCODE -ne 0) {throw 'The headset could not be asked for its picture size. Start your PC VR software, connect the headset, then start this shortcut again.'}
$size=$recommend | ConvertFrom-Json
Add-Type -AssemblyName System.Windows.Forms
$work=[Windows.Forms.Screen]::PrimaryScreen.WorkingArea
$settings=Join-Path $data 'aa_rendering.ini'
$scale=1.0
if(Test-Path -LiteralPath $settings) {
 $match=[regex]::Match([IO.File]::ReadAllText($settings),'(?im)^ResolutionScale\s*=\s*([0-9.]+)\s*$')
 if($match.Success) {$scale=[double]::Parse($match.Groups[1].Value,[Globalization.CultureInfo]::InvariantCulture);$scale=[Math]::Max(.6,[Math]::Min(1.8,$scale))}
}
$dlssMode=3;$compatible=1;$lensCrop=.8
if(Test-Path -LiteralPath $settings) {
 $saved=[IO.File]::ReadAllText($settings)
 $match=[regex]::Match($saved,'(?im)^DLSSMode\s*=\s*([0-5])\s*$');if($match.Success){$dlssMode=[int]$match.Groups[1].Value}
 $match=[regex]::Match($saved,'(?im)^LightingCompatibility\s*=\s*([01])\s*$');if($match.Success){$compatible=[int]$match.Groups[1].Value}
 $match=[regex]::Match($saved,'(?im)^LensCrop\s*=\s*([0-9.]+)\s*$')
 if($match.Success){$hint=[double]::Parse($match.Groups[1].Value,[Globalization.CultureInfo]::InvariantCulture);if($hint -ge .4 -and $hint -le 1){$lensCrop=$hint}}
}
if(!$compatible){$lensCrop=1.0}
$render=Get-AsylumDLSSRenderSize ([int]$size.width) ([int]$size.height) $scale $dlssMode $lensCrop
$w=[int]$render.width;$h=[int]$render.height
$backup=Join-Path $data ('profiles/'+(Get-Date -Format yyyyMMdd_HHmmss_fff))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$files=@();$i=0
$configs=@(Get-AsylumConfigPaths $game)
foreach($path in $configs) {if(!(Test-Path -LiteralPath $path)) {throw "Game configuration absent: $path"}}
foreach($path in (Select-AsylumWritableConfigs $game $configs)) {
 $copy=Join-Path $backup ("$i-"+[IO.Path]::GetFileName($path));$i++
 [IO.File]::WriteAllBytes($copy,[IO.File]::ReadAllBytes($path))
 $files+=@{path=$path;backup=$copy;sha256=(Get-FileHash -LiteralPath $path).Hash}
}
$profile=@{files=$files;runtime=$size.runtime;render=@($w,$h);output=@($size.width,$size.height);build=$manifest.version}
$profile | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $journal -Encoding UTF8
try {
 foreach($path in $files.path) {Set-AsylumVRProfile $path $w $h}
 $observed=@{build=$manifest.version;beforeLaunch=@(Get-AsylumProfileStatus $files.path)}
 $statusPath=Join-Path $data 'profile-last.json'
 $observed | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $statusPath -Encoding UTF8
 Write-Host "Batman Asylum VR $($manifest.version): $($size.runtime), game picture ${w}x${h}, headset $($size.width)x$($size.height)."
 Write-Host 'In the game, hold both thumbsticks down (or press F10) for the VR menu. Leave this window open: it puts your graphics settings back when the game closes.'
 $process=Start-Process -FilePath $exe -WorkingDirectory $bin -ArgumentList @('-bavr','-windowed',"ResX=$w","ResY=$h","-AARenderX=$w","-AARenderY=$h","-AAOutputX=$($size.width)","-AAOutputY=$($size.height)","-AAUpscaleX=$($render.outputWidth)","-AAUpscaleY=$($render.outputHeight)") -PassThru
 $observeAt=[DateTime]::UtcNow.AddSeconds(15);$observedStartup=$false
 while(!$process.WaitForExit(1000)) {
  if(!$observedStartup -and [DateTime]::UtcNow -ge $observeAt) {
   $observedStartup=$true
   try {$observed.afterStartup=@(Get-AsylumProfileStatus $files.path);$observed | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $statusPath -Encoding UTF8}
   catch {Write-Warning "Could not record startup profile: $_"}
  }
 }
 while(Get-Process 'ShippingPC-BmGame' -ErrorAction SilentlyContinue) {Start-Sleep -Seconds 1}
} finally {
 if(!(Get-Process 'ShippingPC-BmGame' -ErrorAction SilentlyContinue)) {
  if($observed) {try {$observed.beforeRestore=@(Get-AsylumProfileStatus $files.path);$observed | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $statusPath -Encoding UTF8} catch {Write-Warning "Could not record exit profile: $_"}}
  Restore-AsylumProfile $profile;Remove-Item -LiteralPath $journal;Remove-AsylumProfileBackups $data;Write-Host 'Original graphics settings restored.'
 }
}
