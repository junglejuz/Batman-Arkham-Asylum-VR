#pragma once
// Pure math shared by the game-thread aim drive, HUD compositor and CPU tests.
#include "stereo_math.h"
#include "menu_state.h"
#include "core/util/xr_math.h"
#include <algorithm>
#include <cstdint>

namespace bvr::aa::head_aim {
// The IsMenuLevel reader can be unknown during a valid first-person scene.
// Unknown is NOT a gameplay verdict: the controlled-pawn + close-main-camera
// checks still have to succeed before this policy permits an engine write.
enum class Gate { off, optionOff, noNative, panel, menu, needPawn, needCamera, active, activePawnFallback };
struct Context {
    bool firstPerson{}, requested{}, native{}, panel{};
    menu_state::Mode mode{menu_state::Mode::unknown};
    Gate evaluate(bool controlledPawn, bool closeCamera) const {
        if(!firstPerson)return Gate::off;
        if(!requested)return Gate::optionOff;
        if(!native)return Gate::noNative;
        if(panel)return Gate::panel;
        if(mode==menu_state::Mode::menu)return Gate::menu;
        if(!controlledPawn)return Gate::needPawn;
        if(!closeCamera)return Gate::needCamera;
        return mode==menu_state::Mode::gameplay?Gate::active:Gate::activePawnFallback;
    }
};
inline bool permits(Gate g){return g==Gate::active||g==Gate::activePawnFallback;}
inline const char* reason(Gate g){
    switch(g){
    case Gate::off:return "first person off";
    case Gate::optionOff:return "head aim off";
    case Gate::noNative:return "native pose unavailable";
    case Gate::panel:return "VR panel owns input";
    case Gate::menu:return "main menu";
    case Gate::needPawn:return "waiting for controlled pawn";
    case Gate::needCamera:return "camera far from pawn";
    case Gate::active:return "active";
    default:return "active via validated first-person pawn";
    }
}
inline int16_t filter_pitch(int16_t input, bool block, bool active) {
    return block && active ? 0 : input;
}
constexpr float pi = 3.14159265358979323846f;
struct Rotation { int32_t pitch{}, yaw{}, roll{}; }; // UE3 FRotator, 65536 units/turn
static_assert(sizeof(Rotation)==12);
inline int32_t wrap(int64_t n) {
    const auto u = uint32_t(n) & 65535u;
    return u >= 32768 ? int32_t(u) - 65536 : int32_t(u);
}
inline float radians(int32_t n) { return float(wrap(n)) * (2 * pi / 65536); }
inline int32_t units(float r) { return wrap(int64_t(std::lround(r * (65536 / (2 * pi))))); }
inline bool valid_pose(const vr::HeadPose& p) {
    const float q[] = {p.qx,p.qy,p.qz,p.qw,p.px,p.py,p.pz};
    for (float n : q) if (!std::isfinite(n)) return false;
    const float norm = p.qx*p.qx+p.qy*p.qy+p.qz*p.qz+p.qw*p.qw;
    return std::fabs(norm-1) < .002f;
}
// UE world: X forward, Y right, Z up. View columns: right, up, forward.
inline math::M body_view(Rotation r) {
    const float p=radians(r.pitch), y=radians(r.yaw), cp=std::cos(p), sp=std::sin(p), cy=std::cos(y), sy=std::sin(y);
    math::M v{};
    v.v[0][0]=-sy; v.v[1][0]=cy;
    v.v[0][1]=-sp*cy; v.v[1][1]=-sp*sy; v.v[2][1]=cp;
    v.v[0][2]=cp*cy; v.v[1][2]=cp*sy; v.v[2][2]=sp; v.v[3][3]=1;
    return v;
}
struct Solution { Rotation body{}, target{}; math::M view{}; };
// Pitch auto-centering must not feed the injected HMD pitch back into the body.
// Accept pitch deltas only during deliberate stick input; yaw stays additive.
// No game-specific tick rates or camera offsets are guessed.
// Commit ONLY after the engine write succeeds. The second eye reuses the first
// eye's complete view and must never advance this state a second time.
// AIMLEVEL (JJ 2026-10-26: after ticking "aim and HUD follow the headset" in first person "you can get stuck looking down at
// the ground"): while the headset owns up / down, the body pitch is LEVEL, never the game's pitch at take-over. Each VR-panel
// close re-took the camera's current pitch and kept it forever (stick pitch is blocked, so nothing could level it).
struct Tracker {
    Rotation offset{}, lastBody{};
    bool committed{};
    Rotation remove(Rotation r, bool acceptPitchDelta=true) const {
        r.pitch=wrap(int64_t(r.pitch)-offset.pitch); r.yaw=wrap(int64_t(r.yaw)-offset.yaw);
        if(!acceptPitchDelta) r.pitch=0;
        return r;
    }
    bool solve(Rotation current, const vr::HeadPose& head, Solution& s, bool acceptPitchDelta=true) const {
        if (!valid_pose(head)) return false;
        s.body=remove(current,acceptPitchDelta); s.view=body_view(s.body);
        auto orientation=head; orientation.px=orientation.py=orientation.pz=0;
        const auto aim=math::mul(s.view,math::pose_view(orientation));
        const float x=aim.v[0][2], y=aim.v[1][2], z=aim.v[2][2];
        s.target=current; // Preserve the game's roll; HMD roll is rendered separately.
        s.target.yaw=units(std::atan2(y,x));
        s.target.pitch=units(std::atan2(z,std::hypot(x,y)));
        return true;
    }
    void commit(const Solution& s) {
        lastBody=s.body; committed=true;
        offset.pitch=wrap(int64_t(s.target.pitch)-s.body.pitch);
        offset.yaw=wrap(int64_t(s.target.yaw)-s.body.yaw);
    }
};
// One head-centred plane for BOTH eyes, including roll and translation. Never
// build separate planes at each eye: that destroys binocular depth.
inline bool hud_plane(const vr::HeadPose eyes[2], float distance, float hw, float hh, float world[4][3]) {
    if (!valid_pose(eyes[0]) || !valid_pose(eyes[1]) || !std::isfinite(distance) ||
        !std::isfinite(hw) || !std::isfinite(hh) || distance<=0 || hw<=0 || hh<=0) return false;
    const auto& a=eyes[0]; const auto& b=eyes[1];
    const float sign=a.qx*b.qx+a.qy*b.qy+a.qz*b.qz+a.qw*b.qw<0 ? -1.f : 1.f;
    float q[4]={a.qx+sign*b.qx,a.qy+sign*b.qy,a.qz+sign*b.qz,a.qw+sign*b.qw};
    const float norm=std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
    if (norm<.001f) return false;
    for (float& n:q) n/=norm;
    const float centre[3]={(a.px+b.px)*.5f,(a.py+b.py)*.5f,(a.pz+b.pz)*.5f};
    for (int c=0;c<4;++c) {
        const float local[3]={c&1?hw:-hw,c&2?-hh:hh,-distance}; float rotated[3];
        xrmath::quat_rotate(q[0],q[1],q[2],q[3],local,rotated);
        for (int j=0;j<3;++j) world[c][j]=centre[j]+rotated[j];
    }
    return true;
}
} // namespace bvr::aa::head_aim
