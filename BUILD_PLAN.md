## RELEASE 0.1.6 (2026-10-28, JJ: "packaging these up for GitHub ... including all the changes we have done from the pull request
## up until now")
Contents since 0.1.5: Godoy301's PR #1 (first-person head aim + head-locked HUD, STEREOPOSE flat-frame fix, world scale; HEADAIM,
AIMLEVEL), CAMPOS, LEANBACK. Version 0.1.6 (CMake, BUILD_ID Asylum-0.1.6, make_package, panel title). Built in build-headaim (Release),
14/14 checks; INSTALLED (batman_asylum.dll 0D062A75, manifest version 0.1.6, 0 mismatches; previous in backups/pre-0.1.6-2026-10-28).
make_package now takes batman_asylum.dll from build-headaim (same CMake options as build-native9); proxies + config tool unchanged
since 0.1.5 and ship from build-native9 as tested. HEADSET (JJ): head aim round 2 (AIMLEVEL) used in his sessions; CAMPOS / LEANBACK
not explicitly confirmed. Committed as a merge with the PR head (dacc119) as second parent so GitHub records PR #1 as merged with
Godoy301's commits. README: head aim / world scale / camera position / lean-back lines + credit. Zip sha256
6e72da6452ce28edec7210ee10fcd7ab4c355c671db3f012a93dda3b1bed5319.

## CHECK vs City / Origins fixes (2026-10-28, JJ: "check Arkham Asylum for all these same issues with the zoom mode, detective mode
## and the grapple hooks becoming available when you look instead of move the camera")
STATIC / SOURCE only (no Asylum launch this session):
- Zoom: DefaultInput.ini XboxTypeS_RightThumbstick = "DebugMenuSelectA | ToggleCameraZoom" (as City / Origins). The City / Origins bug
  came from their third-person head-look boom hold; Asylum has no third-person head steering, so its zoom is the game's own - no
  issue until grapple-look is ported (then port ZOOMFOLLOW with it).
- Grapple points where you look: missing in third person. Asylum's head aim (Godoy301, head_aim.h / first_person.cpp) runs ONLY in
  first person (Gate::off when !firstPerson), always on there, HUD on the head - so in first person the game camera already follows
  the head. Port needed for third person: City/Origins LOOKSTEER (controller pitch / yaw steered without the trigger, drawn from the
  body basis, boom hold + ZOOMFOLLOW), markers9 reference = the steered game camera (MARKERSTEER), MARKERLAG / MARKERPIN in markers9
  (Asylum's own A2c..A2k marker system: Canvas.Project caller A58947 + WorldToPixel 658b73, D3D9 widget replay).
- Detective mode vignette: never examined in Asylum (no notes, no fix). Asylum's materials are D3D9 bytecode through the native9
  bridge, so City / Origins' D3D11 rebuild does not drop in; needs a census (SCREENMARK-style: scanner/detective vs normal full-screen
  pixel shaders) and a D3D9 rebuild with overlays at the shared position.
- Labels / detective vitals: already fixed (A2f, JJ "Looks great").

## LEANBACK (2026-10-27)

JJ (Origins, same first-person code): "When in first person mode and I move my physical head back my virtual camera is moving through
Batman's head". Looked up: Origins BUILD_PLAN "LEANBACK" + first_person.cpp clamp_eye / scene.cpp (the fix being ported); this file
FIRSTPERSON / HEADLOCK / HEADAIM. Nothing else in our notes on the headset's position pushing the lens into the body.
Cause (SOURCE): first_person::apply puts the game camera at his eye point + 25 cm forward (level); native_stereo ctor then adds the
headset pose (delta(eyes[e]), position included) on top, so leaning back 25 cm puts the lens inside his head.
Change (SOURCE): first_person::clamp_eye + native_stereo lean_clamp - after each eye's final view is built (right after
`v=mul(v,delta(eyes[e]))`), in first person (apply reached its end this pair) its origin may come no nearer than "closest to his face
when you lean back" (default 15 cm, 0-40, never more than "camera in front of his face") to his eye point (before the push) along the
pair's level push direction (the forward of the view apply pushed along = the head-aimed body when head aim drives it, else the game
camera). Only that component is limited; sideways / up / down stay free; both eyes alike (stereo kept). Same clamp on the marker eye
views and AFW's other-eye view; the particle centre delta subtracts the shift. Units 90 UU/m = apply's push. Head aim untouched (the
clamp only moves the translation row after it). Saved aa_rendering.ini [AsylumVR] FirstPersonLeanMinCm (Asylum's key style); panel
FIRST PERSON slider under "camera up / down"; command `aa fp lean <cm>`; TESTS line "lean-back held N" (counts eye + marker views).
Built in build-headaim (Release, no warnings), 14/14 checks. INSTALLED 2026-10-27 (game not running): batman_asylum.dll only, sha256
C87FBF2A..., 2108416 bytes, launcher manifest entry updated, all 15 listed files re-checked; previous AIMLEVEL DLL + manifest in
backups/pre-LEANBACK-2026-10-27. Build id unchanged (Asylum-0.1.5-HEADAIM). Not released. HEADSET pending: in first person lean back
slowly - the view stops ~15 cm in front of his face instead of entering his head; lean sideways / up / down still moves freely.

## HEADAIM - Godoy301's PR #1 merged locally (JJ 2026-10-25: "this guy made some changes to our Arkham Asylum mod. can we incorporate these into our one, and also into the other 3 too?")

Looked up: GitHub PR #1 (github.com/junglejuz/Batman-Arkham-Asylum-VR/pull/1, branch Godoy301:feat/first-person-head-aim, head
dacc119, 8 commits; their notes in RE_NOTES "HEAD-AIM".."HEAD-AIM5" and HEAD_AIM_TESTING.md); this file FIRSTPERSON / HEADLOCK /
JOKERCOMPAT; memory arkham-first-person-asylum-origins, github-repos; VR_ESSENTIALS §4 + playbook 09 #pose-timing-contract and
STR-002 (one pose sample for both eyes, publish pairs whole) = exactly what the flash fix restores; Origins scene.cpp VIEWDEBOUNCE3
(same one-frame "no fresh pose generation" symptom, masked there). Nothing else in our notes on headset-aimed first person.

What the PR brings (SOURCE, their CPU tests; the contributor reports HEAD-AIM3 aim and the HEAD-AIM4 flash fix working in their headset):
- **Head aim** (FIRST PERSON -> "aim and HUD follow the headset", on by default, `FirstPersonLookAim`): writes the headset's look
  direction into Controller.Rotation (+0x60) on the game thread, removing its own previous contribution each frame so nothing
  accumulates; stick yaw stays additive, stick pitch is zeroed while it is on. The HUD becomes one head-locked plane for both eyes
  (detective callouts follow it). Gated off in the main menu, with the VR panel open, without a pose or Batman, camera > 12 m away.
- **STEREOPOSE flash fix**: head + both eyes + FOVs published as one sample after xrLocateViews (core openxr_runtime +
  stereo_pose_mailbox.h). Before, the game thread could read a new head before its eyes = 1-2 flat frames, often in smoke.
- **World scale** 0.50x-2.00x at the top of the panel (`WorldScale`, 1.00 = 90 UU/m as before). WORLDSCALEDIR (JJ 2026-10-25:
  "Make all the mods so that when you raise the slider it reduces the size of the world"): flipped to 90 x scale UU/m, label
  "world scale  (higher = the world looks smaller)" like City / Origins; Knight's three sliders already went that way (units per
  metre / eye separation). A `WorldScale` saved by the PR's own builds now means the inverse size.
- Menu-reader diagnostics, [aa head aim] log lines, head_aim.csv in each F2 capture, 3 new CPU tests.

Merge (2026-10-25, uncommitted, on top of 0.1.5 + CAMPOS): the PR branch starts at 0.1.4 (merge base 5991146), so it was merged
3-way against that base (a first attempt against 0.1.5 rolled back 0.1.5 files for a few minutes; repaired, see memory
`contributor-pr-merge-base`). Conflicts only in first_person.cpp with CAMPOS: CAMPOS runs with first person off, head aim only with
it on; head-aim bookkeeping (release_aim / aim_state / pitch gate) only when first person is on. Not taken: the PR's README line
("this fork's development change"); README gets plain lines + credit at release. Kept: their workflow, HEAD_AIM_TESTING.md, tests.
Build id Asylum-0.1.5-HEADAIM. Built in build-headaim (Release, no warnings): 14/14 checks pass. INSTALLED 2026-10-25 (JJ "Yes,
install"): batman_asylum.dll only (proxies unchanged), sha256 94FEC0AA..., launcher manifest updated and every listed file
re-checked; previous CAMPOS DLL + manifest in backups/pre-HEADAIM-2026-10-25. NOT released; PR #1 still open on GitHub.

### HEADAIM round 2 (JJ 2026-10-26, first headset session)
JJ: with "aim and HUD follow the headset" ticked in first person "you can get stuck looking down at the ground and you have to
untick that or tick it in order to be able to release it". LIVE log 23:08-23:09: every VR-panel open = "release ... VR panel owns
input", every close = "active via validated first-person pawn" with a FRESH tracker; Tracker::remove kept the game's pitch at
take-over (committed ? lastBody.pitch : current) and stick pitch is blocked, so a down-looking take-over stayed down.
AIMLEVEL fix (head_aim.h): while the headset owns up / down the body pitch is always level (0); release restores level too. No
feedback loop (body pitch no longer reads the engine). Regression in head_aim_check (take-over at -30 deg, x3).
Panel (JJ "What are the full view sliders in the view menu? Do we need them still? The world scale slider should go into the view
menu"): world scale moved into VIEW (above camera position), as City / Origins; the "Full view" safe-lighting / exact-lens pair
(= centred enclosing lens vs true off-centre lens, which loses lights in one eye) left VIEW for the TESTS drawer (LENSTEST); JJ's
ini has LightingCompatibility=1, so nothing changes for him. Built, 14/14 checks. INSTALLED 2026-10-26 after JJ closed the game
(sha256 85591E3A..., manifest updated + every file re-checked; previous build in backups/pre-AIMLEVEL-2026-10-26). HEADSET pending.

### TEST PLAN HEADAIM (JJ drives, one launch; CAMPOS tests can share it)
| # | what JJ does | what it proves | how to judge |
|---|---|---|---|
| 1 | gameplay, third person, walk through smoke / steam for a minute moving the head | flash fix | no flat frames; log `[aa native] ... flatDraws` stays ~0 |
| 2 | panel top: world scale 0.75, 1.25, Reset | world scale | 0.75 = larger world, 1.25 = smaller; grapple + detective markers stay on their targets |
| 3 | FIRST PERSON on (aim box ticked): look at an enemy, throw a batarang; look at a grapple point | head aim | it goes where you look; grapple marker on the point |
| 4 | stick left / right, then up / down | stick rules | left / right turns; up / down does nothing |
| 5 | look around in first person | head-locked HUD | HUD stays in front, same depth in both eyes |
| 6 | untick the aim box | restore | HUD back in the room, stick up / down works again |
| 7 | pause menu, map, back to main menu with first person on | gates | no jumps, normal menus; Asylum has no pause signal, so note what aim does behind pause |

## CAMPOS - adjustable third-person camera position from Arkham Knight (JJ 2026-10-25: "put the adjustable camera from our Arkham knight mod panel into the other 3 arkham mods")

Looked up: AKVR TPOFFSET (akvr/src/camera.cpp TPOFFSET, hooks.cpp VIEW "camera position"; akvr_native/NOTES.md "TPPITCH + TPOFFSET
2026-10-18", released in AK 0.3.1); this game's FIRSTPERSON / FPGATE (the camera mover this rides on); memory
arkham-first-person-asylum-origins, batman-city-first-person, origins-install-manifest. Playbook / framework: nothing new needed - a
translation of the head-free game camera at the scene-view inputs, the engine rebuilds culling from it (same route as first person).
AK's TPPITCH look-down limit was NOT ported (it is a separate AK-only control; JJ asked for the camera position).

What it does (SOURCE): panel VIEW -> "camera position  (third person)" (open the first time): left / right, down / up, back / forward,
-300..+300 cm each, "Reset position". The game camera's origin moves along its LEVEL right / LEVEL forward and world up (AK's frame,
so it never swings with the head or tilts with the camera's pitch). Applied to the head-free game camera before the eye offsets, so
both eyes, markers and AFW follow it. Only in third-person gameplay: off in first person and through first person's FPGATE checks
(menus, no Batman found, game camera more than 12 m from Batman = main menu stand-in, distant cutscenes). Costs
nothing while all three are 0 (default). The game's own camera collision does not know about the shift: big values can put the
camera into walls (same as AK).
Asylum has no pause signal (as first person): the shift stays behind the pause menu.
Saved: aa_rendering.ini [AsylumVR] CameraRightCm / CameraUpCm / CameraForwardCm. Command: `aa fp cam R U F` (cm). Status: "moved frames N" under TESTS.

Built (Release, no warnings), offline checks pass; installed batman_asylum.dll into JJ's game folder 2026-10-25 (previous DLL + manifest in
backups/pre-CAMPOS-2026-10-25; launcher manifest hash updated and re-checked). Not released. HEADSET pending.

### TEST PLAN CAMPOS (JJ drives, one launch, no restart needed)
| # | what JJ does | what it proves | how to judge |
|---|---|---|---|
| 1 | gameplay, panel VIEW -> camera position: left / right to +60 | the shift happens, along the level right | Batman slides to the left of the view, horizon stays level |
| 2 | turn with the stick and with the head | frame is the game camera's heading, not the head | offset stays to the side of Batman on stick turns; no swing on head turns |
| 3 | down / up +50, back / forward -100 | the other two axes | camera higher / further back; looking down does not tilt the shift |
| 4 | pause menu, map, first person on | the gates | game camera unchanged in menus and first person |
| 5 | Reset position | defaults restore | the game's own framing |

## RELEASE 0.1.5 (2026-10-21, JJ: "ok deploy, update the github page with release notes")

PROFILECLEAN + JOKERCOMPAT (below). 0.1.4 was already public (2026-10-20), so this is 0.1.5 (CMake, build id
Asylum-0.1.5-PROFILECLEAN, panel title, make_package). 11/11 offline checks pass; package zip sha256
4f044fe921f5a78fed0b91abe235e94e8d8acc4160200acef14c4eadc8010abf, installed on JJ's game by the package installer
(game-folder dll = build-native9 dll, manifest 0.1.5). JJ's 62 old profiles folders go on his next launch.

## PROFILECLEAN + JOKERCOMPAT (2026-10-21, 0.1.4 local)

Looked up: WRITEDENIED (this file; journal + restore skip rules kept as they were), memory
`launcher-documents-write-denied`; playbook/guides: nothing relevant (launcher housekeeping, not a VR technique).

PROFILECLEAN (player report): every launch left `%LOCALAPPDATA%\Batman_Asylum\profiles\<time>` behind (JJ's PC: 62
folders). `Remove-AsylumProfileBackups` (Graphics-Profile.ps1) deletes the launcher's timestamp folders only AFTER
Restore-AsylumProfile succeeded and the journal is gone: at game exit and after recovering an interrupted launch. A failed
restore throws first, so its journal and backups stay. It also sweeps older builds' leftovers. Same cleanup in the
installer's and uninstaller's interrupted-launch recovery (uninstaller: matters with -KeepSettings). STATIC: scratch test
(tampered backup -> throw, all kept; good restore -> timestamp folders gone, other folder kept, file restored); 5 scripts
parse clean.

JOKERCOMPAT: Godoy301's fork (v0.1.4-joker-compat, commit 46dda1c, github.com/Godoy301/Batman-Arkham-Asylum-VR) found
the DLL's own exe hash check left the Joker DLC Patcher's exe flat. Ported by hand: launcher, both installers, DLL and
asylum_abi_check accept `ed65ce3c...8dab2` besides the Steam GOTY `4dac1f5e...b028`. The patcher changes 2 bytes at file
0x12153C6/0x12153CB = VA 0x16153C6/CB, inside the small DLC flag routine at 0x16153C0 (vcall +0x4A4, sets bit 19 of
+0x680); nothing of ours hooks or signature-checks near it (SOURCE grep). The DLL logs which exe it accepted. Not taken:
their CI workflows, Apply-JokerCompatBuild.ps1 payload updater, the extra D3D9-fallback diagnostic. LIVE on the Joker
exe: the fork's maintainer reports VR starts; not tested here (JJ's exe is stock). DLL built, ABI check passes.

## UPGRADEBLACK2 (2026-10-20 19:1x) - static-art rule caught a bat sweep's black hold
LIVE 19:03:59: "UPGRADEBLACK: static black menu artwork kept ... primitives10 vertices12" = the 10-triangle full-stage AA
rectangle, which classify() calls a bat only while a sweep runs (continuing, <= 2 s): the sweep's own black hold, never a
minute-long menu mesh. Fix: rectangles are exempt from the static rule (only the stage-covering / edge-sweep mesh path can
stay "bat" for a whole menu). Built, 11/11 checks; NOT yet installed (game running at build time). CORRECTION 2026-10-21: it IS installed (game folder dll pe-timestamp 0x6AD72150, written 19:07:44, manifest 19:10:50) but has not run yet (last launch 19:06-19:10 ran 0x6AD71DCF). JJ tested the first UPGRADEBLACK fix in that launch: the log caught the upgrades backdrop at 19:06:16, 19:08:21, 19:10:10. JJ 19:1x: first-person
camera "may have been an anomaly. It seems okay now"; log 19:05: head bone found, offset -7, -5.3, 13.1 UU.

## UPGRADEBLACK - upgrades screen completely black in the headset, fine on the desktop (JJ 2026-10-20, 0.1.4 local)

Looked up: batman_city TABRACE (black Tab screen = HUD gameplay-flag race; Asylum has no such flag path), Origins BUILD_PLAN
(TABRACE checked: not applicable), City MS menu-state / upgrades notes (MENUHOLD timing only), transitions9 A1x-A2c
(bat classifier). Nothing for a STILL menu mesh caught by the bat test.
LIVE 18:48:30-18:49:37 (JJ in the upgrades screen): batDraws rose ~54/s = one per frame, no bat transition; native pairs
normal. The bat classifier accepts any opaque-black mesh of 8+ triangles covering the stage; the full-view bat layer is
drawn over the menu (1.6x zoom, near plane), so a black backdrop / frame of the upgrades screen turned the view black. The
real sweep changes its mesh every frame (startup log: 288, 439, 686 ... primitives).
Fix (SOURCE, built, 11/11 checks incl. new "Static menu art" fixture, installed locally as 0.1.4): a bat-classified mesh +
placement seen unchanged 10 times (each within 250 ms) is menu artwork, kept in the menu. Log "UPGRADEBLACK: static black
menu artwork kept ... primitivesN verticesM" (first 8), status "staticMenuArt=on/count"; TESTS checkbox "Keep still black
menu art in the menu". HEADSET next: open upgrades (menu visible after a ~0.15 s blink), bat wipes unchanged.

## HEADLOCK - head goes through the first-person camera when running / gliding (JJ 2026-10-20, 0.1.3 HEADSET)

JJ: "First person works, but when you start running or flying, the head moves, so the camera needs to be linked to the
head movements so that the head doesn't intersect the camera at any point." LIVE 18:32 log: "first person head: bone read
faulted - steady eye point" EVERY frame, so the camera never followed the head (fixed collision eye point). STATIC cause:
GetBoneMatrix 0x91D2A0 hands the out matrix to the matrix multiply 0x58E560, which stores with movaps: the out buffer
must be 16-byte aligned (the game's caller aligns its stack); ours was a plain local. The offsets were right: the game's
own read casts controller +0x1A8 (= the pawn the mod finds) with IsA 0x7D94D0 and reads +0x348. Fix 1: alignas(16).
Fix 2, HEADLOCK (both games): the 0.08 s head smoothing may trail the real head bone by at most 3 UU (~3 cm), so a fast
lunge / glide cannot bring the head toward the camera (camera 25 cm in front of the face). Built, offline checks pass
(Asylum d3d9_gpu_transport intermittently fails at random stages while the GPU is 94% loaded by other apps - the released
0.1.3 DLL fails the same way now, it passed 3/3 this morning). Labelled 0.1.4, packaged, installed on JJ's game; NOT
released. HEADSET next: run + glide in first person; log must show "first person head: mesh ..., Bip01_Head - eye point"
and no "bone read faulted".

## RELEASE 0.1.3 (2026-10-20, JJ: "deploy both of those to github")

First person (FIRSTPERSON, below) + launcher access-denied fix (WRITEDENIED, below). Version 0.1.3 (CMake, package, panel title).
11/11 offline checks pass; package zip sha256 5a55cfbee38799a45d701d5636023579a04b0fe39e5e1d71bcc663c06fd6caa5, installed on
JJ's game by the package installer. First person HEADSET pending (shipped OFF by default, listed as new in Known issues).

## FIRSTPERSON - City's first-person mode (JJ 2026-10-20: "add the same first person mode we have for the other arkham games ... add it to origins too")

Looked up: memory batman-city-first-person + ak-first-person; batman_city BATMAN_CITY_PLAN FIRSTPERSON / HEADCAM / FPGATE and scene.cpp fp_update / fp_head_follow / fp_apply (the donor); memory origins-port-city-fixes-by-code-shape (method); Origins never got first person (ORIGINS_PLAN P7 planned only). Playbook/framework: City already cites CAM-001/CAM-013, FAIL-CAM-020, guide 13 (REFramework FirstPerson) - nothing new needed for a straight port.

What it is (City's, unchanged): panel FIRST PERSON (top) or both sticks clicked twice quickly; camera at Batman's eye point
(Location + EyeHeight) following his Bip01_Head bone (smoothed 0.08 s, capped 140 UU), 25 cm in front of his face (head
not hidden), look direction stays the game camera's. Gate (FPGATE): front-end menu level (engine IsMenuLevel, per pair); no pause signal in Asylum yet, so first person stays on behind the pause menu, or the game camera more than 12 m from him (main
menu stand-in pawn) - the setting stays on. Saved in aa_rendering.ini [AsylumVR] FirstPerson / FirstPersonForwardCm / FirstPersonUpCm / FirstPersonHead / FirstPersonHeadSmooth; the package ships it OFF. Commands `aa fp on|off|fwd N|up N|head 0|1|smooth S|status`.

STATIC RE 2026-10-20 (scratch scanner on the retail exe, City's functions found by identical code shape):
| | City | Asylum |
|---|---|---|
| GetPawnViewLocation | 0x7A6970 | 0x867C30: Location +0x54, EyeHeight +0x284 |
| GetViewRotation | 0x7BA650 | 0x87B650: Controller +0x1B4, Rotation +0x60 |
| Pawn.Mesh | +0x36C | +0x348 (game's own head read 0x8A1178; EyeHeight->Mesh 0xC4 as City) |
| Head bone | GetBoneLocation 0x850EE0, FName 0x620 | FName ctor 0x5D5620 (L"Bip01_Head",1,1; ret 0xC) -> MatchRefBone 0x90E8E0 (ret 8, SkeletalMesh +0x1B0) -> GetBoneMatrix 0x91D2A0 (out, index; ret 8), row 3 = world position. No GetBoneLocation of City's shape. |
Player -> controller (City +0x30) and controller -> pawn are found LIVE by the pawn's back-pointer (log line "first person:
player +0x.. = controller .., its +0x.. = Batman"). Hook: native_stereo.cpp calc() (first eye pass) computes the eye point; ctor() moves the validated view before the eye deltas, so markers / particles / AFW follow the moved camera and the between-eyes check still holds.

Built + offline checks pass + installed via the player package (local build, not released). HEADSET pending.

### TEST PLAN FIRSTPERSON (JJ drives, one launch)
| # | do | look for | log |
|---|---|---|---|
| 1 | VR shortcut, main menu | normal menu 3D (first person must NOT move the menu camera even if on) | "first person paused: ..." |
| 2 | gameplay, both sticks clicked twice quickly | view jumps to Batman's eyes, cowl not in view | "player +0x.. = controller", "head: name Bip01_Head", "eye point" |
| 3 | walk, run, jump/glide, a fight/takedown | camera stays at his head, no shaking, not inside his head | head frames rising, "steady eye point" count low |
| 4 | pause menu / map, then back | menu as before, first person back after | "paused: menu" / "active again" |
| 5 | panel: untick "camera follows his head" | steadier but head comes through camera on glides | - |
| 6 | anything wrong: F2 | capture | - |
Follow-ups: pawn never found -> widen the live scan (player range 0x20-0x80, controller 0x100-0x700); head misses -> log names the
reason (no mesh / no skeleton / no bone / cap).

## WRITEDENIED - launcher stuck on "Access denied" to a Documents config (player report, 2026-10-20)

Report (via JJ): `UnauthorizedAccessException` at Graphics-Profile.ps1:52 (Restore-AsylumProfile) writing
`Documents\Square Enix\Batman Arkham Asylum GOTY\BmGame\Config\BmEngine.ini` / `UserEngine.ini`; clearing Read-only and
reinstalling the game did not help. Cause (SOURCE): Windows refused the first VR write to a Documents copy (most likely
Controlled folder access, which blocks PowerShell in Documents), so the file was never changed; the restore then
rewrote EVERY file unconditionally, failed on that one, and left `%LOCALAPPDATA%\Batman_Asylum\profile-active.json`.
Every later launch, the installer and the uninstaller replay that journal first and hit the same refusal - stuck for
good, independent of the game install. JJ's PC has no Documents copies, so this path was never exercised.
Fix (SOURCE, offline check PASS, tests/graphics_profile_check.ps1 extended):
1. Restore skips any file already equal to its original; one refused file no longer stops the others; one clear
   error names each file + where its original is saved (journal kept so nothing is lost).
2. Launch tests every config for write access before changing anything (Select-AsylumWritableConfigs): an unwritable
   Documents copy is left as it is with a warning and VR starts (command line still sets window + size); an
   unwritable game-folder file (holds the frame cap) stops with instructions, nothing changed.
3. Installer: same skip; a refused file only warns (reinstall must always work, it is how players get the fix).
   Uninstaller: same skip; a refused changed file stops BEFORE the data folder holding the originals is deleted.
Not yet packaged (needs a version bump + package). Origins' launcher writes a Documents file too: check it.

## RELEASE 0.1.2 - Pimax menu / HUD double vision (PIMAX2, 2026-10-17)

Report (via JJ): Pimax player, "the main menu and HUD have double vision with OpenXR" (not with SteamVR). Looked up: VR_ESSENTIALS
§8 (VD quad layers have no parallax), memory vd-quad-layers-no-parallax, playbook b955f224 09 "Canted displays"; code: every
runtime-specific branch in openxr_runtime.cpp. The HUD in-eye overlay and the distant menu screen project through each eye's own
pose (orientation + position) - correct on any runtime, canted or not. The ONE per-eye geometry branch that differs between
SteamVR and other runtimes: the full-view transition quads (menu fades / backdrop / bats) got VDXR's head-centre correction
(each eye's quad moved half the eye distance) on every runtime except SteamVR. Pimax places quads from the physical eyes like
SteamVR, so the correction doubled them. SOURCE fix: the shift applies only when the runtime is VirtualDesktop. VD and SteamVR
unchanged. City/AK: VD-only already (hud_eyes_on / g_hudEyes). STATIC: offline check PASS. HEADSET pending (Pimax player).

## RELEASE 0.1.1 - Pimax fixes (PIMAX, 2026-10-17; JJ: "fix all the mods so they work on Pimax correctly, then deploy")

Looked up: workspace tempatman_asylum.log (Pimax player); batman_city BATMAN_CITY_PLAN SVRFALLBACK / SVRACTIVE /
ELEVATED (2026-10-13 Pimax player: Pimax registers its 64-bit runtime in the 32-bit slot); City/Origins dlss_bridge
on_eye (helper never waited on: `if (!g_sh->helperReady) return false`); VR_ESSENTIALS §4 (anything that blocks paces
you), §11; playbook b955f224 09 #runtime-owns-pacing.

Player log (LIVE, Pimax Crystal, 'Pimax OpenXR' 0.1.0 reached in-process, 3880x4592 per eye): menus FOCUSED and
submitted as the distant screen; first gameplay pair at 17:46:54 -> `xr: image-pose age 6531 ms` -> `[aa DLSS] helper
pid ..., ready 1` -> session FOCUSED -> VISIBLE 0.2 s later -> no further pairs; player quit 7 s after. dlss.cpp start()
waited on the render thread for helperReady (5 s loop + NGX) at the FIRST 3D frame; JJ's own A2o log shows the same wait
at 1.06 s. The Pimax runtime demotes an app that stops presenting for seconds.

Fixes (SOURCE, built, offline):
1. PIMAXSTART: the helper starts in the background when the native9Ex D3D11 device is adopted (dlss::prewarm); start()
   never waits (1 ready / 0 starting / -1 failed); a starting helper = stock picture for that eye, no fault, counted as
   helperStartingFrames in the [aa DLSS] heartbeat; 60 s limit for a slow first NGX start.
2. SVRFALLBACK port: no 32-bit runtime loadable -> SteamVR's steamxr_win32.json (active install first) for this launch.
   ELEVATED log line (admin ignores XR_RUNTIME_JSON).
3. Launcher: aa_xr_config32 exits 4 when no 32-bit runtime can be loaded at all; Launch-VR.ps1 then retries with
   SteamVR's 32-bit runtime. Before, the launcher refused to start the game on such PCs. A VD runtime with the headset
   unconnected still exits 2 (no SteamVR switch).

### RELEASE 0.1.1 RESULTS (2026-10-17)
- OFFLINE: asylum_offline_check native = PASS, incl. "DLSS helper background start: ready" and all DLSS overlap cases.
- OFFLINE: aa_xr_config32 with XR_RUNTIME_JSON = VD's 64-bit manifest (imitates the Pimax registration) -> exit 4;
  normal VD 32-bit runtime with no headset -> exit 2. Both launchers parse.
- Package: changed vs 0.1.0 = batman_asylum.dll, aa_xr_config32.exe, Launch-VR.ps1, version.txt, README (Pimax row);
  settings = the 0.1.0 zip's aa_rendering.ini.
- HEADSET pending: JJ (VD) regression check; the Pimax player's report.

## RELEASE 0.1.0 - first public package (2026-10-14; JJ: "package it up with the same instructions i used for Origins, then make a github repo for it and upload. We'll address the decal shader after")
Looked up: PACKAGING_GUIDE.md (all; s12 panel tidy + test tools behind one switch, s9 practice install, s3 zip rules);
Origins RELEASE 0.1.0 (batman_origins/BUILD_PLAN.md, commit 97b9aba: install/*.ps1, tools/make_package.py, README);
memory github-repos (repo exists since 2026-10-11, public; README layout, no jargon, Steam capsule art).
- Panel (s12, PANELWORDS): title "Batman Asylum VR 0.1.0", save + Recentre at top; STEREO RENDERING (Native / AFW),
  VIEW ("Full view, safe lighting (recommended)" / "Full view, exact lens ..."), HUD, MENUS AND SCREENS (Fit / Fill;
  "Floating screen" removed by JJ), DLSS (jargon checkbox and transfer numbers only with test options; "Faster DLSS
  timing (leave on)"), FRAME RATE. "Show test options (only when Claude asks)" (off at every start) shows the TESTS
  drawer: effect stepper, full-resolution still, all Compare buttons, old Diagnostics.
- Test recorders off for players: automatic stills at start (3 x ~20 MB per launch; JJ's captures folder had reached
  19 GB) now only with [AsylumVR] AutoCaptures=1; ui_bindings shader dumps only with test options on.
- Package: install/ (Install-/Uninstall-BatmanAsylumVR .bat/.ps1, the two launchers; the "Previous Bridge" launcher
  is development-only), tools/make_package.py -> dist/BatmanAsylumVR-ArkhamAsylum.zip, THIRD_PARTY_NOTICES.md, player
  README (old status notes moved to STATUS.md). Installer: exe hash check (Steam GOTY), game run once
  (BmEngine.ini), interrupted-launch journal restore, foreign d3d9.dll/xinput1_3.dll saved and restored on uninstall
  (ours = "[batman_asylum-proxy]", now exported by d3d9.dll too, or "_AA_CreateD3D9@4" for pre-0.1.0 builds), mod +
  AsylumVR folder + manifest, JJ's aa_rendering.ini on first install, shortcuts. NO permanent game-setting changes:
  the launcher switches settings for each VR session and restores them byte-exact (unchanged from development).
  Launcher messages reworded for players.
### RELEASE 0.1.0 RESULTS (2026-10-14)
- STATIC: 11/11 CTests (panel navigation test now ends at the test-options box). batman_asylum.dll SHA-256
  33d6641710769f79...; zip 61.8 MB, 28 files, sha256 9e977876953e0463...
- Practice installs (scratchpad copy of the game folder, real exe, redirected data folder): install twice; foreign
  d3d9.dll saved and put back byte-exact; an old development d3d9.dll recognised as ours (first real install had
  saved JJ's old dev proxy as "foreign" - removed by hand, Is-Ours fixed, retested); manifest 15 files 0 mismatches;
  uninstall from the game folder removes all, deletes data + backup. All .ps1 parse clean.
- INSTALLED on JJ's game with the package installer (his aa_rendering.ini kept; shortcuts on desktop + Start menu).
  Previous files: re/backups/before-release-0.1.0.
- RELEASED: commit 33b7802 pushed; GitHub release v0.1.0 (Latest) with dist/BatmanAsylumVR-ArkhamAsylum.zip (61.8 MB,
  GitHub asset digest = local sha256 9e977876953e0463...); repo description updated; repo PUBLIC.
- HEADSET pending for the tidied panel. Open issue carried to 0.1.x: shotgun shells read infinitely far (A2p stepper).

## A2p: effect stepper (toolkit) to name the shell draw

CURRENT A2p INSTALLED (2026-10-14, DLL 0d3352a975a108cde48c1e54846bef53e0c48b68082edcffaaf75358d3a1771c).
LIVE A2o KIND table (archived A2o-run): no kind is an obvious "camera input unchanged" culprit. LocalToWorld
SAME in both eyes for skinned characters c28022b8222ec8fd (look correct, so skinned shaders do not use the
translated LocalToWorld), 48b447cda3a1dc99 (3 draws, 52 prims, 1024 tex, BoneMatrices CHANGE) and shadow
depth kinds 346680f8/640ac123 (ProjectionMatrix same = light view, fine). 6a77a24c8172f293 (1 draw, 38 prims,
128x128) and b3b7b7809a17dd58 (282 draws, no LocalToWorld) have only ViewProjectionMatrix. Reasoning from
inputs alone is not conclusive after two wrong fixes.
TOOLKIT fxstep9 (VR_DEBUG_TOOLKIT "collect the effects" port): panel "TESTS (only when Claude asks)" ->
"Hide each effect in turn" (or aa fxstep). Next LEFT pass lists up to 36 draw kinds by vertex shader
(skipping full-screen 2-triangle passes), then hides each kind in BOTH eyes for 250 ms, logging
"[aa fxstep] step i/N from generation G: hiding VS <hash>", then saves the 10 s replay. Map replay frames
(frames.csv generation) to steps; the step whose frames lose the red shells names their draw kind.
11/11 pass (render_size script stalled in the suite again, 0.3 s alone - pure arithmetic, PowerShell startup).

## A2o: shell theory A2n disproven; per-shader input table; floating screen options removed

CURRENT A2o INSTALLED (2026-10-14, DLL 8ee250e3f017de2e762dd63744b8b9d9ad679cb6d20094563adcdd14da742334).
HEADSET A2n (JJ): shells still broken. LIVE A2n (archived A2n-run): xform9 patched 845 draws total, ALL
with translation 0,0,0 (camera-attached, e.g. sky - SHOULD match between eyes); in gameplay no right row
ever equalled a left row. => "right eye reuses the left LocalToWorld" is WRONG for the shells; the
VR_ESSENTIALS line added in A2n was removed. xform9 now default OFF and skips rows within 1 unit of the eye.
Lead: A2m histogram had ProjectionMatrix differing in exactly 1 draw (not ViewProjectionMatrix): a shader
with a separate view input that stays the same in both eyes would move with the eye (infinity).
TOOLKIT A2o: eye-same report adds one KIND line per vertex shader: draws, prim range, texture, named inputs
that CHANGE between the eyes and those that stay the SAME; every kind's bytecode saved.
PANEL (JJ): "Floating screen" removed from STEREO RENDERING and MENUS AND SCREENS (Fit / Fill remain;
commands still exist). 11/11 pass; render_size_fits_desktop script timed out once more (3rd time, passes
alone in 0.3 s) - flaky PowerShell startup, not this change.
NEXT JJ: shells + panel capture once; read KIND lines for a shader whose View/Projection-type input is SAME.

## A2n: shotgun shells - right eye reused the left eye's eye-relative position

CURRENT A2n INSTALLED (2026-10-14, DLL 9f0a9d1b821daba3b133a455591fd1d71bf298f3aab52c94adb2c330095d7078).
LIVE A2m eye-same report (archived A2m-shells-run/eyesame-report.txt): differing input histogram
ViewProjectionMatrix 906, LocalToWorld 586, ShadowCoordinateScaleBias 41, LightMapScale 32, WorldToLocal 50,
LightPositionAndInvRadius 8, BoneMatrices 4, ProjectionMatrix 1. Ordinary LocalToWorld translation rows
(c8 of VS 8ac897d72b99c8e0, LocalToWorld@c5x4) differ R-L by exactly -5.016/+2.781/+0.570 units = the eye
shift: Asylum uploads LocalToWorld translated by the view origin (PreViewTranslation). Large odd deltas
are kind-pairing mix-ups among identical depth-only draws. A draw whose translation does NOT change
between the eyes moves with the eye = reads as infinity = the shells' constant 554 px disparity (A2j).
Cause (STATIC knowledge, UE3): mesh-particle transforms are built once per frame for the first view.
SOURCE xform9.cpp: LocalToWorld register from each VS's constant table; LEFT scene draws on the main
target record their translation rows; a RIGHT draw whose row equals a left row bit for bit gets
(left eye origin - right eye origin) added for that draw only, restored after. Eye origins from the
pass's eye views (markers9::eye_origins). FixStaleParticles=1 default, "aa stale on|off" live; status
"[aa xform]" pairs/leftRows/rightChecked/rightReused/patched/noShift; first 4 eye shifts and first 8
patched rows logged (compare shift with the measured -5.0/+2.8/+0.6). 11/11 pass. Not retail/headset.
NEXT JJ: look at the shells; they should lie on the floor at its depth. If anything else jumps, "aa stale off".

## A2m: eye-same finder names the inputs that differ between the eyes

CURRENT A2m INSTALLED (2026-10-14). HEADSET A2l (JJ): "reticle is fixed" (grapple ring on point / edge
pointer, head-look works) - A2l ring behaviour ACCEPTED. Shells still wrong. LIVE A2l eye-same report
(archived A2l-run): LEFT 923 / RIGHT 861, paired 844, ALL 844 differ, 0 identical; 79 LEFT-only. So the
shells do NOT share one camera; reference world VS reads LocalToWorld@c5x4 + ViewProjectionMatrix@c0x4.
New hypothesis: the shells' own placement inputs (LocalToWorld or a particle/instance input) differ per
eye so that the eye offset cancels (screen position equal in both eyes = reads as infinity).
TOOLKIT A2m: each pair's differing registers are mapped to the VS constant-table names; logs a histogram
of differing input names, "SUSPECT" draws whose NON-camera inputs differ (L/R first and last rows, VS
table), and the first 20 LEFT-only draws. 11/11 pass (one transient render-size script stall, passed alone).
NEXT JJ: shells + panel capture once.

## A2l: no texture gate for exact ring matches; eye-same finder pairs by kind

CURRENT A2l INSTALLED (2026-10-14). HEADSET A2k (JJ): ring floating mid-view again, opposite to head.
LIVE A2k (archived A2k-run): projected0 the whole run (no point ever inside the head-free view) -> no ring
texture learned -> every ring matched at the game's drawn position was refused as "NOT a learned grapple
texture" (edgeUnlearned 26352) and fell to the HUD sheet. The texture gate now applies only to the
direction fallback; exact game-position matches teach the texture too. Eye-same report A2k: LEFT 462 /
RIGHT 424 draws, order pairing failed (413 mismatches, 0 identical) -> now k-th draw of a kind (VS, PS,
texture, primitives, target size) pairs with the k-th of that kind in the other eye. 11/11 pass.
NEXT JJ: same two tests (head-only look at a grapple point; shells + panel capture).

## A2k: grapple ring follows the game's head-including projection; captures run the eye-same finder

CURRENT A2k INSTALLED (2026-10-14, DLL e13b80d1a54470582f42283c07029df480024759d10e30cbc0440a911bd2432c).
HEADSET A2j (JJ): edge pointer good, but the ring attaches only when the game CAMERA is tilted up to the
point; looking up with the HEAD sends it "down to the bottom of the field of view" where it floats.
LIVE A2j (archived A2j-headlook-run): census first sample world 7424/15360/256 game out 0.2613/0.7283 and
the ring drawn at 0.261/0.730 = the game's own Canvas.Project output, while our head-free projection called
the same point off-screen. => the HUD-pass view used by Canvas.Project INCLUDES the head; rings for points
off the head-free screen were drawn at the head view position and unmatched (A2i only matched by direction).
Also 5937 ring pieces were drawn in frames with NO Canvas.Project (positions moving ~0.01/frame), all with a
WithAim target -> fell to the HUD sheet. The panel "Save last 10 seconds" button did not arm eyesame (only
F2 did), so no shell data yet.
SOURCE: off anchors keep the game's projection output as their match position (gameRef), output unchanged;
matching by proximity first (on-screen refs and gameRef), direction only as fallback. The last projected
set is held 250 ms for frames without Canvas.Project (heldFrames counter). On-point placement when both
eyes see the point: head-free card, else the LEFT eye as the card camera; otherwise the head-locked pointer.
stereo_history::set_save_listener: every history save (panel button, F2, command, compare) arms eyesame9.
OFFLINE 11/11 (A2k-ctest.txt; one transient stall of the guard test, passed alone in 0.1 s and in the rerun).
NEXT JJ: look at a grapple point with the head only (camera level) -> ring on the point; look away -> edge
pointer; then shells + panel capture for the eyesame report.

## A2j: shell casings at the wrong depth + grapple pointer brought in

CURRENT A2j INSTALLED (2026-10-14, DLL 3358cee8de45f4a71eb67a1bb31c6b05a6348761532c0bd8c6a80ce72a57c03b).
HEADSET A2i (JJ): edge pointer "really good", bring it in a little (32 -> 26 deg, ini GrapplePointerDegrees,
10..40). New: red shotgun shells on the floor sit at a different depth than the floor.
LIVE A2i still A2i_20261014_030808_381 (left/right template matching, full res): floor disparity rises
with nearness (corridor end 557 px, mid floor 570, boot 586, near floor 592), but EVERY shell = 554 px at
rows 1677..2142 = same depth for all, at/beyond the far corridor = no stereo separation. Most likely the
shell draws receive identical camera constants in both eyes (left-eye or head-free view), not a placement
error. Particle correction ran 0 times (status patched0), so billboard9 is not the cause. A2i log archived
re/evidence/A2i-shells-run.
TOOLKIT (VR_DEBUG_TOOLKIT.md, first Asylum tool): eyesame9 "eye-same draw finder". F2 (or "aa eyesame")
records the next LEFT/RIGHT scene pair draw by draw (VS/PS/texture/prims/RT + hash of VS c0..c95) and logs
every order-matched scene draw whose constants are bit-identical in both eyes, with VS hash, c0/c1/c3 rows,
texture/RT size and the VS constant-table names (plus one differing world draw for comparison); bytecode
saved to captures\eyesame_shaders. Shadow-map and full-screen post draws are expected to be identical;
the shells should be a small textured world draw on the scene RT. 11/11 checks pass (A2j-ctest.txt).
NEXT: JJ stands looking at shells, F2 -> read "[aa eyesame]" lines, name the camera input the shell VS uses,
then fix it per eye.

## A2i: edge grapple rings -> on the point, or a head-locked edge pointer

CURRENT A2i INSTALLED (2026-10-14, DLL 0caa7af648f055c202f8bbfb0efb819b2cb94dee1198ebeb297a1bca8322b819).
HEADSET A2h (JJ): rings still float mid-view "as if they want to be at the edge of the screen to point you
toward a grapple point"; they move opposite to head motion. JJ chose (asked): off-view = EDGE POINTER, and
it "should stay locked to that part of the screen" (head-locked, no drift).
LIVE A2h (archived A2h-grapple-run): census Project caller658947 = 5648 calls, ALL owned pass, projected0:
every one rejected by anchor()'s |ndc|<=1.1 / in-front test, i.e. the grapple point was beside/behind the
game camera. WithAim found a point 2667/3136 calls (called every frame). No accepted ring that run, so no
offset/texture learned. Conclusion (LIVE, consistent with JJ): the game draws an EDGE ring for off-screen
grapple points; it sat at the game-screen edge = mid-view on our HUD sheet.

SOURCE: anchor() keeps off-screen/behind points as off anchors with the game's screen direction (head-free
view x,y times P00/P11; valid behind the camera), game output untouched. In draw, a piece with no on-screen
match whose screen position (radius>0.25) lies within 30 deg of an off anchor's direction is the edge ring;
run start must be a learned grapple texture (else logged + orphan). Placement: ON the point when both eyes
see it (|ndc|<=0.92) via the existing card with the run's first piece as reference; otherwise a head-locked
pointer: 32 deg off the left eye's forward toward the point, at HUDDistance (90 units/m), card facing the
eye, rebuilt every pass from that pass's eye view (native_stereo now passes eye views). If neither works
the ring is hidden, never left on the HUD sheet. Status adds offPoints/offRejected/edgeRings/onPoint/
pointer/edgeHidden/edgeUnlearned; first 12 edge decisions logged.
OFFLINE: 11/11 pass (A2i-ctest.txt), new: 24 off-screen/behind cases kept with exact direction and
untouched output; pointer exactly 32 deg toward the target at HUD distance, inside both lenses, card valid,
for head yaw/pitch. Not retail/headset.
TEST (one JJ run): look at a visible grapple point first (teaches the ring's look), then turn so it goes off
to the side/behind: ring should sit near the view edge toward it and not drift with head moves; turn back:
ring lands on the point. F2 on anything odd. Check health/XP widgets never jump to the pointer spot
(direction match risk). Then detective vitals regression, quit from menu.

## A2h: grapple ring target from the grapple gun itself

CURRENT A2h INSTALLED (2026-10-14, DLL fa14ccfa0d7d53f09889bbe728f418a99029fc7648df3c5e418b29d1d45f56b6).
HEADSET A2g (JJ 2026-10-14): "random grappling hook reticles ... at HUD depth when there's no immediate
target ... free-floating but moving in reaction to head movements. They shouldn't be there at all unless
there's a target." LIVE A2g (archived A2g-grapple-run): pixelCalls0 the whole run, so A2g never touched
the ring. Replays A2g_history_021913/022102 frames 30-84: ring visible while projected stays flat (no
Canvas.Project that frame -> set empty -> draw falls to the HUD sheet = HUD depth). Ring wanders over a
static world with head motion but stays near grappleable ledges/gantries: most likely a REAL grapple
point drawn without an anchor, not a phantom. Not yet proven either way.

Looked up: VR_WORLD_MARKERS_GUIDE §2 steps 2-4 (record EVERY caller, print raw lists before matchers),
City BATMAN_CITY_PLAN round 9 (grapple gun keeps its target: gun+0x460 GetGrappleTargetPosition), memory
fixed-tables-fill-silently, comment-swallowed-code (caught again by the new offline check), playbook
FAIL-HUD-006/007 @ b955f224. Evidence flaw found: A2g's pixelCalls (and "projected") were counted AFTER the
owned-view/exact-caller filter, so zero calls never proved the route unused.

STATIC: Asylum has no GetGrappleTargetPosition. RGrappleGun natives FindGrapplePointNoAim VA17075e0 and
FindGrapplePointWithAim VA1707620 are thiscall(Stack, Result) ret8, *Result = vtbl+358/+35c result
(chosen grapple point object or null) (A2g-grapple-exec-instructions.txt). Third projection route:
UIScreenObject.Project exec VAca4a60 -> impl VAdc3550 thiscall(out, world) ret8, own scene-client matrix
(dbe710), bypasses the shared helpers (A2h-uiproject-*.txt).

SOURCE (markers9.cpp): census of EVERY caller of Project helper, WorldToPixel helper and UIScreenObject
.Project (owned/other, first sample per caller, 24-slot table with overflow counter, logged every 5 s).
Observe-only hooks on both finders (exact 16-byte guards): object, call/found counts, first 12 changes.
On each accepted Canvas.Project anchor the current finder object is scanned (0x1000 bytes) for that world
triple; an offset with >=6 votes and >=75% of scans is LEARNED. Each game pass then anchors the fresh
(<=250 ms) finder point(s) at that offset, so the existing matcher puts the ring on its world card even
without Canvas.Project. Unanchored draws using a placed-grapple texture are counted with/without a finder
target. HideTargetlessGrapple (ini, default 0; command "aa grapple hide on|off") hides such a piece plus up
to 8 untextured followers within 0.12 NDC ONLY when neither finder has a target. "aa grapple census" logs
the tables now. Status line adds grappleGuard noAim/withAim found/calls, learnedOffset, grappleAnchors,
unanchored/withTarget/noTarget/hidden.

OFFLINE: all 11 checks pass (A2h-ctest.txt); new check: offset learned (+104) from accepted anchors with a
decoy rejected, fresh finder point anchored, stale/null finder adds nothing. Not a retail/headset test.

## TEST PLAN A2h (one JJ launch, normal launcher)
| # | step / switch | what it proves | how to judge |
|---|---|---|---|
| 1 | play to a spot with grapple points, look at one so the normal RB ring appears | finders run; offset learns | log: grappleGuard111, noAim/withAim calls rising, "LEARNED grapple point world offset" |
| 2 | walk where the floating ring showed before; F2 when one appears | ring now on its ledge | replay: ring sits on a ledge; log grappleAnchors rising, unanchored small |
| 3 | (me) census lines | which route places the ring | "[aa markers census]" callers; any new caller during ring = its route |
| 4 | (me) "aa grapple hide on" if unanchored noTarget > 0 | targetless rings vanish | noTarget pieces now counted as hidden; JJ: no ring without a ledge; button prompts still there |
| 5 | regression: vitals line/circle in detective mode | A2f acceptance holds | vitalsAdjusted rising, JJ looks |
Follow-ups: offset never learns -> read learn-scan lines (best offset/error) and anchor at that field or
the object's Location; ring still loose with grappleAnchors>0 -> its screen point is not the head-free
projection (census tells the route); unanchored withTarget>0 -> anchoring/matching gap, not a phantom.

CURRENT A2g INSTALLED (2026-10-14 02:13 Sydney): second Canvas projection
route; accepted A2f vitals retained. HEADSET A2f "Looks great" for vitals,
LIVE vitalsAdjusted10011/fallback0. Occasional hollow grapple/RB ring floats
opposite head without an apparent attachment. Replay020100 frames0/70/80 shows
it while intercepted Project/placement counters stay flat. Missing Project
calls do NOT establish an invalid game target. SOURCE discovered separate
Canvas.WorldToPixel registryA58AC0 -> AB36C0 helper, which inlines VP projection
and bypasses Project. A2g hooks ONLY exact Canvas caller658b73 in the owned
main view/pass; on original success emits head-free pixel XY and same-frame
world anchor for existing accepted marker cards. Original Boolean/false-output,
behind/offscreen/foreign callers preserved. No stale-target hold or guessed
widget hiding. Guards plus pixelCalls/pixelPlaced/pixelRejected and first6
world/pixel logs. All11checks pass:32pixel route/64vitals/32grapple cases EACH
route and4097DLSS pairs each;19owned/fiveprotected/cache/INI verified following
stale closed-profile verified rollback (A2g-profile-recovery). RETAIL/HEADSET
A2g PENDING: hollow-ring use of this new API has not been observed; this is
implemented missing coverage, not an established root cause/fix for that ring.
JJ next launch: same hollow ring/head+gamepad camera, valid grapple targets and
accepted vitals regression; F2 on residual then exit normally. FPS/menu reader
open. Read RE_NOTES A2g and re/evidence/A2g-build-checks.json; older entries historical.

## A2g plan: second Canvas projection path for hollow grapple/RB indicator

HEADSET A2f vitals accepted (JJ "Looks great"); occasional hollow grapple
indicator floats/reverse-head with apparently no target. LIVE A2f adjusted10011,
fallback0 for vitals. Replay020100 frames0/70/80 shows hollow ring/RB drifting
while Canvas.Project count and world placement stay flat. Do NOT conclude that
the game has no valid target from zero intercepted Project calls. Native game
also registers Canvas.WorldToPixel at A58AC0, calling separate AB36C0 helper at
A58B6E/returnA58B73. This helper inlines VP projection; never calls Project.
Looked up: WORLD_MARKERS_GUIDE6/7 offscreen/whole-piece ownership; HUD_GUIDE1/10;
framework11; playbook ch04 freshness/engine UI ownership, FAIL-HUD-006/007 at
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. City ruled WorldToPixel out by LIVE
counts; that result does not transfer to Asylum. Original game target selection
and valid-result boolean are authoritative. No hiding guessed widgets or stale
world-point reuse. Add exact owned-view/Canvas.WorldToPixel call hook: on original
success, same-frame head-free pixel XY and native world anchor for existing
accepted per-eye cards. Preserve false result/output, offscreen and foreign
callers; leave vitals unchanged. Guard entry/caller/ret8/rect fields; separate
passive pixel-call/corrected/rejected counters. Native/harness checks then closed
install after verified stale-profile rollback. Actual hollow-ring API ownership
and headset correction remain pending until this hook is observed in retail.

CURRENT A2f INSTALLED (2026-10-14 01:46 Sydney): native callout anchor
correction, original line/ring layout. HEADSET A2e depth improved but circle
still detached/head-dependent and line bends shear into unnatural diagonals.
LIVE A2e links2920/circles39; first small orange matches a different world point
than the person line. Proximity did NOT prove ring ownership. Archive/replay
A2e-vitals-failure-run, replay013620 frame70 inspected. A2f removes the CPU
orange matcher and per-vertex displacement from production. Correct ONLY
native actor-callout projection owner118a36b before the game builds its record/
widget; shared head-centre-to-person ray meets actual HUD plane, exact-generation
poses/recenter reference. Original Z/W, line/ring/bends/material stay native;
accepted grapple remains separate at world depth. Exact call/consumer guards,
plane reset/recenter invalidation, missing plane/pose stock fallback. All11
checks pass:64 BOTH-eye analytic owner-position cases EACH route,32grapple and
4097DLSS pairs each;19owned/fiveprotected/cache/INI verified, profile inactive.
RETAIL/HEADSET A2f PENDING: original native owner ran in A2e with three actor
points, but rewritten XY and original widget response not observed in retail
yet. Passive vitalsOwner/vitalsAdjusted/vitalsFallback counters and first6 XY
logs distinguish source/offline validation from actual game use. JJ next run:
same detective person vitals, yaw/roll/lean + gamepad camera, grapple regression;
F2 if detached/diagonal remains, then normal exit. FPS/menu reader remain open.
Read RE_NOTES A2f and re/evidence/A2f-build-checks.json; older checkpoints historical.

## A2f plan: correct the game's callout anchor before it builds the widget

HEADSET A2e: depth improved; circle still detached/drifting; line bends shear
into diagonals. LIVE A2e links2920 but circles39, first9-vertex small orange
matches world12320/11491/559 while actual person line targets11702/11085/117.
Thus small-orange proximity is NOT established circle ownership. The source
linear vertex displacement also shears original right-angle connector bends.
Looked up again: playbook06 readback/multi-candidate residual and ch04 engine
UI ownership (FAIL-HUD-007), Knight NOTES REC1/RB1/RB2: colour/nearby-piece
classification split real widgets and guessed residual constants were false.
VR_HUD_GUIDE10, WORLD_MARKERS_GUIDE6/7, framework11; playbook checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Asylum native LIVE caller118a36b
projects all three detective targets. Static158a366 Project call immediately
feeds normalized XY into actor callout records via1562a00; XYZ/W ABI ret8
verified. Correct ONLY this guarded original projection's XY to the shared
head-centre/HUD-plane point before native callout layout. Preserve Z/W,
behind-camera checks, actor identity and original ring/line geometry/material.
Retire CPU orange matching/vertex shear from production. No shader guessing,
masking/replacement circles or donor offsets. Exact pose generation; shared
plane snapshot cleared at recenter/reset; missing inputs retain original.
Offline owner/ABI/XY/Z/W, asymmetric both-eye/recenter/pose/fallback tests and
original line/ring GPU draw checks; full suite, closed-game install, JJ one run.

CURRENT A2e INSTALLED (2026-10-14 01:32 Sydney): vitals leader and original
endpoint circle at HUD depth. HEADSET A2d confirms lines point at people but
rejects world-depth interpolation; circle remains detached/opposite head.
LIVE A2d native13111/links1897/linkMiss4248; replay011656 frame60 inspected and
logs archived in A2d-headset-vitals-run. A2e captures exact recenter reference
with main view, intersects shared head-centre-to-person ray with actual HUD
plane, and keeps ALL line/circle vertices on that plane with homogeneous W.
Small original coloured pieces match native OR head-free source projection;
the circle centre shares the line endpoint, without moving the text or changing
original geometry/colour. Vitals matching precedes grapple routing (orange
endpoint pieces only); accepted grapple stays at world depth. All11checks pass:
24 BOTH-eye plane/circle cases EACH route,32grapple each,4097DLSS pairs each.
19owned/fiveprotected/cache/INI verified; no active profile recovery needed.
RETAIL/HEADSET A2e PENDING: circle binding/layout hypothesis not yet confirmed;
unsupported/textured/unmatched artwork stays original. New circles counter and
first4 source/world logs identify actual coverage. JJ next run: same person
vitals, head yaw/roll/lean and gamepad camera, grapple regression; F2 if ring
still separates, then normal exit. FPS/menu reader remain open. Read RE_NOTES
A2e and re/evidence/A2e-build-checks.json; older checkpoints are historical.

## A2e in progress: vitals line and circle share the HUD plane

HEADSET A2d: JJ confirms lines point at people, but rejects world-depth lines;
the orange endpoint circle remains detached and moves opposite head motion.
Looked up (SOURCE): Knight NOTES SCANHUD2144-2153 projects the head-centre-to-person
ray onto the HUD plane. VR_WORLD_MARKERS_GUIDE6/7: match every component to the
same target, preserve the original glyph. VR_HUD_GUIDE8/10; playbook ch04,
FAIL-HUD-006/007, checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Use Asylum's already matched native world anchor, exact pose generation and
captured recenter reference; all connector vertices stay on the actual HUD
plane. Match the separately laid out circle also against the head-free source
projection, place its centre on the same plane endpoint, retain original
geometry/colour. No depth march, donor offsets or change to accepted grapple.
GPU regressions: perspective-plane disparity, moving targets, yaw/roll/lean,
recenter translation/yaw, both source layouts and circle/line coincidence;
stale/unknown/white/backing/off-target guards. Retail circle coverage remains
to be established separately. Build/check/install closed, then JJ headset check.

CURRENT A2d INSTALLED (2026-10-14 00:41 Sydney): vital-statistics connector
endpoint path; accepted A2c grapple retained. JJ HEADSET A2c "Looks perfect";
LIVE projected2184/placed11042/3variants/shaderMiss0/failure0. New person-vitals
orange line drifts while text sits on HUD; actual replay001229 frames0/40/75
inspected. A2d records native game projection anchors WITHOUT changing their
outputs, matches skinny orange UI endpoint geometry (not widget origin), and
rebuilds per-eye line from world-depth person to unchanged HUD label; small tip
uses world card. Actual CPU uploaded geometry, no new GPU readback; one coloured
triangle draw after HUD/DLSS/AFW. Default ON/live toggle. All11checks pass:
6 connector endpoint cases EACH route + GPU blend/state references,32grapple
cases each,51native50legacy transitions/4097DLSS pairs.19owned/fiveprotected/
cache/INI verified after closed-profile recovery. RETAIL/HEADSET A2d PENDING:
exact native vitals caller/mesh matching not yet established; unsupported/
textured/unmatched remains original. One JJ run detective near/far/moving people,
head+gamepad camera, grapple regression/menu/exit; F2 at residuals. Read RE_NOTES
A2d and A2d-build-checks.json. FPS/menu-state reader open; older entries historical.

## A2d implemented plan: vital-statistics leader endpoint (2026-10-14)

HEADSET A2c JJ: "Looks perfect" (grapple accepted). New detective person vitals
connector must meet person at world depth while text stays on its panel. Actual
A2c replay001229 frame0/40/75 shows orange endpoint on floor beside people,
label remains on HUD; marker projections stop while callout still visible.
Looked up: VR_WORLD_MARKERS_GUIDE6/7; Knight NOTES SCANPROJ/SCANLINE2114-2134 and
later SCANHUD warn exact owner not proven, depth-march leaps/whole-group moves
failed. City world/card method retained; no City/Origins vital-connector-specific
solution found. HUD guide8/10, framework11, playbook FAIL-HUD-006/007 ch04 at
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Methods only, no donor offsets.

Static: exact ProjectABC2A0 also has native game callers apart from Canvas;
read-only Ghidra A2d-callout-project.txt collects seven such methods. Capture
world/raw projected point from bounded native caller list within exact main
view/pass, WITHOUT changing their output/culling. Inspect actual CPU UI geometry
using existing bounded uploaded bytes, supported untextured shader output and
orange skinny mesh. Match an END vertex, not transform origin, to native point;
retain broad/coloured/unsupported/unmatched HUD. Suppress only matched connector/
endpoint shape. Per-eye rebuild line from actual target to unchanged HUD label,
using exact pair eyeVP and actual same HUD-world plane; endpoint marker keeps
world-card orientation. Aggregate coloured triangles, one D3D11 draw after HUD,
post-DLSS/AFW; original grapple path untouched. Default live toggle, matching /
anchor/capture counters for exact retail verification. Target shader/geometry
fixtures plus near/far/head motion/both-endpoint/reference-CPU and GPU state /
blend tests; build/check/install closed after journal recovery if needed.
JJ one launch detective near/far/moving people, head/camera, grapple regression/
menus/exit, F2 at residual then normalexit. No auto launch/kill, FPS remains open.

CURRENT A2c INSTALLED (2026-10-14 00:02 Sydney): City-style world grapple cards
plus literal-position fade family. HEADSET A2b: menus progressed; black window
remains on first-menu->exit, grapple floats at HUD depth/drifts opposite head/
spins. LIVE A2b removed1502/bat693/missing0/failure0,12 authoredfade episodes,
archived A2b-grapple-exit-run. A2c hooks this target's Canvas.Project via exact
STATIC caller/ret8 guards; head-free reference and target-depth card facing GAME
camera position. Original widget shaders/art retained, position shader upgraded
to exact perspective; both eye marker sheets from one widget draw, added after
DLSS/AFW separately from floating HUD. Default ON/live toggle, bounded sprite/
prompt runs, mask/text-atlas guards. All11checks pass:32 marker world-vertex GPU
references +8 old affine failures EACH route; both marker slots5+6 transport /
composition,51native/50legacy transition cases,4097DLSS pairs EACH route.19owned/
fiveprotected/cache/INI verified. Closed leftover VR profile recovered byte-exact
from verified journal BEFORE new baseline/install, archived A2c-profile-recovery.
RETAIL/HEADSET A2c PENDING: actual anchor/piece matching and exit window owner
unproven. One JJ launch: menus->game grapple head yaw/roll/lean and gamepad camera,
return first menu->exit LAST; F2 after residuals. FPS/menu-state reader open.
Receipt A2c-build-checks.json/RE_NOTES A2c. Older entries historical.

## A2c implemented plan: world grapple cards and last exit fade (2026-10-13)

Looked up: VR_WORLD_MARKERS_GUIDE sections2-6 including5b/5c (Canvas.Project
anchor, head-free reference, exact per-vertex perspective card facing game
camera position); City markers.cpp MARKEREYE/BASE/CARD/UPRIGHT/FACE and
RETICLERUN/MASKSTOP (keep all reticle parts; exclude text atlases/large HUD);
Origins ORIGINS_PLAN marker row refers to same City method, no validated
Origins-specific projection offsets found. VR_HUD_GUIDE8/10; framework11;
playbook FAIL-HUD-006/ch04 world geometry and FAIL-HUD-007/ch04 geometry families
at b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Source methods transfer, offsets don't.

Asylum STATIC target table intUCanvasexecProject at25e0a48 points toA588D0;
exec native reads FVector then callsABC2A0, returnsA58947. Project ABI thiscall
(view;out4,in3),ret8, reads actual view VP+210. Validate exact bytes, hook only
that caller inside tagged main draw; head-free base projection replaces result.
Keep world target/reference per game frame and render slot. Preserve original
marker art using perspective shader variant (target Scaleform transform only
writesXY and copies inputZW, so simply patching matrix would still be affine).
Fold exact card at target into Transform, expand ONLY confirmed4instruction
position computation toXYZW/remove inputZW restore; preserve UV/colour/depth
shader code. Capture LEFT/right marker layers from same actual widget draw,
post-DLSS/AFW per-eye full-screen composition, bypass normal floating-HUD route.
Reject unknown shaders, large/text atlases, stale/offscreen targets and masks.
Default ON live toggle, counters/capture status; GPU original shader, target
anchor/eye depth/yaw/roll/lean/game camera change and no-marker/control tests.

Exit: A2b JJ sees one remaining black window on menu->quit. Native run latest
correct A2b,12 authoredfade episodes, removed1502/bat693/failed0. Actual last
box draw isn't identified. Add exact literal clip-position VS support for
uniform untextured black rectangle fades (actual collected clip-VS family
currently rejected solely for no Transform); retain textured/coloured/small/
stencil/bat shapes. GPU observed original clip shader controls, archive run.
Don't claim exact exit owner or headset success without next launch/capture.
Build/check/install whileclosed, record protected/settings hashes. JJ one run
menus->game grapple head roll/yaw/lean then gamepad camera->returnfirstmenu->exit
last, F2 after wrong HUD/exit. No autonomous game launch/kill, FPS/menu reader open.

CURRENT A2b INSTALLED (2026-10-13 21:59 Sydney): full-field fades behind menu
text, default ON. HEADSET A2a partial progress accepted; JJ now sees jump cuts
between menus and requests fade to black, new menu text, fade back to Batman.
LIVE A2a correct build removed1754/bat92/missing0/failedCapture0; archived in
re/evidence/A2a-menu-run. A2b preserves the actual engine fade envelope from
classified rectangles; original windowed draw remains omitted, final WORLD
fades across full field before menu HUD after DLSS/AFW. Same opacity both eyes,
no fade on absent/stale/reset slots; original bat path retained. All11checks:
46native/45legacy transitions,8 rising/peak/falling envelope steps,14 both-eye
full-field/menu-visible GPU cases EACH route,4097DLSS pairs EACH route.19owned/
fiveprotected/cache/INI verified. HEADSET/RETAIL A2b PENDING: check forward/back
through all3 menus, loading/bats/video/game and return in ONE JJ launch, F2 after
any error then normal exit. Actual timing at every menu awaits headset; A2a
bat framing/all loading fades not explicitly confirmed. FPS/menu-state reader
open. Receipt A2b-build-checks.json and RE_NOTES A2b. Older entries historical.

## A2b implemented plan: full-field menu fades behind the text (2026-10-13)

HEADSET A2a: JJ reports great progress, black squares replaced by abrupt menu
cuts. Keep the successful bat/box handling; add full-field fading background
while new menu text appears. LIVE A2a correctly loaded; last status removed1754/
bat92/missing0/failedCapture0. Archive current log before installing.
Looked up: VR_HUD_GUIDE sections5/8/10 (retain panels through transition gaps,
fades are a separate full-field family; own draws after capture); framework11
HUD/menu separation; playbook ch04#ui-not-one-class, FAIL-HUD-007 at
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; City MENUFADE/FADEALPHA/FADEMIN
in BATMAN_CITY_PLAN lines856-874 (use actual fade alpha; low alpha must not
become a full-black flash; hiding the menu caused flashing). No new useful
menu-input trigger in Origins; reuse Asylum's already matched authored fades.

Use existing engine rectangle alpha/neutral-multiply darkness as envelope;
remove its HUD-stage draw but darken the ENTIRE final world image before menu
HUD composition, after DLSS/AFW. No button-polling timer or unknown menu flag.
Latch once for the exact pair, same alpha in both eyes, clear on a new render
with no fade or reset; bat geometry remains separate/after HUD. Preserve menu
text, partial fade opacity and original engine fade-out/fade-in timing. Default
ON live toggle; existing plain fade suppression remains. Tests through target
shader Scope for rising/peak/falling alpha, neutral fringes/coverage/multiply,
right reuse and stale/reset clearing; real GPU full-field fade then menu text
at alpha0/.25/.5/1 on both eyes and state restore. All required checks/install
closed and protected/settings hashes. JJ one launch all menu transitions/back,
loading/bats/game with F2 for remaining errors; do not assert headset result.

CURRENT A2a INSTALLED (2026-10-13 19:59 Sydney): A1z HEADSET REJECTED;
all black boxes and window-size bats remained. LIVE A1z geometryReads28653,
missingUpload0, managedReads0, CPUuploads1861, removed0/bat0. A2a corrects
City-style HUD coverage (multiply/additive preserve alpha; erase/mask attenuate
existing RGBA; mask-only writes add no coverage), authored AA10triangle fade
classification/COLOR1 coverage, and opaque bat sweeps beginning at one edge
before filling the 16:9 stage. Original animated geometry retained at full XR
resolution, fixed1.6x stage zoom/near0.8m union-eye plane. All11checks pass:
38native/37legacy transition cases,11HUD coverage cases +4 old black-box controls
EACH route,4097DLSS pairs EACH route.19owned/fiveprotected/cache/INI verified.
HEADSET/RETAIL A2a PENDING. Geometry fixtures reconstruct observed bounds/counts
using target programs, not complete saved retail vertex streams. Blend/shader
owner for EVERY retail box unproven. Next one JJ title/all3menus/load/bats/video/
game/return-menu launch, F2 after any residual then normal exit. Read RE_NOTES
A2a/re/evidence/A2a-build-checks.json. FPS/menu-state reader open. Older entries
below are historical; A1z headset-pending text superseded by rejection above.

## A2a: classify actual authored-stage fades and sweeping bats (implemented2026-10-13)

HEADSET A1z rejected, all boxes/bats unchanged. LIVE correct A1z/default flags,
geometryReads28653/missingUpload0/removed0/bat0. Native managedReads0 proves
actual reads came from new sparse static uploads, not managed copies. First
large black overlay:10 triangles/12 vertices, bbox[-2.007,2.006]x[-.918,.919].
Bat sweep at18:32:37.926..38.363 correlates with replay:288..3252 triangles,
starts only on left edge[-1.4246,-.744]x[-.591,.377], grows to full authored
16:9 stage[-1.426,1.424]x[-.746,.747], stencil0/opaque. Old +/-0.9 both-axes
rule never starts this bat episode; rectangle rule only2tri also misses actual
mesh family. Native untextured straight/gamma/coverage PS variants in target
cache6fe3/74ad/58f7; second COLOR coverage input was unsupported by interpreter.
Full vertex/material owner for every fade not saved, so distinguish this LIVE
extent evidence from GPU reconstructions and next headset acceptance.
Looked up again: FAIL-HUD-007/ch04 UI shape families, ch06 actual gate evidence,
checkoutb955f224; framework11, CityMENUFADESKIP/BATCUTS and OriginsO1F-H bounds
normalisation failure. Reuse only methods, no donor offsets/assets/writes.

Looked up again for alpha: City gfx_hud.cpp fixed_blend lines374-428 (normal
OVER adds alpha; additive/multiply preserve coverage; mask writes stay off).
Asylum HUD scope previously only detected additive, all other modes forced
ONE/INV_SRC_ALPHA, including multiply/erase/alpha-only passes. Correct coverage
through actual HUD hooks; normal OVER accumulates, multiply/add preserve, erase
 and masks attenuate existing RGBA, stencil/alpha-only on BB add no coverage.
GPU empty-sheet and seeded-sheet references + old-force-alpha controls.

Implement opacity-aware black/neutral sample handling (zero-alpha edges and
white multiplicative fringes), declared COLOR0/COLOR1 coverage inputs, geometric
rectangle proof from core triangles + thin AA border (not primitivecount2).
Require large centered rectangle, uniform dark core, actual area coverage,
limited32 triangles; small panels/colored/nonrectangular art stays original.
Start opaque dense bat sweep when its recorded edge-touching extent is present,
or large complex full stage using letterbox height>=0.65; keep exact-stage
bounded continuation and fixed1.6x zoom/near0.8m union-eye plane. Do not normalize
changing current bat bounds. Actual recorded shader ABI + authored extents/
Count10 AA rectangle/zero-alpha+COLOR1 cases and negative controls in GPU harness;
old rules must reject corresponding positives, original ring/regressions retained.
Build/check/install closed with protected/INI hashes. One JJ launch all menu/
loading/video/game/return transitions+F2, read removed/bat/copy/capture counters
before saying fixed. No autonomous launch/kill; FPS/menu reader stay open.

CURRENT A1z INSTALLED (2026-10-13 17:47 Sydney): corrected blocked UI geometry
source. A1y HEADSET rejected; LIVE geometryReads0/missingUpload21332, pre-game
CPUuploads0. F2 now LIVE accepted with8-10s paired replays. Geometry now reads
existing owned managed CPU bytes directly; sparse static upload shadows also
support non-DYNAMIC resources. Recorded actual ConstantColor/SHORT2 UI shader
pair used in512KiB static VB/IB GPU fades and bat mesh tests, with offsets/base/
min and observed alpha255/176/158.21native/20legacy transition GPU cases and
all11checks pass; both2284x2448 bat transport/alpha/near0.8m plane retained.
Contracting bat geometry stays on the SAME fixed stage within bounded episode,
no changing-bounds normalization.19owned/fiveprotected/cache/INI verified.
HEADSET/RETAIL A1z outcome pending; next one JJ title/menus/load/bats/video/game/
return-menu launch +F2 after residuals. FPS/menu-state routing open. Read
RE_NOTES A1z/re/evidence/A1z-build-checks.json. No game launched/killed.

## A1z: correct Asylum UI geometry source (implemented2026-10-13)

HEADSET A1y rejected: plain black fades and bat wipe remain small windows. LIVE
F2 now works: 6 replays, 8-10s paired pictures, focus accepted. LIVE main-menu
missingUpload21295/geometryReads0, CPUuploads0 before gameplay. Actual retail
ConstantColorPS/SHORT2 UI shader pair collected, not a donor fixture. SOURCE
root: previous geometry shadows only DYNAMIC buffers; managed UI storage is
already kept in managed9::Buffer::bytes but transitions never reads it. Thus
supported UI shaders reach geometry and cannot be classified. This is an exact
code-path mismatch, not evidence to change stage bounds again.
Looked up again: reuse ch06 gate counters/paired pipeline evidence, FAIL-HUD-007
ch04 geometry plus material, checkoutb955f224; guide11 and City bat bounds/Origins
O1F-H no per-frame normalisation. Actual pair B9B82C55E11FC149 is constant-color
PS and Transform/SHORT2 plus shared depth scale/UV transform. Its first recorded
red stencil draw is not itself proven to be bats; the supported pair later
reaches missing-upload gate, actual geometry coverage must still be checked.

Expose bounded read-only ranges from owned managed CPU buffer storage (no locks,
GPU readback or extra full-buffer allocation), valid only unlocked resources.
For default/retained route track small existing uploads regardless DYNAMIC flag
with sparse bounded shadow ranges (max16*256KiB; large allocations allowed but
only copied existing writes). Use actual recorded shader/declaration + static
managed large-buffer fixtures, including offsets/base/min indices and observed
black fade opacities; old dynamic-only route must fail positive controls.
Keep black fade vs animated complex geometry split, native full-XR bat target,
fixed stage zoom/0.8m union-eye plane after HUD/DLSS/AFW. Preserve animation when
complex bat bounds contract by matching the SAME stage transform within bounded
episode, not enlarging each frame to new bounds. Add counters/evidence for
actual eligible black UI pieces/placement when classification still rejects.
Build/check/install closed; settings/owned/protected hashes. JJ one grouped
launch title/all menus/load/bats/video/game/return with F2. No auto launch/kill.

CURRENT A1y INSTALLED (2026-10-13 17:14 Sydney): A1x HEADSET REJECTED,
black squares unchanged. Retail A1x removed/bat/geometryReads all0; actual draw
rejected before geometry. A1y supports target LOG/EXP gamma UI, multiplicative
fade blending and read-only ALWAYS depth in the captured UI lane. F2 accepts
current headset OR desktop focus/brief presses, requests still+preceding replay.
15 transition GPU cases EACH native/legacy route/all11checks pass;19owned/
fiveprotected/cache/INI verified. Original near full-view bat route retained.
Actual retail shader/depth owner and headset outcome STILL PENDING. Bounded
rejected UI bindings (max24) saved to Batman_Asylum/ui_bindings for exact followup.
Unsupported/textured/fixed-function draws retained. FPS and unknown menu-state
routing remain open. Next one JJ launch title/all menus/load/video/game/return,
F2 after wrong transition, normal exit. See RE_NOTES A1y and build-checks receipt.

## A1y black-square correction: implemented plan (2026-10-13)

HEADSET A1x REJECTED: black squares remain at all reported title/menu/load/video
points. LIVE two runs archived A1x-black-squares-run: correct A1x/default options,
removed=bat=geometryReads=missingUpload=0, CPUuploads84. No history hotkey lines;
only automatic stills saved. Geometry-size widening cannot fix an earlier reject.
Looked up again: playbook ch06 measure/dump stages and ch cross-project-index,
FAIL-HUD-007/ch04 (geometry+blend, not shader identity), checkoutb955f224;
framework11; existing City MENUFADESKIP and A1x references. Target cache UiSolidPS
uses LOG/EXP gamma and multiplicative fade color; A1x parser rejects it before
geometry. UiSimpleVS has post-Transform depth scale and vertex-color passthrough.
These are target STATIC leads; retail shader ownership still needs observation.

Fix target gamma/untextured shader support and actual blend-aware fade matching,
retaining unsupported fixed-function and textured paths until actual binding evidence. Preserve texture/colored/
small/complex art and original bat capture; test using original target shaders
plus old-reject controls, different depths/alphas and bounded upload formats.
Record bounded rejects with shader bytecode/declarations/actual constants for
any remaining retail gap in same launch, no readback or continuous disk IO.
Fix F2 focus gate for active headset app and brief presses; save still + history.
Build/check/install while game closed, protected hashes preserved. JJ ONE launch
through all three menus/load/video/game/return, F2 after residual; no autonomous
launch or kill. Keep source/GPU/retail/headset states distinct.

CURRENT A1x INSTALLED (2026-10-13 14:20 Sydney): separate original-geometry
bat capture at XR output resolution, fixed1.6x magnification and a shared full-FOV
0.8m plane after HUD/DLSS/AFW. Large uniform black rectangle fades omitted;
observed A1w letterboxed stage (+/-0.919052Y) covered. Both options default ON.
All11checks pass:9 original-target-shader transition cases EACH route, separate
mask transfer to BOTH2284x2448 asymmetric eyes/roll/IPD/alpha,96 particle +288
menu/gameplay cases EACH route and4097DLSSpairs perroute.19owned/fiveprotected/
shader cache/savedINI verified. A1w gameplay-only smoke kept. RETAIL/HEADSET A1x
transition classification not yet confirmed; unsupported geometry stays original.
FPS remains open. Next JJ ONE launch: title->all three menus->bat wipe->story
video->gameplay->return menu; F2 after a wrong transition saves prior10s. No game
launched/killed. Read RE_NOTES A1x and re/evidence/A1x-build-checks.json.

## A1x fading black rectangles and native full-view bats: implemented plan

JJ wants plain black rectangles/fades removed at title->menus (all three),
load->story video and video->game; animated bat wipe retained, enlarged, sharp,
near the face like City/Origins. At plan start A1w installed/game running; game later closed normally. A1x now installed; headset result pending.
Looked up: City BATCUTS/STAGEBOX/WIPEHD/BLACKFILL/MENUFADESKIP; Origins O1F-O1H
failure: never normalize changing black bounds or hide bats before real copy.
VR_HUD_GUIDE6/7 (fades are separate from HUD); framework11 UI lanes; playbook
FAIL-HUD-007 and ch04 #ui-not-one-class, checkoutb955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Do not transfer donor offsets/shader hashes/events or modify donor files.
STATIC Asylum Frontend.umap has six RSeqAct_FadeScreen objects and nativeMainV2
GFx movie. Original global shaders extracted/disassembled read-only: Transform
VS matrix rows c5..8, untextured ConstantColorPS c0 outputs only that constant.
This provides a provable black primitive discriminator; shader identity alone
must not classify shape. MCP404 bypassed with read-only headless decompilation.

Build owned-device post-UI geometry classifier: uniform black untextured 2tri
rectangle whose transformed corners cover the full stage -> omit plainfade.
Large complex black transition geometry -> capture ORIGINAL animated geometry
at full XR output resolution with fixed enlargement, preserve original animation,
no changing per-frame bounds. Separate near-head plane at0.8m added after HUD,
DLSS and AFW; original UI remains untouched for unsupported/nonblack/small pieces.
Bounded CPU shadows of dynamic UI buffers avoid synchronous GPU geometry reads;
UP geometry supported directly. Exact eye/generation association, state restore,
new GPU transport slot and same native/legacy ownership guards. Failures retain
original drawing or composite captured bats into ordinary HUD fallback. Live
switches/counters/captures provide next-run evidence. Original target shaders GPU
regressions for fades vs small panels/colored/title art, complex bat masks and
both-eye full-view coverage/state restore; full suite then build/installation
only after JJ closes game. One JJ launch: title->three menus->load->video->game
then return-menu, rolling capture when wrong. Source/GPU/retail/headset distinct.

CURRENT A1w INSTALLED (2026-10-13 01:03 Sydney): smoke correction now applies
only during gameplay; front-end menus retain original head-facing behavior, as
JJ requested. Guarded native IsMenuLevel flag is sampled once per game pair and
carried per exact render slot; unknown/teardown uses original facing. A1v gameplay
mapping retained.11 checks pass, including96 original-shader cases +288 menu/
gameplay/unknown routing cases EACH bridge.19owned/fiveprotected/cache/INI hashes
verified; Quality3 preserved. A1v LIVE59050 swapped corrections, no failures;
JJ wants that gameplay behavior kept. HEADSET A1w menu routing pending; FPS open.
Next JJ one launch: front-end smoke, opening gameplay smoke, return front-end;
rolling capture if wrong, normal exit. Read RE_NOTES A1w/receipt. Older historical.

## A1w RESULTS: gameplay-only correction built, checked and installed

Completed native-state gate without new engine calls/hooks/writes. Both eye
passes carry one menu verdict, so queued frames cannot pick up a newer menu
state.96 original shader cases +288 enabled-preference routing cases EACH bridge
pass; menu/unknown match original pixels and gameplay matches A1v reference.
Static target reader/setter guards and invalid-chain checks pass. All11checks,
4097DLSSpairs perroute and independent installation hashes verified. Retail
A1w state transitions/headset outcome pending; no autonomous game launch/kill.

## A1w gameplay-only particle facing: completed plan (2026-10-13 Sydney)

JJ wants A1v correction retained in gameplay, original behavior restored in menu.
LIVE A1v log retained in re/evidence/A1v-smoke-run: swapped corrections59050,
axisMismatch25726, noPass/failures0. HEADSET preference supports keeping gameplay
behavior; no blanket claim for all particle materials. Looked up: reused playbook
FAIL-STR-023/ch14 #per-view-decisions, commitb955f2244f7c5d5f3b3e5f403b1b6721a3362423;
City/Origins menu_state observe engine calls, no draw-count heuristics. Asylum
native intAWorldInfoexecIsMenuLevel at9c6f20 reads GEngine26b62b8 ->+44 -> TArray
at+30 -> first viewport -> cached+290 mask400 via9b62a0; setter9cb716 computes
map-name equality and updates exactly that mask. Use this existing engine signal,
read-only, snapshot at game-thread pair start and carry per exact render slot.
Unknown/teardown retains original facing. No timers/shader identity/menu geometry
heuristics. Runtime guards for actual reader/setter bytes, offline fake-chain
valid/invalid tests and original-shader menu/gameplay/menu GPU switch with default
ON. Keep A1v gameplay mapping. Build/check/install if closed, settings protected;
JJ one launch front-end menu/gameplay/return menu, smoke and capture.

CURRENT A1v INSTALLED (2026-10-13 00:50 Sydney): corrected Asylum's swapped,
negated particle-facing axes. A1u HEADSET REJECTED; retail named7/patch0/
axisMismatch72999 showed its guard blocked every particle correction.
Actual target binder now reproduced in96 GPU cases EACH native/legacy route;
all11checks pass. Default StableParticles ON/live toggle retained;19owned/
fiveprotected/cache/savedINI hashes verified. HEADSET A1v pending. Gamma A1t
accepted; FPS still open. Next JJ ONE launch: menu AND opening gameplay smoke,
head roll/yaw/lean, then gamepad-camera turn; rolling capture and normal exit.
See RE_NOTES A1v and re/evidence/A1v-build-checks.json. Older entries historical.

## A1v RESULTS: built, tested and installed

Completed planned guard/mapping fix, preserving ordering/signs, original constants,
real-eye projection and game-camera-only facing.96 original-shader GPU cases
EACH route;48 reproduce actual binder, all48 rejected by formerA1uguard.72
old-facing controls differ perroute. Head/game yaw/pitch/roll/lean/both eyes;
restoration/stateblock/stale/light-view guards and all11 regressions pass.
Independent19owned/fiveprotected/cache/INI hash verification recorded in receipt.
No game launched; headset acceptance remains pending JJ.

## A1v smoke correction: completed plan (2026-10-13 Sydney)

HEADSET A1u rejected: menu AND gameplay cards still rotate with head motion.
LIVE saved in re/evidence/A1u-smoke-run: StableParticles1, named7, patched0,
axisMismatch72999/noPass0/failures0. A1u shader fixture bypassed actual engine
parameter preparation; its uploaded axes matched our assumption rather than
Asylum's binder. Do not report the A1u smoke fix as headset-confirmed.
Looked up: reuse FAIL-STR-023/ch14 #per-view-decisions and framework09; playbook
checkoutb955f2244f7c5d5f3b3e5f403b1b6721a3362423. Target STATIC binder0xb540a0
normalizes negative InvViewProjection rows1/0 into CameraRight/CameraUp,
respectively (uploads0xb547ca/e7). These are negated SWAPPED view axes, unlike
the A1u fixture. Names/classification are correct; the strict axis guard rejects
this layout. No donor offsets reused; string refs/vtable/disassembly from target.

Correct guard/mapping to recognise both verified swapped and existing direct
layouts, retaining signs and current-eye identity. Preserve current safeguards
and per-draw restoration, game-camera-only basis/centre, real eye VP/lighting.
Add original-shader GPU cases using the target binder's swapped/negative axes,
including game/head pitch/yaw/roll/lean/both eyes and constant restoration. Old
A1u matching logic must demonstrably reject target-layout controls. Add bounded
first mismatch diagnostic and direct/swapped match counters for retail evidence.
Build all regressions and install if game closed; current configs/settings/cache
protected. JJ one launch menu/opening smoke + gamepad-camera turn and rolling
capture; headset result pending until JJ. No autonomous launch/kill.

CURRENT A1u INSTALLED (2026-10-13 00:08 Sydney): smoke/particle facing now
follows the head-free game camera, while projection and lighting retain the real
head-tracked eye. Enabled by default; live checkbox "Smoke follows game camera,
not head". Named target shader bindings, exact render-slot and uploaded-eye-axis
guards; constants restored after each draw. Eleven checks pass, including 40
original-Asylum-shader GPU cases on EACH bridge and 4097 DLSS pairs on each.
19 owned/five protected files, shader cache and saved renderingINI verified;
Quality3 retained. HEADSET: JJ accepts A1t brightness; A1u smoke pending.
FPS optimization remains open. Next JJ ONE launch: roll/yaw/lean at menu smoke
and opening gameplay smoke, then turn game camera with the stick (cards should
still follow that camera). Rolling capture/FPS in same run; normal exit.
Read RE_NOTES A1u and re/evidence/A1u-build-checks.json. Entries below historical.

## A1u RESULTS: built, GPU-validated and installed

All planned changes completed; original-shader tests cover screen-facing and
axis/position-facing cards, both eyes, yaw/roll/lean, changed game-camera aim and
authored spin. 40 cases per bridge match corrected reference pixels within1LSB;
28 old-facing controls differ per bridge. Constants restore bit-exactly;
stateblock refresh, stale-slot, nonparticle and light-camera guards pass.
Eleven checks pass, including native/legacy4097 DLSS pairs and8192 legacy Presents.
No retail game launched. Installation backup and independent hash verification
are in re/evidence/A1u-build-checks.json. Headset motion outcome pending JJ.

## A1u smoke/particle cards: completed implementation plan

HEADSET A1t brightness now accepted byJJ. Smoke cards rotate with head roll/yaw
and lean in menu/opening gameplay; JJ wants their facing to follow ONLY the game
camera. Captures A1t_233143/233144/233212 inspected; logs preserved separately
in re/evidence/A1t-smoke-run before fixtures. Still images show cards/clouds but
do not alone prove the rotation; JJ's motion report is HEADSET evidence.

Looked up: playbook FAIL-STR-023, ch14 #per-view-decisions (shared facing for both
eyes), checkoutb955f2244f7c5d5f3b3e5f403b1b6721a3362423; framework09 camera/basis;
City BATMAN_CITY_PLAN BILLBOARD2026-10-08 and its billboard.cpp head-free basis
map. City cbuffer sizes/offsets are NOT transferred. Ghidra MCP currently404;
not required for target's shipped DX9 shader reflection.
STATIC target evidence: RefShaderCache-PC-D3D-SM3.upk contains252 vertex shaders
with named CameraRight/CameraUp, LocalToWorld and ParticleUpRightResultScalars.
Actual original shader extracted/disassembled in re/evidence. Its CameraRight/
Up expand the cards; CameraWorldPosition also chooses camera-facing axis/velocity
bases (lean/IPD). CameraPosition supplies tangent-space lighting and projection
matrix supplies real eye view; those must remain unchanged.

Build target-owned named-CTAB parser and bounded weak shader metadata. Publish
game-base and eye matrices per exact native render slot through ctor/BeginDrawing.
At the particle draw, require both uploaded axes to match the current eye axes
(sign preserved), replace them with game-camera axes; adjust CameraWorldPosition
by gameCentre-eyeCentre to retain any translated origin. Preserve particle's own
animation/spin, projection/lighting/history; restore ALL constants immediately
after draw. No input buffer mutation or global matrix replacement. Stateblock/
reset invalidates cached shader metadata; unsupported shaders fail closed.
Default ON with live overlay checkbox and capture status/counters. Both native
and legacy owned-device paths share the module. GPU test uses original Asylum
shader: head yaw/roll/lean + both eyes/changed game camera, exact reference pixels,
old view control differs, constants restore, nonparticle/light-view guards.
Then full regression/installer protect current configs/options. JJ ONE launch:
menu roll/yaw/lean then same opening smoke, stick turn should still reorient;
rolling capture and FPS in same run, normal exit. Headset outcome remains pending
until JJ; no autonomous game launch/kill or donor changes.

CURRENT A1t INSTALLED: loading/movie gamma corrected for native BGRA source
sampling into sRGB eyeRTVs; typeless copy + matching source sRGB view avoids
encoding twice.6 real GPU ramp/color checks reproduce old128->188 and corrected
128->128, including UNORM/RGBA/BGRX fallbacks and alpha/black.11 checks pass incl
4097 native DLSS pairs and legacy regression;19 owned/five protected plus current
renderingINI hashes preserved. Same native9Ex route/resolution/lighting, saved
Quality3 retained. HEADSET A1s JJ: Off~65-80, On~60-80, improvement;90FPS still open.
Two retained A1s logs both DLSSon, so reported Off has no retained timing control.
Coarse512x512 R32F geometry chunks in BOTH eyes suggest shadow work; reuse/culling
needs target validation. Native+DLSS transfer/waits also cost severalms perpair.
Next JJ ONE launch: title/loading/story video gamma then same scene/FPS/capture,
normal exit. Gamma headset result pending. Read A1t-build-checks.json and RE_NOTES.
Older entries below historical.

## A1t: movie/loading gamma and remaining lighting cost

HEADSET A1s: JJ reports native65-80FPS, DLSS often60-80, better than previous
route; loading screens and cutscene videos now too bright. Two latest logs saved
in re/evidence/A1s-headset-runs BEFORE GPU fixtures. Both available logs use
Balanced/Quality; the reported Off comparison is not retained, so no matched
Off/on timing claim. LIVE native9Ex active, source1644x1764, XR format29 (sRGB),
zero helper fence/completion timeouts. Coarse slow chunks mix full-size F16 scene
passes; metadata does not establish individual shadow-map/filter cost.

Looked up: playbook failure FAIL-STR-033 and ch09 #first-pixels-gamma / typeless
views, ch19 existing per-view light scissor optimization; checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Framework10 submission/color formats;
VR_HUD_GUIDE room-fixed full-FOV screens. Donor gamma notes search found no
specific alternative. Source bug: flat_copy makes BGRA_UNORM SRV over already
encoded movie bytes, then writes them into sRGB eyeRTV, encoding twice. Existing
RGBA screen path already decodes; native9 introduced BGRA. Build typeless BGRA
copy + matching sRGB SRV when outputRTV encodes. UNORM output retains UNORMview;
native3D/HUD/DLSS source path remains byte-preserving. No arbitrary gamma slider.
GPU regression: native9->DX11 actual shared BGRA, grey ramps/color channel edges,
full projected screen and both SRGB/UNORM destinations, RGBA and BGRX controls;
old view must reproduce128->188, corrected should preserve128 within1LSB.
Then build/11 checks, install if closed, protect CURRENTconfigs/saved options.
JJ one launch: title/loading/story video brightness then same lit scene FPS and
rolling capture. No shadow quality reduction/reuse until pass ownership/camera
dependence validated; inspect current engine shadow settings and record leads.

CURRENT A1s INSTALLED: native Direct3D9Ex -> Direct3D11 shared GPU route is
now the default; no9On12/11On12/D3D12 rendering device on this route. Native
camera/FOV/HUD/controls/captures retained; managed CPU/GPU resource compatibility
and24bit INTZ depth transport added. Eleven checks pass, including both full-size
2836x3040 eye images, hooked HUD/depth, AFW GPU checks and4097 DLSS stereo pairs
on nativeDX11; legacy regression/8192 Presents also passes. ProtocolABI8 uses
unique per-launch DLSS session names.19 owned/five protected hashes verified;
saved DLSSMode0/resolution/lighting untouched. Exact A1q install backed up.
Usual VR shortcut chooses native9Ex; Previous Bridge shortcut retains9On12.
Retail FPS/stereo/ground/reset/AFW acceptance pending, not a proven90FPS fix.
JJ next ONE launch: same opening scene/DLSSoff30-60s, F2 if needed, normal exit.
Read NATIVE9_BRIDGE.md and re/evidence/A1s-build-checks.json. Older entries below
are historical; A1r was never installed.

## A1s native Direct3D9Ex -> Direct3D11 (INSTALLED; headset pending)
JJ explicitly asks to try more directDX11 route, removingDX12 translation.
A1r built/9 checks passed including timeline; NOT installed. Installed remainsA1q.
A1r isolatedbridge copyremoval saves only~.15ms/pair; not main slowdown proof.
Source checkpoint re/backups/A1r-source-validated; evidence A1r-ctest-soak-built-not-installed.
Looked up: playbook ch10 #d3d9-xr-interop-routes/#who-owns-9on12 managed-pool
closure and explicitsync rules, commitb955f2244f7c5d5f3b3e5f403b1b6721a3362423.
MicrosoftDX9LH sharing/device-lost and D3DPOOL docs:9Ex doesn'tallocateMANAGED;
blind poolremap discards locks/content/restore semantics. Arkham-fixer native9
depth-buffer detection INTZ replacement inspected as lead only; preserve rawZ
precision and originalstencil semantics; no offsets/shaderhash transfer.
FIRST native GPU harness evaluates plain9, Exfactory baseCreateDevice, fullEx:
managed allocation/locks, shared creation AND openingexistingEx texture inplain9,
explicitCPUevent completion, matchedAdapterLUID toD11, exacttwo-colorGPUreadback.
Then implement compatibility for target managedtextures/cubes/vertex/indexbuffers
withCPU shadows, GPU copies, lock/readback/dirty/autogen/state identity/reset, not
argument-only remap. Keep native9 path in isolated module/optionalstartup route;
share twoeye color/HUD targets withD11, producer event queries + bounded consumer
completion before reuse (9Ex shared textures have no keyed mutex). Existing native
camera/IPD/pose/pacing/UI/core/helper retained. Native depth path needs INTZ/RESZ
validated GPU transport and24bit preserving packing, not half-floatfarZ rounding.
Source and GPU checks before retail launch; no autonomous game launch/kill,
no gameexe/globaldriver/City/Origins edits. Current bridge remains rollback.

## A1r direct owned-resource transport and scene timeline (in progress)
Full-size isolated completion result: old0.423ms/pair vs direct0.277ms/pair,
GPUelapsed0.417 vs0.272. Pixels match, but0.15ms savings is NOT enough toexplain
orclose60-80FPS gap. Do not present copy removal as main fix or ask another
FPS-only launch. Add bounded nonblocking GPU timeline in existing sampled scene:
64-draw chunks (up to4096 draws/eye), first target/PS hash/Z/blend metadata and
CPU spans. Keeptop8 slowest completed chunks per eye eachheartbeat; report cap
exhaustion/rejected samples. Never wait/flush in retail collector, aggregate only
afterparent timestamp/disjoint completes. Existing timing sampling2pairs/s bounds
overhead. This will locate heavy scene work (mixed chunks remain coarse evidence),
not infer translation overhead from gameage. Explicit runtime9On12-only mono
control remains needed if translation vs engine/shader work cannot bedistinguished.

Looked up again: A1q-run preserved before fixtures. Depth copies zero between
captures, helper notstarted; base scene CPU12-15ms/pair in slow sections; bridge
colour~.54ms/eye, XR~.4ms/eye, desktop~.3ms. A1q FPS remains60-80 perJJ.
Playbook ch06 completion-vs-enqueue, ch19 blocking-wait-floor/per-section costs,
cross-project-index D3D9 interoperability rows, commitb955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Microsoft CreateWrappedResource/ReleaseWrappedResources/ReturnUnderlyingResource:
shader-read bind override only ifresourcepermits; acquire scoped, release+flush
returnsCOMMON then signal ownedqueue fence before9On12 return. Preserve returned
resource safety, not a blind Flush deletion. Existing City/Origins do not own
D3D9 resources; ABI/addresses unchanged. Remaining base scene dominates; no
claim colour alone explains gap. Remove redundant GPU image copy + full-source
RGBA conversion by sampling actual checked-out BGRAsource during Present lease,
shader-convert at existing XR lens/DLSS-input draw. Guard samplecount/format/
resourceflags/viewcreation; keep original owned-copy route on unsupportedsource.
Flat/core fallback must shader-convert between unrelatedBGRA/RGBA families.
Offline consumer will inspect actual borrowed9On12 BGRA pixels whileacquired,
comparefinalRGBA output versus stockcopy with bothasymmetric lenses, then change
nextgameimage toprovefreshness, fence/drain/reset. Tests retain legacycopy path,
HUD/DLSS/AFW/pacing/captures. Bench completed full-resolution paths ifpossible;
report enqueue separately. Installonlyafterchecked source supports directroute;
no game launch, no resolution/lighting/savedDLSS changes. FPS still requiresJJ.

## A1q installed: optimize native rendering with DLSS off

HEADSET A1p: JJ says ground/shadows fine, no FPS gain; DLSS off about75FPS
versus40-50 withDLSS in the opening gameplay section. Desktop4K about230FPS;
City/Origins faster. The base VR deficit remains a priority independently ofDLSS.
LIVE evidence preserved in re/evidence/A1p-dlss-ab BEFORE fixtures. Previous log
is menu DLSSon thenoff; original gameplay DLSSon timing log is unavailable, so
these are not matched gameplay timing captures. Current Off log source2836x3040
per eye (17,242,880 pixels/pair), output2284x2448 per eye; desktop4K8,294,400.
This is2.079x source pixels, plus two scene passes and the D3D9->9On12->11On12
interop path. No inference that pixel count alone explains the observed FPS.
Off scene CPU about4.3-5.35ms/eye, GPU elapsed about4.95-6.08ms/eye; elapsed
includes queue effects/interception, not pure GPU busy time. Unused depth copy
still cost .45-.68ms CPU/eye. A1p nativePSO creations only395/17765requests;
no FPS gain despite much better reuse. Long OOM stability remains unaccepted.

Looked up: playbook docs/19-d3d12-and-performance.md #dlss-in-vr, performance
ledgers and per-section bridge-cost table, checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; VR_ESSENTIALS sections3/4/9 and
framework04-graphics-api-interception (owned-device boundaries). City
DLSS_PLAN.md D1b/D2: modern model eval can exceed rendering savings; early
clarity fix removed inputFXAA and adjusted mip bias. Origins dlss_bridge.cpp
CAMMV/MIPBIAS method inspected; target semantics still require Asylum proof.
NVIDIA official NvApiDriverSettings.h SR mode3=snippet-controlled, preset13=M;
NVAPI GetNGXOverrideState is READ ONLY. SDK declaration/query IDs from
references/luma-framework-2026-10 @5d7ee5470d8f09e27dfe529a386af46d8c6f8e33.

SOURCE A1q: HUD scene-depth selection/DS queries/transport run only for a depth
consumer: DLSS, AFW, or an explicitly armed still capture. Off/AFWoff retains
normal game depth writes and HUD extraction while skipping unused copies/fences/
flushes. Still captures arm the next scene and wait up to1.5s for fresh depth;
titles/flat captures do not wait, absence of depth cannot stall forever. Rolling
stereo history is unchanged. Texture-detail hook still forwards every engine
texture bind but skips descriptions/COM probes/redundant sampler writes when
its correction is inactive. Active correction inspects bound textures at scene
start (handles enabling while already bound); original sampler reads, reversible
mild bias and HUD exclusion retained. Temporal jitter period follows actual
output/input ratio instead of fixed32; one phase for both eyes, independent
histories and existing reset/relative-camera motion rules retained. No resolution,
lighting, game quality, backend or frame-pacing reduction.

DLSS investigation: official signedCNN3.7.20 candidate downloaded from NVIDIA
v3.7.20 @a11125971f4d5f19ce17ca8cbe6f703eaa16de76; provenance/license in
third_party/dlss_cnn. Initial benchmark labels/timings are REJECTED as model
comparison: verbose NGX proves provider replaced the requestedDLL with cached
310.9.0 (160_E658700.bin) and forced global preset13. Mode3 in driver log is
snippet-controlled, NOT forcedQuality. No claimed CNN speedup, no CNN deployment.
Current310.9.1 package retained; provider-selected path recorded. Read-only own-PID
NVAPI query after feature creation confirms status0/preset13/flags23f or63f;
override displayed in overlay and capture status. Global/app driver settings are
unchanged. Benchmark --cnn is a local research flag, not a shipped model choice.
UltraQuality native NGX feature request was rejected in the research benchmark;
that pre-existing option is not accepted as a supported runtime mode yet.

OFFLINE: final9 checks pass; A1q-ctest-soak.txt includes4097 completed DLSS pairs,
8192 Present soak, two separate eye histories/colour, rollback/mode-off/reset,
depth/AFW/HUD/lens/fullsource fixtures and previous260 static/dynamic pixel checks.
New actual scene write/clear/rebind fixture makes ZERO depth transfers without
consumers, then explicit still request preserves .25/.75 physical scene depth
beforeclear in both eyes; DLSS/AFW requiredepth verified.512 real stock texture
binds make ZERO metadata probes, then active mild bias/readback/restore still
passes. Two test-only setup mistakes corrected before deployment: unused scene
wrote colour into the fixture's blue source, and its default-pool DS was retained
acrossReset; now colour writes suppressed and DS released. Budget fixture gets a
fresh sampled pair after the preliminary scene. NVAPI helper-only follow-up GPU
check passes4.45s; A1q-ctest-helper-final.txt/A1q-helper-final.log.
These are offline correctness proofs, not FPS/blur/OOM/headset acceptance.

INSTALLED:18 owned destination hashes verified and five CURRENT protected
config/executable hashes unchanged; aa_rendering.ini byte-exact unchanged,
DLSSMode0 preserved, no active game/helper/profilejournal. Backup, hashes and
validation in re/evidence/A1q-build-checks.json. Added NVIDIA NVAPI MIT notice.
No autonomous game launch/kill, City/Origins edits or upstream reference edits.

NEXT SINGLE JJ run: same opening gameplay/native with savedDLSSoff for30seconds,
note FPS then exit normally. Ordinary logs/captures automatic; no comparison
button or crash wait required. Check depth transport falls tozero between the
on-demand captures and whether scene/pair time falls. If stillunder90, investigate
remaining base scene/translation/colour submission cost rather than declaring
DLSS-off acceptable. Separately prepare a controlled Asylum-only DLSS model
choice that actually survives driver override; don't change global driver policy.
Remaining: baseFPS, DLSS cost/mid-distance clarity, long stability and retail AFW
flicker/ground artifacts/head jitter. A1p ground/shadows accepted byJJ only.
Earlier checkpoints below are historical.

## A1q DLSS cost/blur (in progress)
Looked up: playbook ch19 #dlss-in-vr (registration/jitter/input hygiene/presets),
commit b955f2244f7c5d5f3b3e5f403b1b6721a3362423; City DLSS_PLAN D1b/D2
results: modern310.9.1 evaluation cost dominates, preset hints same cost, noFXAA
plus mip bias sharpened. Origins dlss_bridge CAMMV/MIPBIAS conventions inspected.
NVIDIA SDK header defaultK is transformer/highercost; E deprecated in310.9.1.
NVIDIA v3.7.20 official CNN DLL pinned a11125971f4d5f19ce17ca8cbe6f703eaa16de76,
Authenticode valid NVIDIA, source hash/license in third_party/dlss_cnn.
JJ: noFPSgain A1p/ground fine; DLSSoff~75FPS, DLSS midrangeblur. Saved previous
log is menu DLSSon thenoff; original gameplay DLSSon log no longer in one-generation
history, don't falsely call these matched gameplay timings. Current full native
Off actualsource2836x3040 vs Balanced savedinput1322x1478/out2284x2448. City case
plus helper~3ms/eye makes model cost plausible, not proof of handoff-free efficiency.
A1p nativePSO creations350->395 insteadthousands, ground HEADSET accepted, no long
stability acceptance. Preserve A1p-dlss-ab before helper fixtures. Benchmark current
and signed CNN library at equal dimensions, validate actual loaded DLL path/version
and supportedAA modes before exposing model choice; retain savedOff. Fix mode-sized
jitter sequence and add live texture-bias choice only with GPU validation. No shader
blur pass skipped without evidence; no default resolution/quality degradation.

## A1p installed: exact dynamic depth bias

LIVE A1o:665/679 sampled nearest misses change integer DepthBias;668 raster,
16 shader, zero root, one slope and no clamp differences.22835 requests/1085
hits/21750 creations; freeVA1576->882MiB. Samples are heuristic nearest-key
comparisons, not universal proof, but repeatedly isolate the varying bias.
A1o log/profile/INI preserved in re/evidence/A1o-run before GPU fixtures.
A1o reuse/FPS/memory remain unaccepted. No retail A1p result yet.

Looked up: Microsoft DirectX-Specs VulkanOn12 dynamic depth bias/API/DDI0099:
https://github.com/microsoft/DirectX-Specs/blob/master/d3d/VulkanOn12.md
INT bias already casts toFLOAT; dynamic flag enables RSSetDepthBias, and
SetPipelineState must restore its declared bias. SDK26100 OPTIONS16 and
GraphicsCommandList9 support gated on actual adapter; real 4070Ti test supports
it. Playbook ch19 identity/lifetime/budget and ch06 proof discipline, checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; existing framework04 interop.
TranslationLayer PipelineState.cpp RoundTripPSOs/GetCachedBlob inspected (saved
A1o-run/PipelineState.cpp): preserve original cached-blob descriptor compatibility.

SOURCE A1p: new dynamic_pipeline.cpp/h. Supported owned-device graphics PSOs
use shared native PSO with dynamic-bias flag and zero initial bias/clamp/slope.
Each original request returns a compact COM variant retaining its EXACT integer
bias and FLOAT clamp/slope, and a shared owned descriptor/native PSO template.
SetPipelineState, CreateCommandList initial state, Reset and ClearState unwrap
native PSO and then set exact original dynamic values. SDK-defined vtables,
real current-device probe/interfaces/hooks checked; no engine offsets reused.
Immutable root/shaders/blend/depth/input/formats remain in exact cache key.
No bias quantization or lighting/quality changes. Standard PSO COM identities,
GetDevice and per-variant private metadata retained. GetCachedBlob lazily creates
an original static counterpart and returns a compatible original blob; requests
carrying cached blobs remain stock. Unsupported hardware/APIs, existing dynamic
PSOs, unsupported IID/cached blobs/nonfinite values or creation failures retain
original paths. Compact variants and shared templates log live counts; any
remaining translation-layer/software pipeline growth is not eliminated by proof
of native PSO reuse. Weak template registry bounded512, native cache512 entries/
16MiB keys; caller-held object lifetime not artificially capped. Drained reset
clears mod cache/template lookup references, not caller ownership.

OFFLINE final9 checks pass, A1p-ctest-soak.txt. Real GPU4096 distinct integer
bias variants hold caller refs while sharing ONE native PSO,4095 reuse hits.
260 pixel comparisons vs original static factory: D24/D32, LESS/GREATER depth,
13 bias/clamp/slope cases including INT_MIN/MAX and +/-16777217, each across
Set/Create/Reset/Clear/rebind. Distinct original images ensure meaningful test;
full16x16 RGBA equality. Standard COM/private metadata distinct between variants,
original static cached blob reload and caller-held render after cache clear pass.
Existing real D3D9 interop/depth/HUD/AFW/DLSS checks also run with dynamic path
active and pass; extended4097 DLSS pairs/8192 Presents. This is GPU fixture proof,
not headset FPS/crash/shadow acceptance. A1p-build-checks marks retail pending.
Installed17 owned hashes verified; five CURRENT protected hashes unchanged,
no active profile journal/game/helper. No autonomous game launch/kill, no other
Batman mods, upstream reference, executable or driver policy modified.

NEXT SINGLE JJ launch: same beginning scene/native for~1minute, note FPS and
whether ground/shadows look correct in both eyes; normal exit. Automatic logs
show native creation vs variants, exact binds, live handle/template counts,
static/blob fallback count and memory trend. No comparison/crash wait needed.
Read evidence before declaring success or choosing next cause. Retail stability,
frame rate, AFW flicker/ground/jitter remain pending.

## A1p dynamic depth bias (in progress)
Looked up: A1o samples665/679 depthBias changes, zero root changes,16 shader,
~22k calls/70s, freeVA1576->882. Microsoft DirectX-Specs VulkanOn12 dynamic
state section: DDI0099 INT bias was already castFLOAT; dynamic bias supported
via OPTIONS16/GraphicsCommandList9 and PSOflag4, SetPipelineState restores bias.
Playbook ch19 full state identity/lifetimes and ch06 proof discipline at
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; existing framework04 interop.
Implement feature-gated exact per-variant PSO handle + shared native dynamic
PSO; unwrap SetPipelineState/CreateCommandList/Reset/ClearState, set exact
original bias/clamp/slope each bind. Proper COM/private data; original cached
blob compatibility via lazy static counterpart with owned shared descriptor.
No quantization. Probe real driver capabilities and hooks first; fallback stock
on unsupported features. D24/D32, forward/reverse depth pixel references across
all bind paths, thousands distinct bias variants/shared native state, cached
blob and private-data/lifetime checks before deploy. No autonomous game launch.

## A1o installed after A1n retail failure

LIVE A1n:38560 graphics requests,788 hits (~2.04%),37772 successful creations;
cache stayed512 entries/~3.9MiB keys but driver/engine process VA still depleted
1613.7->585.1MiB, private1721->2640MiB. Native pairs~50-55/s in gameplay.
Saved all logs/profile/INI in re/evidence/A1n-run before fixtures. A1n exact reuse
is NOT an accepted retail fix; don't claim 90FPS or cured OOM. Cache growth itself
bounded; low reuse means active/irrelevant differences or working-set churn need
measurement, not blind cache-size/bias quantization.

Looked up again: playbook ch06 measurement and ch19 full identity/lifetimes,
cross-project-index/framework routes (no direct PSO cure), checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Microsoft D3D12_BLEND_DESC explicitly
ignores RT1..7 when IndependentBlendEnable=false:
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_blend_desc

SOURCE A1o only omits documented ignored blend slots from KEY for valid blend
values. Malformed/future values retain full original key; independent blending
retains all eight slots. All shaders/raster/depth/input/RTV formats still exact.
Original descriptor/API arguments unchanged. Proposed omission of inactive RTV
formats was REJECTED by real GPU validation: setting unused RTV7 format can fail
original API. Keep all eight formats and regression-check exact failure HRESULT.
No depth-bias quantization, disabled lighting, reduced resolution or cache-size
increase. Pipeline reuse retains512 entries/16MiB key limit.

Adds sampled (one/32 misses) nearest key-group fingerprints: root+device,
shader bytes, stream output, blend, raster, depth/stencil, input and target/flags;
depthBias/slope/clamp difference counters. Candidate selection is diagnostic,
not proof of a causal field or cache equivalence. Full bytes still determine
actual reuse. Heap trace/private/free/driver usage and creation counters remain.

OFFLINE nine final checks pass (A1o-ctest-soak.txt). Real GPU verifies moved
bytecode1024-request reuse; documented ignored blend fields reuse, active independent
blend stays separate; direct original and cached paths render identical red pixels;
red/green shaders separate. Invalid sample count, unused format and malformed blend
requests preserve original HRESULT. Nearest sampling sees known bias variations.
Eviction/clear/caller-held GPU render survives; previous full depth/HUD/AFW/DLSS/
profile checks and4097 DLSS pairs/8192 Presents pass. Synthetic test is not retail
FPS/pressure acceptance. Installed17 owned and five CURRENT protected hashes
verified; no active profile journal or game/helper. Audit A1o-build-checks.json.

Next SINGLE JJ run: same native opening gameplay scene30seconds, observe FPS,
normal exit. Automatic field-group/bias counts and memory trend should distinguish
real state churn from ignored blend misses. No comparison/crash wait. Read evidence
before any new tweak. User drives launches; no City/Origins/upstream edits. Retail
OOM/FPS, AFW artifacting/ground/jitter remain open.

## A1o failed A1n lookup and follow-up (in progress)
Looked up again: ch06 measurement/ch19 identity, cross-project-index and guide
routes (no direct PSO cure), playbook b955f2244f7c5d5f3b3e5f403b1b6721a3362423.
A1n LIVE38560 requests/788 hits (2.0%), VA1614->585MiB; FPS~50-55. Full exact
cache is not a successful retail fix. Microsoft D3D12_BLEND_DESC documents that
RT1..7 are ignored when IndependentBlendEnable=false; descriptor NumRenderTargets
bounds active format array. Normalize only these documented inactive slots in
cache KEY, not original requests; valid/invalid variant GPU regression. Sample
nearest-key differing groups (root/shader/SO/blend/raster/depth/input/target) and
bias deltas in same build so the next run can resolve active-state churn without
another blind cache-size or bias tweak. No graphics quality/state quantization.

## A1n installed: exact graphics pipeline reuse

LIVE A1m confirms21338 graphics pipeline creations by14:40:19 (~65s), zero
creation failures;16 large reservation traces carry pipelineActivity1 and
full call chain: NVIDIA ->D3D12Core ->owned graphics hook ->d3d9on12 worker.
FreeVA falls1593->903MiB after startup. Actual trace saved A1m-run. This proves
pipeline creation contributes to address-space growth. It does NOT prove all
creations are duplicates, all growth is PSOs, or creationCPU alone explains
40-50FPS (cumulative graphics creationCPU~1033ms, async worker costs not measured).

Looked up: playbook ch19 canonical identity/full descriptions/lifetime budgets,
commit b955f2244f7c5d5f3b3e5f403b1b6721a3362423; Microsoft D3D12 graphics
pipeline descriptor and Managing Graphics Pipeline State (immutable state reuse):
https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_graphics_pipeline_state_desc
https://learn.microsoft.com/en-us/windows/win32/direct3d12/managing-graphics-pipeline-state-in-direct3d-12
Upstream9on12 PipelineStateKey/GetPipelineState/Trim remains a lead; current
Windows cache behavior is established only by LIVE intercepted API calls.

SOURCE: new pipeline_cache.cpp/h. Exact content key serializes every descriptor
field (no padding), actual shader bytes, semantic strings, stream declarations/
strides; canonical device/root identity with retained root reference. FNV hash
is only lookup; full byte equality required.512-entry LRU/16MiB key storage,
512KiB maximum key. Driver PSO memory is opaque and not included in key limit.
Unsupported IID/null/oversize/unsupported cached-blob requests use original API;
failed creations are not cached. TLS recursion guard, serialized cache mutation,
COM references retained; caller-held objects survive eviction. Drained reset
clears cache references. Shader/raster/bias/lighting state unchanged; no lossy
bias quantization. Passive counters log requests/hits/real creations/bypass/
evictions/live/key bytes. Stream PSOs remain original (zero in A1m).

OFFLINE final nine checks pass, A1n-ctest-soak.txt. Real GPU:1024 requests from
moved shader byte buffers reuse same COM PSO; different red/green shader gets
separate PSO. Invalid request preserves original failure HRESULT.520 bias-state
variants force eviction within limits; clearing removes entries; caller-held
red/green PSOs then render exact9-point pixels using D3D12 command lists/readback.
All prior stereo/depth/HUD/AFW/DLSS/rollback checks pass including4097 DLSS pairs
and8192 physical Presents. This synthetic reuse is not a measured retail duplicate
rate or proof of FPS/crash fix. A1n-build-checks explicitly marks retail pending.
Installed17 owned files; five current protected files unchanged, no active profile
journal/live game/helper. Backup/runtime hash in A1n-build-checks.json.

Next SINGLE JJ launch: same opening gameplay scene in native, observe FPS for
about a minute then normal exit. No transfer comparison required. Automatic
reuse and memory counters establish whether actual repeated creations are
eliminated and whether remaining pressure comes from distinct states. Observe
image correctness along with speed. No autonomous launch/kill, no City/Origins
changes; ground/AFW/jitter and retail performance/stability acceptance remain open.

## A1n pipeline reuse (in progress)
Looked up: A1m LIVE/deep trace proves21,338 graphics PSO calls/minute and
16 large reservations inside PSO creation, not just a driver module guess.
Playbook ch19 canonical identity/full descriptions and lifetime budgets,
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; Microsoft graphics PSO descriptor
and Managing Graphics Pipeline State define immutable reusable state. Upstream
9on12 cache GetPipelineState/Trim is a lead. No bias quantization or state changes.
Implement bounded512-entry/16MiB-key exact content cache, retain root identity,
include all scalar/array/shader/input/SO fields, bypass cached blobs/unsupported
interfaces/oversized keys, clear at drained Reset. Real D3D12 red/green rendering,
repeated requests from copied bytecode, distinct-state/failure/eviction/reset
checks before install. Actual duplicate hit rate/FPS/OOM still requires headset.

## A1m installed: deeper allocation provenance and pipeline counters

A1l user run preserved in re/evidence/A1l-run/, including normal exit. It
reproduced VA depletion WITHOUT transfer comparison: after initialization
freeVA1613.9MiB/largest1536.1 ->702.1/largest610.4MiB. GPU usage remained well
below budget. Recurring15.8MiB heap reservations terminate in NVIDIA/D3D12 at
stack12; those frames do not establish which rendering operation requested
them. A1l's stack limit was too shallow. Forty-to-fifty gameplay FPS and original
retail OOM remain unresolved; no new headset performance acceptance claimed.

Looked up again: playbook ch06 measurement/environment and ch19 lifetime/budget
references, checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Microsoft
D3D9On12 src/9on12PipelineStateCache.cpp GetPipelineState/Trim describes pipeline
creation and cache trimming. Saved upstream-main source in A1l-run/d3d9on12-source;
this is a lead, not evidence that the installed Windows build leaks PSOs.
https://github.com/microsoft/D3D9On12/blob/main/src/9on12PipelineStateCache.cpp

SOURCE A1m extends reservation stacks12->48. Adds passive CreateGraphicsPipelineState
and Device2 CreatePipelineState counters and CPU durations; unchanged descriptors,
arguments/results and cache policy. Thread-local pipeline activity is saved with
reservation records; asynchronous driver allocations may lack that marker.
Counters cover process calls through hooked implementations; nested calls may
count twice. Allocation ring remains bounded128; no allocator logging/symbol work.

OFFLINE final9 checks passed: actual allocation trace/release; observed11 graphics
pipeline creations/zero failures while existing colour/depth/HUD/DLSS/reset checks
pass;8192 Presents and4097 DLSS pairs remain stable in the tiny synthetic fixture.
Evidence A1m-ctest-soak.txt/A1m-build-checks.json. Installed17 owned hashes verified;
five restored-baseline protected hashes unchanged by installation. Current A1l
profile journal remained after exit; archived journal/current bytes into
A1m-profile-recovery, validated all four backup hashes, restored their exact bytes,
then removed only the journal. No later desktop settings were present in current
configs: they still carried1644x1764 VR profile. No game/helper process running.

Next single JJ launch: ordinary gameplay at the same opening scene for30seconds,
then normal exit, no comparison/crash wait. Deeper allocations plus pipeline count
will decide whether growth is pipeline compilation/cache or another driver/engine
path. No autonomous launch/kill or changes to City/Origins. A1m is diagnostic,
not a claimed OOM/FPS fix.

## A1m follow-up to A1l run (in progress)
Looked up again: existing ch06/ch19 lifetime and budget references at
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; Microsoft D3D9On12
9on12PipelineStateCache.cpp GetPipelineState/Trim confirms PSO allocation and
cache policy are relevant leads, not proven target causes. Source snapshot
saved in re/evidence/A1l-run/d3d9on12-source (upstream main, not Windows binary).
A1l reproduction without comparisons: freeVA1614->702MiB. Recurring15.8MiB
heap reservations have NVIDIA/D3D12 frames;12-stack limit hides origin.
Extend to48 and count/timestamp graphics and stream PSO creation; preserve all
arguments/results. No speculative graphics changes. A1l logs preserved.

## A1l installed: allocation-source evidence; crash/FPS still open

JJ confirms A1k remains40-50FPS; third DLSS transfer comparison coincided with
E_OUTOFMEMORY. Preserved complete run before fixtures: re/evidence/A1k-crash/.
LIVE: first comparison02:22:56; VA largest free already fell2045->672MiB
before it. At02:24:14 game Present returned8007000e; free158.1MiB,
largest21.4MiB, private2473.4MiB, reserved1077.2MiB. Local GPU~1794MiB
against11226MiB budget. Wrapper count3 throughout; helper texture set gen1,
two feature creations only. Address-space depletion precedes comparisons;
this is not evidence of exhausted VRAM or proof that comparison switching leaks.
Test1 safe50.25/overlap55.38/safe67.99; test2 safe42.42/overlap49.50/safe43.25;
test3 safe42.25 then crash during overlap. Scene stability uncontrolled, so
these do not justify a clean speedup claim. Frame rate remains unresolved.

Looked up again: playbook ch06 measurement/environment, ch19 lifetimes/budgets,
FAIL-PACK-005 (target already LAA), commit b955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Microsoft D3D9On12 issues72/90 are similar reports only, not applicable fixes:
https://github.com/microsoft/D3D9On12/issues/72
https://github.com/microsoft/D3D9On12/issues/90

SOURCE A1l adds bounded NtAllocateVirtualMemory current-process MEM_RESERVE
tracing for >=1MiB reservations,128 records/12 stack PCs, thread-local recursion
guard, no heap/log/symbol calls inside detour. Module offsets emitted at heartbeat.
Dropped-record counts and address-reuse caveat explicit. This covers that API,
not every mapping/allocator; absence of stacks does not exonerate a component.
No quality/performance setting changes and NO claimed retail fix.

OFFLINE:9 checks pass including real2MiB reserve/stack/release test; optional
AA_DLSS_SOAK fixture completed8192 D3D9 Presents/transfers (freeVA1479.2MiB
through all samples) and4097 paired NGX completions (private memory245.6->247.4
then223.2MiB), correct eye colours/ownership/timeouts. Tiny synthetic images,
no actual engine/XR runtime; does not prove full-resolution game stability.
Saved A1l-ctest-soak.txt and A1l-build-checks.json. Installed17 owned files,
five current protected hashes unchanged. Source IDs and manifest A1l.

NEXT single JJ launch: ~30s ordinary same-scene gameplay, normal exit. Do not
repeat comparison or wait for crash. Read reservation call stacks and budget
trend before any further speculative fix. No autonomous launch/kill, no City/
Origins edits. Ground/HUD/AFW/head-pose and performance acceptance remain open.

## A1l investigation: repeated OOM, 40-50 FPS

Looked up again: playbook ch06 measurement/environment and existing ch19
three-ledger/resource lifetime references, commit b955f2244f7c5d5f3b3e5f403b1b6721a3362423.
New Microsoft D3D9On12 issues72/90 are reports, not established target fixes.
A1k LIVE failure at Present 0x8007000e: VA free158MiB/largest21.4MiB,
private2473MiB, reserved1077MiB; GPU1794MiB of11226MiB budget.
VA depletion starts BEFORE first comparison. Wrapper count remains3, no
creation growth. Source transfer comparisons only switch overlap; captures
are bounded but their lifetime and long-running shared-resource path need
stress validation. Preserve re/evidence/A1k-crash before fixtures.
Plan: stress existing real DLSS path across thousands of pairs, isolate growing
allocation route, then fix/retest/build before requesting another headset run.

## A1k installed 02:19 Sydney, 2026-10-12

HEADSET/user evidence: A1j did not improve gameplay (~50 FPS, blank wall90),
while desktop4K in the same section averages230. Crash screenshot reports
E_OUTOFMEMORY at D3D9Viewport.cpp:202. Neither the remaining frame-rate gap nor
the retail crash is resolved/proven fixed by this build. Target already LAA.
LIVE logs preserved in re/evidence/A1j-crash-performance: sceneCPU~5ms/eye,
GPU elapsed~4.5-5ms/eye, DLSS helper~2.5-3ms/eye; classifier cost is small.

SOURCE fixes: removed duplicate Native radio control in AFW details and gave
those controls their own ID scope; unique Floating screen selector ID. Bounded
8-entry canonical-resource/full-description wrapped-texture cache replaces
per-capture wrapper creation. Acquire/release/fences/ReturnUnderlyingResource
remain per capture; Reset releases cached wrappers and flushes deferred cleanup.
AFW now performs physical desktop Present every rendered frame, not every other
frame. Added process private/free/largest-free/reserved VA and video-memory budget
telemetry, including failed Present HRESULT. This does not identify the original
failed allocation or prove that wrapper churn caused it.

OFFLINE: all9 checks pass (re/evidence/A1k-ctest.txt), including actual production
ImGui gamepad navigation from first to last controls and back in Native/AFW,
real GPU transport/reset/depth/HUD/DLSS checks. 4096 transfers create zero new
wrappers after warmup. Private memory238.5 ->269.7 ->270.1MiB across two2048
batches; VA free1509.9 ->1479.3 ->1479.3MiB. Synthetic trend only, not retail
memory stability or headset performance acceptance. Build audit:
re/evidence/A1k-build-checks.json;17 owned and5 current protected hashes verified.

References reused: playbook b955f2244f7c5d5f3b3e5f403b1b6721a3362423,
FAIL-PACK-005/ch06 #laa-address-space, ch19 canonical identity/full resource
identity and budget ledgers; framework04 interop/reset, framework12 section6,
VR_HUD_GUIDE and PACKAGING_GUIDE. Microsoft wrapped-resource lifetime guidance:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-11-on-12
Beren5556 repo is BioShock1/2 Remastered downloads/docs, not Infinite; source
private. Public performance notes suggest overlapping DLSS, guarded depth reuse,
and headset submission before mirror. Asylum already overlaps native DLSS;
these are leads, not transferable offsets or evidence of a speed fix here.
https://github.com/Beren5556/BioShock-VR-DLSS-Downloads/releases/tag/v0.2.17-en
Saved public documents: re/evidence/beren5556-v0.2.17/.

Next SINGLE JJ launch: use the usual VR launcher, visit the same opening scene,
observe native FPS, enable AFW and navigate from first to last panel options and
back, then return to native. Memory/scene budgets log automatically. Save stereo
history if artifacts recur; exit normally. No autonomous game launch or kill.

# Asylum build and grouped tests

## A1k crash, scene cost and AFW navigation (installed02:19 SydneyOct12)

Looked up again: playbook failure-atlas FAIL-PACK-005 -> chapter06
#laa-address-space; target PE already0x0122/LAA, so the 2GB flag fix does not
apply. Chapter19 resource identity/full-description/lifetime and budget ledgers,
framework04 interop/reset, framework12 section6 input ownership, VR_HUD_GUIDE,
PACKAGING_GUIDE; City STICKNAV accepted but Asylum already has it. Checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Microsoft 11On12 docs require balanced
acquire/release and deferred-destruction flush; persistent wrapped resources
are the normal example. Beren5556 public v0.2.17 performance notes cover
overlap, guarded depth reuse and headset-before-mirror; source is private.
These are leads, not proof of this target's bottleneck.

HEADSET A1j: no FPS improvement; normal scene~50, blank wall90, desktop4K~230.
Screenshot: D3D9Viewport.cpp202 E_OUTOFMEMORY rendering-thread exception.
LIVE native sceneCPU~5ms/eye, GPU elapsed~4.5-5ms, helper~2.5-3ms/eye;
classifier only~40ms across5s. AFW logs~90 pairs/s, but repeated mode switches
when navigating panel. SOURCE duplicated Native RadioButton ID appears only
when AFW details are shown. Three bridge routes create wrapped resources every
capture; audit/reuse by actual identity, bounded lifetime, Reset cleanup and
real repeated GPU transport. Crash cause not yet established: add process VA/
commit/largest free block and video memory evidence, not a speculative LAA patch.

Missing owned d3d9.dll restored from matching manifest hash; no running game.
Crash logs/previous log/profile/INI preserved in A1j-crash-performance before
fixtures. Stale profile journal predates subsequent desktop settings: preserve
current user3840x2160/DoF-off changes rather than restore old user2560x1440 bytes.
Build changes, validate real GPU repetition/reset and actual ImGui navigation,
install while closed with owned/protected hashes. JJ drives one grouped launch;
no autonomous game launch/kill or City/Origins writes.

## A1j gameplay draw-hook cost (installed00:47 SydneyOct12)

Looked up: reuse A1i chapter19 budget/three-ledger and chapter06 measurement
lookup; new chapter19 cached-resource full-description/COM identity paragraphs,
render-pass-hazard-atlas classifier (must reflect LIVE bindings, not creation),
framework04 section7 resize/reset invalidation. Checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. A mutation-
invalidated live snapshot with StateBlock Apply invalidation is necessary;
shader-creation-only cache is unsafe. Target-owned D3D9 hooks/ABI only.

HEADSET A1i menu reaches90; beginning gameplay remains<60. LIVE menu90 then
82, gameplay53-60 with some66. Physical desktop Present~.26-.44ms, pacing
waitGate0-1ms/s, overlap sync3/no timeouts, helperGPU~2.7ms/eye. Colour and
depth transport ~.5-.8ms/eye; XR bridge including DLSS waits~2.7-3ms/eye.
These are CPU wall spans, not proof of GPU scene cost. Startup JSON proves
user BmEngine rebuild sets smoothTrue/max300; the raised base ceiling reaches
it. No62 cap in menu now. Evidence preserved in A1i-performance.

SOURCE: HUD/depth draw hook repeats RT/DS/viewport/descriptor and COM identity
queries for EVERY scene draw, despite unchanged bindings. Cache LIVE state
until observed setter/StateBlock Apply, invalidate at eye boundaries/reset,
keep identity fallback and full target metadata. Record classifier CPU time/
refresh counts. GPU regression must restore through stateblocks, reject small
private targets, preserve real geometry before clear and existing HUD/filters.
Measure warm-query reduction on real9On12 draw sequence, retain safe uncached
option for timed comparison. No quality/resolution/DLSS-mode change; actual
retail FPS benefit and GPU scene cost still require JJ's grouped run.

DONE: live cache defaultsON; successful RT/DS/viewport/depth-state setters,
owned StateBlock Apply, eye boundaries and Reset invalidate. Full descriptions
and COM identity fallback retained. A persistent diagnostic toggle permits
uncached queries. Owned Begin/Present scene CPU spans and an eight-slot GPU
timestamp ring samples at most two pairs/s and ends BEFORE final colour/XR/DLSS transfer; results use GetData0,
no forced flush/spin/wait. GPU elapsed can include queue starvation/interference;
it is not a pure GPU busy time or an independently additive frame budget.
Microsoft [stateblock contents](https://learn.microsoft.com/en-us/windows/win32/direct3d9/saving-all-device-states-with-a-stateblock)
and [query states](https://learn.microsoft.com/en-us/windows/win32/direct3d9/queries)
checked: D3D9 blocks restore viewport/render states, RT/DS restored explicitly.

Eight final Release checks PASS. Actual9On12 fixture:256 unchanged draws,
uncached512 binding refreshes vs cached1; classifier CPU~.093-.094ms vs
.008ms. Whole draw-submission timings are noisy/driver-buffer dependent
(cached2.446/.420ms vs uncached.113/.110ms); no total draw/FPS gain is established
by that fixture. Stateblock bypass restore, small private target exclusion,
geometry .25/.75 survivingclear1, HUD/private filters/RIGHT reuse all pass.
Nonblocking timestamp results from both eyes are valid, no rejected samples.
DLSS actual17 overlap pairs, fault paths/AFW/history regressions still pass.
17 owned/five CURRENT protected hashes independently verified. Runtime SHA
E28DE2877186FD4D5789222029E6C5DE2E674068E92B661549FAFCD62C4D2837.
See A1j-build-checks/A1j-ctest; A1i live logs preserved before fixture writes.
No launch/kill, graphics-quality lowering, or City/Origins changes.

Next SINGLE grouped launch (JJ drives):
1. Usual Asylum VR shortcut, saved native/Balanced/resolution. Let menu settle
   15-30s, then return to the same beginning gameplay location. Check HUD/detail.
2. Open F10/both sticks -> FRAME RATE -> Compare scene tracking speed. Stand
   still until panel returns (~27s). Automatically runs optimization OFF/ON/OFF
   for8s each, logs completed-pair FPS before captures, saves still/history per
   phase, restores all prior settings. No headset typing needed.
3. If artifacts appear, use Save last10 seconds/F2. AFW comparison can be run
   in this SAME launch if desired; cache FPS comparison uses native throughout.
4. Exit normally. Preserve logs/profile-last and compare phase rates, scene
   CPU/GPU elapsed, classifier refresh/time and helper/transfer budgets.
   Actual gameplay FPS gain is pending; if small, scene timing ranks next work.


## A1i frame-cap and presentation correction (installed00:22 SydneyOct12)

Looked up: City BATMAN_CITY_PLAN round2026-10-03 frame cap62 from
DefaultEngine.ini rewritten into BmEngine every launch; Origins RE_NOTES
stock limiter semantics differ (MaxSmoothedFrameRate=-1). Playbook chapter09
#runtime-owns-pacing/#mirror-is-a-scheduler, FAIL-PERF-022, chapter19 three
ledgers/matched A/B and fake frame counting; framework04/07/10. Checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Donor regeneration is a lead;
verify target configuration and use target-owned D3D9 presentation parameters.

HEADSET JJ: 50-60fps early gameplay, ~60 in simple menu; Origins90fps idle
on the same9950X3D/4070Ti. LIVE A1h: overlap1 sync3, no fence/completion
timeouts, menu61-62pairs/s consistently (~16.15ms); gameplay45-59 and
later42, waitGate0, headset90Hz. HelperGPU2.65-3.3ms/eye. No evidence the
menu must be GPU-bound just because DLSS completion wait is4-5ms/pair.
STATIC: Asylum DefaultEngine [Engine.GameEngine] smoothTRUE,max62;
launcher omits this base file. Own CreateDevice/Reset do not normalize desktop
PresentationInterval. After exit generated configs restored by launcher;
restored config read is not proof of runtime setting. Preserve A1h live logs.

Build reversible profile covering base AND generated config with explicit
unlimited smooth flag; log active launch profile before/after engine startup.
Force owned game CreateDevice/Reset presentation interval IMMEDIATE, log
requested/effective params and periodic physical desktop Present time.
Keep native DLSS/mode/input/output/graphics quality; meaningful profile
regeneration and GPU present-param tests, then build/install closed and
independent17 owned/five protected hashes. JJ launches; no launch/kill.
Actual cap removal/FPS gain and any remaining CPU/GPU budget need next run.


DONE: base and generated configs journaled, smoothing disabled/max300 fallback;
owned D3D9 desktop interval forcedIMMEDIATE on creation/reset. Startup profile
values/hashes captured before restoration; periodic CPU spans isolate bridge
transport/XR/desktop Present waits. No quality/resolution/mode reduction.
8 checks PASS; actual base config dry-run and rebuilt generated settings stay
uncapped; real GPU swapchain reportsIMMEDIATE.17 owned/five CURRENT protected
hashes verified (A1i-build-checks). No game launch/kill or other-mod changes.

Next grouped launch (JJ): usual Asylum VR shortcut, same native/Balanced and
resolution. Wait15-30s in same menu; check whether completed FPS exceeds62.
Continue to opening gameplay and observe FPS/HUD/detail. Profile-last.json
captures beforeLaunch/afterStartup/beforeRestore; named CPU transport/XR/
desktop-Present times distinguish the remaining budget if still slow.
Optional transfer-speed A/B after warm-up; capture last10s if artifacts recur.
Exit normally; preserve logs. Actual cap removal/90fps remains unverified.

## A1h performance work (installed00:07 Sydney, 2026-10-12)

Looked up: City DLSS_PLAN FINISHFIRST/run24/OVERLAP/run25 (safe73 vs
overlap88.4fps in its fake-headset benchmark; shared GPU fences, CPU-side completion, LEFT
deferred while RIGHT renders); playbook chapter19#blocking-wait-floor and
three ledgers (separate pacer floor from actual work), pattern STR-012/021;
framework04/10 ownership/submission. Checkout b955f224. Asylum already has
the matching IPC6 per-eye request slots/helper fence support; the A1g client
serialized both eyes with sync2. Donor timings are leads, not Asylum proof.

HEADSET A1g: image quality improved, poor FPS versus other Batman games.
LIVE last run: Balanced1322x1478 ->2284x2448, native33-37pairs/s (27-31ms),
waitGate0-1ms/s, NGX~2.35ms/eye, image-pose age63-78ms/exact tags matched.
Raw scene depth .993402779.. .998947680, no exact0/1: preservation fix reaches
retail geometry. Logs saved in re/evidence/A1g-performance before GPU checks.

Implemented native LEFT DLSS submission without a CPU completion wait, render
RIGHT while helper handles LEFT, then one bounded output-fence wait for both.
Acquired LEFT stays held until completion; HUD is added after final DLSS copy, preserve
atomic stock pair rollback and recorder ordering. AFW retains synchronous
path. Missing shared-fence support falls back to existing safe path. Expose
safe/overlap timed A/B plus named handoff timings for next grouped launch.
Actual GPU regressions cover both histories, missing RIGHT and early LEFT
completion. Build/install closed, no autonomous launch/kill.

Release builds and all8 checks PASS. Real GPU regressions verify17 distinct
red/blue overlap pairs, histories, missing RIGHT/early finish/reset/mode-off,
no held-image safe fallback, helper CPU timeout before NGX, final deferred
LEFT recorder snapshot. Evidence: A1h-ctest.txt and A1h-dlss-gpu.log;
17 owned/four CURRENT protected files independently verified (A1h-build-checks).
Saved Balanced/resolution unchanged. Actual FPS gain is not yet measured.

Next single grouped run (JJ launches): native/Balanced at current resolution,
wait30s for initialization/automatic captures, remain still and press DLSS ->
Compare DLSS transfer speed. It hides the panel, runs8s safe/overlap/safe,
logs completed stereo FPS and transfer waits, saves stills/history each phase,
then restores settings and reopens the panel. Check HUD/fusion/detail and
move head; F2/top button preserves preceding10s if ground/jitter recurs.
Brief AFW flicker check still pending. Exit normally to preserve logs.
Do not launch/kill game, reduce resolution to mask overhead or change other mods.

## A1g distance-detail correction (installed23:34 Sydney)

Looked up again: playbook failure-atlas FAIL-PERF-017 and chapter19
#dlss-in-vr (depth/jitter/motion registration before missing object vectors),
chapter06 "Measure, don't theorize", cross-project-index temporal histories;
framework04/06/10 and City DLSS D2 (filtering/FXAA methods only). Checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Asylum constructor evidence
ghidra-camera.txt lines790 onward converts shader depth-unpack parameters,
not the input projection matrix. A1d's blanket raw-DS inversion was unsupported.

HEADSET A1f: distant detail/shimmer highest priority; AFW still flickers,
black ground and slight camera jitter remain. LIVE user captures: five depth
previews contain ONLY red255 (8-bit evidence, not proof every raw value=1).
Actual Quality1530x1708 ->2284x2448; active NGX and ~2.75ms/eye. Source-depth
selection/copy waits until the final post/UI blit, allowing an intervening DS
clear; this and the shader-unpack/raw-DS convention confusion must be corrected.
Preserve live logs in re/evidence/A1f-user-detail before GPU checks.

Build real depth preservation before clear/rebind, raw depth+numeric capture,
raw hardware projection instead of shader-unpack inversion, relative-camera
DLSS motion precision, and meaningful GPU regressions. Audit pose attribution
without changing timing blindly. Retain stock/DLSS and native/AFW timed A/B.
Build/check/install while closed; JJ launches. Headset outcome remains pending.

DONE: physical depth frozen before clear/rebind; hardware projection retained;
relative-camera DLSS motion; raw depth/numeric captures; image-pose age logging;
chronological replay export. Eight checks pass;17 owned/four protected hashes
verified. See A1g-ctest.txt and A1g-build-checks.json. Backup install-20261011_233405_940.

One JJ-driven launch, highest priority first:
1. Usual VR shortcut, native stereo/DLSS Quality. Revisit the same hallway;
   look at distant railing/brickwork while holding still and moving the head.
2. Press "Compare stock and DLSS" in the DLSS drawer. It captures both phases
   automatically at the same game resolution; no headset typing needed.
3. Take one "Capture full-resolution still" at the problem ground so precise
   physical depth values and scene projection can be checked alongside colour.
4. Run "Compare native and AFW" afterward, then quit normally. New pose-age
   logs distinguish queued frames from mismatched pose attribution.

Do not claim the distance/AFW/ground/jitter issue fixed in the headset yet.

## A1f installed ? grouped next launch

Looked up: RE_NOTES A1f records City DLSS D1/D2, Origins O1C, playbook
PERF-006/007/chapter19 and eye-capture-point at b955f224, framework04/10,
VR_HUD_GUIDE and packaging. Reuse that lookup; do not rediscover these fixes.

A1f installed22:48 Sydney. Eight checks pass, including old-depth AFW control
180/180 missing sky vs corrected0/180, real NGX sky-motion and both histories,
actual NO-SRV final-eye replay, multi-frame JPEG decode/export, sampler bias
state integrity and window proc overriding physical size.17 owned hashes and
four current protected Asylum files verified. An external Origins DLL update
before our install is separately recorded. See A1f-build-checks.json.

One JJ-driven launch, no typing inside the headset:
1. Use usual VR shortcut. Keep Quality/1.00 output/native. Check medium-distance
   textures and motion; FRAME RATE now counts completed stereo frames. Note
   actual observed input/output sizes. Profile DoF/MotionBlur=False, MSAA1.
2. When the black ground artifact happens, F10/hold both sticks and press the
   TOP "Save last10 seconds (stereo)" button immediately. F2 also saves.
   Recorder already contains the preceding period; do not wait until later.
   Full-resolution still button is optional for detail. Replay files are in
   %LOCALAPPDATA%/Batman_Asylum/captures/A1f_history_*/review.html.
3. "Compare stock and DLSS" runs native stock ->saved DLSS ->stock; each step
   saves stills/replay, then restores saved settings. Stay at the same surface,
   sweep/tilt head slightly; observe whether the black artifact exists stock.
4. "Compare native and AFW" runs both paths and captures each step; check sky,
   ground, geometry/HUD and flicker. Native starts selected next launch. Quit
   normally; I read replay, camera/generation/NGX/depth/window/AFW logs afterward.

Do not claim a headset FPS gain or AFW/ground fix from GPU fixtures alone.
Capture is sampled at up to10pairs/s,960px/eye,JPEG94; full-frame-rate/lossless
video is not implemented. Duration can shorten under64MiB cap/backpressure;
actual span/drop count and timestamps are recorded. NGX input changes apply
next launch; UI must not imply immediate engine resolution change.

## A1e work - resolution, City/Origins panel and AFW

Looked up: playbook failure-atlas FAIL-PERF-020 (window-sized scene upscaled
to the eye), docs/19-d3d12-and-performance.md#composed-reprojection and matched
A/B / window-occlusion control; STR-012/013/014/015 (eye tickets, disocclusions,
per-eye histories), checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Framework06 render-target recommendations, guide10 history ownership;
VR_ESSENTIALS sections3/4/6/9; VR_HUD_GUIDE; City window.cpp (game-only imports,
viewport clamp) and AFW section11/source, Origins scene.cpp panel sections.
Use methods only: Asylum's imports are independently enumerated and no City
viewport offsets/clamp instruction patches are copied.

HEADSET A1d: JJ reports low resolution/blur; requests both AFW and asymmetric
FOV. LIVE now proves actual retail DLSS: both-eye Quality,7034 evaluations,
zero noDepth/fallback/timeouts;996x1112 input ->1494x1668, source1238x1328,
XR2284x2448. Direct-asymmetric comparison uses1238x1328 ->1856x1992.
Preserved A1d-headset-run.log/A1d-retail-dlss.log. Tower acceptance remains
unreported. Resolution must be increased at the engine's scene size, retaining
a visible desktop-fit mirror; enlarging only XR output does not restore detail.

Implement a game-only logical client size and monitor query override while
keeping the actual desktop window fitted; validate real D3D9 source dimensions
independently. Add persistent0.6x-1.8x headset-based resolution controls for next
launch, City/Origins drawers, native default / AFW optional, real per-eye FOV,
and depth-based current/previous/combined AFW without warping the HUD. Group
headset resolution/AFW/FOV/DLSS checks into one launch after offline verification.

A1e DEPLOYED21:56 Sydney: all eight checks pass, including actual2284x2448
GPU source with a smaller mirror/logical WM_SIZE, AFW pixel checks BOTH
directions with asymmetric lenses and forward/inverted-Z, and real NGX
alternating-eye generations.17 owned hashes and six CURRENT protected files
verified; backup re/backups/install-20261011_215616_960. Installed-build.json
and A1e-build-checks.json are authoritative. No agent game launch/kill.

One JJ launch, no live typing:
1. Launch usual VR shortcut. Default1.00x requests2284x2448 here; F10/both
   sticks ->VR RESOLUTION verifies the actual observed game size. Judge menu
   and campaign sharpness/head movement. Changes to scale apply NEXT launch.
2. STEREO RENDERING ->Compare native and AFW; look steadily for20s. Three
   captures are automatic and native is restored. Judge Batman edges, glass,
   shadows/reflections and HUD fusion. AFW remains selectable afterward.
3. VIEW ->compare direct/lighting-compatible asymmetric FOV if needed;
   HUD ->adjust distance/size; DLSS ->compare Quality/DLAA/off. Save gameplay
   capture, check pause/detective/reflections, quit normally and report.

Read raw native scene rect, [aa window] requested/observed/physical client,
[aa depth] target sizes, both-eye DLSS/helper success, [afw] warped/flat/holes,
paired final eye pictures and source/depth sheets. An observed source-size
counter alone cannot establish high-resolution engine scene/depth coverage.
Full retail resolution/AFW/headset acceptance is pending; tower acceptance
remains unreported. Higher resolutions cost GPU/memory; AFW transparency and
camera-only DLSS motion have known limits. Full donor feature parity is ongoing.

## TEST PLAN A1d - tower comparison and actual DLSS

Looked up: RE_NOTES A1d records playbook14 shadow ownership, playbook10
projection companions / FAIL-STR-030 and playbook19 per-eye DLSS at b955f224;
framework04/10/14, City/AKN source and this target's read-only matrix decompile.
A1c LIVE confirms startup cancellation and actual UI extraction; JJ's new tower
shadow report needs a scene-lighting correction, not another HUD transform.

A1d SOURCE/BUILD/OFFLINE: centred engine-lighting frustum with exact asymmetric
XR remap, actual scene depth transfer, per-eye x64 NGX evaluations with shared
jitter, camera motion/history resets and atomic stock-pair fallback. Persistent
Quality request defaults ON. All eight Release checks pass, including genuine
DLAA and Quality evaluations for both eyes, measured depth/motion and failure
rollback. HEADSET shadow correction and RETAIL DLSS remain unverified.

One grouped JJ launch; captures/switches run in the mod, no live typing:
1. Launch the usual Batman Asylum VR.bat; skip opening with a face button.
   At the main menu inspect the right-eye tower and text fusion/head movement.
2. Open F10 or hold both sticks0.6s. Press "Compare tower shading" and keep
   looking at the tower for about20s. The panel hides; three timed captures
   compare corrected stock, original stock and corrected saved DLSS. Original
   settings are restored and the panel returns. No restart for comparisons.
3. Check DLSS status and actual input/output sizes. Continue gameplay and judge
   edges, movement, animated characters, pause/menu/detective/reflections.
   Save a gameplay capture; quit normally and report what you saw.

Inspect paired eyes, HUD-free game_frame, isolated hud_sheet, depth_scene,
status.txt, [aa shadow compare], [aa camera], [aa depth], [aa DLSS] and helper
NGX logs. Both eyes must evaluate successfully without depth/motion/fence
failures, and A/B must establish whether centred reconstruction fixes the tower.
If it persists, enumerate shadow-mask/depth consumers before shader edits.
Retail depth producer validity and real headset quality/performance cannot be
inferred from the GPU fixture. Output is currently capped below full XR Quality
because the desktop-fit source/crop is smaller; animated motion is not supplied.

A1d DEPLOYED 2026-10-11 20:53 Sydney: all17 owned hashes independently
verified; six CURRENT protected files unchanged (Asylum EXE/configs and current
Origins DLL/settings). DLL SHA256
B52CC19283A87754097ABBCFB17AF23EF79DBCCF169795C33F942A1F012B7D4D.
Receipts: A1d-before-install.json, A1d-ctest.txt, A1d-dlss-gpu.log,
A1d-build-checks.json and installed-build.json. Previous owned build:
re/backups/install-20261011_205312_602. No agent game launch/kill.

## TEST PLAN A1c - actual HUD sheet and upright/skippable opening

Looked up: RE_NOTES A1c lists playbook04 HUD separation, FAIL-HUD-014/007
at b955f224, VR_HUD_GUIDE and framework input/temporal sections. A1b HEADSET
confirms scene stereo, unfusable HUD, low video and failed controller skip.
LIVE identical text pixels in both eye captures disprove the earlier canvas
hook's visible coverage despite nonzero canvasEdits. Current source/client
extents match; retain the source-size fix. See DLSS_PLAN.md for the next route.

A1c SOURCE/BUILD/OFFLINE: actual D3D9 LEFT UI extraction with independent
stencil/private-target preservation; matching-generation RIGHT reuse;
transparent GPU transfer and common4m world plane through actual eye lenses;
desktop HUD restored after capture. Opening room screen centred at eye level,
raw pad polling and owning-thread native force-stop. Eight Release checks pass,
including actual production HUD capture/blend/state restoration and target
render-stop ABI. These checks do not prove the retail classifier/headset result.

One grouped JJ launch; no mid-headset typing:
1. Launch the usual Batman Asylum VR.bat. Look at opening video: upright,
   centred at eye height and fixed in space. Press A/B/X/Y, Start or Back once;
   check that the sequence goes straight to the title/menu.
2. Title/main menu: scenery still3D, text readable/fusable with head motion.
   F10/both sticks opens settings; HUD counters should advance. Save capture
   records game_frame, hud_sheet and both composed eyes without extra launch.
3. Continue campaign; inspect HUD, pause/menu and detective/map/private effects.
   Save a capture if anything is wrong; quit normally, then report observations.

Read actual [aa HUD] boundary/captured/rightSkipped/unsupported and [aa startup]
request/thread/stop lines beside the pictures and paired generation counters.
Keep once-only UI CPU work, markers/fades, reflections and endurance open.
DLSS proceeds after this check with real depth/motion/jitter, per-eye history,
then HUD after upscaling. It is not active in A1c. Installed receipt authoritative.

A1c DEPLOYED 2026-10-11 18:44 Sydney: all15 owned hashes independently
verified; six CURRENT protected files unchanged (Asylum EXE/configs and current
Origins DLL/settings). DLL SHA256
13FC69525C79627DA42C5F1CB998FA3DB5BAF17512B5EEB881BC3A9F7E6EC8B6.
Receipts: A1c-before-install.json, A1c-ctest.txt, A1c-build-checks.json,
installed-build.json. Previous owned build: re/backups/install-20261011_184407_539.
No agent game launch/kill. HEADSET and retail extraction/cancellation pending.

## TEST PLAN A0 — isolated clone and real graphics bridge

Looked up: ASYLUM_PLAN.md reference sections, playbook b955f224.

| # | switch / action | proof | judge |
|---|---|---|---|
| 1 | source clone + fingerprint tool | isolated donor/target identities | source digest receipt, Asylum hash, no donor offsets linked |
| 2 | GPU bridge fixture (no game) | 9On12-to-11On12 transfer | exact color readback, repeated return, resize, managed texture compatibility |
| 3 | ordinary Steam launch | vanilla behavior | proxy forwards only; VR factory replacement requires -bavr |
| 4 | dedicated VR launcher | real game bridge and Origins-style panel | game-device ownership, actual source dimensions, XR/session log, capture |
| 5 | menu -> campaign -> pause/map/detective -> menu -> quit | first live baseline plus transition/reset evidence | one run, all captures/status together; camera hooks require Asylum guards |

Native camera, AFW and DLSS become testable only after their Asylum render seams
and depth are established. Do not present a cinema bridge as native stereo.
No game launch by the agent; JJ drives the installed build's first run.

### A0 RESULTS

2026-10-11: Release Win32 build installed on E:, 14 manifest-owned files
independently hash-verified. Runtime DLL SHA256
`D3F817FAF3AB01D035A3FD368382CA4EC36AC7FD7AA20694802B30C497A8520E`.
All three CTests passed on the final source: foreign-host/proxy guard, actual
D3D9 GPU transport, and ANSI/UTF-16 graphics-profile scope/byte-exact rollback.
The GPU fixture exercises eight BGRA-to-RGBA color transfers, managed textures,
fenced ownership return and device reset/resize. The frozen x64 helper also
builds successfully, but is not installed or active: Asylum depth, motion vectors
and per-eye input remain unimplemented.

Receipts: `re/evidence/A0-ctest.txt`, `A0-build-checks.json`,
`installed-build.json`, `A0-before-install.json`, `donor-source.json`.
Protected Asylum executable/configuration and Origins runtime/settings bytes
are unchanged after installation. Installer backups are under `re/backups/`.
Grade at initial deployment: BUILD/OFFLINE + DEPLOYED. First live outcomes and
the superseding A0a build are recorded below.
Native stereo, AFW, DLSS, game HUD/shaders and transitions remain pending.

### A0 source progress before first live run

Frozen Origins O1H core, dependencies, input, licenses and x64 helper source;
only Asylum's adapter/graphics bridge and guarded Asylum command observers link.
GPU bridge uses plain 9On12, adopts the game GPU, unwraps its completed image,
copies on an ordered 11On12 queue, flushes, and returns ownership with a checked
fence. XR image release explicitly flushes 11On12 commands. FPU behavior flags
are passed through unchanged. Actual GPU checks already passed exact color
transfers, managed textures and resize; foreign-host/normal-launch guards and
ANSI/UTF-16 graphics-profile scope/rollback checks also passed. The installed
build converts the game surface into RGBA8 before feeding the core, and passes
runtime-recommended output dimensions independently of the desktop window size.

STATIC/Ghidra: completed focused decompilation of Asylum command executors and
three candidate roots (`ghidra-render-focused.txt`). Root 0x00cc2310 is tiled
capture, and 0x00cc2ab0 is another capture/readback route; neither establishes
the gameplay draw seam. 0x00ccae50 is the ordinary draw candidate: it uses the
second Begin/End command family, viewport+0x20 client and its draw slot +8.
No camera or stereo mutation is enabled from this inference. The two tiny
Asylum command executors have full 16-byte loaded guards and preserve their
original return/one-call behavior while recording counts/stacks/viewport data.
The first live run must distinguish the ordinary caller from capture paths.
Broad Ghidra analysis timed out at 120 s (149 s reported total) but saved its
partial database; focused decompilation completed independently. This was a
separate workspace database, not the user's open Ghidra session.

References remain ASYLUM_PLAN.md's cited routes/checkout. Before first launch,
capture transport/session status, command A/B counts, and menu/campaign/pause/
map/detective/menu/quit in one run. No per-eye camera, AFW, DLSS or optical shader
claim follows from a successful flat game-frame bridge.

## TEST PLAN A0a — first-launch graphics-factory recursion

Looked up: playbook `06-debugging-methodology.md` "Crash-dump workflow"
(instruction/register/stack evidence), existing chapter 10 9On12 ownership
route, checkout b955f224. No Asylum-specific dual-factory precedent found.

LIVE 2026-10-11 14:31: user launched A0. Target guard and VirtualDesktopXR
instance initialization succeeded. Two plain9On12 factories were created;
no game-device/Present log followed. WER reports Asylum DLL+0xb67d7 access
violation. Local crash dump resolves this to CRT stack probing, with 8,699
instances of DLL+0x2f933 on the captured stack. That instruction immediately
follows the `originalCreate` call in our CreateDevice detour. This establishes
recursive factory-hook dispatch, before any image transport or native camera.

| # | action | proof | judge |
|---|---|---|---|
| 1 | replace global factory CreateDevice detour with per-factory COM forwarding wrapper | each factory owns its original dispatch, independent of nested/shared system implementations | no global factory trampoline; unchanged arguments/HRESULT/object lifetime |
| 2 | actual GPU fixture creates two wrapped plain9On12 factories/devices | covers the missed startup shape | both devices succeed, QI identity/refcounts correct, existing exact transfer/managed/reset checks pass |
| 3 | final build/deploy and verify rollback receipt | concrete corrected build | three CTests plus installed hashes, game config restored after failed launch |
| 4 | JJ repeats grouped route on A0a | real game reaches owned device and XR frame, command callers gathered | LIVE logs and headset image; if no device, inspect returned HRESULT/caps before further stereo changes |

A0a RESULTS: implemented per-factory COM forwarding; no global CreateDevice
hook/trampoline remains. The GPU fixture creates devices through two wrapped
9On12 factories, checks COM identity, then runs all previous transport checks.
Final Release build passes all three CTests. A0a installed, 14 hashes verified,
protected bytes unchanged during this deployment. DLL SHA256
`0A5506CE8C17BFC358A993B8C0E1ECF84FE051405694A6E3440EB124CECCF261`.
Receipts: `A0a-ctest.txt`, `A0a-build-checks.json`, `A0a-before-install.json`,
current `installed-build.json`; failure proof `A0-factory-crash.json` and both
`A0-*-launch.log` files. JJ confirmed A0's second run displayed desktop only,
no headset or panel, and closed it normally before A0a installation.
A0a HEADSET follow-up: JJ confirms the game now appears in the headset, but
as a small window in the top-right. LIVE receipt `A0a-headset-run.log` proves
game-owned 9On12 device, GPU adoption, actual 2284x2448 source, two 2284x2448
XR images, running/focused VirtualDesktopXR session and panel input events.
Native stereo/AFW/DLSS remain pending. Framing correction is A0b below.

The initial A0 protected receipt is a historical snapshot: subsequent user
game runs and an externally updated Origins DLL mean it no longer describes
all current bytes. A0a's protected receipt captures the current pre-deployment
baseline and was verified entry-by-entry after installation. The Asylum
installer does not target Origins files. Active graphics journals are restored
before replacement, and game-modified baseline files are not overwritten from
an older unrelated installation snapshot.

Live command B runs on the render thread; its stack contains no enqueue-site
caller. Do not treat an empty exe stack as proof against 0x00ccae50. Its viewport
vtable RVA is 0x01d2c380, whose static slot +0x54 is 0x00cc29c0. This establishes
the command's viewport type/begin method, not the game-thread draw/camera ABI.

## TEST PLAN A0b — small screen in the top-right of the headset

Looked up: VR_HUD_GUIDE sections 6/8, City round 6 SCREENROOM/SKVR SCREENPAD,
framework guide 11 section 1 (viewport/scissor versus UI placement), playbook
chapter 04 "Get UI off the backbuffer and onto a layer" (full-view projection
carrier, preserve source aspect; crop-to-fill is a deliberate presentation
choice). Checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423. No matching
Asylum-specific fix found. Distinguish presentation from native rendering.

HEADSET: JJ confirms A0a now launches, but the whole image appears in a window
in the top-right, about a quarter of the view. LIVE: game and both XR images
are 2284x2448, GPU transport/session are active. The panel input opened/closed
in the log. No source/eye pictures were saved in that run.

Ranked causes: (1) copied cinema screen is only 2.4 m wide at 2.5 m, roughly
51 degrees wide, with a room anchor established during startup; this explains
small coverage and can leave it away from the current gaze. (2) image transfer
or engine viewport could itself contain an inset; constant-colour centre-only
checks cannot exclude this. No claim yet that larger source resolution fixes it.

| # | action / live switch | proof | judge |
|---|---|---|---|
| 1 | Asylum defaults to full-view flat preview; `aa preview full` / `fit` / `window` | presentation follows the current located eyes, not an old startup anchor | full runtime FOV carrier; common distant plane, correct per-eye projection, aspect preserved; full intentionally crops excess edges, fit preserves all content |
| 2 | patterned actual GPU transfer and projected screen fixture | excludes channel flip, inset and wrong corner placement | different source quadrants + nine destination samples; both eye corners filled at translated/rotated poses |
| 3 | capture control and bounded automatic source + both-eye pictures | real source versus presentation evidence in one run | captures include actual composed eye images, metadata and source; no inside-headset typing required |
| 4 | JJ menu -> gameplay -> F10 -> pause/map/detective -> quit | framing, controls, later native-render leads | centred larger preview, controls still work, inspect paired captures and logs together |

Use a distant flat preview only while native camera integration is unfinished;
do not advertise it as stereoscopic gameplay. Window mode remains a live control.
A0b SOURCE/BUILD/OFFLINE/DEPLOYED: full preview defaults on in Asylum. Uses a
common distant plane aimed from the current located eye pair, fits the union
of their actual FOV rays, and projects separately through each eye. Fill
uniformly scales/crops; Fit uniformly shrinks until every source corner is
inside both eye frusta. Room screen remains a live control. Full projection
carrier remains in LOCAL/world space, never a narrow or VIEW-space layer.

All three CTests pass on the final Release build. Transport fixture now checks
nine pixels across four different source quadrants, eight transfers, then uses
the actual 11On12 preview shader to prove both asymmetric eye images are filled
at translated/pitched/yawed/rolled poses and equivalent quaternion signs.
Fit checks all four corners against both actual lenses. The previous solid
centre-only check would not exclude an inset/flip. Existing two-factory,
managed-pool, fenced-return, reset/resize and profile rollback checks still pass.

Installed A0b: 14 owned file hashes and protected bytes verified; DLL SHA256
`1044A5D7BD87ADEC5971949283C10F227250C69BC99B86BD80919D6F19C12E36`.
Receipts: `A0b-ctest.txt`, `A0b-build-checks.json`, `A0b-before-install.json`,
current `installed-build.json`. No agent game launch/kill.

Two bounded automatic captures occur 2 s / 30 s after ordinary rendering starts
with an already-focused XR session; panel Save capture / `aa capture` adds a
manual capture. Each folder contains actual `game_frame.bmp`, both composed eye
images and status, under local Batman_Asylum/captures. Room-screen eye saves
were missing from the copied core; they now save before releasing each image.
Counters only trigger diagnostic captures, never decide VR/gameplay mode.
First three render-command observations also record the raw viewport+0x20
client/vtable/Draw-slot lead for native discovery, without new engine mutation.

A0b HEADSET framing result pending. Correct source dimensions and GPU fixtures
do not exclude an inset produced inside the actual game; inspect source versus
both composed eyes from the next run before claiming that possibility resolved.
Full preview is still mono imagery; native stereo/AFW/DLSS remain unimplemented.

## TEST PLAN A1 - anchored screens, logo cancel, first native stereo

HEADSET A0b: JJ confirms settings panel is correct; titles/menus appear too
close and follow his head. Source: preview plane was rebuilt from current
eye poses every frame. LIVE: command B client vtable 0x023a5f88, Draw
0x008a0230. This is now the target-specific camera discovery entry.

References: VR_HUD_GUIDE sections 6/8, framework guides 08 section 3/4 and
09 basis composition; playbook chapter 17 #side-effect-gate and #reentry-endurance,
RE-01, checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Origins/City movie EOF skip failed (decoder waits forever): use Asylum's
own non-waiting movie-stop method from its game-thread tick.

One JJ launch: face/Start/Back button during opening logos -> main menu
(look around: screen stays in room; whole image visible) -> Continue game
(native selected automatically when a validated gameplay view is available)
-> check depth/head motion -> F10 native/screen comparison -> pause, map,
detective and a reflective surface -> Save capture -> normal quit.

Offline: instruction guards/ABI exits, independent eye state allocator,
actual bridge/shader transport, stationary screen corners under changing head
poses, rollback and foreign-host checks. Live acceptance: both eye tags and
pose generations, no missed tags, stable base camera between passes.
Native remains an initial integration until JJ's gameplay run verifies
render re-entry, side effects, HUD, shader reconstruction and endurance.

A1 SOURCE/BUILD/OFFLINE/DEPLOYED: final Release installed, 14 owned hashes
verified. Four checks PASS (A1-ctest.txt), including real Draw/Calc/ctor/movie
ABIs. GPU fixture tests the SAME RoomPlane policy used by runtime: world
corners stay identical after head motion, lens projection changes, recenter
reanchors. Raw screen copy no longer consumes composed-eye capture bits.

Native defaults ON, waiting for a validated ordinary main-camera owner.
Independent right history, one generation for both eyes, base-camera equality
and matching paired Presents are guarded. Missing/repeated eye or generation
mismatch stops native instead of mixing unrelated images.

Installed DLL SHA256 `3C703393EE973E171821568ED959BB226F1E819FA98510B6A3E5B5D211991112`. Receipts A1-before-install.json,
A1-build-checks.json, installed-build.json. Six CURRENT protected files
(Asylum EXE/configs, Origins DLL/settings) stayed byte-identical during
deployment. No executable/package/movie/save edited, no agent launch/kill.

A1 HEADSET/LIVE pending: logo cancel, room anchoring, stereo depth/head tracking,
side effects, HUD, shaders/reflections and endurance. Stable base camera alone
is not proof of once-only simulation/audio/UI. AFW/DLSS still pending.
# TEST PLAN A1b - canvas fusion, matching extents, upright/skippable movies

Looked up: RE_NOTES A1b records VR_HUD_GUIDE3/4/6/8, playbook09 alignment
differential, a3 projection1/2, STR-002/004 at b955f224, and BioShock Infinite's
render-root chain at41f004bc. A1a HEADSET: split/sheared image. LIVE: stable
pairs but2284x2448 backbuffer versus2284x1440 Windows client. Static startup
memory-Bink and canvas findings/evidence boundaries are in RE_NOTES.

A1b SOURCE/BUILD/OFFLINE/DEPLOYED: eight checks PASS: Present attribution, opening movie
identity, scene/canvas math versus independent XR rays, desktop-fit dimensions
and aspect, foreign-host guard, real GPU transport/upright anchored preview,
config byte rollback and real target ABI. Here source1238x1328 versus XR2284x2448;
uniform upscaling lowers source detail while avoiding clipping. Deployed version
and hashes are authoritative in re/evidence/installed-build.json.

One grouped JJ launch (no mid-headset typing required):
1. Launch Binaries/Batman Asylum VR.bat. Tilt head during opening video to
   judge upright world placement, then A/B/X/Y or Start/Back to skip to menu.
2. At title/main menu judge scenery depth AND text fusion under head turn/tilt.
   F10/both sticks: check actual native paired status and nonzero Menu/HUD
   projections; Save capture. Automatic paired captures and bounded extent/FOV
   logs run without requesting a report while in the headset.
3. Continue campaign: depth, head-motion stability and HUD; pause/return, map,
   detective and reflective surface. Save a capture for visible issues, quit
   normally to restore graphics configuration, then report the observations.

Read client/backbuffer/raw rect, actual submission FOV, canvasEdits,
completedPairs/tagMiss/baseCameraChanged and source/both-eye captures together.
Offline math/GPU/ABI checks do not prove engine canvas coverage, headset fusion
or once-only simulation/audio/UI. Those gates and AFW/DLSS remain open.

A1b deployed 2026-10-11 18:03 Sydney: all 15 owned installed hashes verified;
six CURRENT protected files stayed byte-identical (Asylum EXE/configs and
Origins DLL/settings). DLL SHA256
BE6457EF865B2299C927ECDF82368894805C7120ECD97E6FD599FDDE63CB6F56.
Receipts: A1b-before-install.json, A1b-ctest.txt, A1b-build-checks.json,
installed-build.json; measured extent receipt A1a-window-extents.json.
Previous owned build backed up at re/backups/install-20261011_180334_671.
The initial receipt verification incorrectly compared array-valued fields;
the saved BEFORE hashes were normalised into six records and each compared
successfully. No protected bytes changed; no game launch or kill by agent.

# TEST PLAN A1a - real menu stereo, one-use Present attribution

A1 HEADSET: menu flat despite native selected. LIVE: prepass caller learned;
ordinary menu draws stayed mono, then one pair followed by permanent validation
failure. SOURCE cause/fix and references are recorded in RE_NOTES A1a.

Offline: native_present_sequence reproduces L/R then extra untagged movie
Presents, rejects true missing/repeated/cross-generation eyes, covers rejected
ctor verdict and failed-transfer/reset interruption. Existing four checks
retain actual GPU transport, projection/anchoring, rollback and target ABI.

One JJ launch after A1a deployment: at PRESS START inspect Batman/window depth
and head movement -> enter main menu and inspect its 3D background -> F10
check `Rendering: Native stereo - validated paired scene` (not just checkbox)
-> Continue campaign -> pause/return and inspect transitions -> normal quit.
Source/eyes automatically save on first completed native route's next LEFT;
Save capture stays available. Caller census, completedPairs, tagMiss and
baseCameraChanged decide follow-up; no autonomous game launch/kill.

A1a SOURCE/BUILD/OFFLINE/DEPLOYED: Release installed; five checks PASS.
DLL SHA256 `14B2CC923B7C24E0BFFBCCEB22D713A6FD213B55E91836877E7DE5A6EA989C0A`.
All 14 owned installed hashes verified; six CURRENT protected files remained
byte-identical. Receipts: A1a-before-install.json, A1a-ctest.txt,
A1a-build-checks.json, installed-build.json. No agent game launch/kill.
A1a HEADSET pending: sustained title/menu/gameplay stereo and head tracking,
menu/HUD fusion, transition recovery, side effects, shaders and endurance.

