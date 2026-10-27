<p align="center">
  <img src="https://cdn.cloudflare.steamstatic.com/steam/apps/35140/capsule_616x353.jpg" width="616"
       alt="Batman: Arkham Asylum Game of the Year Edition">
</p>

# Batman Asylum VR

Batman: Arkham Asylum (Game of the Year Edition) in PC VR, with per-eye DLSS and OpenXR for Virtual Desktop and
SteamVR. The game draws both eyes itself, every frame, from the same moment (native stereo), so depth is correct and
nothing lags behind when the camera swings. Made the same way as
[Batman City VR](https://github.com/junglejuz/Batman-Arkham-City-VR) and
[Batman Origins VR](https://github.com/junglejuz/Batman-Arkham-Origins-VR).

## What you get

- **Native stereo 3D** with full head tracking (look around and lean in), on the game's own third-person camera.
- **DLSS for each eye** (NVIDIA RTX graphics cards): each eye is drawn smaller and upscaled by NVIDIA's DLSS, which
  also gives clean anti-aliasing. Choose the quality in the VR menu.
- **OpenXR, with Virtual Desktop and SteamVR**: runs on your PC VR software's own OpenXR runtime (no extra layers or
  injectors). A separate SteamVR shortcut starts the game through SteamVR without changing your PC's VR settings.
- **The render size comes from your headset**: the launcher asks your PC VR software for its picture size each time.
- **HUD on a panel in front of you**, at a depth both eyes agree on.
- **Grapple prompts on the grapple points themselves**, at their real depth. When a grapple point is out of view, a
  pointer near the edge of your view shows which way it is.
- **Detective mode details** (the lines and rings on people) point at the people they belong to.
- **Menus and loading screens** on a screen in the room.
- **First person, if you want it**: tick FIRST PERSON in the VR menu, or click both thumbsticks twice quickly. The
  camera sits at Batman's eyes and follows his head; it switches back by itself on the main menu and in movies, and whenever the game moves its camera away from him. Leaning back stops just in front of his face.
- **Aim with your head in first person** (on by default; VR menu, FIRST PERSON > "aim and HUD follow the headset"):
  batarangs and the batclaw go where you look, and the HUD stays in front of your eyes. The right stick turns you left
  and right; up and down come from your head. By [Godoy301](https://github.com/Godoy301).
- **World scale** (VR menu, VIEW): higher makes the world look smaller.
- **Camera position**: move the third-person camera left / right, up / down and closer / further (VR menu, VIEW >
  camera position).
- **An in-headset VR menu**: hold both thumbsticks down (or press F10). Render size, DLSS, HUD size and distance, and
  more. Settings are saved.

## What you need

- **Batman: Arkham Asylum Game of the Year Edition** on Steam, on Windows 10 or 11. (The mod is made for that exact
  version of the game; the installer checks it. The [Joker DLC Patcher](https://www.nexusmods.com/batmanarkhamasylum/mods/160)
  is fine too.)
- **A PC VR headset and the software you use for PC VR games** (for example Virtual Desktop, SteamVR or the Meta Quest
  Link app).
- **A gamepad.** Quest Touch controllers work through Virtual Desktop, which presents them as a gamepad.

Tested with: Quest 3 through Virtual Desktop, RTX 4070 Ti (about 65-80 frames per second). A SteamVR launcher is
included; it has not been tested with this game yet.

## Installing

1. Start the game once from Steam as normal, wait for the main menu, and quit (this makes the game's settings files).
2. Download `BatmanAsylumVR-ArkhamAsylum.zip` from [Releases](../../releases) and unpack it anywhere (7-Zip works).
   Most of its size is NVIDIA's DLSS library.
3. Double-click `Install-BatmanAsylumVR.bat`. If Windows SmartScreen appears: More info, then Run anyway. The
   installer puts everything into the game folder; you don't copy anything there yourself.
4. Start your PC VR software and make sure it is the active OpenXR runtime (Virtual Desktop and SteamVR each have a
   setting for this).
5. Start the game with the **Batman Asylum VR** shortcut on your desktop. To play through SteamVR instead, use
   **Batman Asylum VR (SteamVR)**.

Each VR start sets the game to a window at your headset's picture size with V-sync, motion blur, depth of field and
the game's own anti-aliasing off, and puts your own graphics settings back when the game closes. Starting the game
from Steam still plays the normal flat game.

## In the game

| | |
|---|---|
| VR menu | hold both thumbsticks down for half a second, or F10 |
| Recenter | the VR menu's "Recentre view and screens" button |
| First person on / off | the VR menu's FIRST PERSON, or click both thumbsticks twice quickly |
| Save the last 10 seconds of both eyes | the VR menu's save button, or F2 |

## Known issues

- Spent shotgun shells on the floor show at the wrong depth (they look far away).
- Some menu changes can briefly show a black window.
- AFW (one eye drawn, the other warped) is in the VR menu but is experimental: use Native.
- First person is new: Batman's head is not hidden (the camera sits just in front of his face), and it stays first
  person behind the pause menu.

## If something is wrong

| What you see | What to do |
|---|---|
| Nothing in the headset, the game plays on the monitor | Start your PC VR software before the game, make it the active OpenXR runtime, and start the game from the Batman Asylum VR shortcut. |
| A window says the headset could not be asked for its picture size | Connect the headset to your PC VR software first, then start the shortcut again. |
| The installer stops: "the game has not made its settings yet" | Start the game once from Steam, wait for the main menu, quit, run the installer again. |
| The installer or launcher says the game is a different version | The mod works only with the Steam Game of the Year Edition. |
| Pimax (or other VR software with no 32-bit OpenXR runtime): nothing in the headset | The shortcut then uses SteamVR's 32-bit runtime by itself: install SteamVR. Do not run Steam or the game as administrator. |
| The launcher says Windows does not allow changes to a settings file ("Access denied") | Usually Windows Security's "Controlled folder access". Allow PowerShell there (Virus & threat protection > Ransomware protection > Allow an app through Controlled folder access), or untick Read-only on the file it names. A settings file in Documents that cannot be changed is left as it is and VR starts anyway. |
| Low frame rate | VR menu: lower the render size (VR resolution), or choose a faster DLSS setting. |

## Removing it

Run **Uninstall Batman Asylum VR** from the Start menu (or `Uninstall-BatmanAsylumVR.bat` in the game's Binaries
folder). It removes the mod and the shortcuts and deletes the mod's own VR settings, logs and pictures (in
`%LOCALAPPDATA%\Batman_Asylum`). Your game settings are never changed outside a VR session, so there is nothing else
to put back.

## Building from source

Visual Studio 2022 and CMake:

```
cmake -S . -B build-native9 -G "Visual Studio 17 2022" -A Win32
cmake --build build-native9 --config Release
cmake -S helper64 -B build64 -G "Visual Studio 17 2022" -A x64
cmake --build build64 --config Release
ctest --test-dir build-native9 -C Release
python tools/make_package.py
```

The DLSS helper needs NVIDIA's NGX SDK. Development notes: `STATUS.md`, `BUILD_PLAN.md`, `RE_NOTES.md`,
`FIX_CHANGES.md`.

## Credits

Built on [bioshock-vr](https://github.com/mohamad-balouza/bioshock-vr) (MIT), MinHook, Dear ImGui and the OpenXR SDK.
The per-eye DLSS method follows the [Luma Framework](https://github.com/Filoppi/Luma-Framework)'s Arkham Knight mod.
Joker DLC Patcher support, first-person head aim, the flat-frame fix and the world-scale slider by
[Godoy301](https://github.com/Godoy301/Batman-Arkham-Asylum-VR).
DLSS and NVAPI are NVIDIA's. Full list: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
The picture at the top is the game's own Steam store artwork, shown from Steam (Batman: Arkham Asylum © Warner Bros.
Entertainment); it is not part of this repository.

Batman: Arkham Asylum © Warner Bros. Entertainment Inc. Batman and related characters are trademarks of DC Comics. This
is an unofficial fan mod and needs your own copy of the game.
