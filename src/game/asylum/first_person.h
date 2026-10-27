#pragma once
// FIRSTPERSON (JJ 2026-10-20: "add the same first person mode we have for the other arkham games ... add it to origins too").
// Batman City's mode (batman_city scene.cpp FIRSTPERSON / HEADCAM / FPGATE) with Asylum's own engine offsets (RE_NOTES
// "FIRSTPERSON"): eye position follows Batman; optional head aim adds VR look to stick turning.
#include <cstdint>
#include <cstddef>
#include "menu_state.h"
namespace bvr::vr { struct HeadPose; }

namespace bvr::aa::first_person {

void install(uintptr_t imageBase, const wchar_t* settingsPath); // aa_rendering.ini [AsylumVR] + both-sticks double click
// Game thread, before the FIRST eye's CalcSceneView of a pair (the second eye reuses it). `blocked` = why the game's own
// camera must stay this frame (menu), or nullptr.
void update(void* localPlayer, const char* blocked);
// Game thread: start once per ordinary draw, then reuse the first constructor's
// complete view for the other eye. Return whether head aim was actually applied.
void begin_frame(bool nativeActive, const menu_state::Observation& mode, uint32_t generation);
bool apply(float view[4][4], float unitsPerMetre, const vr::HeadPose& head);
// LEANBACK: the final eye's world origin (headset movement included); pushed forward along this pair's level push direction if
// leaning back would bring it nearer than the set distance to his head point. True = moved. Same units as apply.
bool clamp_eye(float origin[3], float unitsPerMetre);
void status_line(char* out, size_t size);
void note_hud(uint32_t generation, bool followsHead, bool success);
bool save_trace(const wchar_t* directory); // bounded 20 Hz history, written only on capture
bool on();
void set_on(bool on);
void toggle();
void draw_ui();
void draw_camera_ui(); // CAMPOS: third-person camera position sliders (VIEW section)
bool command(const char* args); // after "fp "

} // namespace bvr::aa::first_person
