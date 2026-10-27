#pragma once
#include "stereo_math.h"
#include <cstdint>
struct IDXGISwapChain;
namespace bvr::aa::native {
struct Camera {math::M view{},projection{};float angles[4]{},crop[4]{1,1,0,0};float jitterX{},jitterY{},unitsPerMetre{90.f};uint32_t generation{},width{},height{},inputWidth{},inputHeight{};int eye{};bool valid{},centred{},afw{},headHud{};};
bool camera(Camera&);
bool head_hud(uint32_t generation);
bool afw_present();
void set_centred(bool);
bool centred();
bool install();
void begin_render(void* command);
void on_present(IDXGISwapChain*);
void discard_present();
const char* rendering_status();
unsigned completed_pairs();
void status();
void draw_ui();
void draw_world_scale_ui();
float world_scale();
bool selected();
void select(bool);
void recenter();
int present_eye();
}
