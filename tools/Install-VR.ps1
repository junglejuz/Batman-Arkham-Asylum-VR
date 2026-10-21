param([string]$GameDir='E:/Games/Steam/steamapps/common/Batman Arkham Asylum GOTY')
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$bin=[IO.Path]::GetFullPath((Join-Path $GameDir 'Binaries'))
if(Get-Process 'ShippingPC-BmGame' -ErrorAction SilentlyContinue) {throw 'Close Asylum before installation.'}
if((Get-FileHash -LiteralPath (Join-Path $bin 'ShippingPC-BmGame.exe')).Hash -notin @('4dac1f5e2ac6710b7378fdce74601f616f4753e3756cb5fda63c7519cc2eb028','ed65ce3c52baca7adaea00cb0e5def3d8a78bd05f7ceacd68a68462e92d8dab2')) {throw 'Unsupported Asylum executable.'}
if(Test-Path -LiteralPath (Join-Path $env:LOCALAPPDATA 'Batman_Asylum/profile-active.json')) {throw 'An Asylum graphics profile is still active.'}
$manifestPath=Join-Path $bin 'asylumvr_manifest.json'
$previous=$null
if(Test-Path -LiteralPath $manifestPath) {$previous=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json}
$sources=[ordered]@{
 'd3d9.dll'='build-native9/Release/d3d9.dll'
 'xinput1_3.dll'='build-native9/Release/xinput1_3.dll'
 'batman_asylum.dll'='build-native9/Release/batman_asylum.dll'
 'aa_xr_config32.exe'='build-native9/Release/aa_xr_config32.exe'
 'AsylumVR/Launch-VR.ps1'='tools/Launch-VR.ps1'
 'AsylumVR/Graphics-Profile.ps1'='tools/Graphics-Profile.ps1'
 'AsylumVR/Render-Size.ps1'='tools/Render-Size.ps1'
 'AsylumVR/DLSS/aa_dlss64.exe'='build64/Release/aa_dlss64.exe'
 'AsylumVR/DLSS/nvngx_dlss.dll'='build64/Release/nvngx_dlss.dll'
 'AsylumVR/Uninstall-VR.ps1'='tools/Uninstall-VR.ps1'
 'AsylumVR/LICENSE-bioshock-vr.txt'='LICENSE-bioshock-vr.txt'
 'AsylumVR/LICENSE-luma-framework.md'='LICENSE-luma-framework.md'
 'AsylumVR/LICENSE-MinHook.txt'='third_party/minhook/LICENSE.txt'
 'AsylumVR/LICENSE-ImGui.txt'='third_party/imgui/LICENSE.txt'
 'AsylumVR/LICENSE-OpenXR.txt'='third_party/OpenXR-SDK/LICENSE'
 'AsylumVR/LICENSE-NVAPI.txt'='LICENSE-NVAPI.txt'
 'Batman Asylum VR.bat'='tools/Batman Asylum VR.bat'
 'Batman Asylum VR (SteamVR).bat'='tools/Batman Asylum VR (SteamVR).bat'
 'Batman Asylum VR (Previous Bridge).bat'='tools/Batman Asylum VR (Previous Bridge).bat'
}
foreach($name in $sources.Keys) {
 if(!(Test-Path -LiteralPath (Join-Path $project $sources[$name]))) {throw "Missing source: $name"}
 $dest=Join-Path $bin $name
 if(Test-Path -LiteralPath $dest) {
  $owned=$previous.files | Where-Object name -EQ $name
  if(!$owned -or (Get-FileHash -LiteralPath $dest).Hash -ne $owned.sha256) {throw "Existing file is not our unchanged file: $dest"}
 }
}
$backup=Join-Path $project ('re/backups/install-'+(Get-Date -Format yyyyMMdd_HHmmss_fff))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
. (Join-Path $PSScriptRoot 'Graphics-Profile.ps1')
$baseline=@();$i=0
foreach($path in (Get-AsylumConfigPaths $GameDir)) {
 $dest=Join-Path $backup ("config-$i-"+[IO.Path]::GetFileName($path));$i++
 Copy-Item -LiteralPath $path -Destination $dest
 $baseline+=@{path=$path;backup=$dest;sha256=(Get-FileHash -LiteralPath $path).Hash}
}
$baseline | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $backup 'graphics.json') -Encoding UTF8
if($previous) {
 foreach($record in $previous.files) {
  $dest=Join-Path $backup $record.name;New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force | Out-Null
  Copy-Item -LiteralPath (Join-Path $bin $record.name) -Destination $dest
 }
 Copy-Item -LiteralPath $manifestPath -Destination $backup
}
$records=@()
foreach($name in $sources.Keys) {
 $src=Join-Path $project $sources[$name];$dest=Join-Path $bin $name
 New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force | Out-Null
 Copy-Item -LiteralPath $src -Destination $dest
 $hash=(Get-FileHash -LiteralPath $dest).Hash
 if($hash -ne (Get-FileHash -LiteralPath $src).Hash) {throw "Install hash mismatch: $name"}
 $records+=@{name=$name;sha256=$hash;bytes=(Get-Item -LiteralPath $dest).Length}
}
foreach($record in $baseline) {if((Get-FileHash -LiteralPath $record.path).Hash -ne $record.sha256) {throw 'Installation changed game settings.'}}
@{version='0.1.0-A2p';game=$bin;installed=(Get-Date -Format o);backup=$backup;files=$records;capabilities='A2p toolkit effect stepper: panel TESTS button or aa fxstep hides each 3D draw kind (vertex shader) 250 ms in turn, logs the generation each step starts, then saves the replay; A2o eye-same finder KIND summary (per vertex shader: named inputs that change vs stay the same between the eyes, bytecode saved); FixStaleParticles default OFF and skips camera-attached zero rows; Floating screen options removed from the panel; A2n engine-cached eye-relative transforms: right-eye scene draws whose LocalToWorld translation equals a left-eye one get the eye shift (left minus right eye origin) for that draw only, main scene target only, FixStaleParticles default on, aa stale on|off; A2m eye-same finder lists which NAMED vertex inputs differ between the eyes and every draw whose non-camera inputs differ (SUSPECT lines), plus LEFT-only draws; A2l rings matched at the game drawn position need no learned texture (only the direction fallback does); eye-same finder pairs draws by kind not list order; A2k grapple rings matched at the game drawn position (game projection includes the head), last projected points held 250 ms for frames without Canvas.Project, on-point placement falls back to the eye camera; every capture (panel button too) runs the eye-same finder; A2j toolkit eye-same draw finder (F2 or aa eyesame: one LEFT/RIGHT scene pair compared draw by draw, identical vertex constants listed with constant-table names, bytecode saved); grapple pointer 26 deg (GrapplePointerDegrees); A2i edge grapple rings: off-screen/behind Canvas.Project points kept with game screen direction (output untouched), edge ring pieces matched by direction, placed ON the point when both eyes see it else as a head-locked pointer 32deg toward it at HUD distance, never left on the HUD sheet; A2h grapple target from the game grapple gun finders (NoAim/WithAim, observe only): world offset learned from accepted Canvas.Project anchors, fresh finder point anchored per pass; every-caller census of Project/WorldToPixel/UIScreenObject.Project; unanchored grapple pieces counted against finder state, optional HideTargetlessGrapple (default off); A2g Canvas.WorldToPixel second marker route: exact native registry/helper/caller and ret8 ABI, head-free pixel XY and same-frame world cards after original success only; original invalid/behind/offscreen/foreign outputs preserved, separate passive counters; accepted A2f native detective callout owner projection correction: exact actor HUD caller/consumer guards, shared head-centre ray into real HUD plane with same-generation poses and recenter reference; original Z/W and game line/ring/bend/material generation retained, no CPU orange matcher or vertex shear; plane reset/recenter invalidation, missing input stock fallback; target Canvas.Project head-free world anchors and exact per-vertex camera-facing grapple cards; paired original widget capture after DLSS/AFW, bounded sprite/prompt runs, owned slots5+6; literal-position black fade family; authored engine fade envelope drives full-field world blackout before menu HUD after DLSS/AFW; exact pair alpha, reset/missing/stale clear and live toggle; authored10tri AA rectangle/core-area proof, transparent and neutral fringes, COLOR0/COLOR1 coverage; opaque dense edge bat sweep and16:9 stage bounds; City-style HUD coverage for multiply/additive/erase/mask draws and mask-only write protection; owned managed CPU buffer access and sparse non-DYNAMIC uploads for native UI geometry, recorded retail SHORT2/ConstantColor fades and bat mesh regression; fixed stage bat continuation during contracting bounds; target gamma LOG/EXP UI and multiplicative fade matching; read-only ALWAYS depth UI support; F2 captures still plus replay under desktop OR headset focus; bounded actual rejected UI shader/constant/declaration evidence; original bat geometry captured at XR output size with1.6x fixed stage magnification and full-asymmetric-FOV near0.8m plane after HUD/DLSS/AFW; large uniform black rectangle fade removal incl target letterboxed stage, bounded CPU upload geometry shadows, separate GPU transport and retail capture counters; Gameplay-only particle correction from guarded engine IsMenuLevel state carried per exact eye slot; front-end retains original facing; Named DX9 sprite facing uses shared game-camera axes/centre with real eye projection; per-draw exact constant restore, bounded shader cache, stateblock/reset safety and live StableParticles option; matched RGBA/BGRA/BGRX sRGB room-screen gamma; Default native Direct3D9Ex to Direct3D11 shared GPU transport; no9On12, 11On12 or D3D12 rendering device on native route; managed CPU/GPU twins, locks and dirty updates, child identity, stateblock/reset recovery; INTZ physical depth transported at24bit precision and decoded R32Float; previous-bridge launcher; ABI8 unique DLSS sessions and offline explicit teardown; Stock native path bypasses unused depth transport and DLSS texture metadata/sampler work; on-demand still depth retained; ratio-sized temporal jitter; observed NVIDIA DLSS override reported read-only; Feature-gated exact dynamic depth bias with per-variant COM handles, unwrapped Set/Create/Reset/Clear bindings and static cached-blob compatibility; D24/D32 forward/reverse GPU references incl integer extremes; Bounded exact graphics pipeline reuse (shader content/full field identity; 512 entries, 16MiB keys), real GPU repeated/variant/eviction/reset validation; Unique AFW navigation controls; bounded canonical-identity graphics wrapper reuse and reset cleanup; process VA/video memory telemetry; AFW physical presentation every frame; Mutation-invalidated live D3D9 scene tracking cache, StateBlock Apply/reset safety; nonblocking per-eye scene CPU/GPU elapsed budgets; timed same-quality tracking off/on/off comparison; Base and generated graphics profiles cover stock62fps cap with byte-exact exit rollback; owned D3D9 desktop interval IMMEDIATE; named CPU budget and startup profile telemetry; native same-tick stereo default; physical scene depth preserved before clears/rebinds, raw-DS projection retained; optional depth-based AFW with relative cameras; native DLSS shared-fence overlap with deferred acquired LEFT and atomic stock pair fallback; per-eye DLSS histories and precise relative-camera motion, fixed output and mode-sized engine input; mild scene texture LOD correction; HUD after DLSS and AFW; exact asymmetric lens, upright skippable movies; fitted physical mirror and persistent controls; up to ten seconds of final paired-eye replay at960px per eye/10fps, background JPEG export and local review; timed safe/overlap/safe DLSS performance comparison; headset performance acceptance pending'} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $project 're/evidence/installed-build.json') -Force
Write-Host "Asylum A2p installed and hash-verified: $($records.Count) owned files."
