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

## A2g RESULTS: hollow grapple/RB indicator and second Canvas projection

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

Looked up: WORLD_MARKERS_GUIDE6/7 (whole-widget ownership, offscreen arrows
stay on HUD), HUD_GUIDE1/10, framework11. Playbook ch04 source freshness and
engine UI ownership, FAIL-HUD-006/007, checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. City BATMAN_CITY_PLAN502 ruled
WorldToPixel out with LIVE zero calls; that result does not transfer to Asylum.
Asylum native target selection Boolean is authoritative. No guessed hiding or
reusing an old world point to manufacture an attachment when no target exists.

HEADSET/LIVE A2f: JJ confirms vital-statistics line/circle look great; native
owner rewrites10011/fallback0. Main marker projections4056/placements22008,
failures0,3variants. Hollow circle/RB capture020100 frames0/70/80 shows wrong
placement while projected3405/placed18524 counters stay flat around02:00:20-
02:01:00. Replay demonstrates the visible residual but no actual empty-target
state has been measured. A2f-grapple-hollow-run archives log/settings/evidence.
Do not tell JJ this proves a stale/invalid grapple target.

STATIC: native table25e0a38 -> UCanvasexecWorldToPixelA58AC0. It calls helper
AB36C0 atA58B6E/returnA58B73 with ECX view, world3/out2, ret8. Helper directly
reads VP+210 and float rectangle+28/+2C/+30/+34; computes pixelX=originX+
(ndcX+1)*.5*width,pixelY=originY+(1-ndcY)*.5*height. Native failure returns0
and leaves output untouched; success1. It does not call ABC2A0. Ghidra hash-
guarded read-only decompile and instructions in A2g-world-to-pixel*.txt and
A2g-world-pixel-helper*.txt; native registry in A2g-projection-registry.json.
Grapple native getter registration leads also recorded, no hooks/changes to
those routines (GetNoAim etc; no live state semantics claimed).

SOURCE: exact optional helper-entry/caller guards; original executes FIRST.
Only original-success exact owned main-pass caller is converted to head-free
pixel coordinates and same current-frame world anchor; original return Boolean
never changed. Existing grapple cards handle original textures/prompts at
actual world point. Other view/callers, behind/outside/false remain original;
vitals owner correction unchanged. No target selection/movement/visibility edits,
no GPU reads or held anchors. Guard absence disables only this extra coverage.
Passive separate pixelCalls/pixelPlaced/pixelRejected reports even false/failed
matches. Actual API ownership of hollow ring remains a runtime hypothesis.

OFFLINE/GPU: all11checks23.58s. EACH route32 head-free pixel/world-card cases
with independent normalized/pixel reference, native rect offsets, near/far,
game camera turn, head yaw/roll/lean; both-eye target projection placement.
Original-false output, foreign view/caller, behind/offscreen, zero rect, disabled/
stale preserved. Existing64vitals owner positions,32world grapple pixel checks,
4097DLSS pairs each,51native50legacy transitions retained. A2g-full-test.log
and A2g-ctest-soak.txt preserve full results. Not a retail/headset test.

INSTALL:19 owned files replaced; runtime SHA256 D66FA012701B78518F7DA70EC20BD74D78454FBE3C91A7C6346CE8AACF840BE3
backup D:\Documents\Antigravity\VR_Modding\batman_asylum\re\backups\install-20261014_021348_810. Closed stale graphics-profile journal+4 current configs
archived A2g-profile-recovery; original backups hash-verified then existing
Restore-AsylumProfile applied, post-restore verified, literal journal removed.
Refreshed baseline5 protected current config/EXE hashes, shader cache and saved
renderingINI unchanged by installation. A2g-before-install/A2g-build-checks exact
records; no assets/City/Origins/runtime/driver writes. No FPS claim.

## A2f RESULTS: native actor-callout projection before original layout

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

Looked up AGAIN after A2d/A2e incomplete fixes: playbook06 readback and
multi-candidate residuals, ch04 engine UI ownership, FAIL-HUD-007; Knight NOTES
REC1/RB1/RB2 around2274-2382: colour/proximity split real widgets, residual world
constants were not proof of ownership. VR_HUD_GUIDE10, WORLD_MARKERS_GUIDE6/7,
framework11. Checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Target-specific
source/log evidence replaces the small-orange-circle assumption. No upstream edit.

SOURCE ROOT CAUSES: A2e applies an along-line projected displacement to every
vertex, so original right-angle bends become diagonals. A2e first tip logs are
9-vertex pieces near world12320/11491/559 while actual person connector world
11702/11085/117; only39 tip matches vs2920 line matches. It is incorrect to call
those pieces the original circle without proving ownership. This matcher is
removed, including its draw interception and final-eye triangle compositing.

STATIC/LIVE OWNER: A2e native Project caller118a36b observed all three actor
positions. At VA158a366 callABC2A0 returns XY/Z/W;158a36b consumes XYZ, checks
Z<=1, converts XY to native centred normalized coordinates (x*.5,-y*.5), then
appends a32-byte actor record via1562a00 at158a42e. Actor pointer/XYZ/flags kept.
Asylum-specific E8 and consumer bytes guarded at runtime and tests; Project
ret8 ABI checked. Evidence A2f-callout-instructions.txt/A2f-project-convention.txt/
A2f-project-tail.txt plus A2d-callout-project decompile1589c20. These establish
native actor-callout input ownership; exact visible ring response still pending.

SOURCE IMPLEMENTATION: only this caller in exact main-view/pass changes XY,
using world target transformed via baseView90units/m, inverse recenter yaw/
position into XR, shared head centre at exact gamePass.gen, and actual bridge
HUD plane snapshot under lock. Original Z/W, return pointer, native visibility,
actor identity, original widget layout/geometry/textures/shaders retained. LEFT
and RIGHT produce identical layout XY; head orientation affects the shared HUD
plane's normal eye projection, not independent callout geometry. Plane snapshot
published from actual compose_original_hud; invalidate on HUD settings/recenter,
bridge reset/release and markers reset. Missing plane/pose, other callers/views,
behind/edge retain original. Original grapple Canvas caller/card unaffected.
No GPU readback, replacement ring/masking, or donor addresses.

OFFLINE/GPU: all11checks22.97s. EACH route64 owner-position cases vs independent
analytic z=-4 ray/plane hit, distances450/1800, game camera turn, head yaw/roll/
lean, recenter offset/yaw and asymmetric lenses. Both corrected XY identical;
original Z/W bit-exact. Wrong caller/view/pass/plane/behind/edge/toggle retained;
plane invalidation/reset verified.32 existing grapple shader pixel references
and4097DLSS completions each;51native50legacy transitions. Full raw transcript
A2f-full-test.log. Old triangle GPU fixture remains an isolated transport test,
not proof of the retired runtime matcher or the actual native callout artwork.
No retail launch or new headset validation performed by agent.

INSTALL: runtime SHA256 5E0D825CE0D7BC04F384523C4E5D233A6CE13EEBF0BCC6853A27C7E08B635C86
backup D:\Documents\Antigravity\VR_Modding\batman_asylum\re\backups\install-20261014_014645_290.19owned files/five CURRENT protected config+EXE hashes plus
shader cache/renderingINI independently verified; profile inactive, no recovery.
Saved settings unchanged. A2f-before-install/A2f-build-checks have exact records.

## A2e RESULTS: shared HUD depth for line and circle (2026-10-14)

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

Looked up: Knight akvr_native/NOTES.md SCANHUD2144-2153: shared head-centre
ray meets HUD plane; line, tip and label all on that plane, no rotation-dependent
layout. VR_WORLD_MARKERS_GUIDE6/7: route all pieces to one point, retain original
art and distinguish HUD/world markers. VR_HUD_GUIDE8/10; framework guide11;
playbook ch04 FAIL-HUD-006/007 at b955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Only the method transfers; Asylum's own already matched native points retained.

SOURCE: markers9::Pass snapshots native recenter yaw/position with baseView and
exact eye generation. Convert native target to base view (90 units/m, forward-Z
flip), inverse recenter into absolute XR; intersect ray from exact pair's shared
eye centre with actual bridge hudWorld corners. Project this common hit through
each eye's HUD quad. Shift all source geometry IN plane coordinates, then keep
full homogeneous W; the label's farthest endpoint remains at original position.
The old world-eye target disparity/depth gradient and world-card tip are removed
ONLY for vitals. Ordinary grapple card method remains separate.

Circle source match: current retail record showed only tip0 first captures. A
separate small piece can use head-free projection while line uses native-eye
projection; A2e matches centre to either projection of the same captured world
point. Original coloured glyph's geometry is recentered on the shared hit,
regardless of source layout or draw order. This source hypothesis is not proof
of the exact retail circle binding. Unknown/textured/white/unmatched/backing
pieces fail closed. Counter circles and first4 circle logs provide passive
coverage evidence. Do not claim actual circle correction until retail/headset
observed. Inspector shader and primitive caps remain unchanged.

GPU: final build ALL11 tests in21.91s. EACH route24 eye/layout/order/recenter/
moving-person cases with independent analytic z=-4 plane intersection. Original
ring topology centre equals line endpoint, every vertex satisfies perspective
HUD-plane equation, label unmoved, premul colour; broad/white/off-target/stale/
toggle negatives.32 accepted grapple world-card pixel references each; both
post-HUD triangle GPU blend/state restore fixtures;4097DLSS completions each,
51native50legacy authored transition tests. Evidence A2e-ctest-soak.txt and
build-native9/Testing/Temporary/LastTest.log. No retail launch performed.

INSTALL: runtime SHA256 1BE564F97B43159CC9DE57BE3CD721D147D9F2FFF53283509CBF69623CB52445
backup D:\Documents\Antigravity\VR_Modding\batman_asylum\re\backups\install-20261014_013236_511. Installer and independent verification cover19
owned files and five protected config/EXE files plus shader cache/renderingINI.
Game closed, profile inactive; no recovery, game-assets or donor edits. See
A2e-before-install.json/A2e-build-checks.json for exact paths/hashes.

## A2d RESULTS: person vitals leader line (2026-10-14)

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

Looked up: VR_WORLD_MARKERS_GUIDE6/7 and Knight NOTES2114-2134 SCANPROJ/
SCANLINE (separate label and world endpoint, hide old line/rebuild per eye),
later SCANHUD warns actor ownership/depth-march match was unproven; user's
Asylum request explicitly wants endpoint at person, so do not port Knight's
later all-at-HUD-depth preference. VR_HUD_GUIDE8/10/framework11; playbook
FAIL-HUD-006/007 ch04 at b955f2244f7c5d5f3b3e5f403b1b6721a3362423. No relevant
City/Origins vitals-specific fix found, accepted grapple method reused only.

CAPTURE OBSERVED: A2c_history_20261014_001229_355 frames0000/0040/0075 show
124/90/91bpm and nervous/calm panel on floating HUD; orange circle/leader ends
on floor beside standing people in both eyes and varies with camera. Latest
A2c grapple projection count stops while this widget remains visible; this is
evidence of separate ownership, not proof of its exact native caller. A2c run
archived in A2c-vitals-run. Actual actor selected by script is not captured.

STATIC: read-only exact-hash Ghidra A2d-callout-project.txt decompiles seven
native game project callers; ten known return RVAs on ProjectABC2A0 collected
in source. Existing target Project entry/caller/ret8 guards retained. New
native path records world+original divided output only for exact main-view
instance/active tagged main draw. No native projection output/culling rewritten.
These callers are leads, not evidence that any particular one owns vitals.

SOURCE: nativeSets16frames x64 anchors deduplicated perworldpoint; leftmost
original layout retained. capture_connector only on actual owned backbuffer,
post-scene UI boundary, no depth writes (disabled or read-only ALWAYS), no
stencil/mask, supported untextured UI shader output and bounded CPU geometry.
transitions9 inspect_ui exposes named/clip-position shader geometry/output;
existing owned managed bytes/sparse uploads supply data, no GPU readback.
Triangle/line lists/strips supported for connector probing; fades still accept
only triangle topology. Orange hue, skinny triangle area, extent and END
vertex proximity .035NDC select line, not transform origin. Broad panels/white
art retain original; small orange tip may use same matched native anchor card.
Only matched original stroke suppressed, label/text stays on floating HUD.

SOURCE final draw: connector_vertices uses exact pair eyeVP for person, actual
same HUD quad homogeneous corner mapping for text. Warps original mesh with
object displacement attenuating to0 at label end; source bends/thickness/colour
retained. Tip maps through existing world-card projection. New per-vertex
premultiplied-colour triangle pass aggregates up to8192vertices (one bounded
256KiB dynamic VB); state-restoring RT/viewport/blend/depth/raster/IA/VS/PS/GS
and linked shader instances. Called right after original HUD composition, so
label plane/recenter/scale matches actual HUD; native/deferred DLSS/AFW paths
share composition. No new scene/depth render. WorldVitalsConnectors defaults1,
live switch, native/links/linkMiss status and first bounded native/capture logs.
Accepted grapple world anchors/card/shader route unchanged.

GPU final A2d-ctest-soak.txt all11 pass in35.13s.6 BOTH-eye target-shader/CPU
geometry cases perroute: zero Transform translation, skinny orange bent line
matched by endpoint to moving person through yaw/roll; object endpoint exactly
matches each eye's world projection, far endpoint exactly matches unchanged
perspective HUD plane (nonconstant W), premultiplied RGB. Broad orange/white/
stale/toggle negative controls. Separate actual D3D11 triangle pass compared
to independent blend equation in both eyes; untouched outside pixels, RT/
viewport/topology restoration. Existing32grapple cases,8 old-affine differences,
marker slots5/6 transport,51native50legacy transitions,14menu fade/11HUDalpha,
particle/menu routing and4097DLSS pairs perroute all retained. GPU fixture
supplies a native anchor: no claim of actual retail widget/caller match yet.
First compile used Windows far macro as variable, renamed. No failed builds
installed. No autonomous retail game launch/kill.

INSTALL: closed leftover profile journal restored from validated4 original
backups with expected path/hash checks; active copies/rollback archived at
A2d-profile-recovery. New baseline after rollback;19owned/fiveprotected/cache/
saved modINI verified. Runtime SHA256 5DE4EFC30B6F7B758C246CDC85206CF0E89143B15E84A30BE52E22393C3394F9.
Backup D:\Documents\Antigravity\VR_Modding\batman_asylum\re\backups\install-20261014_004146_603.
A2d-build-checks.json and A2d-before-install.json. No donor/game asset/global
settings edits; only owned transient profile rollback. Actual native endpoint
source/shape matching, all moving targets, tip accuracy and headset stability
await JJ's next grouped run/F2. FPS and menu-state reader remain open.

## A2c RESULTS: target-depth grapple and final exit fade candidate (2026-10-14)

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

Looked up: VR_WORLD_MARKERS_GUIDE2-6,5b/5c; City markers.cpp MARKEREYE/BASE/
CARD/UPRIGHT/FACE, RETICLERUN/MASKSTOP; Origins ORIGINS_PLAN marker row directs
to City (no validated Origins-specific projection hook found); VR_HUD_GUIDE8/10;
framework11; playbook FAIL-HUD-006/ch04 world geometry and FAIL-HUD-007/ch04
geometry families at b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Used methods,
never donor addresses/assets/edits. Current Ghidra MCP returned404; STATIC exact
PE disassembly/registration evidence used instead, A2c-marker-projection.json.

STATIC target: table25e0a48 contains intUCanvasexecProject2065CC4->A588D0.
Native decodes FVector, reads canvas+64 view, callsABC2A0 atA58942/returnsA58947,
converts NDC to canvas pixels. Project ABI ECX view/out4+world3 stack,ret8;
actual VP+210, outputs(x/w,y/w,z/w,w). Runtime entry and caller-byte guards plus
hash-guarded ABI test added. This does NOT prove the grapple uses that caller in
retail; new first4 target logs/counters and next F2 run must establish it.

SOURCE markers9: rewrite ONLY exact caller in tagged main draw, exact view
instance, with head-free baseVP. Up to64 on-screen world anchors/frame,16sets,
64 exact render slots; dedup world points. Both final asymmetric eye VPs derived
from same base camera/pose. Renderer refreshes exact slot at draw because
BeginDrawing can precede constructor publication. Anchor/card at object's real
world point/depth, facing head-free game-camera POSITION with level right/up;
size fitted to game's reference projection. Head motion changes only eye VP.
Unknown/stale/behind/offscreen remain original. Renderer only sees markers after
verified scene/UI boundary, so ordinary scene draws bypass marker work.

SOURCE perspective: actual collected complex UI VS only transformsXY then
MOVs inputZW. Patch just the exact MUL(c6,y), MAD(c5,x)/MAD(c7,z)/MAD(c8,w)
position sequence toXYZW and remove inputZW restore. Preserve UV/colour/output
signature/ViewSpaceDepth. Fold clip K0/Kx/Ky into original c5..c8 and force
Z=.5W (no depth test); patch restores original shader/all16 transform values,
9 changed render states, RT/DS/viewport. Bounded128 shader variants, creation
invalidation with SRW lock and retained COM handle during draw. Live preference/
projection count atomic. No affine fallback silently advertised as perspective.

SOURCE matching: nearest reference within.12NDC, original flat Transform, exact
supported shader; initial sprite texture8..256px eachdimension starts run;
prompt/remaining pieces may continue up to8 draws at same anchor; masks, ZERO
source erase, multiply end run, large text atlas alone stays HUD. These are
source-derived guards needing Asylum retail validation, not transferred City
texture hashes. Private transparent LEFT/RIGHT full-XR D3D9 layers replay only
widget GPU draw, same art/textures/blends, no second UI logic/cut-out/re-layout.
Capture both eyes from LEFT/AFW physical widget pass; nativeRIGHT skips duplicate.
Native bridge slots5/6 added (slots0..4 unchanged). Per-eye exact generation
marker overlay after DLSS/AFW, alongside captured HUD and before bat layer.
Allocation/shader failures retain original draw; actual transfer counters/
headset outcomes pending. WorldGrappleMarkers default1, live checkbox.

SOURCE exit candidate: support original literal clip-position MOV VS without
Transform CTAB, identity geometry analysis; only existing proven black rectangle
matcher can omit it and supply full-field fade alpha. No literal-position bat
reroute, unsupported/textured/coloured/small draws retained. Actual final quit
box has no saved vertex/material owner yet: do not call its exact cause proven.
Expanded classifier made HUD-alpha test's full black quad a legitimate fade;
fixture now uses a small panel, same centre-pixel blend controls. Initial GPU
test hung on malformed sequence scan (fixed bounded advance/constant pattern);
transport fixture null ownDevice corrected to sheet's GetDevice. Final checks
passed; unsuccessful intermediate builds never installed.

GPU final A2c-ctest-soak.txt: all11 pass in24.52s.32 BOTH-eye marker cases EACH
route, original target complex shader vs independently projected world-card
vertices, near450/far1800 depth, game-camera heading, head yaw/roll/lean and
asymmetric lenses, original RT/VS/constants restored.8 old affine controls fail.
Actual two9->11 marker transports/whole-eye composition match every reference
source colour/alpha pixel. Real routing negatives erase/mask/large text atlas/
unsupported shader plus missing/stale anchors.5 new literal-position fade cases:
3 opacities suppressed vs visible original controls and2 small/coloured retained.
51native/50legacy transitions,14 full-field menu fade checks,11HUDalpha checks,
particle96/menu288 cases and4097DLSS completions perroute retained. Retail exact
marker owner/piece matching/camera stability still require JJ's grouped launch.

INSTALL: initial attempt blocked by active profile journal while game closed.
Verified original backup hashes and expected4 config paths; archived active
configs/journal, ran existing Restore-AsylumProfile, verified originals, removed
only journal. Refreshed preinstall baseline afterwards. This is restoration of
owned transient VR changes, not a new graphics-policy change. Record
A2c-profile-recovery/rollback.json.19owned/fiveprotected/cache/user renderingINI
verified after install. Runtime SHA256 29D8E7B6E27CC0BDDAE56F532C4CA653A45934A0188BC137BBD1EA0AA1466250.
Backup D:\Documents\Antigravity\VR_Modding\batman_asylum\re\backups\install-20261014_000215_003.
A2c-build-checks.json and A2c-before-install.json. No autonomous game launch/kill,
no City/Origins/game assets/global driver changes. FPS/menu-state reader open.

## A2b RESULTS: full-field menu fade with visible text (2026-10-13)

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

Looked up: VR_HUD_GUIDE sections5/8/10 (menu continuity, separate full-field
fades, own draws after HUD capture); framework guide11; playbook
ch04#ui-not-one-class, FAIL-HUD-007 at checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. City BATMAN_CITY_PLAN MENUFADE/
FADEALPHA/FADEMIN lines856-874: actual opacity avoids late black flashes,
hiding the menu causes flashing. Origins donor lookup has no independent
menu-input trigger useful here. Applied method to Asylum's proven draw family,
not City offsets/probes/assets. No upstream checkout or donor edits.

LIVE A2a: correct build in 21:41:36..21:43:19 run; actual kind1 AA10tri fade and
kind2 dense authored bat sweeps logged. Last counters1754 rectangles removed,
92bat draws, geometryReads126773/missingUpload0/failedCapture0. This closes the
previous no-match failure. JJ reports great progress but abrupt menu switches.
Specific bat framing/all loading moments are not independently headset accepted.

SOURCE: classify returns actual core opacity, including neutral-white multiply
fringes and declared COLOR1 coverage. Suppressed fade Scope accumulates maximum
matched opacity on LEFT (or AFW physical eye mapped LEFT); begin_eye clears for
new render even when pose generation repeats. RIGHT shares LEFT only at exact
generation; absent/stale/reset/toggled-off fade returns zero. No input poll,
frame-count timer, forced full-black threshold or unknown-menu-state gate.
Pair envelope published with the existing transition capture before XR processing.
compose_menu_fade uses existing state-restoring full-viewport mask shader and
one cached immutable1x1 R8 texture (no GPU readback); alpha is engine opacity,
blackFill1. compose_hud dims world first, then projected menu HUD, then existing
bat layer. DLSS immediate/deferred/stock rollback and AFW both-eye overlay paths
all reach this composition. Temporal reconstruction/depth/scene capture untouched.
Native3D menus use this path; actual first flat-to-native handoff timing remains
part of grouped runtime validation, not established by GPU test.
Default FullViewMenuFades1 with live "Full-view fades behind menu text" checkbox;
previous option relabelled "Remove windowed black fades". Settings not rewritten
at install. Telemetry records start/end and sampled actual pair alpha. No claim
that a button with no engine transition produces a new fade.

GPU: 8 real target-shader Scope envelope steps0/.125/.5/1/1/.5/.125/0, exact
pair reuse, mismatched/absent/repeated-generation/reset/toggle clear; AA original
straight/coverage/gamma programs check actual opacity at255/176/158 as well as
suppression. Transition counts46native/45legacy. 14 exact compose_menu_fade
checks EACH route: BOTH eyes at0/.125/.5/1/.5/.125/0, every37x41 world pixel
matches independent background*(1-alpha), alpha remains255; same HUD blit after
fade keeps white centre text255 even at black. Deliberate tiny/offset caller
viewport and bound RT restored. HUDblend11cases/old4controls, original bat
full2284x2448 stereo transport, particle/menu routing and4097DLSS pairs each
retained. Final A2b-ctest-soak.txt all11 pass in24.70s. No retail auto-launch/kill.

INSTALL:19owned/fiveprotected/cache/savedINI independently verified.
Runtime SHA256 B63E79821C4767B234B137D3C49F4B6D14C8E46D8EB735823B053969713963C8.
Backup D:\Documents\Antigravity\VR_Modding\batman_asylum\re\backups\install-20261013_215927_380.
Receipt A2b-build-checks.json, current preinstall hashes A2b-before-install.json.
No game asset/config/City/Origins/global-driver changes. Actual A2b menu timing,
both-eye background fade/new text and headset comfort require JJ's grouped run.
FPS and unknown menu-state reader remain open; no performance claim.

## A2a RESULTS: HUD coverage and actual authored-stage geometry (2026-10-13)

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

Evidence: A1z-transition-run contains the correct installed retail log and six
8-10s paired F2 replays. The first large black candidate is10tri/12vertices,
x[-2.00709,2.00603],y[-.91838,.91937],alpha1/stencil0. Recorded bat sweep
18:32:37.926..38.363 starts288tri at x[-1.42462,-.74408],y[-.59072,.37663],
grows to3252tri/full stage x[-1.42596,1.42366],y[-.74625,.74724],alpha1/stencil0.
Previous two-triangle rectangle and +/-0.9 both-axis bat gates reject these.
Complete per-vertex/material identity of those specific draws is not saved.

Looked up again: City src/core/gfx/gfx_hud.cpp fixed_blend lines374-428,
normal OVER vs multiply/additive alpha and mask-only writes; VR_HUD_GUIDE.md;
playbook FAIL-HUD-007/ch04#ui-not-one-class and ch06 evidence gates,
checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423; framework guide11;
City MENUFADESKIP/BATCUTS/STAGEBOX and Origins O1F-H changing-bounds failure.
Used methods only; no donor offsets/assets/edits.

SOURCE: hud9 Scope formerly forced ONE/INV_SRC_ALPHA for nearly all draws,
including multiply/erase/masks. This can create opaque black HUD coverage on an
empty transparent sheet. Normal OVER now accumulates coverage, multiply/additive
preserve it, ZERO/INV_SRC_ALPHA and ZERO/SRC_ALPHA attenuate prior RGBA; stencil/
alpha-only stock-backbuffer masks don't write colour into captured HUD. Original
RGB blending and all saved states restore. No added per-draw metadata probes.

SOURCE: transitions9 pixel VM decodes declared COLOR0/COLOR1 coverage; sample
analysis accepts zero-alpha and neutral-white multiply fringes. Rectangle proof
uses opaque core corners/edges/centre, actual covered area and thin AA border,
max32tri; rejects coloured/small/stencil/additive/unsupported art. Fresh opaque
dense bat sweep starts at outside edge while straddling vertical centre, or on
full letterboxed stage, rather than waiting for both bounds +/-0.9. Same-stage
250ms-gap/2s episode and fixed1.6x zoom prevent animation collapse/renormalization.

GPU: final A2a-ctest-soak.txt all11 pass;38native/37legacy transition cases,
including9 AA10tri positives across original straight/coverage/gamma shaders,
5 retained negatives,3 authored edge/fullstage/low-opacity cases. Both routes
add11 actual HUDScope GPU tests,4 old forced-alpha black-box controls; empty
and seeded sheets, multiply variants/erase/mask/additive/normal OVER/stencil/
alpha-only. Original shader binaries are test evidence only, not installed.
4097DLSS pairs EACH route; full2284x2448 stereo batmask transfer and near-plane
coverage retained. No autonomous retail launch/kill. Actual A2a counters,
mask submission and headset acceptance remain pending.

INSTALL: runtime SHA256 A326ED478352DD4E06FB10A0EB7A8AEA57FA22EC1170633B0B90E5C4D7FD265C.
Backup re/backups/install-20261013_195946_120; receipt A2a-build-checks.json,
current preinstall hashes A2a-before-install.json.19owned/fiveprotected,
RefShaderCache and saved renderingINI independently verified. FPS and unknown
menu-state reader remain open; this change makes no performance claim.

## A1z RESULTS: native UI storage access, fading rectangles and bats (2026-10-13)

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

HEADSET A1y: JJ still sees all black squares and bat transition within a window.
LIVE A1y archived in re/evidence/A1y-transition-run, including original ui_bindings
and disassembly. F2 now accepted under desktop/headset focus; saved up to84 paired
pictures/9.88s perpress. Replay172214 shows three menu transitions/actual black
bat geometry in small panel/solid black end frame. Contact sheet retained.
Counters: geometryReads0, missingUpload21332, CPUuploads0 before gameplay then84,
shaderRejects47177/stateRejects4476, removed/bat0. This closes "only unsupported
shader" as complete cause: supported shader draws were blocked on CPU data.

Looked up again: reuse playbook ch06 gate/paired-evidence discipline, FAIL-HUD-007
ch04, commitb955f2244f7c5d5f3b3e5f403b1b6721a3362423; framework11; City BATCUTS/
STAGEBOX/MENUFADESKIP and OriginsO1F-H warnings. No donor assets/offsets changed.

SOURCE root: original register_buffer tracked only DYNAMIC and <=256KiB resources.
Native managed9 already retains complete CPU buffer bytes, but geometry() never
used them. Thus normal static/managed UI indexed draws could not be examined.
New managed9::copy_buffer checks exact owned device/logical resource, unlocked
state, byte range and<=256KiB requested range; reads EXISTING CPU storage, no
Lock/readback/extra full-buffer copy. Resource virtual defaults false; only buffer
storage supplies bytes. Handles buffers created before hooks, large allocations,
managed reset survival and inactive/evicted upload shadows. Native geometry uses
this first, then sparse upload snapshots. Static/default/previous-route resource
registration now ignores DYNAMIC flag, supports allocation<=32MiB with at most
16 small<=256KiB shadow ranges (4MiB cap); coalesces overlapping valid ranges,
clears on DISCARD and uses absolute offsets. Existing CPU writes only. Legacy
writes larger than256KiB remain unsupported/retain original drawing. No GPU wait
added. UBYTE4/UBYTE4N read added for actual packed COLOR declaration.

LIVE captured shader pair B9B82C55E11FC149 is untextured ConstantColorPS (mov oC0,c0)
and Transform VS c5..8 with shared depth c13, UV matrix9..12, SHORT2 POSITION.
First diagnostic was red stencil-only geometry count21/COLORWRITE0; that single
draw is NOT itself proven to be the bat/fade owner. GPU tests reuse exact retail
bytecode/ABI to close buffer/shader-source gap; headset classification still
requires actual newly-readable geometry. Stage/shape/black guards retained.
Bounded16 black candidate evidence lines report actual clip bounds/alpha/primitive
count and stencil enable/compare/ref after successful reads, separate from prior
unsupported binding files. Do not promote a GPU mesh fixture into retail proof.

Bat composition retains original animation/geometry, full XR-size target, fixed
1.6x stage magnification and common0.8m plane covering both eye frusta. During an
active bat episode, smaller complex black geometry with the EXACT same Transform
is kept on that fixed stage for<=250ms inter-draw gap/max2s episode. A matching
opaque full-stage rectangle stays with that bat episode; other fades omit. Small
black art outside episode stays on normal HUD. No current-bounds normalization.
This is a bounded geometry continuation lead, not native movie-event ownership;
a different UI piece on the same stage within that episode remains a possible
ambiguity to judge from captures. Original unsupported routes are retained.

GPU A1z: all11checks24.55s;21native/20legacy transition cases. Actual recorded
shader/SHORT2 uses non-DYNAMIC512KiB MANAGED VB+IB, nonzero stream/index offsets,
base8/min4 and observed alpha255/176/158: removes new fades vs visible old drawing.
Old DYNAMIC-only classifier necessarily has no source for these resources.
Original bat mesh through those same static buffers is redirected once, leaves
original window clean, XR mask corners opaque/center transparent. Native direct
CPU reads plus wrong-owner/range/locked guards checked; legacy uses sparse writes.
Contracted bat geometry and unrelated small artwork control pass. Prior both-eye
2284x2448 alpha/asymmetric-FOV/roll/IPD transport, target gamma,96 sprite/288 menu
routing cases EACH bridge,4097DLSSpairs EACH and8192 legacy Presents pass. First
fixture failed because its old small-art control simulated the same active bat
stage; new positive contracted-bat case plus separate inactive-episode small-art
negative control now verify the intended distinction. No retail acceptance implied.

INSTALL19owned/fiveCURRENTprotected/cache/savedINI verified. RuntimeSHA256
BC45C3F00012FB4A2FFF6F055E9EC4BFD2EAF64B2702EC0C7186090F239AA452.
A1y backup re/backups/install-20261013_174724_845; receipt A1z-build-checks.json.
No game/config/shader-asset/global driver/City/Origins writes, no game launch/kill.

NEXT JJ one usual-shortcut launch title/allthree menus/load/bats/video/game/return
menu, F2 after any residual then normal exit. Read managedReads/geometryReads,
removed/bat and black candidate clip/stencil records with replay; confirm every
fade and bat start/end before marking fixed. If geometry reads succeed but shape
matching fails, use actual stored clip/primitive evidence. If rectangles are
stencil-masked bat shapes, preserve/reconstruct their original stencil rather
than removing them. FPS and unknown IsMenuLevel state from A1y remain separate
open work; this build makes no FPS or menu-state fix claim.

## A1y RESULTS: A1x retail rejects; UI path correction and F2 (2026-10-13)

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

HEADSET A1x rejected by JJ: all reported black squares still appear. LIVE correct
A1x loaded, default FullViewBats/RemovePlainFades1 in both retained runs; removed0,
bat0, geometryReads0, missingUpload0, CPUuploads84. Thus A1x size threshold was
not the blocking condition. Two logs and5 automatic stills are retained; no
history/button lines or new replay folders. F2 did not produce evidence. Cause
of missed key is not directly recorded by A1x; foreground-only gate/held-only
poll are identified SOURCE failure opportunities and corrected in A1y.
A1x raw alpha verifies solid rectangular fade: alpha255 initially, then uniform
blackRGB alpha176 or158 over91.9% sheet. A1x RAW report/contact sheet in evidence.
Those fade alphas alone DO NOT establish UiSolidPS ownership (that program's
single-evaluation alpha has different behavior). Do not promote the shader lead
into a retail root cause or report A1y headset-confirmed without actual counters.

Looked up again: playbook ch06 measure/dump-pipeline and cross-project-index,
FAIL-HUD-007/ch04, checkoutb955f2244f7c5d5f3b3e5f403b1b6721a3362423; framework11;
existing City MENUFADESKIP/OriginsO1F-H references. No donor assets/offsets changed.

STATIC/SOURCE: target cache UiSolidPS@58f7 uses LOG/EXP; A1x rejected either before
geometry. Support exact log2(abs(x))/exp2 and saturate; intermediate-inf->finite
black output retained. Uniform gray outputs only match fades for actual
DESTCOLOR/ZERO multiplicative blending; ordinary gray/color artwork retained.
Allow Z-enabled UI only when private captured lane, ZWRITE false and compare
ALWAYS; depth-tested geometry retains original route. Bounds/shape/black checks
remain. No blanket HUD-alpha stripping or texture suppression. Unsupported
shader/state/missing-shader cases record bounded unique target-host-only bindings:
bytecode, declaration, actual VS/PS constants, viewport/blends/draw count, max24.
No fixture logs are mislabeled as retail; no GPU readback in that recorder.
Status exposes shader/state/constant rejects, not just downstream geometry counts.

F2 SOURCE: low-bit latched or held edge; accept own foreground OR current OpenXR
FOCUSED state. Request both full-size still and existing preceding10s replay;
log acceptance/ignored focus. The headset input key path still needs JJ's run.
The capture buffer/encoder/memory limits and source resolution are unchanged.

GPU:15 transition cases per native/legacy route (A1y-ctest-soak.txt). Added actual
cache UiSimpleVS@3a84 depth-scale0.25 and UiSolidPS, three multiplicative fade
opacities with visible old controls; former opcode rejection reproduced. Ordinary
gray artwork retained; depth-independent ALWAYS accepted, LESS rejected. Prior
bat separate slot4->both2284x2448 alpha/asymmetric-eye coverage, sprite96 and
menu-routing288 each bridge,4097DLSSpairs each and8192 legacy Presents pass.
All11checks passed22.28s. This proves those paths, NOT which one retail uses.

INSTALL:19owned/fiveCURRENTprotected/cache/INI independently verified, settings
retained. RuntimeSHA2561C3077C029E14D142ED7D8ADA89C626C579B44D2B8AA68EF8598154D9CCB2317;
A1x backup re/backups/install-20261013_171439_609; A1y-build-checks.json receipt.
No game launched/killed, game/config/shader assets/global driver/City/Origins
writes. Native bridge and near bat composition retained. No FPS claim.

NEXT RETAIL/HEADSET: JJ one usual-shortcut run through title/three menus/bats/
video/game/return menu; F2 immediately after residuals, normal exit. Read both
capture logs and ui_bindings. If shader/state rejects dominate, use dumped
actual bytecode/decl/constants, not another guessed target-cache fixture. If
geometry unknown dominates, inspect actual upload size/usage/identity. If fades
match but bats do not, distinguish their actual primitive/textured material
before suppression. A1x particle routing was unknownStock throughout, not proof
that menu/gameplay state reader worked; record as separate open issue.

## A1x RESULTS: plain fades and native full-view bat composition (2026-10-13)

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

Looked up: City BATCUTS/STAGEBOX/WIPEHD/BLACKFILL/MENUFADESKIP;
Origins O1F-O1H (changing mask bounds hid bats; generated/copy/submitted are
separate evidence); VR_HUD_GUIDE sections6/7; framework guide11; playbook
FAIL-HUD-007/ch04 #ui-not-one-class, checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. No donor files/offsets/assets changed.

SOURCE: transitions9 hooks only owned device creation and small dynamic UI buffer
uploads; copies existing CPU writes (bounded16 buffers <=256KiB each), never
synchronizes a WRITEONLY buffer with the GPU. All four primitive routes classify
linear untextured black output plus geometry, orthographic stage bounds and
uniform rectangle alpha. Two-triangle stage fills omit; complex large black
geometry is re-rasterized ONCE into a private XR-size color/depth target with
fixed1.6x projection magnification. Recent opaque full-stage bat continuation
is retained on the bat plane. Normal small/colored/textured/unsupported draws
stay original; native eye association/state restore and reset cleanup retained.
Mask transport uses separate native9Ex slot4/retained legacy transport; original
HUD fallback retains animation on transport failure. Captured masks are painted
last into each eye after DLSS and AFW, using the same generation poses and a
common physical0.8m plane covering both asymmetric/canted frusta with6% margin.
Flat/title branch uses existing transition submission provider. Plane follows
full head orientation intentionally for a brief field-covering wipe.
Both live overlay switches default ON without rewriting JJ's savedINI.
Capture status and5s heartbeat expose removed/bat/geometry/missing-upload/failure
counters; native bat plane is included in composed eye/history captures.
Flat independent XR quad is not included in the pre-compositor scene capture.

STATIC: read-only focused Ghidra decompile and UEViewer list identify six
RSeqAct_FadeScreen objects and MainMenu/MainV2 in Frontend.umap. Native viewport
black fills were found but their ownership of these fades is NOT established.
Target GlobalShaderCache original Transform VS/ConstantColorPS extracted and
used in GPU fixtures; these signatures are NOT yet proved to own retail bat
geometry. Source material ownership does not transfer from City/Origins.

LIVE A1w CAPTURES: logs/INI archived in re/evidence/A1w-transitions-run.
Read RAW32-bit BMP alpha (Pillow BMP decoder drops it): first captured HUD has
91.799% opaque black, x0..1889/y83..1942 at1890x2026, clipY+/-0.918..0.919.
Thus a +/-0.95 full-stage guard would miss this actual large rectangle. Fixed
+/-0.9 stage coverage test added with a real rendered positive/negative control.
A later HUD is mostly transparent; do not infer opacity from its black RGB.
Raw results in raw-hud-alpha.json. This proves the observed sheet's coverage,
not its shader/fade actor identity or the A1x retail fix.

GPU: all11checks pass, A1x-ctest-soak.txt.9 transition cases EACH native/legacy
route reproduce visible old fades at3 opacities and the observed letterboxed
stage, preserve small panels/colored/title art, preserve complex original
geometry with transparent center, and restore matrix/RT/viewport; buffered
WRITEONLY VB+IB uses CPU upload shadows. Captured black mesh is transferred
through the production mask route and projected into BOTH2284x2448 images with
actual GPU premultiplied blending: corners black, center original scene, roll,
IPD and asymmetric FOV. Independent math validates0.8m coverage/largeIPD and
opposite quaternion signs. Prior96 sprite/288 menu-routing cases each bridge,
4097DLSSpairs each and8192 legacy Presents still pass. This validates plumbing,
not every retail GFx shader/batch classification or real runtime timing.

INSTALL:19 owned files, five current protected game files, shader cache and
saved renderingINI independently hash verified. RuntimeSHA256
24BD9FA50B728216840ABA092367741345202C1E0DF616D2642D0CABD4525F83.
A1w backed up at re/backups/install-20261013_142057_288.
Receipt A1x-build-checks.json; preferences inclQuality3 unchanged.

HEADSET/RETAIL A1x: REJECTED in two subsequent JJ runs; black squares unchanged, removed/bat/geometryReads0. See A1y above. The following original test plan is historical. One JJ-driven launch covers all transition points:
1. Usual VR shortcut; title to each of the three front-end menus.
2. Load saved game; keep animated bats, verify full view, then story video/game.
3. Return to front-end menu; confirm original menu smoke/gameplay correction.
4. If anything remains wrong, F2 immediately after it saves preceding10s; exit
normally. Read transition counters and paired pictures before another rebuild.
If removed/bat counters are zero, inspect target GFx shader/declaration/upload
format rather than widening filters blindly. If capture works but near plane
missing, separate captured/transport/composition/XR submission evidence. No FPS
fix claimed; current performance investigation remains separate and open.

## A1w RESULTS: original menu smoke, corrected gameplay smoke (2026-10-13)

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

HEADSET preference: JJ asks to keep A1v in gameplay but restore the original
menu behavior. LIVE A1v confirms correction reaches actual particle draws:
59050 swapped patches/25726 axis mismatches, noPass0/failures0. Preserved logs
and settings in re/evidence/A1v-smoke-run before fixtures. Do not infer acceptance
of every particle material from this request; it establishes the requested scope.

Looked up: reused playbook FAIL-STR-023/ch14 #per-view-decisions, checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. City/Origins menu_state observes game
signals; no copied offsets/ABI. VR_HUD_GUIDE checked for label work; no HUD/menu
layout changes. STATIC target shipped Frontend.umap/Frontend_Batman.umap and
native registry intAWorldInfoexecIsMenuLevel ->0x9c6f20. Its empty-name path reads
GEngine0x26b62b8 -> pointer+44 -> viewport array+30/count+34 -> first viewport ->
+290mask400 via0x9b62d9. Map-load code0x9cb708 compares configured menu map to
loaded map, then0x9cb716 writes EXACTmask400. Saved bounded disassembly at
re/evidence/menu-level-flag.txt. These are target facts, no donor map offsets.

SOURCE: new menu_state module uses that guarded cached flag READ ONLY. Three
runtime code signatures plus static ABI checks cover getter, setter and global
chain. Counts/pointers/readability checked; invalid or teardown state is unknown.
Game-thread viewport draw samples once per pair; ctor publishes mode with the
same serial as particle camera data, including AFW. Render-thread draw correction
runs only on known gameplay passes; front-end and unknown retain original shader
constants. Main front-end level signal is distinct from an in-game pause overlay:
background gameplay stays on the gameplay policy. Toggle now labelled "Stable
smoke facing during gameplay"; saved StableParticles preference unchanged.
menuStock/unknownStock counters added to captures/heartbeats. No new function
calls or hooks into the engine, menu FPS timers, draw-count heuristics or persistent
head tracking changes. A1v axes/centre/restoration retained in gameplay.

OFFLINE: fake native read chain transitions menu/gameplay/menu, unrelated flags
preserved, null/invalid/teardown returns unknown. Original shipped particle shader:
96 prior facing cases retained perroute, plus288 routing cases EACH native/legacy
route with same tracked eye/default correction ON. Menu/unknown reproduce STOCK
pixels; gameplay reproduces A1v reference within1LSB; all constants restore.
11/11checks pass including4097DLSSpairs perroute, AFW/gamma/HUD/profile/ABI;
re/evidence/A1w-ctest-soak.txt. Static/source/GPU evidence do not establish retail
menu-flag observation or headset result; pending JJ's next normal launch.

INSTALLED01:03 Sydney:19owned/fiveCURRENTprotected/cache/current renderingINI
independently hash verified. Quality3 retained. RuntimeSHA
7B8BF1D685D183A6764D4996EF0A9F8E840B7414938A7F797170BFDAEE215905.
Backup/verification in re/evidence/A1w-build-checks.json. No game launch/kill,
asset/executable/config/global driver/donor changes. FPS optimization remains open.

## A1v RESULTS: target particle binder layout corrected (2026-10-13 Sydney)

CURRENT A1v INSTALLED (2026-10-13 00:50 Sydney): corrected Asylum's swapped,
negated particle-facing axes. A1u HEADSET REJECTED; retail named7/patch0/
axisMismatch72999 showed its guard blocked every particle correction.
Actual target binder now reproduced in96 GPU cases EACH native/legacy route;
all11checks pass. Default StableParticles ON/live toggle retained;19owned/
fiveprotected/cache/savedINI hashes verified. HEADSET A1v pending. Gamma A1t
accepted; FPS still open. Next JJ ONE launch: menu AND opening gameplay smoke,
head roll/yaw/lean, then gamepad-camera turn; rolling capture and normal exit.
See RE_NOTES A1v and re/evidence/A1v-build-checks.json. Older entries historical.

HEADSET A1u: JJ confirms all menu AND gameplay smoke still rotates with head
roll/yaw; facing should respond only to gamepad/game-camera changes. This rejects
the A1u headset fix. LIVE A1u runtime confirmed by log build ID; StableParticles1,
named7, patched0, axisMismatch72999, noPass0, failures0. Logs/settings preserved
before fixtures in re/evidence/A1u-smoke-run; latest rolling replay004346 viewed,
opening corridor/smoke visible under head roll. Not a shader-classification miss.

Looked up: reused FAIL-STR-023/ch14 #per-view-decisions and framework09 camera
basis; playbookb955f2244f7c5d5f3b3e5f403b1b6721a3362423. No new technique/donor
addresses. Target STATIC UTF16 CameraRight/Up string refs lead to binder names
0xb53cd0 and vtable0x1ff9388 -> Set0xb540a0. The binder normalizes inverse-view-
projection row0/1, NEGATES them and uploads row1 as CameraRight (0xb547ca), row0
as CameraUp (0xb547e7). Constructor0xaecf34 inverseview and0xaecf4c inverseP*
inverseV -> self+0x290 establish source axes. Disassembly retained in
re/evidence/particle-facing-binder.txt; shipped exe hash guarded. Shader CTAB
names were correct; A1u fixture prepared ordinary right/up constants instead of
reproducing the target engine binder. All real facing draws therefore failed its
strict matching guard despite the synthetic fixture passing.

SOURCE A1v: current-eye identity/unit/perpendicular guards recognise both direct
and SWAPPED axis pairs, preserve each uploaded sign and map each to the matching
game-camera axis. Named shader, exact-slot, foreign-device/light-view/stale guards
remain. Same relative centre correction/real eye VP/lighting/particle spin and
immediate constant restoration. Capture/status now records direct/swapped match
counts; bounded first4 guard diagnostics print raw uploaded and expected axes
so a further layout miss is diagnosable from the same launch.

OFFLINE GPU:96 original Asylum shader cases EACH native9Ex/retained9On12 route.
48 use independent inverse-VP normalization/negation/swapping from actual target
binder; other48 retain direct-layout coverage. OldA1u predicate rejects all48
target cases, establishing regression. Corrected pixels match head-free-facing
reference through REAL eye projection within1LSB;72 old-facing controls differ.
Covers two particle alignment modes, both eyes, changed game yaw/pitch/roll,
head yaw/pitch/roll/lean/combined motion; authored spin and all18 register constants
restore exactly. Existing stateblock refresh/nonparticle/stale/light-view guards
pass.11/11 regressions pass including4097 DLSS stereo pairs perroute, gamma,
AFW/HUD/depth and profile checks (re/evidence/A1v-ctest-soak.txt).

INSTALLED00:50 Sydney:19owned/fiveprotected/cache/currentINI independently hash
verified. SavedQuality3 retained; game closed, no game launch/kill. RuntimeSHA
F751116A7FAC31BD154546DC46BB090E6088E1032BB6A0FEE2B9DB1B4C42722E.
Backup and checks: re/evidence/A1v-build-checks.json. HEADSET A1v pending;
FPS optimization still open, no new speed claim. Next JJ one run includes menu
and opening smoke roll/yaw/lean + gamepad-camera turn, rolling capture, normal exit.

## A1u smoke/particle facing RESULTS (2026-10-13 Sydney)

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

Looked up: playbook FAIL-STR-023 and docs/14-render-pass-hazard-atlas.md
#per-view-decisions (consistent billboard facing), checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; framework09 camera/basis. City
BATMAN_CITY_PLAN.md BILLBOARD2026-10-08 and src/game/arkhamcity/billboard.cpp
provide the head-free basis method only. No donor offsets, buffer layouts or
shader identities transferred; City/Origins source and installs untouched.

HEADSET: JJ confirms A1t brightness better and reports smoke cards rotate with
head roll/yaw. A1t snapshots inspected (menu233144/opening233212); still images
alone cannot establish motion. Retail logs/settings preserved BEFORE fixtures
in re/evidence/A1t-smoke-run. Latest native source1890x2026, Quality3 input
1520x1696/output2284x2448, completed-pairFPS62; not a performance comparison.

STATIC: installed RefShaderCache-PC-D3D-SM3.upk has252 SM3 vertex shaders with
named CameraRight/CameraUp and particle bindings. CTAB report in
re/evidence/particle-ctab-bindings.json. Original2484-byte/111-instruction shader
and disassembly retained locally as GPU-test evidence, not packaged. Its
CameraRight/Up expand cards; CameraWorldPosition drives position/axis-facing
branches (lean/IPD). CameraPosition is lighting input and remains eye-correct.
Engine view constructor0xaecf31 inverts the updated view, then0xaecf74 stores its
origin at+0x390; see re/evidence/particle-view-origin-ctor.txt. MCP404 did not
block independent offline reflection/disassembly.

SOURCE: new game/asylum/billboard9 module classifies target-owned vertex shaders
by bounded named CTAB parsing. Weak metadata cache4096 entries, no retained COM
references; stateblock/reset invalidates current binding. Native ctor publishes
head-free/eye axes and relative world-space centre delta per exact render slot.
Draw scope requires current native view, matching eye-axis constants and known
particle shader; substitutes game-camera axes and game-minus-eye centre while
preserving signs, translated origin and w. Projection, lighting, local matrices,
authored animation/spin unchanged; original constants restored immediately.
Nonparticle/foreign-device/stale/light-view draws pass through. Live toggle
StableParticles defaults true, persists only user changes, resets temporal
history on change; capture/heartbeat counters record classification/patch/guards.
Both native9Ex and retained9On12 use the same owned-device module.

OFFLINE GPU: original shipped shader,40 cases EACH route across screen/axis
alignment, neutral/yaw/roll/lean/combined pose, both IPD eyes and game-camera
turns. Corrected pixels match independent head-free-card reference within1LSB;
28 old-facing controls differ each route. All18 shader constant registers
restore bit-exactly. Stateblock shader refresh and nonparticle/nonnative/
foreign-device/stale-slot/light-camera guards verified with correction enabled.
Full regression11/11 passes (re/evidence/A1u-ctest-soak.txt): native/legacy4097
DLSS pairs,8192 legacy Presents, existing HUD/depth/AFW/gamma/profile checks.
This proves GPU behavior under controlled inputs, not retail/headset acceptance.

INSTALLED:19 owned files/five protected, actual shader-cache and current saved
INI hashes verified independently. Quality3/resolution/lighting preserved.
RuntimeSHA8AE8E1EA232445C0D8549002510AFF48D91D84F103B438DB6B0E894B651B8039.
Receipt re/evidence/A1u-build-checks.json; backup path recorded there. No game
launch/kill, asset/executable/config edits, driver writes or donor modifications.
HEADSET A1u pending; FPS remains open and no speed improvement claimed here.

## A1t gamma RESULTS / A1s headset performance (2026-10-12 Sydney)

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

Looked up: playbook FAIL-STR-033/ch09 #first-pixels-gamma and typeless views;
ch19 existing per-view light scissors, checkoutb955f2244f7c5d5f3b3e5f403b1b6721a3362423.
Framework10 submission/color formats; VR_HUD_GUIDE room-fixed full-view screens.
Source scope: native9 BGRA scene buffers sampled as encoded UNORM into a linear
native3D output remain correct; only the flat room-screen SRV needed sRGB decode
before the sRGB XRRTV encoded again. encoded_colour.h now selects the matching
resource family and view, and flat_copy invalidates stale cached view formats.
GPU: actual native9 shared BGRA plus RGBA/BGRX controls,16 grey/color bands and
alpha/black; six source/RTV combinations within1LSB. Old sourceview reproduces
128->188. Both native/legacy4097 DLSS stereo pairs, AFW regressions, native ABI,
profiles/overlay checks pass. Installed19owned/fiveprotected/settings verified.
Saved Quality3 is JJ's new preference; it was preserved, not reset to historical0.

A1s LIVE logs preserved in re/evidence/A1s-headset-runs before fixtures. Run1
23:15:09-23:16:44 and run2 23:18:00-23:18:19 both native9Ex; retained logs use
Balanced4 then latest Quality3, source1644x1764 / DLSS1322x1478 -> XR2284x2448;
changing mode live does not retroactively prove a startup source-size comparison.
XRformat29 confirms sRGB screen destination. Zero helper fence/completion timeouts;
normalWM_CLOSE both runs. Scene timings vary with view. Example heavy final
heartbeat CPU LEFT3.908/RIGHT3.692ms + colour1.540ms/eye + HUD.072ms/eye +
XRbridge2.336ms/eye + physicalPresent.373ms ~=15.87ms/pair, near61FPS observed.
Depth time is INSIDE scene budget; do not add it twice. GPU elapsed includes
queue starvation/interception, is not pure busy time. No matched flat230FPS trace,
so this explains observed spans, not that every millisecond is necessary.
Coarse512x512/R32F format114/Z-enabled geometry chunks in both eyes (.69-1.14ms
in samples) are consistent with shadow depth, but mixed chunks do not prove all
cost is shadows. Full-resolution F16 lighting chunks are also substantial. Saved
engine configs list DynamicShadows=True, MaxShadowResolution512; not proof of
active engine values. Potential optimization: validate per-light shadow camera/
object/material equality and pair ownership before reusing eye-independent maps;
verify existing light scissor/culling still applies under the modified FOV. No
quality reductions, blind right-eye shadow skip or offsets reused in this build.
NVIDIA preset13 remains observed; x64 helper/GPU transport overhead remains open.

## A1s RESULTS ? native9Ex direct route installed 2026-10-12 21:53 Sydney

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

SOURCE/BUILT/OFFLINE: Details and reference provenance in NATIVE9_BRIDGE.md.
Eleven extended checks passed in18.94s. Native module uses9EVENT producer and
11EVENT consumer completion before shared slot reuse; actual full-size images
checked at every pixel. Native hooked .25/.75 scene depth survived later clear1;
HUD private stencil alpha and asymmetric eye remap pass. Native DX11 helper
completed4097 distinct-eye stereo pairs with no fence timeouts; memory57.5->60.1
MiB in this small synthetic fixture, not retail address-space proof. Retained
9On12 suite completed4097 DLSS pairs/8192 Presents and pipeline regressions.
Initial assembled fixture hit fixed-name helper collision error183 and unsafe
null-mutex readback; ABI8 PID/tick session names plus fail-closed fixture and
explicit native/helper teardown fix it. Separate build-native9 avoids mixing
validated legacy artifacts with the native candidate. No game launched/killed.
INSTALLED:19 owned, five CURRENT protected files and rendering INI independently
hash verified. Receipt A1s-build-checks.json; oldA1q full owned-file backup path
recorded there. Previous Bridge launcher selects this build's retained9On12 path.
LIVE/HEADSET: none forA1s yet. FPS/native compatibility/ground/flicker remain
pending. First grouped run uses saved Off at same scene/resolution, then capture
and normal exit. Optional DLSS/AFW checks fit the same run; crash-prone last.

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

# Asylum rendering evidence

## A1j - live scene state cache and gameplay budget (installed00:47 SydneyOct12)

HEADSET A1i: JJ now sees up to90fps in the menu; beginning gameplay still<60.
Same9950X3D/4070Ti. Prior frame-cap correction therefore helped, but does not
complete performance work. Origins90fps idle is a useful hardware reference,
not a matched-scene comparison. Image quality improvement remains accepted.

LIVE A1i run00:26:47-00:27:52: menu90 then82, gameplay53-60/some66, pair~17-19ms.
Pacer waitGate0-1ms/s; physical desktop Present .26-.44ms per pair. Native
overlap1/sync3, exact pose tags, zero fence/completion timeouts. Colour transport
.62-.84ms/eye, depth .48-.67ms/eye (one capture per eye), XR bridge including
DLSS/acquire waits~2.7-3ms/eye; helperGPU~2.7ms/eye. These CPU wall spans include
submission/blocking and must not be treated as GPU execution or independently
summed across overlapping work. Saved Balanced4: input1322x1478->output2284x2448,
game source1644x1764, fitted desktop1644x1440. Evidence in A1i-performance.
profile-last afterStartup/beforeRestore shows user BmEngine smoothingTrue/max300
while base/game configs remainFalse/max300: target regenerates/re-enables user
smoothing, but propagates the raised ceiling. This is configuration evidence,
not a live memory property read; menu90 is the headset/retail effect evidence.

LOOKUP: reuse A1i City cap/Origins limiter leads, playbook19 three ledgers,
matched A/B and chapter06 measurement. New playbook19 cached full-resource
description/COM identity and render-pass-hazard-atlas classifier (LIVE bindings,
not shader creation), framework04 section7 reset invalidation. Checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Microsoft primary documentation:
[all-state block contents](https://learn.microsoft.com/en-us/windows/win32/direct3d9/saving-all-device-states-with-a-stateblock),
[D3D9 queries](https://learn.microsoft.com/en-us/windows/win32/direct3d9/queries).
No donor ABI/offset transferred. Target cache observes successful setters and
StateBlock Apply; copying creation-time state alone would be unsafe.

SOURCE: hud9 Scope previously re-read RT/DS/viewport/full descriptors/Z state
and COM identity on every draw. Mutation-invalidated snapshot reuses actual
LIVE values; RT0/DS/viewport/ZENABLE/ZWRITEENABLE/ZFUNC setters dirty it, and
StateBlock Apply invalidates both snapshots even if it bypasses setters.
Matching interface pointers avoid unnecessary QueryInterface; differing
pointers still compare canonical IUnknown. Get-state failures default to no
depth selection. Eye boundaries/reset release cached COM resources before
Reset. Foreign devices/blocks do not mutate owned cache. Structural scene-depth
selection, freeze-before-clear/rebind, private HUD filters and RIGHT sheet reuse
are preserved. FastSceneTracking defaults1 in OWN aa_rendering.ini; diagnostics
toggle persists, timed comparisons/fixtures use nonpersisting changes.

SOURCE: scene CPU starts at owned tagged render begin, ends at D3D9 Present
entry BEFORE final colour/XR/DLSS transport. Eight-slot GPU timestamp ring samples
at most two native pairs/s to bound marker overhead. It uses
timestamp/frequency/disjoint queries and GetData flags0, never waits/flushes;
busy slots skipped, disjoint/error samples rejected, resources reset safely.
Measures elapsed graphics timeline including scene/HUD/depth interception;
queue starvation or concurrent helper work can enlarge it, so not pure busy
time. Counts/per-eye means/maxima logged separately from existing bridge spans.
FRAME RATE Compare scene tracking speed holds resolution, DLSS and native
rendering constant, timed8s OFF/ON/OFF; completed-pair FPS recorded BEFORE
automatic still/history captures and one-second settle intervals. Restores
prior cache/lens/AFW/DLSS settings. No user live reporting required.

OFFLINE/GPU: all8 final checks PASS (A1j-ctest). Actual9On12 256 unchanged
scene draws per eye:512 binding refreshes with live queries,1 cached; classifier
CPU .094/.093ms vs .008/.008ms. Full draw submission is NOT reliably faster
in tiny fixture (cached2.446/.420ms vs uncached.113/.110ms), due to driver batching
and GPU work; only getter/classifier reduction is demonstrated. Real StateBlock
restores stale viewport/depth states after explicit RT/DS restoration (RT/DS
are outside D3D9 stateblocks); cached live state correct afterwards. Geometry
.25/.75 survives subsequentclear1/rebind; small private filters/HUD/right reuse
and all DLSS/AFW/final-history regressions pass. Timestamp ring returns both
eyes (fixture elapsed5.347/1.555ms), zero rejected samples; not retail timings.
LEFT/RIGHT scene timing labels are exact in native; AFW uses one drawn HUD
source lane, so these are not two independently rendered AFW eyes.

DEPLOYED: A1j00:47:33 SydneyOct12,17 owned files/five CURRENT protected hashes
verified; backup re/backups/install-20261012_004733_408. Runtime SHA256
E28DE2877186FD4D5789222029E6C5DE2E674068E92B661549FAFCD62C4D2837.
Receipts installed-build/A1j-build-checks/A1j-dlss-gpu. No game launch/kill,
quality reduction, other mod writes or persistent game-config modifications.
HEADSET A1j gameplay FPS benefit remains pending. Next grouped launch uses
same gameplay spot and automatic tracking A/B, then scene/transfer timing
comparison to rank remaining bottlenecks; ground/AFW/jitter stay open.

## A1i - stock62fps cap and desktop presentation (installed00:22 SydneyOct12)

HEADSET: JJ reports50-60fps early gameplay and ~60 in the sparse menu.
JJ's PC is9950X3D + RTX4070Ti; Origins holds90fps standing at its start.
This is a useful same-hardware comparison, not a matched render/GPU benchmark.
LIVE A1h menu:61-62 completed pairs/s consistently, mean~16.15ms/pair;
gameplay mostly45-59, later42. Headset90Hz, waitGate0. Overlap selected1,
sync3, ~3450 queuedLEFT requests, zero fence/completion timeouts. HelperGPU
~2.65-3.3ms/eye; pair output CPU wait~4-5ms (cumulative). Pose age usually
16-47ms, exact tags match. Timed safe/overlap A/B was not run, so no precise
causal FPS gain claim. Logs preserved before GPU fixtures in A1h-performance.

LOOKUP: City BATMAN_CITY_PLAN round2026-10-03 explicitly found the62fps cap
in DefaultEngine.ini regenerated into BmEngine on launch. Origins RE_NOTES
has different limiter semantics (stock max=-1). Playbook chapter09
#runtime-owns-pacing/#mirror-is-a-scheduler, FAIL-PERF-022, chapter19
three ledgers/matched A/B/count completed pairs; framework04/07/10.
Checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423. No donor offsets copied.

STATIC TARGET: Asylum DefaultEngine.ini [Engine.GameEngine] lines453-455
smoothTRUE,max62. Prior launcher only covered generated BmEngine and user
configs; it omitted the base source of that merge. The menu cadence strongly
suggests the stock cap but restored post-exit config values do NOT prove an
active runtime property. City regeneration behavior is a lead, not Asylum
runtime proof. D3D9 CreateDevice/Reset also previously kept the game's desktop
presentation interval, with no explicit owned normalization or timing.

SOURCE FIX: Get-AsylumConfigPaths now journals DefaultEngine BEFORE BmEngine
and the two actual user configs. Applies bSmoothFrameRate=False to base and
generated files, MaxSmoothedFrameRate=300 as a nonbinding fallback if smoothing
is re-enabled; XR remains the intended cadence owner. Existing near-square
resolution/DoF-off/motion-blur-off/no-MSAA settings retain the same values.
Everything is restored byte-exact after normal exit; no persistent base change
on installation. Saved Balanced4, output2284x2448 and input sizing unchanged.

SOURCE FIX: owned initial D3D9 CreateDevice and every owned Reset force
PresentationInterval=IMMEDIATE. Additional devices keep original parameters;
factory forwarding, FPU preservation, window sizing and flags remain. Logs
requested/effective interval. Periodic CPU wall timing separates colour/HUD/
depth transport, XR bridge (includes DLSS/acquire waits), physical desktop
Present; these are CPU submission/blocking times, not GPU execution timings.

SOURCE EVIDENCE: launcher profile-last.json records beforeLaunch, after15s
startup, and beforeRestore hashes/raw values of all4 configs. Captures before
restoration distinguish engine rewrites from restored settings. Startup/exit
telemetry errors warn and do not bypass profile restoration. Config read-back
remains configuration evidence; actual completed-pair FPS establishes effect.

OFFLINE: real actual config COPIES all apply successfully (A1i-profile-dry-run).
ANSI/UTF16 tests copy configured base over generated to emulate startup rebuild;
smoothing staysFalse/max300/VsyncFalse/near-square. Journal includes both,
editor settings unchanged, byte-exact rollback of BOTH originals verified.
GPU: real9On12 swapchain created from requested intervalONE normalized to
IMMEDIATE and actual GetPresentParameters confirms it. Existing depth/HUD,
17 realNGX overlap pairs, missingRIGHT/reset/mode-off/timeout/recorder checks
pass. These are not headset cap removal or90fps acceptance.

DEPLOYED00:22 SydneyOct12: all8 tests PASS,17 owned hashes and five CURRENT
protected Asylum EXE/base/generated/user config hashes independently verified.
Backup install-20261012_002212_526. RuntimeSHA256
59E12B77ECBAE9212D51CEF68A8F5BD183CB575AB26069F7E11041C4E840820D.
Receipts A1i-build-checks/A1i-ctest/A1i-dlss-gpu/A1i-profile-dry-run and
installed-build.json. No game launch/kill; no City/Origins edits.

NEXT GROUPED LAUNCH (JJ): usual Asylum VR shortcut (owns reversible profile),
same native/Balanced/resolution. Wait15-30s in same menu and check FPS;
then initial gameplay spot. Automatic profile and CPU-budget logs diagnose any
remaining limit without typing in headset. Capture prior10s if artifacts recur;
optional existing transfer-speed A/B after warm-up. Close normally; compare
profile-last.json and pacetrace. Actual>62fps/90fps, HUD/ground/AFW/jitter still
require runtime/headset validation. Do not lower quality to conceal overhead.


## A1h - native DLSS overlap (installed00:07 Sydney, 2026-10-12)

HEADSET A1g: JJ confirms improved image quality; frame rate remains poor.
LIVE baseline preserved BEFORE GPU fixtures in re/evidence/A1g-performance:
Balanced1322x1478 ->2284x2448, about33-37 completed stereo pairs/s,
27-31ms/pair; waitGate0-1ms/s, helperGPU2.33-2.49ms/eye, safe sync2.
Exact image-pose tags match, queue age63-78ms (not measured headset latency).
Raw retail DS .993402779.. .998947680, no exact0/1, ZFUNC4/LESSEQUAL;
physical projection .999/-9.99. Depth preservation reaches actual geometry.

LOOKUP: City DLSS_PLAN FINISHFIRST/run24 and OVERLAP/run25 (safe73 vs
88.4fps overlap in its fake-headset benchmark), playbook chapter19#blocking-wait-floor/three
ledgers, STR-012/021, framework04/10 ownership/submission. Reference checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. City results suggest a METHOD,
not Asylum offsets, ABI, GPU timing or headset acceptance. No City/Origins edits.

SOURCE: Asylum IPC6 already supports per-eye requests and shared fences.
Client now signals its own GPU input fence after copy/keyed-mutex release,
submits LEFT, and immediately allows engine RIGHT rendering. Helper waits
on CPU for input completion, evaluates oldest eye request first, releases
ownership and signals output completion. Client waits for RIGHT serial
(both eyes completed), validates both replies, copies RIGHT, then finishes
held acquired LEFT before RIGHT evidence/release. Final HUD is painted after
NGX. Subsequent input fence orders shared-output reuse after these GPU copies;
no per-eye CPU GPU drain in overlap. Unsupported Device5/shared-fence support,
no acquired LEFT hold and AFW use existing synchronous safe transfer.

SOURCE: Stock LEFT (with HUD) and RIGHT backups preserve atomic fallback if
RIGHT depth/evaluation is missing or LEFT final copy fails. Reset/mode-off
between eyes discards queued LEFT; resource resize cannot replace its shared
textures before completion. Early teardown drains held LEFT with bounded waits.
Helper input-fence timeout now rejects BEFORE NGX/keyed-mutex acquisition;
no foreign-context GPU wait on timeout or absent shared fences.

SOURCE/UI: Overlap defaults ON, optional DLSSOverlap setting in own INI.
DLSS drawer exposes overlap toggle and actual transfer state. 'Compare DLSS
transfer speed' runs native safe / overlap / safe,8s each with unchanged DLSS
mode, output/input resolution, centred enclosure and AFW off. Saves paired
stills/history plus phase pair FPS and blocked-fence wait status. Restores
saved mode/lens/AFW/overlap afterward; timings end before blocking still capture.
Stay still for the comparison. Existing periodic captures can affect a phase;
use an already settled scene (30s after initial rendering) for cleaner evidence.

GPU (actual RTX4070Ti, own9On12/11On12 bridge + separate64-bit NGX process):
17 overlap pairs, LEFT returns queued before accepted evaluation, final LEFT
RGBA255/0/0/255 and RIGHT0/0/255/255, separate histories valid, no fence
timeouts. Missing RIGHT keeps original stock-green LEFT and invalidates both
histories. Early LEFT finish and subsequent pair pass. Reset/mode-off between
eyes and disabled pair-hold safe fallback pass. Deliberately unsignaled input
fence rejected on CPU in100ms, bounded output answer, no NGX dispatch.
Recorder regression decodes FINAL green LEFT replacing initial red snapshot,
RIGHT blue, chronological native+AFW multi-frame JPEG/CSV export. Existing
physical-depth, large-origin motion, AFW, HUD and profile/ABI checks all pass.
These GPU regressions establish ownership/correctness, NOT retail FPS gains.

DEPLOYED00:07 Sydney: both Release builds; all8 checks PASS;
17 owned files and four CURRENT protected EXE/config hashes independently
verified. Backup and runtime hash in A1h-build-checks.json/installed-build.json.
Saved Balanced4 and resolution retained. No game launch/kill. Receipts:
A1h-ctest.txt, A1h-dlss-gpu.log, A1h-history-fixture, A1g-performance.
HEADSET A1h performance, HUD, ground, AFW and motion/jitter remain unverified.

NEXT GROUPED LAUNCH (JJ): native/Balanced at unchanged resolution, allow30s
for warm-up/automatic captures, stay still, DLSS drawer -> Compare DLSS transfer
speed. Check final HUD/fusion/image quality; F2/top button saves preceding10s
if motion or ground artifact recurs. Native FPS and image-pose age go to logs.
Then a brief AFW check/capture if desired. Close normally for log preservation.


## A1g - distance detail: physical scene depth and DLSS motion

Installed 2026-10-11 23:34 Sydney; eight checks pass and17 owned/four
protected Asylum files independently verified (A1g-build-checks.json).
Runtime SHA25691D07567A2B34DAC1640A1D492149FA842F3B5045467E3B201F48DFAD164F981.
JJ's highest priority is distant detail/shimmer. AFW still flickers HEADSET;
black ground and slight camera jitter remain unresolved until this new run.

Looked up again: failure-atlas FAIL-PERF-017, chapter19#dlss-in-vr
(registration before blaming missing actor velocity), chapter06 "Measure,
don't theorize", cross-project-index temporal history routes; framework04/06/10;
City DLSS D2 (filtering/FXAA methods, no copied shader identity).
Playbook checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423.

LIVE A1f: Quality1530x1708 ->2284x2448, active NGX, ~2.75ms/eye;
native invalid/tagMiss/baseCameraChanged=0. All FIVE actual depth previews
contain only red255, including nearby visible characters. These are8-bit
images, so they do NOT establish exact raw1 for every pixel. Actual clips
contain no AFW rows (button presses outside AFW interval); do not pretend
they show its flicker. Native ground/moving-character captures and stock/DLSS
detail crops inspected. See A1f-user-detail/capture-audit.json and preserved
game/helper logs. Out-of-order recorded clips independently observed.

STATIC correction to A1d: ghidra-camera.txt lines790 onward uses the RHI
flag to change shader depth-UNPACK parameters at FSceneView+0x620; projection
at+0x90 remains the ctor input. It does not justify flipping the physical
D3D9 DS's projection. A1g camera/NGX/AFW retain the original hardware depth
terms; first-camera logs distinguish physical projection from shader flag.
Actual retail shader/DS convention must still be checked with new raw values
and geometric ZFUNC logs. Old A1d/A1e/A1f inversion claims below are superseded.

SOURCE: hud9 freezes the owned source-sized scene DS before a depth clear or
rebind, retaining the phase with most Z-writing geometry instead of blindly
copying the last bound surface at the end of postprocessing. Eye/generation
freshness, private HUD stencil exclusion and transport fences remain. DLSS
forms inverse(Pcurrent) * inverse(Vcurrent) * Vprevious * Pprevious in DOUBLE,
cancelling large common world coordinates before rounding the GPU matrix.
Both-eye histories and declared jitter remain separate/paired as before.

GPU: real9On12 physical geometry .25 LEFT/.75 RIGHT is deliberately cleared
to1 and rebound before final scene/UI blit; BOTH saved geometry depths survive.
Real NGX DLAA/Quality pair evaluations and missing-RIGHT rollback pass.
Physical-forward-depth motion at world120000,-70000,300000 reads .750000;
GPU vector -0.099670/-0.000000 vs analytical -0.099700/0. Existing forward/
inverted AFW/asymmetric/crop/history checks pass. Reversed-order recorder
input exports chronological CSV/images/cameras; JPEG decoding still passes.
Eight tests total: A1g-ctest.txt. These are regressions, NOT retail acceptance.

Full still captures now add depth_scene.bmp.raw: APD1 magic, width,height,
DXGI_FORMAT as four little-endian uint32, tightly packed original DS rows;
depth_scene.bmp.txt gives exact min/max and clear0/clear1 populations. No8-bit
inference is needed next run. Raw readback happens only on capture request.
XR logs image-pose age and exact-generation hit every5s: queue age includes
engine/GPU work, is not headset latency. Timing/pose delay is NOT blindly shifted.
Next launch: native Quality first at the same hallway/ground; use timed
stock/DLSS comparison and a full still. AFW comparison afterward. DoF/motion
blur are profile-disabled; shader-level absence/reflection refresh unproved.

## A1f ? performance, scene texture detail and temporal stereo capture

Looked up: City DLSS_PLAN D1/D2 (actual mode-sized scene input, measured NGX
cost, mild MIPBIAS and competing AA); Origins BUILD_PLAN O1C (Quality ratio,
AA/DoF baseline); playbook PERF-006/007, chapter19 three ledgers/dlss-in-vr,
chapter09#eye-capture-point, framework04/10, VR_HUD_GUIDE and PACKAGING_GUIDE.
Checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423. No donor shader fingerprints,
engine addresses, reflection ABI or D3D11 hooks are used in the D3D9 target.

HEADSET A1e: medium-distance low detail/blur, poor FPS with Quality; moving
camera causes black ground artifacts. AFW flickers badly in both eyes.
LIVE A1e: source2284x2448, crop1838x2050, NGX2756x3074 ->XR2284x2448.
Game renders at100% before DLSS; NGX alone costs4.36-4.98ms PER EYE.
Late run25-29 completed pairs/s. Native invalid/tagMiss/baseCameraChanged=0.
AFW ~39pairs/s, outer holes53.8%, previous-image contribution61%; neither
failed depth nor camera tickets explains that coverage loss. Logs preserved
before fixture helper reuse in re/evidence/A1e-live-performance/.
The main window initially fit1241x1331, then grew2284x1440 despite logical
2284x2448. Configuration-off DoF/motion blur is established by profile source,
not proof every scripted blur/post-process path is absent.

SOURCE A1f: Quality lowers actual engine input to2/3 desired output divided
by the measured compatibility-lens crop (initial conservative crop hint.8).
Near-square engine aspect is retained. Default Quality request1902x2040 ->
output2284x2448 here; exact observed crop/input remains visible. Other modes
use1/.77/.58/.5 fractions;0.6-1.8 output scale is retained. DLSS mode changes
AA live; next launch applies that mode's engine resolution. NGX output is now
the explicit desired size, eliminating accidental input*1.5 supersampling.
Mild log2(input/output) sampler bias applies only to mipmapped scene textures,
excludes post targets/UI, and preserves original state reads to avoid recursive
bias. Game MSAA is temporarily1; DoF/motion blur remainFalse and configuration
bytes restore on exit. No claim of NGX optimal-settings query or complete
object velocities. Native DLSS-off no longer prepares unused DLSS input;
AFW skips the unused stock-LEFT rollback copy. The physical window constrains
position AFTER the game's window procedure, including unhooked positioning.
LEFT remains acquired until RIGHT completes, making paired fallback legal.

AFW shader defect: target camera LIVE has inverted depth P22=.001/P32=9.99.
Cleared/beyond-far depth unprojects behind the camera in the old splat and is
discarded. Normalize it to the projection's far asymptote (a direction at
infinity). This is a proven SOURCE/GPU defect; attributing all retail flicker
or54% holes to it still needs the next runtime/headset check. Relative view/
projection transforms are composed in double precision before world-origin
cancellation is lost. Previous real-eye history keeps view/projection parts.
DLSS motion uses the same far interpretation, while its NGX depth remains raw.

Capture is final submitted scene+HUD AFTER DLSS/AFW, BEFORE compositor
reprojection; it excludes the separate settings-panel layer. Top panel button
"Save last10 seconds (stereo)" / foreground-game F2 exports history BEFORE the
press. Pair generation/mode must match, duplicate eyes are rejected. AFW
records both drawn and synthesized images, with flat-fallback metadata.
Runtime images need no SRV bind: an owned same-format-family copy is downsampled
first. Three GPU EVENT/staging slots; GetData DONOTFLUSH and Map DO_NOT_WAIT;
no GPU/worker wait in the game lane. Busy work drops samples, never queues XR.
Up to10paired frames/s at960px per eye, JPEG94, compressed ring64MiB cap and
bounded2-frame encoder queue; actual duration/drop count shown. Exported local
review.html uses recorded frame intervals; JPEG sequence, CSV, camera records
and README remain reviewable without codecs/dependencies. Full-XR-resolution
still capture remains separate. Timed stock/DLSS and native/AFW comparisons
capture every step automatically and restore settings; completed-frame FPS
replaces the misleading panel-update FPS. This is not a full-rate/lossless MP4.

GPU/OFFLINE: eight checks pass. AFW retail-style cropped/padded depth and large
moving world origin,12 alternating frames: corrected sky missing0/180 vs old
control180/180. Existing asymmetric/forward/inverted-Z expectations still pass.
DLSS preserves raw sky depth0 and produces0/0 translation motion; real NGX
DLAA/Quality eyes and AFW histories, motion/jitter and paired fallback pass.
Replay: actual NO-SRV typeless GPU images, matching native and RIGHT-first AFW
pairs, generation rejection, JPEG decoder LEFT red/RIGHT blue, three-frame
export (last LEFT green) and replay JS syntax checked. D3D9 sampling verifies
original state reads/no bias accumulation/UI restore. Logical source/WM_SIZE
and direct positioning survive a fixture window proc that re-enlarges it.

DEPLOYED22:48 Sydney: A1f,17 owned hashes verified; four CURRENT Asylum
protected EXE/configs unchanged. Origins settings also unchanged; its DLL was
independently updated22:47 BEFORE our install and is recorded as an external
observation, not rolled back. Backup re/backups/install-20261011_224802_083.
DLL SHA256 AFA335577B04D3DCC3940DFDBC8C7405A2F0C21E5A811CF2CA9FA1F44E7FE1BC.
Receipts: A1f-build-checks.json, A1f-ctest.txt, A1f-dlss-gpu.log,
A1f-history-fixture/review.html and installed-build.json.
No agent game launch/kill. Retail replay duration/cost, FPS/texture acceptance,
AFW flicker resolution and black-ground attribution remain pending JJ's launch.
Reflections cached on game time remain a lead (City PERCAPTURE/Origins O1I),
not an independently identified Asylum seam; capture the artifact first.

## A1e - full-size source, City/Origins panel, asymmetric FOV and AFW

Looked up: BUILD_PLAN A1e lists FAIL-PERF-020, STR-012/013/014/015,
playbook19#composed-reprojection and matched A/B at checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423; framework06 render size/10 histories,
VR_ESSENTIALS3/4/6/9, VR_HUD_GUIDE, City window.cpp/AFW section11/source and
Origins scene.cpp panel drawers. Microsoft D3DPRESENT_PARAMETERS and Present
documentation confirms explicit windowed backbuffer size may differ from the
physical client and a null destination fills that client:
https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dpresent-parameters
https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-present
No City engine offsets, resize ABI or clamp instruction edits were transferred.

HEADSET A1d: JJ reports low resolution and blur; explicitly requests BOTH AFW
and asymmetric FOV. LIVE A1d now confirms real retail Quality with7034
evaluations, zero fallback/noDepth/timeouts. Source1238x1328, cropped input
996x1112, DLSS1494x1668, XR2284x2448 explain lost detail. Switching to direct
asymmetric uses1238x1328 ->1856x1992; tower acceptance remains unreported.
Preserved A1d-headset-run.log and A1d-retail-dlss.log before GPU checks.

SOURCE A1e: launcher uses the runtime recommendation times a saved0.6-1.8
scale (default1.00), independently of desktop height. Game-only USER32 import
overrides provide a logical client of the requested render size and expanded
monitor limits. CreateWindowExW/SetWindowPos/MoveWindow keep the REAL window
visible within the primary desktop. Main-window WM_SIZE also reports the logical
size to the game. Other DLL/runtime/mod queries retain the physical client.
Target imports were enumerated from the hash-guarded Asylum PE. SplashScreenClass
is excluded. This differs from City's oversized-window/clamp patch route.
Actual GPU source size is independently observed and displayed beside requested
logical/client sizes; a mismatch is reported instead of claiming a higher size.
Retail scene rect/depth target coverage still needs the next launch's logs.

Panel uses City/Origins drawers: VR RESOLUTION first/default-open, STEREO
RENDERING, VIEW, HUD, MENUS AND SCREENS, DLSS (SMOOTHER EDGES), FRAME RATE and
Diagnostics. Resolution applies NEXT launch. Native always starts selected.
Both FOV choices retain the exact runtime asymmetric lenses: direct engine
projection or centred lighting enclosure plus exact remap. Lens choice, HUD
distance/size and DLSS mode persist in aa_rendering.ini; AFW is a deliberate
live selection, with its warp mode saved. DLSS output now keeps the chosen mode
ratio, bounded8192, before final XR resampling; it no longer caps temporal output
to XR size and forces a sub-1 ratio when the input is supersampled.

AFW adapts City's two-pass depth splat/gather and current/previous/combined hole
fill with edge feathering. Asylum supplies its own validated camera/ticket,
hardware depth convention and centred-enclosure crop. One eye is drawn per
simulation frame; the other is synthesized from that SAME frame and head sample.
RIGHT retains independent engine history; jitter advances once per two frames;
DLSS accepts alternating single-eye generations with independent histories.
This is not stale-image AER. Mode transitions use the frame's AFW ticket, not
the current UI switch. Bad tickets/depth yield an explicitly counted mono
fallback, never an old eye. AFW history reset is queued to the render thread.

HUD is omitted from AFW's world input. An Asylum-only post-synthesis hook adds
the same isolated sheet through BOTH real eye lenses, including mono fallback,
on one upright room plane. Native uses the same extracted compositor. Captures
save final XR eyes even when source/XR dimensions match; AFW captures occur after
both HUDs. "Compare native and AFW" times/captures native ->AFW ->native and
restores settings in about20s; tower comparison remains under Diagnostics.

OFFLINE A1e: all eight final Release checks pass. Actual9On12 backbuffer and
GPU transfer2284x2448 succeed with a much smaller client; production logical
GetClientRect, window fitting and WM_SIZE forwarding are exercised. Real
depth-based AFW pixels match independent expectations in BOTH directions with
different asymmetric lens centres, forward and inverted-Z. Stale pass/missing
depth are rejected. Actual NGX DLAA/Quality native pairs and four alternating
AFW single-eye generations evaluate successfully. Production HUD extraction,
stencil/private-target preservation, upright projection and native ticket/ABI
checks still pass. These are GPU/static evidence, not retail headset acceptance.

DEPLOYED21:56 Sydney:17 owned files verified, six CURRENT protected files
unchanged. DLL33610DB1FD6FBB6E6D52F5D7026C82339F84966D18D2789BBFA87231AD912453.
Backup re/backups/install-20261011_215616_960; A1e-before-install.json,
A1e-ctest.txt, A1e-dlss-gpu.log/A1e-build-checks.json and installed-build.json.
No agent game launch/kill; Origins/City remain read-only. Next JJ run must
confirm full retail scene/source/depth sizes, sharpness, FOV/head movement,
AFW HUD fusion/artifacts and actual pacing. Camera-only DLSS motion, transparent
AFW surfaces/reflections/disocclusions and higher GPU/memory cost remain limits.
Game-specific markers, reflection refresh, DoF shader policy, once-only UI CPU
work and full City/Origins feature parity remain separate work.

## A1d - tower lighting compatibility and real per-eye DLSS

Looked up: playbook docs/14-render-pass-hazard-atlas.md, "A stereo-only artifact
is infrastructure until proven content" and "Shadow cascades: one eye is the
authority, the other reuses"; docs/10-graphics-apis.md, "Projection companions
are an OpenGL problem too" and validate-every-upload / FAIL-STR-030;
docs/19-d3d12-and-performance.md#dlss-in-vr. Checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Framework guides04 (native matrices),
14 (shadows) and10 (temporal history), VR_ESSENTIALS sections2/3/6/9/10,
VR_HUD_GUIDE, AKN src/probe.cpp turned-eye rationale, and City dlss_bridge.cpp
camera motion/shared-helper transport. Origins/City shadow/depth notes supply
investigation leads, not an Asylum shader/offset fix. BioShock Infinite notes
at frozen41f004bc6a1e4fa8be0d0e0749f9ac5a0453e75e supply render-root methodology
only. Microsoft D3D11On12 resource rules distinguish ordinary unwrapped D3D9
surfaces from separately allocated keyed-shared D3D11 textures. No upstream
reference checkout or Origins files were changed.

HEADSET A1c: JJ reports an unusual shadow on the right-eye tower in the menu.
LIVE preserved in re/evidence/A1c-headset-run.log: owning render-thread startup
stop succeeds after gamepad cancellation; native completes3784 pairs with zero
invalid/tagMiss/baseCameraChanged. Actual UI boundary/capture/RIGHT skipping
advance (7569 boundaries,235453 captured draws,235374 skipped RIGHT draws,
zero unsupported). This establishes retail interception, not headset HUD fusion
or movie-height acceptance. Saved A1c_20261011_190717_335 scene/eye pictures show
the tower artifact already in the HUD-free game image; a HUD overlay change
cannot repair the underlying scene shading. Image observations do not identify
a specific shadow shader or prove all apparent duplicate details are bugs.

STATIC: re/evidence/ghidra-A1d-matrix.txt is a hash-guarded read-only headless
decompile of this Asylum executable. Its FSceneView constructor recomputes full
inverse matrices; AA_CANDIDATE_6e1510 is a general inverse, not a centred-only
shortcut. Missing inverse-companion updates were ruled out at this seam. The
constructor's device-depth conversion honors this target's RHI inverted-Z flag
at RVA022b21f0; the camera ticket uses the corresponding hardware-depth matrix.
No foreign-game shader/register or ABI is assumed to transfer.

SOURCE correction: default native rendering encloses each XR lens in a centred
engine frustum (m20/m21 zero), retaining actual eye poses and depth terms. The
GPU crops that enclosure through the exact asymmetric XR lens. This addresses
legacy screen-space lighting that assumes a centred projection, without turning
the eye cameras, stretching an axis, reducing XR FOV or suppressing shadows.
This is a compatibility candidate; shared shadow/depth producers remain possible
if the A/B does not remove the artifact. "Lighting compatibility" and commands
aa lens centred/asymmetric retain a reversible comparison. "Compare tower
shading" automatically captures centred stock AA, original asymmetric stock AA,
then centred with the saved DLSS choice, restoring settings afterward.

SOURCE DLSS: hud9 identifies depth on source-sized Z-enabled/Z-writing scene
draws and snapshots it before the final scene-to-backbuffer UI boundary. Depth
eye/generation must match the native camera ticket. D24S8/D32/D16 non-MSAA depth
is copied through fenced9On12/11On12 into a readable surface. Separate keyed
D3D11 color/depth/motion/output textures hand actual pixels to Asylum's own x64
NGX helper on the matching adapter. Current/previous per-eye matrices reconstruct
camera motion; both eyes share one32-phase Halton jitter sample and retain
independent histories. Cropping preserves declared input-pixel jitter. Mode,
recenter, cut, resize, untracked Present and failed pairs invalidate histories.
Completion, evaluatedMode and both eyes are checked; a RIGHT failure restores
the saved stock LEFT plus stock RIGHT before pair submission. HUD is composited
after upscaling. Quality is requested by default; aa_rendering.ini persists
the chosen mode. A requested mode is not advertised as an active evaluation.

OFFLINE GPU A1d: all eight Release checks pass. The production remap is checked
against independent lens rays. Real D3D9 D24S8 geometry supplies .25 LEFT/.75
RIGHT depth. The actual x86 client and x64 NGX helper evaluate BOTH eyes in
DLAA64->64 and Quality64->96. Readback confirms depth .750000, camera-motion
X -10.0391 pixels versus expected -10.0400, Y zero despite jitter, and a missing
RIGHT depth rejects/rolls back the pair. See A1d-ctest.txt and A1d-dlss-gpu.log.
The fixture starts only its own helper, never the game. No retail DLSS evaluation
or headset shadow improvement has yet been observed on A1d.

LIMITS: camera motion only; animated characters can smear. The1238x1328
desktop-fit source is retained; the enclosing-frustum crop further reduces
actual per-eye DLSS input. Output uses the selected mode's provisional ratio,
capped to XR size, then is resampled into2284x2448. Panel/captures report actual
input/output dimensions; this is not full2284x2448 DLSS Quality or an NGX optimum
resolution claim. Stock AA is retained. Retail depth coverage/convention,
shadow comparison, image quality, pacing/endurance, markers/fades/reflections,
once-only UI CPU work and AFW remain acceptance gates.

## A1c - actual HUD capture, eye-level movie, owning-thread skip

Looked up: VR_HUD_GUIDE sections1/3/4/6/8; playbook docs/04-ui-and-hud.md
#single-native-hud-draw and #ui-not-one-class, failure-atlas FAIL-HUD-014
(private filter passes and doubled UI), FAIL-HUD-007 (fullscreen UI is not one
class), checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Framework guide12
sections1/2 for XInput polling; guide10 for separate temporal history. Origins
RE_NOTES O1C supplies a scene-blit/UI boundary lead; BioShock Infinite's
render-root chain at41f004bc6a1e4fa8be0d0e0749f9ac5a0453e75e is a method
reference only. Asylum's D3D9 classifier is independently exercised below.
DLSS prerequisites/routes are recorded in DLSS_PLAN.md.

HEADSET A1b: JJ confirms scene stereo; title/menu/gameplay HUD cannot fuse,
opening video is too low and the pad cannot cancel it. LIVE preserved in
re/evidence/A1b-headset-run.log: source and Windows client both1238x1328;
native pairs exceed5200 with zero invalid/tagMiss/baseCameraChanged. Canvas
edits increment, but title text is at identical source pixels in BOTH composed
eye captures (A1b_20261011_181856_565). The FCanvas hook therefore did not reach
the visible UI lane. Hook activity and algebra were not engine coverage proof.
The obsolete runtime FCanvas transform hook has been removed.

SOURCE A1c: hud9 hooks only the owned 9On12 device. On the main backbuffer,
a two-triangle, no-depth/no-blend full-size render-target blit establishes the
scene/UI boundary. Subsequent LEFT backbuffer UI draws go to a transparent
sheet with its own cleared D24S8 stencil. Colour blending/mask-only writes are
preserved and alpha coverage accumulated separately; private render targets
retain their original draws. RIGHT reuses the LEFT sheet only for a matching
generation with a completed capture; unsupported extraction retains normal
draws. Ordered GPU 9On12/11On12 transport returns ownership through a fence.
The scene stays HUD-free; one upright eye-level4m world plane is projected
through each actual asymmetric eye lens into the full XR images. A premultiplied
blend preserves the world around the HUD, including additive RGB at zero alpha.
Recentre reanchors it; the desktop HUD is restored after XR capture. Captures
now include hud_sheet.bmp as well as scene and composed eyes. Bounded boundary,
draw/skip/failure logs and panel counters distinguish extraction from intent.

SOURCE/OFFLINE movie: the yaw-only room anchor is retained, but its centre is
now the viewer's eye-level forward axis instead of the asymmetric frustum's
bounding-box centre (which shifted it down). Fill extents cover the frustum
about that centre; binocular Fit still includes the whole video. Actual GPU
projection checks upright edges, eye-level centre, fixed world corners under
head translation/rotation and recenter. This does not prove headset placement.

STATIC startup: existing tick6d1cf0 recognises the Unicode engine opening list,
already observed LIVE in A1b. Guarded render stop6cca00 takes one word/ret4;
force1 calls6cb200 with advance0, whose guarded cleanup skips startup-list
advancement. The new tick chooses the engine's own owning-thread predicates
IsInGameThread715880 / IsInRenderingThread715850 after the original tick,
and verifies unchanged handle/index before one native stop. Game-thread stop
still uses its own three-word ABI. Dedicated pad polling reads unfiltered
XInput using the existing thread-local menu-poll bypass, consumes the skip
press through release, and logs request plus actual owning-thread stop.
Only known opening entries qualify; story/loading movies are excluded.
Target instruction/ABI checks do not prove live controller cancellation.

OFFLINE A1c: the production GPU fixture invokes the real hooked D3D9 draw
path: stencil mask + half-alpha text, private-target effect, LEFT capture,
RIGHT reuse, fenced transfer, exact transparent-sheet pixels and both
asymmetric-eye projected blends without clearing the world. It also verifies
desktop HUD mirroring/state restoration and reset. All eight Release checks
pass. This is GPU/STATIC evidence, not a retail classifier or headset result.

Open gates: actual menu/gameplay capture boundary and headset fusion; startup
cancellation; world markers at object depth; fullscreen fades/transition
classification (do not assume a shader identity separates them); once-only UI
simulation and long re-entry soak; reflections. The UI raster is reused, but
engine UI CPU work may still run twice. AFW and active DLSS remain pending.
Native scene/history ownership and the desktop-fit source are retained.

Read ASYLUM_PLAN.md for the target hash and reference routes. Playbook RE-01 and
PORT-00/01/02, chapter 10 ownership/FPU routes and chapter 06 "Crash-dump
workflow", checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423.

## Draw route — static executable and one live viewport type

| Address / field | Evidence | Boundary |
|---|---|---|
| 0x00ccae50 | Uses command B Begin/End, viewport+0x20 client and client vtable+8 Draw; decompiled ordinary FViewport::Draw candidate | Game-thread caller and native stereo behavior not yet observed |
| 0x00ccae50 entry / 0x00ccb472 return | SEH prologue, no incoming stack argument inferred, bare `ret` at exit | Different ABI from Origins' viewport draw; do not reuse its extra stack argument |
| 0x006a9310 | `mov ecx,[esp+4]; call 0x00ccae50; ret 4` | Confirms ECX receiver dispatch for this call; wrapper's own one stack argument is not the callee ABI |
| viewport+0x20 client | Client virtual slot+8 receives viewport and local canvas | Need the live client's object/vtable and Draw entry before following CalcSceneView |
| command B 0x00c96cd0 | Loaded 16-byte signature matched; runs in JJ's A0 desktop fallback, returns original command size 8 | Queued render-thread stack cannot recover the enqueue-site caller |
| viewport vtable 0x0212c380 | LIVE command B logged RVA 0x01d2c380 | Object address was transient; later external read failed and yielded no field evidence |
| vtable+0x54 -> 0x00cc29c0 | Static contents of the observed live vtable, command executor's called slot | Viewport begin method, not scene constructor or game-thread Draw |
| 0x00cc2310 / 0x00cc2ab0 | Tiled screenshot / capture-readback roots | Do not select them as ordinary gameplay draw roots |

`ordinary-draw-call-leads.json` lists bounded direct relative-call matches and
padding-derived parent candidates. It includes the tiny receiver wrapper above
and calls from gameplay/editor-like functions; matches are leads, not live caller
proof. Ghidra's isolated focused database supplies control-flow evidence only.

Next: establish stable game Present/XR delivery with A0a, identify the live
viewport client and Draw dispatch, then trace its Asylum CalcSceneView and view
construction arguments. Render-thread command observation alone does not prove
that drawing twice is safe or gives one simulation step. Native implementation
must own both eye views/history, preserve one located XR pose per pair and gate
submission on both completed images. Projection and shader reconstruction
companions need Asylum-specific evidence. Never copy donor offsets/layouts.

## First launch crash and correction

A0 initialized VirtualDesktopXR, but its global CreateDevice detour recursed
after two system/9On12 factories. A matched local crash dump contains 8,699
returns into the detour (DLL+0x2f933), then faults in CRT stack probing
(DLL+0xb67d7). Original trampoline memory was not included in the dump, so its
exact internal branch target is unknown. A second A0 launch failed to hook the
factory and fell back desktop; JJ confirmed no headset or F10/controller panel.

A0a uses per-factory IDirect3D9 COM forwarding with original API arguments,
HRESULTs and independent ownership. Its fixture creates real GPU devices through
two wrapped factories and checks COM identity, managed textures, eight exact
BGRA-to-RGBA transfers, resource return and reset/resize. Three CTests pass; A0a
is installed and hash-verified. Game/headset result remains pending.

## A1 - target-specific camera and movie implementation (STATIC / SOURCE)

A0b LIVE command B supplied client vtable 0x023a5f88, Draw slot+8
0x008a0230. Focused decompilation follows this exact client, not donor offsets.

| Seam | Target ABI/evidence | Implementation |
|---|---|---|
| FViewport::Draw 0x00ccae50 | ECX receiver, no stack words, bare ret 0xccb472; ordinary branch emits command B | outer renderer only, fixed viewport/client vtables; automatically learn the owner from its first validated main camera |
| CalcSceneView 0x0089e620 | ret 14h at 0x89f090; family/location/rotation/viewport/player-index | scope only the current main viewport |
| FSceneView ctor 0x00aecc60 | ret 60h at 0xaed7d2, 24 words; main call returns to 0x89f017; matrix words 15/16, rect words 11..14 | inject complete per-eye poses and asymmetric frusta; preserve engine near/far; reject non-perspective inputs |
| View state 0x00af25a0 | ULocalPlayer ctor calls at 0x89f868 and writes result to +0x84 | allocate independent right-eye history (eight bounded identities); no donor allocator |
| command B 0x00c96cd0 | LIVE own viewport +4 | render-thread eye/generation labels; separate completed-camera verdict checked at Present to handle Begin executing before construction |
| Bink player tick 0x006d1cf0 | vtable 0x01ecff08 slot+4, one float, ret4 | identify actual known startup Bink handle and startup index; poll/consume pad button, run original tick once |
| Bink stop 0x006d1fa0 | same interface slot+1c; ret0c (delay, wait, force); game calls (0,0,1) at ordinary viewport exit | issue native non-waiting force-stop only on engine-validated game thread; render queue stops current and remaining startup sequence |
| IsInGameThread 0x00715880 | compares GetCurrentThreadId with +0x26760e4 when initialized | guards movie-stop call; poll thread never touches engine state |

Instruction/hash guards run before native/movietick hooks. Existing frozen
Origins source is a math/queue reference only. Matrices are 16-byte aligned.
One located generation feeds both eyes; native intent defaults ON, first
validated ordinary main-view caller activates it without a manual checkbox.
Both renders reach the GPU bridge; first desktop Present is suppressed.
Screen-only/no-view intervals use a LOCAL distant plane anchored once after
FOCUSED; Fit default preserves all source content inside both lenses.

A1 is an initial native integration: source inspection identifies a Draw route
outside engine Tick, and the code rejects any changed base view/projection/rect
between eye passes. This check does NOT prove simulation/audio/FX/UI side
effects absent. The client draw still includes UI; HUD isolation and once-only
UI work remain pending. Re-entry endurance, view-state lifetime across maps,
reflections and all post-processing still need game/headset evidence.
Do not label the static ABI fixture or GPU fixture as headset validation.

References: framework guides 08 section3/4 and09 basis composition; playbook17
#side-effect-gate / #reentry-endurance, RE-01; checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Evidence: A0b-headset-run.log,
ghidra-client-draw.txt, ghidra-camera.txt, abi-instructions.txt, constructors.txt,
ghidra-movie-stop.txt, tests/asylum_abi_check.py.
# A1b - extents, upright screens, startup identity and canvas fusion (2026-10-11)

Looked up: VR_HUD_GUIDE sections3/4/6/8 (shared room plane through each actual
lens, live menu world, upright distant movies); playbook chapter09 five-minute
alignment differential (head-motion stretch and constant angular separation),
a3-stereo-projection sections1/2 (four-angle projection and independent rays),
STR-002/STR-004 pair publication/provenance. Playbook checkout
b955f2244f7c5d5f3b3e5f403b1b6721a3362423. No upstream edits.

HEADSET A1a: JJ sees two eyes but cannot fuse them; head motion stretches/shears.
Opening video must be vertical in world space and gamepad-skippable.
LIVE A1a: preserved A1a-headset-run.log shows over4500 completed pairs, zero
tagMiss/invalid/baseCameraChanged/failure. Recommendations/backbuffer2284x2448;
read-only GetClientRect on PID35660 measured2284x1440 (outer2290x1469), desktop
2560x1440/workarea2560x1392. Paired174156_755 captures show repeated/stretched
lower scene starting around the client height. This supports a window-derived
scene extent mismatch, not an eye-ticket failure; correction needs headset proof.

SOURCE: launcher now fits both dimensions inside primary workarea with64px
border allowance, retaining near-square eye aspect. Here that is1238x1328
source versus2284x2448 XR output, uniformly upscaled. Source detail is lower;
this is NOT full recommended-size scene rendering. Render/output are separately
journalled and config byte rollback retained. First source/client/rect logs
establish actual extents. OFFLINE: fit sizes/aspects verified across desktops.

SOURCE/OFFLINE: RoomPlane uses placement yaw only; sides stay parallel world-up
even when placed under head pitch/roll. Corners remain anchored until recenter.
Actual GPU fixture checks upright edges, translated/pitched/yawed/rolled lens
coverage and binocular Fit through the runtime preview shader.

STATIC: GameThreadPlayMovie006cfcd0 calls BinkOpen with memory flag04104400,
so first argument need not be a filename. Startup advancement006d0c90 uses
player+70 TArray, count+74, index+7c, stride1Ch with Unicode FString first.
SOURCE: tick recognises exact known opening entries including extensionless
names independently of ANSI Bink filename recognition. Force-stop still needs
unchanged handle/index and target IsInGameThread, uses(0,0,1) and consumes the
face/Start/Back press until release. Story/loading entries excluded. Name tests
do not prove live cancellation. Evidence: ghidra-A1b-movie*.txt,
ghidra-A1b-startup-open.txt and re/movie_refs.py.

STATIC: FCanvas ctor00aaff50 has two stack words/ret8 at00ab0092, target+4,
transform TArray+24, first80-byte entry with64-byte matrix and CRC+40.
Call00ab0054 invokes cdecl CRC00627f70(ptr,64,0). SOURCE: only the main
viewport canvas during a native eye is transformed: original pixel layout ->
shared upright4m plane -> relative eye pose -> actual asymmetric lens, with
target CRC recomputed. The native3D scene route remains. No donor offsets.
OFFLINE: shared scene/canvas math matches independent XR rays for corners and
centre under translation and combined yaw/pitch/roll; scene depth preserved.
ABI fixture checks target ctor/CRC instructions. Engine canvas coverage,
material tiles, HUD markers, fades and once-only UI work remain live/headset
gates. canvasEdits and bounded first-eye logs reveal actual hook activity.

BioShock Infinite (BioShock3) is the relevant UE3 reference; the first two
remasters use a Vengeance branch. Frozen reference checkout
41f004bc6a1e4fa8be0d0e0749f9ac5a0453e75e:
references/bioshock-vr-latest/docs/bioshockinfinite/ENGINE_NOTES.md,
render-root chain around2899.
Viewport replay includes Present; client-only replay loses eye provenance;
compare cached base cameras across passes. These methods support Asylum's
measured outer draw route. Infinite's DX11 branch/argument layout do not prove
Asylum's D3D9 ABI or exact UE3 revision. Actual submission FOV now logs after
the asymmetric override; inherited symmetric-baseline telemetry was misleading
as a stereo-tag report, not evidence that stereo submission was symmetric.

Eight checks pass on A1b Release. Live/headset acceptance remains pending;
native side effects/endurance, reflections, AFW and active per-eye DLSS open.

# A1a - menu native activation and consumed Present tickets (2026-10-11)

HEADSET: JJ reports no stereo in the menu despite native being selected.
LIVE evidence: `re/evidence/A1-headset-run.log` preserves the 17:22:50 run.
Owner RVA003f9d4b is learned at 17:22:55 while command B is still zero;
ordinary rendering starts 17:23:19, but queuedPairs/edits remain zero through
17:23:39. One native pair reaches the bridge at 17:23:44, followed by the
permanent "right Present has no matching left generation" fault 154 ms later.
The title capture contains a real Batman/moon/window scene, so the intended
route is stereo scene rendering under menu UI, not a permanently flat menu.

SOURCE: owner learning incorrectly accepted CalcSceneView from the non-render
prepass. Present reused `haveRenderSlot`/right eye after that ticket's image
was consumed; additional movie/loading Presents could fault a completed pair.
STATIC: VA007d6c86 tail-jumps to FViewport::Draw; VA007f9d46 calls that wrapper
(return RVA003f9d4b). Direct ordinary call VA007fe8da returns RVA003fe8df.
These are leads, not a substitute for the live ordinary caller census.

A1a restricts learning/invalidation to the ordinary branch with a render
surface (viewport+44), validates the main ctor, and consumes each BeginDrawing
ticket once. The render-thread sequence still rejects repeated fresh left,
missing fresh left, and cross-generation right. Failed bridge copies/reset
discard the ticket and partial pair; desktop Present is not suppressed on a
failed capture. Eye/generation publication occurs once at the copied Present.
The panel/capture reports actual rendering rather than desired selection;
first native automatic capture starts on LEFT to save one pair's eye images.

References applied: playbook FAIL-RE-020 / chapter11 #shared-callee-inference,
STR-004 pass provenance, STR-002 / chapter09 #the-unit-of-publication-is-the-pair-not-the-eye,
checkout b955f2244f7c5d5f3b3e5f403b1b6721a3362423. Framework guide08 sections1/3
and VR_HUD_GUIDE sections6/8: user scope chooses same-frame native stereo;
the guide's generic AFR recommendation is not adopted. Existing target-specific
0/5/24 ABIs and history allocator remain guarded. This fix does not prove
once-only UI side effects, headset fusion, shaders/reflections or endurance.



## FIRSTPERSON (STATIC 2026-10-20)
Pawn / head-bone offsets for first person, found by City's code shape: table and evidence in BUILD_PLAN "FIRSTPERSON"; code comment at the top of src/game/asylum/first_person.cpp.

## HEAD-AIM (fork development)

Reuses the Asylum Controller.Rotation +0x60 layout documented above and in
BUILD_PLAN FIRSTPERSON, with the existing live pawn +0x1B4 controller backlink.
No new native seam or donor address was inferred. Writes run on the game thread
after the main-view matrix and first-person distance gates, once per draw pair.
The previous owned yaw/pitch contribution is removed before adding the current
head orientation; new stick/engine deltas remain. The render view uses that body
basis and the original full per-eye XR transform once. Both eyes share the cached
first-person view. Readback and restoration failures are counted.

The HUD uses one head-centred plane built from the two exact-generation eye
poses. Detective callout projection reconstructs that moving plane with its
own game-pass generation instead of using old compositor corners. Target markers
retain their world positions. Third-person and disabled head-aim paths retain
the existing room plane.

CPU math evidence is in tests/head_aim_check.cpp. Controller readback is not
evidence of gadget consumption: simulation may use the previous tick's rotation.
Pause/cinematic gating and actual target/impact correspondence remain retail
checks. See HEAD_AIM_TESTING.md for the grouped session and log fields.

## HEAD-AIM2 (retail gate failure)

User-supplied HEAD-AIM status: native paired scene, enabled/requested=1,
active=0, writes=0, reason=not gameplay, fpFrames=14354, headFrames=7177,
headMisses=0. Particle unknownStock=54338 (menuStock=0) confirms the shared
menu-state observation was unknown, not positive menu evidence. All 256 trace
samples were inactive (249 not-gameplay, 7 native-pose-unavailable). Thus the
new direction write and head HUD were never exercised; the first-person pawn
and camera path was already running. This is a gate defect in HEAD-AIM.

Keep the menu reader's existing offsets/validation and record its failure
stage/count/capacity. Only head aim gains a fallback to the established native
first-person guards: valid controlled pawn/controller reciprocal relationship,
finite eye position, validated main camera within the existing 12m limit.
Known menu, disabled settings, absent native pose and VR input ownership still
deny writes. Unknown remains unknown in diagnostics and other consumers.
The portable production gate regression covers this captured failure and the
remaining denials. Retail gadget consumption and unrecognized menus/cinematics
remain unverified; HEAD-AIM2 is the first build able to reach the drive on this
reported setup.


## HEAD-AIM3 (walking pitch feedback)

User confirms HEAD-AIM2 head aim works, reports reversed pitch only while
walking. New log confirms active writes/readbacks with zero failures; attached
status/CSV are from old HEAD-AIM (writes=0), so no per-frame moving trace is
available. Body pitch varies even with sampled RX/RY=0. Auto-centering of the
injected Controller pitch is a plausible feedback source, not a proven native
call-site identification. Preserve committed body pitch without deliberate
vertical stick input; continue accepting yaw deltas. Regression simulates the
engine zeroing pitch for 300 frames with head +45/-45, then an additive stick
change. No new offsets/hooks into the game executable.

Final XInput axis callback covers real/composed pad, excludes nested calls and
overlay polling. Asylum installs it to observe actual delivered axes and apply
optional vertical-only block (persistent, off by default); valid first-person
publication expires after 250ms. Trace includes movement, raw/delivered pitch,
and blocking/input decisions. Mouse pitch unsupported by stabilized policy;
engine pitch adjustments during deliberate stick pitch remain inseparable.
Retail walking/stereo/gadget behavior awaits user test, separate from CPU tests.


## HEAD-AIM4 (complete stereo pose publication)

User accepts HEAD-AIM3 walking/head aim, requests automatic stick pitch lock
and removal of its separate checkbox. Lock now follows FirstPersonLookAim;
legacy FirstPersonBlockStickPitch is ignored. Head-aim body pitch stays fixed
regardless of stick input; game-facing right Y is zeroed in the valid native
first-person context. Head aim off restores stock vertical input, yaw unchanged.

Latest supplied HEAD-AIM3 log has 57 native-pose-unavailable transitions, no
native matrix/tag/base-camera faults. CSV: 251 active samples and 5 unavailable.
User sees 1-2 flat third-person frames in both eyes, often in smoke. SOURCE:
xrLocateSpace publishes head/id before xrLocateViews populates the generation
ring. Native draw reads head then that ring using separate locks: scheduling
between publications returns false and eye=0, so first person is skipped and
the bridge receives the stock flat frame. Smoke causality itself is unproven.

Publish a complete StereoPoseMailbox after both locate calls and pose-ring
publication; native draw copies head, eyes, FOV and generation under ONE mutex.
The writer does not hold the mutex over OpenXR calls. The previous complete
sample remains readable during locate, as with any latest-sample API; invalid
tracking at completion and teardown explicitly invalidate it. No held images,
unbounded last-valid tracking fallback, arbitrary particle changes or new RE
seams. Legacy head/eye APIs retained for other consumers.

Portable regression deterministically pauses the writer after new head locate
but before eyes; reader gets complete previous generation, then new generation.
Tests invalid tracking, teardown/recovery and 100000 concurrent publications.
Native log adds coherentPoseUnavailable/flatDraws plus bounded fallback reasons.
Windows/CPU validation is separate from smoke/headset confirmation.


## HEAD-AIM5 (world scale)

User accepts HEAD-AIM4 flash fix. Add World scale (0.5..2, default 1) and reset
at top of F10, saved as AsylumVR/WorldScale. Larger means larger apparent world:
units/metre = 90 / scale. Snapshot once per draw pair for all eye transforms,
AFW, marker eye matrices and particle centre deltas. Marker HUD intersections
convert world points with per-pass units; edge pointers retain physical depth.
Keep avatar bone attachment/forward offset and pawn-camera distance safety gate
in original game units, independent of visual world size. HUD stays XR metres.
Reset DLSS/AFW history when the first rendered camera of the new scale arrives,
not while queued older cameras are still being consumed. Camera carries units.
CPU scene projection test checks physical binocular disparity and translation
at four scales, unchanged FOV/rotation, default and invalid value bounds.
Actual scale/markers/AFW perception needs headset confirmation.
WORLDSCALEDIR (merge into 0.1.5, JJ 2026-10-25): direction flipped to match the other Arkham mods - units/metre = 90 * scale,
higher = smaller-looking world (BUILD_PLAN "HEADAIM").
