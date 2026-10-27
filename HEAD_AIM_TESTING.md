# First person: headset aiming and HUD

Development build: `Asylum-0.1.4-HEAD-AIM5`, based on the
`Godoy301/Batman-Arkham-Asylum-VR` fork, version 0.1.4.

Enable **FIRST PERSON** and **aim and HUD follow the headset** in the VR panel.
The second setting is enabled by default when no saved configuration exists.
With head aiming enabled, the right stick turns only horizontally;
vertical pitch is controlled by the headset. The separate blocking option
was removed; the old `FirstPersonBlockStickPitch` key is ignored.
The HUD follows the headset position and orientation, including tilt, on a
single plane with depth for both eyes. Target markers remain attached to their
targets in the world.

The setting is saved in `%LOCALAPPDATA%\Batman_Asylum\aa_rendering.ini`, section
`AsylumVR`, key `FirstPersonLookAim`. Commands: `aa fp aim on`, `aa fp aim off`,
`aa fp status`. Disabling the option restores the base orientation and the
room-fixed HUD.

### Gameplay validation (introduced in HEAD-AIM2)

The first real test showed `enabled=1`, `active=0`, `writes=0`, and
`reason=not gameplay`, even though the first-person camera followed the
character. The menu reader was in an unknown state; the first version required
confirmed gameplay and therefore never applied head aiming.

The current version also allows application when the reader is unknown **and**
the native first-person camera and controlled pawn pass identity, position, and
proximity validation. An identified menu, missing pose, invalid pawn, or open VR
panel still blocks the feature. The unknown state continues to be reported as
unknown; no menu-reader offset was changed without game evidence.

After reinstalling, close F10, move your head with the stick at rest, and press
F2. Look for `build=Asylum-0.1.4-HEAD-AIM5`, `active=1`, increasing `writes`,
and `hudFollowing=1`. `reason=active via validated first-person pawn` identifies
this validation fallback. The user confirmed aiming and pitch stability in
HEAD-AIM3. The flat-image flash fix in HEAD-AIM4 still requires headset testing.

## Build and validation scope

On Windows, use the build commands from the README. The new CTest is named
`first_person_head_aim`. The **Windows head aim checks** workflow builds the
Win32 mod and runs six CPU tests; it does not run the game, GPU tests, OpenXR,
or build the 64-bit DLSS helper.

## Installing the test DLLs (including the manifest)

The `head-aim-test-dlls` artifact contains only the test DLLs. Also use the
complete **0.1.4** ZIP from the
[original releases](https://github.com/junglejuz/Batman-Arkham-Asylum-VR/releases).
Keep that original ZIP so you can return to the previous version.

1. Close the game. Extract the complete 0.1.4 ZIP to a working folder outside
   the game folder, for example `Downloads\AsylumVR-HeadAim`.
2. Extract the test artifact separately. Locate `batman_asylum.dll`,
   `d3d9.dll`, and `xinput1_3.dll` (the artifact may place them in
   `build-native9\Release`). Copy **those three DLLs** to the working folder,
   next to `Install-BatmanAsylumVR.bat`, replacing the versions from the
   original package. Preserve all other files and the `files` folder from the
   complete ZIP.
3. Run `Install-BatmanAsylumVR.bat` **from that modified working folder**.
   It installs the test DLLs and recreates `Binaries\asylumvr_manifest.json`
   with hashes for the installed files. Existing VR settings are preserved.
4. Launch through the normal VR shortcut and follow the test below.

Replacing DLLs directly in `Binaries` leaves the old manifest hashes in place.
This causes `A mod file in the game folder is missing or changed (d3d9.dll)`
before the game even starts. If you already replaced them this way, the
procedure above also repairs the installation. You do not need to uninstall
first. Do not delete the manifest or remove launcher verification.

To return to the original version, extract the complete ZIP again **without the
test DLLs** to another folder and run its installer. It restores the original
files and recreates the matching manifest. Running the installer from an
unmodified original package also removes the test functionality.

The portable tests verify aim direction against independent rays, controller
turning added to headset rotation, no accumulation of headset rotation,
recentering, restoration, invalid poses, and binocular HUD projection under
movement. They do not prove in-game gadget behavior.

## One PC VR test session

1. Launch through the VR shortcut. At the main menu, confirm normal behavior.
2. Enter the game and enable first person. Without touching the right stick,
   look left/right, up/down, and tilt your head. The HUD should remain in front
   of you with the same depth in both eyes.
3. Aim a gadget at two separate targets. Move only your head, then use the
   stick with your head still, and finally move both together. Compare the
   visible aim with the gadget's actual direction/impact. Also test grapple
   point selection: its marker should remain on the target.
4. Look to one side for a few seconds without moving the controller: there
   should be no continuous turning or duplicate application of rotation. Turn
   through the opposite direction, recenter, and repeat on the other side.
5. Toggle the option, open/close the VR panel, test pause/map, a camera
   transition, and return to the main menu. Test tracking loss/recovery.
   Verify controls, aim restoration, and absence of unexpected jumps. Repeat
   with DLSS and, if used, AFW.
6. If any problem occurs, press **F2**, exit normally, and preserve the files
   below before starting several new sessions.

## Required evidence

In `%LOCALAPPDATA%\Batman_Asylum`:

- `batman_asylum.log`: current session; `batman_asylum.prev.log`: previous
  session. The logger already preserves one previous generation and flushes
  every message to disk.
- `captures\A2p_*\status.txt`: includes the build identifier and `headAim=`.
  The same folder contains the available scene, HUD, and eye images.
- `captures\A2p_*\head_aim.csv`: up to 256 recent samples, at most 20 per
  second during activity, plus state changes. It includes monotonic time in
  milliseconds, generation, requested/observed orientation, quaternion, and
  stick input. History remains in memory and is written only on capture, making
  it possible to inspect movement between the five-second log messages.
- `captures\A2p_history_*`: stereo history requested by F2 when complete
  pairs are available. The A2p names are inherited from the original capture
  system.

The `[aa head aim]` lines are emitted on state changes and every five seconds,
avoiding per-frame disk writes. They record:

- `requested/enabled/active`, `reason`, `gen`, and `ageMs`: configuration,
  effective application, suspension reason, pose generation, and age of the
  latest write;
- `menuMode`, `menuRead`, and `viewportCountCap`: observed menu-reader
  state, failed stage, and viewport list count/capacity. These fields also
  appear in the CSV to distinguish a real menu from an unavailable read;
- `bodyPitchYaw`, `targetPitchYaw`, `beforePitchYaw`, `readbackPitchYaw`:
  base orientation, requested orientation, orientation observed before the
  update, and orientation read back afterward;
- `headQuat` and `stickRXRY`: headset orientation and latest stick values
  delivered by the input compositor, sampled during the update;
- `writes/failures/restores`, `hudGen/hudFollowing/hudComposites/hudFailures`:
  confirmed writes, failures, restorations, and HUD composition;
- `fpFrames/headFrames/headMisses`: camera application and head-bone reads.

A correct readback proves only that `Controller.Rotation` received the value;
it does not prove that every gadget consumed it. The update happens during
camera construction, after that frame's simulation: gadget reaction may occur
on the next tick. Compare images/impacts and the existing `[aa grapple]` logs
to confirm the actual result. Increasing `ageMs` or stalled generations
indicate that no new updates are being applied even if the last sample was
active.

## Known limitations

The repository identifies the main menu, but still does not have a validated
signal for every pause/map menu or every cinematic camera. This change
suspends aiming in the identified main menu, when the VR panel owns input,
without a valid native pose, without a valid pawn/controller, or when the
camera is far from Batman. If the menu-reader state is unknown, the fallback
depends on validation of the first-person camera and controlled pawn, not on
positive gameplay identification. Pause, nearby cinematics, and an unrecognized
menu with the pawn nearby require the test above; there is no new claim that
those states are detected.

In this Linux environment, the MSVC build and headset validation are not
available. The Windows workflow result and the real headset session must be
checked separately; passing CPU tests does not mean headset approval.

## HEAD-AIM4: automatic blocking and third-person flashes

Head aiming now automatically blocks the vertical right-stick axis. Disabling
head aiming restores normal vertical control. Horizontal turning remains.
Test it on/off, close F10, and verify movement, aiming, and gadgets.

Head and both eye poses are now published together. Previously, the game could
read a new head generation while the eyes were still being located, causing
one flat frame. The new camera uses a complete sample from a single generation.
Real tracking loss still invalidates the sample; a valid pose is not forced
when the runtime reports failure.

Walk through the same smoke repeatedly while moving your head, then repeat
outside it. Press F2 immediately after any remaining flash. Send the log and
files from the NEW capture folder; the version should be HEAD-AIM4. The log
adds `coherentPoseUnavailable`, `flatDraws`, and `[aa native fallback]`
lines with pose, camera, and viewport state. The specific relationship between
smoke and failure has not been proven; the tests reproduce the incomplete
publication window.

The CSV keeps `raw_ry`, `move_lx`, `move_ly`, `pitch_input`, and
`block_stick_pitch` (now the automatic head-aiming policy).
`stick_ry` is the input delivered to the game.

## HEAD-AIM5: World scale

At the top of F10, the **World scale** slider ranges from **0.50x to 2.00x**.
**1.00x** preserves the original size. Since the merge into 0.1.5 (2026-10-25),
higher values make the world look SMALLER, as in the other Arkham mods (the
fork's build had it the other way). **Reset** returns to 1.00x.
The value is saved as `AsylumVR/WorldScale` in `aa_rendering.ini` and appears
in `status.txt` and the log's `[aa world scale]` lines.

It applies in first and third person, native stereo, and AFW. The scale is
shared by both eyes and physical head movement, with markers using the same
conversion. HUD and menu retain their physical size; the camera remains
attached to the same character point. It does not change FOV.

Test 0.75x, 1.00x, and 1.25x in a familiar room while tilting/translating your
head, aiming a gadget, and looking at grapple/detective markers. Check Reset,
restart the game to verify persistence, and repeat with AFW if used. The user
confirmed the HEAD-AIM4 flash fix; also check the same smoke for regressions.
World scale still requires headset validation.
