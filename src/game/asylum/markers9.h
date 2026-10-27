#pragma once
#include "stereo_math.h"
#include <d3d9.h>
#include <functional>
#include "transitions9.h"
namespace bvr::aa::markers9 {
bool install_engine(uintptr_t base);
void publish(uint32_t slot,uint32_t frame,uint32_t generation,void* view,const math::M& baseView,const math::M& baseProjection,const math::M eyeVP[2],float referenceYaw=0,const float* referencePosition=nullptr,const math::M* eyeView=nullptr,bool headHud=false,float unitsPerMetre=90.f);
void end_game_pass();
void begin_render(uint32_t slot,bool active);
void begin_eye(int eye,uint32_t generation);
void vertex_created(IDirect3DVertexShader9*);
bool draw(IDirect3DDevice9*,bool uiLane,UINT primitives,const std::function<HRESULT()>& original,HRESULT& result);
// Snapshot of the actual room HUD plane, cleared at recenter/reset. The native
// callout owner constructs its own line, ring and bends from the corrected XY.
void publish_hud_plane(const float world[4][3],bool headFollowing=false,const float* shape=nullptr);
void invalidate_hud_plane();
// Retained only for the offline GPU blend/state fixture, not production UI.
struct ConnectorVertex {float clip[4]{},colour[4]{};};
IDirect3DSurface9* sheet(int eye);
uint32_t generation();
void reset();void draw_ui();void status_line(char*,size_t);
// "hide on|off" (targetless grapple rings), "census" (log every projection caller).
bool command(const char*args);
// Eye origins (world units) of the pass with this generation, from its eye views.
bool eye_origins(uint32_t generation,float left[3],float right[3]);
bool fixture(IDirect3DDevice9*,const wchar_t* directory,bool(*completed)(IDirect3DSurface9*,IDirect3DSurface9*)=nullptr);
}
