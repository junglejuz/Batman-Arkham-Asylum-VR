# Batman Asylum VR - development status (newest first)

README.md is the player page since release 0.1.0; these are the development checkpoints it used to hold.

CURRENT 0.1.5 RELEASED (2026-10-21): PROFILECLEAN + JOKERCOMPAT; BUILD_PLAN "RELEASE 0.1.5".

CURRENT PROFILECLEAN + JOKERCOMPAT (2026-10-21, in 0.1.5): launcher deletes its settings backup folders after a good restore; Joker DLC Patcher exe accepted (Godoy301). BUILD_PLAN "PROFILECLEAN + JOKERCOMPAT".

CURRENT 0.1.3 RELEASED (2026-10-20): FIRSTPERSON + WRITEDENIED; BUILD_PLAN "RELEASE 0.1.3".

CURRENT FIRSTPERSON (2026-10-20, in 0.1.3): City's first-person mode (panel / double-click both sticks). HEADSET pending; BUILD_PLAN "FIRSTPERSON".

CURRENT WRITEDENIED (2026-10-20, in 0.1.3): launcher no longer gets stuck when Windows refuses a Documents config write. BUILD_PLAN "WRITEDENIED".

CURRENT 0.1.2 RELEASED (2026-10-17): Pimax menu/HUD double vision - VD-only per-eye quad shift no longer applied on other runtimes. BUILD_PLAN "RELEASE 0.1.2".

CURRENT 0.1.1 RELEASED (2026-10-17): Pimax fixes - DLSS helper starts in the background (the 6.5 s first-frame wait made Pimax drop the game), SteamVR 32-bit fallback in mod + launcher. See BUILD_PLAN "RELEASE 0.1.1".

CURRENT 0.1.0 RELEASED (2026-10-14): build A2p + panel tidy, package installer on JJ's game. See BUILD_PLAN "RELEASE 0.1.0". Open: shotgun shells (RE_NOTES A2p effect stepper).

CURRENT A2p INSTALLED (2026-10-14): toolkit effect stepper (panel TESTS "Hide each effect in turn") to name the shell draw. HEADSET pending; RE_NOTES A2p.

CURRENT A2o INSTALLED (2026-10-14): shells still open (A2n theory disproven, fix off); eye-same finder per-shader input table; Floating screen options removed. HEADSET pending; RE_NOTES A2o.

CURRENT A2n INSTALLED (2026-10-14): shotgun shells - right eye reused left-eye eye-relative positions; right-eye rows equal to a left row get the eye shift (FixStaleParticles, aa stale on|off). HEADSET pending; RE_NOTES A2n.

CURRENT A2m INSTALLED (2026-10-14): grapple ring ACCEPTED (A2l, JJ "fixed"); eye-same finder now names differing inputs for the shells. HEADSET pending; RE_NOTES A2m.

CURRENT A2l INSTALLED (2026-10-14): A2k ring fix without the learned-texture gate; eye-same finder pairs by kind. HEADSET pending; RE_NOTES A2l.

CURRENT A2k INSTALLED (2026-10-14): grapple ring matched where the game drew it (its projection includes
the head) + held through frames without a projection; capture button now runs the shell eye-same finder.
HEADSET pending; RE_NOTES A2k.

CURRENT A2j INSTALLED (2026-10-14): grapple pointer 26 deg; toolkit eye-same draw finder on F2 for the shell
casings drawn with no stereo depth (all shells 554 px vs floor 557..592). HEADSET pending; RE_NOTES A2j.

CURRENT A2i INSTALLED (2026-10-14): off-screen grapple points now kept; the game's edge ring goes ON the
point when you can see it, else a head-locked pointer 32 deg toward it at HUD distance (JJ's choice). A2h
log: all 5648 grapple projections were off-screen and rejected. 11 checks pass. HEADSET pending; RE_NOTES A2i.

CURRENT A2h INSTALLED (2026-10-14): grapple ring anchored from the grapple gun's own target
(learned from accepted Canvas.Project anchors), every-caller projection census, optional hide of rings
the game has no target for (default off, "aa grapple hide on"). A2g showed pixelCalls0: its route never
ran. 11 checks pass incl. new grapple-target check. HEADSET pending; read RE_NOTES "A2h" + TEST PLAN A2h.

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

CURRENT A1v INSTALLED (2026-10-13 00:50 Sydney): corrected Asylum's swapped,
negated particle-facing axes. A1u HEADSET REJECTED; retail named7/patch0/
axisMismatch72999 showed its guard blocked every particle correction.
Actual target binder now reproduced in96 GPU cases EACH native/legacy route;
all11checks pass. Default StableParticles ON/live toggle retained;19owned/
fiveprotected/cache/savedINI hashes verified. HEADSET A1v pending. Gamma A1t
accepted; FPS still open. Next JJ ONE launch: menu AND opening gameplay smoke,
head roll/yaw/lean, then gamepad-camera turn; rolling capture and normal exit.
See RE_NOTES A1v and re/evidence/A1v-build-checks.json. Older entries historical.

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

CURRENT A1q installed: stock native rendering skips unused physical depth
copies and inactive texture metadata/sampler work. Same resolution/lighting;
DLSSMode0 preserved. Nine checks incl4097 DLSS pairs/8192 Presents pass;18 owned
and five protected hashes verified. New GPU checks prove zero unused depth
copies /512 stock texture binds with zero metadata probes; still depth retained.
A1p ground/shadows fine perJJ, FPS unchanged, Off~75 vs desktop4K230. BaseFPS
still open. NVIDIA own-process read-only status confirms driver model preset13
and cached310.9.0 replacing candidate; CNN comparison rejected, no CNN shipped.
NEXT same native opening scene/DLSSoff30s then normal exit; no comparison needed.
Read BUILD_PLAN/RE_NOTES A1q and re/evidence/A1q-build-checks.json. Earlier
checkpoints below historical.

CURRENT A1p installed: exact dynamic depth bias on supported owned device;
per-variant handles share native pipelines without quantizing bias. Nine checks
pass incl4096 bias variants/ONE native PSO and260 static/dynamic pixel references.
A1o misses665/679 change bias; retail A1p FPS/memory/shadows still pending.
Next JJ run same native scene~1min, note FPS/ground/shadows, normal exit.
Read BUILD_PLAN/RE_NOTES A1p. Earlier checkpoints below historical.

CURRENT A1o installed: documented ignored blend slots normalized in cache
KEY; all other state retained, sampled nearest miss-group/bias telemetry.
Nine checks pass. A1n retail reuse only2%, memory/FPS NOT fixed. Read
BUILD_PLAN/RE_NOTES A1o before another attempt. Next JJ run30s same native
gameplay then normal exit; no comparison/crash wait. Older notes historical.

CURRENT A1n installed: bounded exact graphics pipeline reuse. Nine checks
pass, including1024 duplicate requests and separate red/green GPU pixels after
eviction/clear. A1m trace proves21k pipeline creations/minute with allocations
in that path; actual duplicate rate and FPS/crash improvement pending.
Next JJ launch same scene/native for~1min then exit; no comparison required.
Read BUILD_PLAN/RE_NOTES A1n. Older checkpoints below are historical.

CURRENT A1m installed:48-frame reservation provenance plus passive pipeline
creation counters. Nine checks pass; retail OOM/FPS unresolved. A1l ordinary
gameplay confirms growth without comparison. Next single JJ run30s gameplay
then normal exit; no comparison/crash wait. Read BUILD_PLAN/RE_NOTES A1m.
Older checkpoints below are historical.

CURRENT A1l installed: bounded reservation call-stack tracing;9 tests pass.
A1k still40-50FPS and OOM. Read BUILD_PLAN/RE_NOTES A1l first; next JJ run
is30s ordinary gameplay then normal exit, NO compare test/crash wait.
Older checkpoints below are historical.

Current checkpoint: **A1k installed02:19 SydneyOct12; nine checks pass.**
Missing proxy restored; AFW navigation corrected; bounded graphics wrapper reuse
and memory telemetry. Gameplay FPS/crash resolution still unvalidated.
See BUILD_PLAN/RE_NOTES A1k and re/evidence/A1k-build-checks.json.
Older checkpoints below are historical.

<p align="center">
  <img src="https://cdn.cloudflare.steamstatic.com/steam/apps/35140/capsule_616x353.jpg" width="616" alt="Batman: Arkham Asylum Game of the Year Edition">
</p>

> **Work in progress - not ready to play yet, and there is no release.** Batman: Arkham Asylum Game of the Year Edition in PC VR, made the same way as
> [Batman City VR](https://github.com/junglejuz/Batman-Arkham-City-VR) and
> [Batman: Arkham Knight VR](https://github.com/junglejuz/Batman-Arkham-Knight-VR): the game draws both eyes itself
> (native stereo), with per-eye DLSS and OpenXR for Virtual Desktop and SteamVR. The notes below are the developers'
> working notes. The picture above is the game's own Steam store artwork, shown from Steam (Batman: Arkham Asylum Game of the Year Edition © Warner Bros.
> Entertainment, Batman and all characters © DC). This is an unofficial fan mod and will need your own copy of the game.

# Batman Asylum VR - development build0.1.0-A1j

**A1j installed00:47 Sydney2026-10-12; eight checks pass.**
JJ confirms the previous cap fix lets the menu reach90fps; beginning gameplay
still stays below60. A1j reduces repeated scene-state queries while preserving
current image quality, resolution, DLSS and HUD/depth behavior. Scene CPU/GPU
timings now distinguish rendering from final VR transfer. Tests establish
correct state restoration and fewer queries; actual gameplay FPS gain is pending.

On the usual VR shortcut, return to the same gameplay spot and let it settle.
Open F10/both sticks -> FRAME RATE -> **Compare scene tracking speed**, then
stand still until the panel returns (~27s). It automatically compares optimization
off/on/off for8s each, records frame rates and stereo captures, and restores
settings. No headset typing needed. See BUILD_PLAN/RE_NOTES A1j and
re/evidence/A1j-build-checks.json. Distant detail stays at the accepted quality;
ground artifacts, AFW flicker and camera jitter still need headset validation.

Historical A1i checkpoint follows.

**A1i installed00:22 Sydney2026-10-12; eight checks pass.**
Asylum's base configuration still contained the stock62fps limiter and was
omitted from earlier launch profiles. The VR shortcut now temporarily configures
both base and generated settings, restores them after exit, and records their
startup values. Owned desktop presentation explicitly disables V-sync.
Same saved DLSS mode/resolution; no image-quality reduction. Actual headset
cap removal and90fps remain pending; named CPU timings help diagnose any
remaining limit. Relaunch through the usual Asylum VR shortcut and check the
same menu/gameplay spot after15-30s. See BUILD_PLAN/RE_NOTES A1i.

Historical A1h checkpoint follows.

## A1h native DLSS overlap

**A1h installed00:07 Sydney2026-10-12; eight checks pass.**
A1g image quality is improved according to JJ. A1h overlaps native LEFT DLSS
with engine RIGHT rendering, with bounded shared-fence ownership and stock-pair
fallback. Overlap is selected by default; AFW retains safe synchronous transfer.
Resolution and saved DLSS mode are unchanged. Headset FPS improvement is pending.

Open DLSS -> **Compare DLSS transfer speed** after30s in a settled scene and
stand still. It runs safe/overlap/safe for8s each, saves stereo stills/replays
and logs completed frame rates, then restores settings. The overlap checkbox
allows manual comparison. See BUILD_PLAN/RE_NOTES A1h and A1h-build-checks.json.
F2 or the top overlay button saves the preceding10s of both final eyes.
Ground artifacts, AFW flicker, jitter and full donor feature parity remain open.

Historical A1g: scene depth freezes before later clears/rebinds; physical-DS
projection and precise relative-camera motion corrected. Retail raw depth now
contains actual geometry. Captures include raw depth/statistics; replay exports
chronologically. This visual improvement is preserved in A1h.

Historical A1f checkpoint:

**A1f is installed. Eight offline/GPU checks pass; headset acceptance is pending.**
Quality DLSS now uses a smaller game input and reconstructs to the chosen output
resolution, with mild scene texture detail correction. Resolution scale controls
output detail; changing the mode applies its engine resolution on the next launch.
The VR launcher disables DoF, motion blur and game MSAA, restoring originals on exit.
Optional AFW includes far-depth and camera-precision corrections; native starts
selected by default. FRAME RATE counts new completed stereo frames.

The top panel button **Save last10 seconds (stereo)**, or **F2** while the game
has focus, saves what happened BEFORE the press. Both final eye images include
HUD after DLSS/AFW. Replay uses up to10pairs/s at960px per eye, JPEG94, with a
bounded memory ring; actual buffered duration/drop count is displayed.
Open `%LOCALAPPDATA%/Batman_Asylum/captures/A1f_history_*/review.html` to replay;
paired images, timestamps and camera records are in the same folder. The
full-resolution still button remains available. Stock/DLSS and native/AFW
comparison buttons capture each step automatically, then restore settings.

The black ground artifact still needs captured retail evidence. Camera-only
motion may smear animated objects; AFW reflections/disocclusions and the
reported flicker need the grouped headset check in BUILD_PLAN A1f.

Historical A1e notes follow.

# Batman Asylum VR - development build 0.1.0-A1e

**A1e is installed. Higher-resolution retail rendering and AFW need a headset check.**
Game resolution now requests the headset recommendation (1.00x by default;
2284x2448 here) independently of the fitted desktop mirror. The City/Origins
panel structure includes VR RESOLUTION first, STEREO RENDERING, VIEW, HUD,
MENUS AND SCREENS, DLSS, FRAME RATE and Diagnostics. Saved0.6x-1.8x resolution
changes apply on the next VR launch; observed game sizes are shown separately.
Native starts selected. Optional AFW draws one eye and builds the other using
the same frame's depth, with independent histories. "Compare native and AFW"
saves a timed comparison automatically. Both VIEW choices use the exact headset
asymmetric field of view; lighting compatibility remains the default.
HUD distance/size and DLSS mode save automatically; HUD is added AFTER DLSS
and AFW. DLSS now retains the selected output ratio before XR resampling.
Eight checks pass, including actual2284x2448 GPU source with a smaller desktop
window, AFW pixel placement through both asymmetric/inverted-Z views and real
NGX alternating-eye evaluations. A1d retail DLSS is confirmed in logs; the new
source/scene/depth sizes and headset quality still need the BUILD_PLAN A1e run.
Camera-only DLSS motion can smear animation; AFW can distort glass/reflections
and exposed edges. Full City/Origins feature parity is still being developed.

Launch the usual VR shortcut ->F10/both sticks ->VR RESOLUTION (actual size),
then STEREO RENDERING ->Compare native and AFW (about20s, automatic captures).
Inspect gameplay/HUD/head movement, save a capture, quit normally.

The A1d details below are historical; its desktop source/output limits have
been replaced in A1e. RE_NOTES A1e and installed-build.json are authoritative.

Separate Asylum GOTY port based on the accepted Origins O1H core. Target:
Steam x86 ShippingPC-BmGame.exe on E:. Dedicated D3D9/XInput proxies adopt
the game's plain 9On12 device and transfer its completed surface through
ordered 11On12 into OpenXR. VDXR/default-runtime and SteamVR launchers,
F10/both-stick panel, near-square graphics profile, captures and manifest-owned
installation/restoration are implemented. No agent game launch.

A1 adds Asylum-specific native stereo hooks (zero/five/24-word Draw/Calc/ctor
ABIs), independent right-eye view state, one located pose per pair and
render-thread eye attribution. Native is selected by default and activates
after the actual ordinary viewport caller supplies a validated main scene
view. The panel allows a native/screen comparison; a failed camera/pair
validation stops native rendering and returns to a floating screen.

Flat movies and fallback frames use a distant room-fixed screen showing
the whole source image. The title/main-menu 3D background is intended to use
native scene rendering. In screen mode, head motion changes the viewing angle; the screen
stays in LOCAL space until recenter or a mode/size change. Full-FOV projection
carriers avoid the Virtual Desktop narrow-layer issue. A/B/X/Y, Start or Back
cancels the opening Bink sequence through Asylum's owning-thread force-stop.
Story and loading movies do not qualify. No movie files are altered.

**A1d is installed; its tower correction and retail DLSS need headset validation.**
The default lighting-compatible native path renders a centred enclosing frustum
and maps it through the exact XR eye lenses. This addresses legacy lighting
projection assumptions while keeping native eye poses and full XR field of view.
"Compare tower shading" captures corrected stock, original stock and corrected
DLSS automatically in about20s, then restores settings. A1c's actual retail
startup gamepad stop and D3D9 HUD interception are confirmed in logs; headset
HUD fusion and opening-height acceptance are not yet separately reported.

Real per-eye DLSS now uses captured D3D9 scene depth, current/previous camera
matrices, shared jitter and independent eye histories through the installed x64
NGX helper. Quality is requested by default; mode changes persist locally.
Successful evaluations of BOTH eyes are required. Failures retain both stock
eyes; the isolated HUD is composited after upscaling on its shared4m plane.
All eight checks pass, including actual DLAA/Quality GPU evaluations, measured
depth/motion and missing-RIGHT rollback. These do not establish retail depth
coverage, shadow correction or headset image quality. Camera motion only can
smear animated objects. Desktop-fit source1238x1328 is cropped to each lens,
upscaled by DLSS at the displayed actual sizes, then resampled to2284x2448 XR;
this is below full-XR Quality detail. Once-only UI work, markers/fullscreen
fades, reflection fixes, engine-wide DoF, AFW and endurance remain open.
See RE_NOTES, BUILD_PLAN and DLSS_PLAN for evidence and limits.

Launch `E:/Games/Steam/steamapps/common/Batman Arkham Asylum GOTY/Binaries/Batman Asylum VR.bat`.
F10 or both sticks held 0.6 s opens the panel. Save capture writes source,
scene depth, isolated HUD and composed eyes to `%LOCALAPPDATA%/Batman_Asylum/captures`.
Normal Steam launches
remain flat. Installation writes only manifest-owned Asylum files, preserves
configuration bytes, and backs up the preceding development build.

One grouped JJ run: skip logos -> inspect tower -> F10/both sticks -> Compare
tower shading (automatic captures) -> check actual DLSS state/dimensions ->
Continue campaign -> movement/HUD/pause/detective/reflections -> Save capture
-> normal quit. No live headset typing is needed.
