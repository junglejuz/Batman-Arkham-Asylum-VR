// FIRSTPERSON for Asylum: Batman City's first-person mode (batman_city scene.cpp fp_update / fp_head_follow / fp_apply),
// re-found in Asylum by code shape (RE_NOTES "FIRSTPERSON", STATIC 2026-10-20 on the retail exe; same module as Origins'
// first_person.cpp except the offsets and the head-bone read):
//   APawn::GetPawnViewLocation 0x867C30 (City 0x7A6970, same instructions): Location +0x54, EyeHeight +0x284.
//   APawn::GetViewRotation 0x87B650 (City 0x7BA650, same instructions): Controller +0x1B4 (Rotation +0x60).
//   The game's own head-bone read 0x8A1178: FName ctor 0x5D5620 (L"Bip01_Head", 1, 1; ret 0xC) on pawn +0x348 (= Pawn.Mesh;
//   EyeHeight -> Mesh is City's 0xC4), then USkeletalMeshComponent::MatchRefBone 0x90E8E0 (thiscall FName; ret 8; -1 =
//   none; SkeletalMesh +0x1B0) and GetBoneMatrix 0x91D2A0 (thiscall FMatrix* out, index; ret 8): world position = row 3.
//   Asylum has no GetBoneLocation of City's shape.
// The player -> controller link (City +0x30) and the controller -> pawn field are found LIVE by the pawn's back-pointer.
#include "first_person.h"
#include "head_aim.h"
#include "scene.h"
#include "core/ui/overlay.h"
#include "core/input/xinput_bridge.h"
#include "core/util/log.h"
#include <windows.h>
#include <imgui.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>

namespace bvr::aa::first_person {
namespace {
using W = uint32_t;
constexpr W kPlayerScanBegin = 0x20, kPlayerScanEnd = 0x80;   // ULocalPlayer -> PlayerController (City +0x30)
constexpr W kPCScanBegin = 0x100, kPCScanEnd = 0x700;         // PlayerController -> Pawn
constexpr W kPawnController = 0x1B4, kPawnLocation = 0x54, kPawnEyeHeight = 0x284, kPawnMesh = 0x348, kMeshSkeleton = 0x1B0;
constexpr W kFNameCtor = 0x5D5620, kMatchRefBone = 0x90E8E0, kGetBoneMatrix = 0x91D2A0;
constexpr float kHeadCapUU = 140.0f; // ~1.5 m: anything further is not his head (City)
// HEADLOCK (JJ 2026-10-20: "when you start running or flying, the head moves ... the camera needs to be linked to the head
// movements so that the head doesn't intersect the camera at any point"): the smoothing may trail the real head by at most
// this much (~3 cm at 90 UU/m), far less than the 25 cm the camera sits in front of his face.
constexpr float kHeadMaxLagUU = 3.0f;
using FNameCtorFn = void*(__thiscall*)(void* name, const wchar_t* text, int findType, int unused);
using MatchRefBoneFn = int(__thiscall*)(void* mesh, W nameIndex, W nameNumber);
using GetBoneMatrixFn = float*(__thiscall*)(void* mesh, float* out, int bone);

uintptr_t base = 0, imageEnd = 0;
wchar_t settings[MAX_PATH] = {};
std::atomic<bool> enabled{false}, followHead{true}, lookAim{true};
std::atomic<ULONGLONG> pitchGateMs{}, stickSampleMs{};
std::atomic<int16_t> rawPitch{}, deliveredPitch{}, moveX{}, moveY{}, turnX{};
void filter_sticks(unsigned index,int16_t* lx,int16_t* ly,int16_t* rx,int16_t* ry) {
    if(index) return;
    const auto now=GetTickCount64();
    rawPitch=*ry; moveX=*lx; moveY=*ly; turnX=*rx;
    const auto stamp=pitchGateMs.load();
    *ry=head_aim::filter_pitch(*ry,lookAim.load(),enabled.load() && stamp && now-stamp<250);
    deliveredPitch=*ry; stickSampleMs=now;
}
bool pitch_input() {
    return !lookAim.load() && GetTickCount64()-stickSampleMs.load()<250 &&
        std::abs(int(deliveredPitch.load()))>8689; // XInput right-stick deadzone
}
std::atomic<float> forwardCm{25.0f}, upCm{0.0f}, headSmoothS{0.08f};
// LEANBACK (JJ 2026-10-27: "When in first person mode and I move my physical head back my virtual camera is moving through Batman's
// head"): the headset's own movement is added on top of the eye point 25 cm in front of his face, so leaning back 25 cm puts the lens
// inside his head. The final eye may come no nearer than this to his head point along the push (level); sideways / up / down stay free.
std::atomic<float> leanMinCm{15.0f};
bool fpPair = false;                        // this pair is drawn in first person (apply reached its end)
float pairLevel[2] = {};                    // the level direction this pair's forward push was made along
bool pairLevelValid = false;
std::atomic<unsigned> leanClamps{0};
// CAMPOS (JJ 2026-10-25: "put the adjustable camera from our Arkham Knight mod panel into the other 3 arkham mods"; AKVR
// TPOFFSET, akvr camera.cpp / hooks.cpp "camera position"): the third-person camera shifted left / right, down / up,
// back / forward, along the game camera's LEVEL heading and world up, so it never swings with the head. Gameplay only:
// not in first person, the main menu, or while the game camera is away from Batman (cutscenes elsewhere).
std::atomic<float> camRightCm{0.0f}, camUpCm{0.0f}, camFwdCm{0.0f};
std::atomic<unsigned> frames{0}, headFrames{0}, headMisses{0}, camFrames{0};
W playerOff = 0, pawnOff = 0;               // found live
float eye[3] = {};
bool eyeValid = false;                      // this pair's eye point (game thread)
bool camValid = false;                      // CAMPOS: this pair's Batman found (eye[] = his eye point, no head follow)
float headOff[3] = {};
bool headOffValid = false;
LARGE_INTEGER headLastT = {};
W headName[2] = {};
bool headNameReady = false;
std::atomic<const char*> gateWhy{""};
uintptr_t pairPawn{}, aimPawn{}, aimController{};
bool pairAimCandidate{}, pairViewValid{}, pairFollowsHead{};
head_aim::Context pairContext;
menu_state::Observation pairMode;
uint32_t pairGeneration{};
math::M pairView{};
head_aim::Tracker aimTracker;
struct AimReport {
    uint32_t generation{}, writes{}, failures{}, restores{};
    ULONGLONG lastWriteMs{};
    head_aim::Rotation body{}, requested{}, observed{}, before{};
    float head[4]{};
    int16_t stickX{}, stickY{}, rawY{}, moveX{}, moveY{};
    bool pitchInput{}, pitchBlocked{};
    bool active{};
    const char* reason{"not started"};
    menu_state::Observation mode;
};
SRWLOCK aimReportLock=SRWLOCK_INIT;
AimReport aimReport;
struct TraceSample { AimReport report; ULONGLONG at; };
std::array<TraceSample,256> aimTrace{};
unsigned traceWrite{},traceCount{};
ULONGLONG traceLastMs{};
// Caller holds aimReportLock. No disk IO in the game-thread pose update.
void trace_sample(bool force=false) {
    const auto now=GetTickCount64();
    if(!force && now-traceLastMs<50) return;
    aimTrace[traceWrite++%aimTrace.size()]={aimReport,now};
    if(traceCount<aimTrace.size()) ++traceCount;
    traceLastMs=now;
}
std::atomic<uint32_t> hudGen{}, hudFrames{}, hudFailures{};
std::atomic<bool> hudFollowing{};

void aim_state(const char* why, bool active=false) {
    AcquireSRWLockExclusive(&aimReportLock);
    const bool changed=aimReport.reason!=why || aimReport.active!=active;
    aimReport.reason=why; aimReport.active=active; aimReport.generation=pairGeneration;aimReport.mode=pairMode;
    trace_sample(changed);
    ReleaseSRWLockExclusive(&aimReportLock);
    if(changed) BVR_LOG("[aa head aim] %s; generation=%u",why,pairGeneration);
}

bool rd(uintptr_t p, void* out, size_t n) {
    __try { memcpy(out, reinterpret_cast<const void*>(p), n); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
uintptr_t word(uintptr_t p) { W v = 0; return rd(p, &v, 4) ? v : 0; }
bool object(uintptr_t p) { // a C++ object of the game (vtable in its image)
    if (p < 0x10000 || (p & 3)) return false;
    const uintptr_t vt = word(p);
    return vt >= base && vt < imageEnd;
}
bool finite3(const float* v) { return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]); }
void gate(const char* why) {
    if (!why) why = "";
    if (why == gateWhy.exchange(why)) return;
    BVR_LOG("[aa] first person %s%s", *why ? "paused: " : "", *why ? why : "active again");
}
// +0x60 is the Asylum Controller.Rotation read by GetViewRotation (BUILD_PLAN
// FIRSTPERSON). Use only the live pawn/controller backlink already validated
// by this module. No donor addresses, new hooks, or writes from the render thread.
bool controller_rotation(uintptr_t pawn, uintptr_t pc, head_aim::Rotation& r) {
    return pawnOff && object(pawn) && object(pc) && word(pawn+kPawnController)==pc && word(pc+pawnOff)==pawn &&
           rd(pc+0x60,&r,sizeof(r));
}
bool write_rotation(uintptr_t pc, const head_aim::Rotation& r) {
    __try { memcpy(reinterpret_cast<void*>(pc+0x60),&r,sizeof(r)); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void release_aim() {
    if (!aimController) return;
    head_aim::Rotation current{};
    bool restored=false;
    if(controller_rotation(aimPawn,aimController,current))
        restored=write_rotation(aimController,aimTracker.remove(current,pitch_input()));
    AcquireSRWLockExclusive(&aimReportLock);
    if(restored) ++aimReport.restores; else ++aimReport.failures;
    ReleaseSRWLockExclusive(&aimReportLock);
    BVR_LOG("[aa head aim] release controller=%08X restored=%u; only owned pitch/yaw contribution removed",unsigned(aimController),restored);
    aimController=aimPawn=0; aimTracker={};
}
bool drive_aim(math::M& view, const vr::HeadPose& head) {
    const uintptr_t pc=word(pairPawn+kPawnController);
    if(aimController && (pc!=aimController || pairPawn!=aimPawn)) release_aim();
    head_aim::Rotation current{};
    head_aim::Solution s{};
    const bool acceptPitch=pitch_input();
    if(!controller_rotation(pairPawn,pc,current) || !aimTracker.solve(current,head,s,acceptPitch)) {
        release_aim(); aim_state("invalid controller or headset pose"); return false;
    }
    if(!write_rotation(pc,s.target)) {
        // Best-effort rollback if an exceptional write crossed a bad page.
        write_rotation(pc,current);
        AcquireSRWLockExclusive(&aimReportLock); ++aimReport.failures; ReleaseSRWLockExclusive(&aimReportLock);
        release_aim(); aim_state("controller write failed"); return false;
    }
    head_aim::Rotation observed{};
    const bool readback=controller_rotation(pairPawn,pc,observed);
    if(!readback || memcmp(&observed,&s.target,sizeof(observed))) {
        write_rotation(pc,current);
        AcquireSRWLockExclusive(&aimReportLock); ++aimReport.failures; ReleaseSRWLockExclusive(&aimReportLock);
        release_aim(); aim_state("controller readback failed"); return false;
    }
    aimController=pc; aimPawn=pairPawn; aimTracker.commit(s);
    // The engine now owns head yaw/pitch for aiming. Rendering uses the body
    // basis and the existing FULL per-eye pose exactly once (including roll).
    view=s.view;
    const int16_t stickX=turnX.load(),stickY=deliveredPitch.load();
    AcquireSRWLockExclusive(&aimReportLock);
    const auto why=head_aim::reason(pairContext.evaluate(true,true));
    const bool changed=!aimReport.active||aimReport.reason!=why;
    aimReport.active=true;aimReport.reason=why;aimReport.generation=pairGeneration;aimReport.mode=pairMode;
    ++aimReport.writes; aimReport.lastWriteMs=GetTickCount64();
    aimReport.body=s.body; aimReport.requested=s.target; aimReport.observed=observed; aimReport.before=current;
    aimReport.head[0]=head.qx; aimReport.head[1]=head.qy; aimReport.head[2]=head.qz; aimReport.head[3]=head.qw;
    aimReport.stickX=stickX; aimReport.stickY=stickY;
    aimReport.rawY=rawPitch.load(); aimReport.moveX=moveX.load(); aimReport.moveY=moveY.load();
    aimReport.pitchInput=acceptPitch; aimReport.pitchBlocked=lookAim.load();
    trace_sample(changed);
    ReleaseSRWLockExclusive(&aimReportLock);
    if(changed)BVR_LOG("[aa head aim] %s; generation=%u",why,pairGeneration);
    return true;
}
uintptr_t pawn_of(uintptr_t pc, W off) {
    const uintptr_t q = word(pc + off);
    if (q == pc || !object(q) || word(q + kPawnController) != pc) return 0;
    float eh = 0;
    return rd(q + kPawnEyeHeight, &eh, 4) && std::isfinite(eh) && eh > -50.0f && eh < 300.0f ? q : 0;
}
uintptr_t find_pawn(uintptr_t player) {
    if (playerOff && pawnOff) {
        const uintptr_t pc = word(player + playerOff);
        if (object(pc)) if (const uintptr_t pawn = pawn_of(pc, pawnOff)) return pawn;
    }
    static ULONGLONG s_lastScan = 0; // no pawn (menus, some cutscenes): look again twice a second
    const ULONGLONG now = GetTickCount64();
    if (now - s_lastScan < 500) return 0;
    s_lastScan = now;
    for (W lo = kPlayerScanBegin; lo < kPlayerScanEnd; lo += 4) {
        const uintptr_t pc = word(player + lo);
        if (!object(pc)) continue;
        for (W po = kPCScanBegin; po < kPCScanEnd; po += 4)
            if (const uintptr_t pawn = pawn_of(pc, po)) {
                if (lo != playerOff || po != pawnOff)
                    BVR_LOG("[aa] first person: player +0x%X = controller %08X, its +0x%X = Batman (pawn %08X)", lo,
                            unsigned(pc), po, unsigned(pawn));
                playerOff = lo; pawnOff = po;
                return pawn;
            }
    }
    return 0;
}
bool head_name() {
    if (headNameReady) return true;
    __try { reinterpret_cast<FNameCtorFn>(base + kFNameCtor - 0x400000)(headName, L"Bip01_Head", 1, 1); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    headNameReady = true;
    BVR_LOG("[aa] first person head: name Bip01_Head = %u/%u", headName[0], headName[1]);
    return true;
}
bool bone(uintptr_t mesh, float* out) { // as the game: the bone's index, then its world matrix
    __try {
        void* m = reinterpret_cast<void*>(mesh);
        const int index = reinterpret_cast<MatchRefBoneFn>(base + kMatchRefBone - 0x400000)(m, headName[0], headName[1]);
        if (index < 0) return true; // out stays zero = no such bone
        // The engine's matrix multiply (0x58E560) stores with movaps: the out matrix MUST be 16-byte aligned, as the
        // game's own caller's is (0.1.3 passed an unaligned one: every read faulted, LIVE 2026-10-20 18:32).
        alignas(16) float matrix[16] = {};
        reinterpret_cast<GetBoneMatrixFn>(base + kGetBoneMatrix - 0x400000)(m, matrix, index);
        out[0] = matrix[12]; out[1] = matrix[13]; out[2] = matrix[14];
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    return true;
}
// HEADCAM (City): the eye point follows his head bone, smoothed (real time, so both eyes of a pair share it) and capped.
void follow_head(uintptr_t pawn) {
    if (!followHead.load(std::memory_order_relaxed)) { headOffValid = false; return; }
    auto miss = [](const char* why) {
        headOffValid = false;
        if (headMisses.fetch_add(1, std::memory_order_relaxed) < 6) BVR_LOG("[aa] first person head: %s - steady eye point", why);
    };
    const uintptr_t mesh = word(pawn + kPawnMesh);
    if (!object(mesh)) return miss("no mesh at pawn +0x348");
    if (!object(word(mesh + kMeshSkeleton))) return miss("mesh has no skeleton");
    if (!head_name()) return miss("Bip01_Head name failed");
    alignas(16) float head[4] = {};
    if (!bone(mesh, head)) return miss("bone read faulted");
    if (!finite3(head) || (head[0] == 0.0f && head[1] == 0.0f && head[2] == 0.0f)) return miss("no Bip01_Head bone");
    const float d[3] = {head[0] - eye[0], head[1] - eye[1], head[2] - eye[2]};
    if (fabsf(d[0]) > kHeadCapUU || fabsf(d[1]) > kHeadCapUU || fabsf(d[2]) > kHeadCapUU) {
        char b[96];
        snprintf(b, sizeof(b), "head %.0f, %.0f, %.0f UU from the eye point (cap %.0f)", d[0], d[1], d[2], kHeadCapUU);
        return miss(b);
    }
    LARGE_INTEGER now, f;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&f);
    const float dt = headLastT.QuadPart ? float(double(now.QuadPart - headLastT.QuadPart) / double(f.QuadPart)) : 0.0f;
    headLastT = now;
    const float tau = headSmoothS.load(std::memory_order_relaxed);
    const float a = !headOffValid || tau <= 0.001f ? 1.0f : 1.0f - expf(-(dt > 0.1f ? 0.1f : dt) / tau);
    float lag[3], lag2 = 0.0f;
    for (int i = 0; i < 3; ++i) { headOff[i] += (d[i] - headOff[i]) * a; lag[i] = d[i] - headOff[i]; lag2 += lag[i] * lag[i]; }
    if (lag2 > kHeadMaxLagUU * kHeadMaxLagUU) { // HEADLOCK: pull the smoothed point up to the real head
        const float k = 1.0f - kHeadMaxLagUU / sqrtf(lag2);
        for (int i = 0; i < 3; ++i) headOff[i] += lag[i] * k;
    }
    for (int i = 0; i < 3; ++i) eye[i] += headOff[i];
    headOffValid = true;
    if (headFrames.fetch_add(1, std::memory_order_relaxed) == 0)
        BVR_LOG("[aa] first person head: mesh %08X, Bip01_Head - eye point = %.1f, %.1f, %.1f UU", unsigned(mesh), d[0], d[1], d[2]);
}
void save() {
    if (!settings[0]) return;
    wchar_t v[32];
    WritePrivateProfileStringW(L"AsylumVR", L"FirstPerson", enabled.load() ? L"1" : L"0", settings);
    WritePrivateProfileStringW(L"AsylumVR", L"FirstPersonLookAim", lookAim.load() ? L"1" : L"0", settings);
    swprintf_s(v, L"%.1f", forwardCm.load()); WritePrivateProfileStringW(L"AsylumVR", L"FirstPersonForwardCm", v, settings);
    swprintf_s(v, L"%.1f", upCm.load()); WritePrivateProfileStringW(L"AsylumVR", L"FirstPersonUpCm", v, settings);
    swprintf_s(v, L"%.1f", leanMinCm.load()); WritePrivateProfileStringW(L"AsylumVR", L"FirstPersonLeanMinCm", v, settings);
    WritePrivateProfileStringW(L"AsylumVR", L"FirstPersonHead", followHead.load() ? L"1" : L"0", settings);
    swprintf_s(v, L"%.3f", headSmoothS.load()); WritePrivateProfileStringW(L"AsylumVR", L"FirstPersonHeadSmooth", v, settings);
    swprintf_s(v, L"%.1f", camRightCm.load()); WritePrivateProfileStringW(L"AsylumVR", L"CameraRightCm", v, settings);
    swprintf_s(v, L"%.1f", camUpCm.load()); WritePrivateProfileStringW(L"AsylumVR", L"CameraUpCm", v, settings);
    swprintf_s(v, L"%.1f", camFwdCm.load()); WritePrivateProfileStringW(L"AsylumVR", L"CameraForwardCm", v, settings);
}
bool cam_offset_set() {
    return camRightCm.load(std::memory_order_relaxed) != 0.0f || camUpCm.load(std::memory_order_relaxed) != 0.0f ||
           camFwdCm.load(std::memory_order_relaxed) != 0.0f;
}
// CAMPOS: the camera origin moved along the level right / level forward of the game camera and world up.
void cam_offset(float view[4][4], const float* o, float unitsPerMetre) {
    float rx = view[0][0], ry = view[1][0];
    float len = sqrtf(rx * rx + ry * ry);
    if (len < 1e-3f) return; // rolled on its side: no level heading
    rx /= len; ry /= len;
    float fx = view[0][2], fy = view[1][2];
    len = sqrtf(fx * fx + fy * fy);
    if (len > 1e-3f) { fx /= len; fy /= len; } else { fx = ry; fy = -rx; } // straight down / up: forward from right
    const float cm = unitsPerMetre * 0.01f, r = camRightCm.load(std::memory_order_relaxed) * cm,
                f = camFwdCm.load(std::memory_order_relaxed) * cm, u = camUpCm.load(std::memory_order_relaxed) * cm;
    const float n[3] = {o[0] + rx * r + fx * f, o[1] + ry * r + fy * f, o[2] + u};
    for (int j = 0; j < 3; ++j) view[3][j] = -(n[0] * view[0][j] + n[1] * view[1][j] + n[2] * view[2][j]);
    camFrames.fetch_add(1, std::memory_order_relaxed);
}
float get(const wchar_t* key, float fallback, float lo, float hi) {
    wchar_t t[32] = {};
    GetPrivateProfileStringW(L"AsylumVR", key, L"", t, 32, settings);
    if (!t[0]) return fallback;
    const float v = float(wcstod(t, nullptr));
    return std::isfinite(v) && v >= lo && v <= hi ? v : fallback;
}
} // namespace

void install(uintptr_t imageBase, const wchar_t* settingsPath) {
    base = imageBase;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(imageBase);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(imageBase + dos->e_lfanew);
    imageEnd = imageBase + nt->OptionalHeader.SizeOfImage;
    wcscpy_s(settings, settingsPath);
    enabled.store(GetPrivateProfileIntW(L"AsylumVR", L"FirstPerson", 0, settings) != 0);
    lookAim.store(GetPrivateProfileIntW(L"AsylumVR", L"FirstPersonLookAim", 1, settings) != 0);
    forwardCm.store(get(L"FirstPersonForwardCm", 25.0f, -20.0f, 80.0f));
    upCm.store(get(L"FirstPersonUpCm", 0.0f, -40.0f, 40.0f));
    leanMinCm.store(get(L"FirstPersonLeanMinCm", 15.0f, 0.0f, 40.0f));
    followHead.store(GetPrivateProfileIntW(L"AsylumVR", L"FirstPersonHead", 1, settings) != 0);
    headSmoothS.store(get(L"FirstPersonHeadSmooth", 0.08f, 0.0f, 0.5f));
    camRightCm.store(get(L"CameraRightCm", 0.0f, -300.0f, 300.0f));
    camUpCm.store(get(L"CameraUpCm", 0.0f, -300.0f, 300.0f));
    camFwdCm.store(get(L"CameraForwardCm", 0.0f, -300.0f, 300.0f));
    input::set_stick_filter(&filter_sticks);
    overlay::set_chord_double_tap(&toggle);
    BVR_LOG("[aa] first person %s; headset aim/HUD=%u; headset aim locks stick pitch; stick yaw remains additive (both sticks clicked twice quickly toggles first person); camera position right %.0f, up %.0f, forward %.0f cm",
            enabled.load() ? "ON" : "off", lookAim.load(), camRightCm.load(), camUpCm.load(), camFwdCm.load());
}

void begin_frame(bool nativeActive, const menu_state::Observation& mode, uint32_t generation) {
    pairMode=mode;pairGeneration=generation;
    pairContext={enabled.load(),lookAim.load(),nativeActive,input::game_pad_blocked(),mode.mode};
    const auto verdict=pairContext.evaluate(false,false);
    pairAimCandidate=verdict==head_aim::Gate::needPawn;
    pairViewValid=pairFollowsHead=false; eyeValid=false; camValid=false; pairPawn=0; fpPair=false;
    if(!pairContext.firstPerson || !nativeActive || pairContext.panel || mode.mode==menu_state::Mode::menu)
        pitchGateMs=0;
    if(!pairAimCandidate) {
        release_aim();
        aim_state(head_aim::reason(verdict));
    }
}

void update(void* localPlayer, const char* blocked) {
    eyeValid = false;
    camValid = false;
    const bool fp = enabled.load(std::memory_order_relaxed);
    if ((!fp && !cam_offset_set()) || !localPlayer) return;
    // FPGATE (City, JJ 2026-10-11: "must always be disabled when going into main menu ... or any other loading screen and
    // menus"): the setting stays on, the game's own camera is used while a menu is up. CAMPOS too.
    if (blocked) { if (fp) { pitchGateMs=0; release_aim(); gate(blocked); } return; }
    const uintptr_t pawn = find_pawn(reinterpret_cast<uintptr_t>(localPlayer));
    if (!pawn) { if (fp) { pitchGateMs=0; release_aim(); gate("no Batman to follow (menu or cutscene)"); aim_state("no pawn"); } return; }
    pairPawn=pawn;
    float loc[3] = {}, eh = 0;
    if (!rd(pawn + kPawnLocation, loc, 12) || !rd(pawn + kPawnEyeHeight, &eh, 4) || !finite3(loc) || !std::isfinite(eh)) {
        if (fp) { pitchGateMs=0; release_aim(); aim_state("invalid pawn position"); }
        return;
    }
    eye[0] = loc[0]; eye[1] = loc[1]; eye[2] = loc[2] + eh; // APawn::GetPawnViewLocation
    if (!fp) { camValid = true; return; } // CAMPOS: only the near-Batman check below needs him
    follow_head(pawn);
    eyeValid = true;
    static bool s_logged = false;
    if (!s_logged) { s_logged = true; BVR_LOG("[aa] first person: eye (%.1f, %.1f, %.1f) = location + eye height %.1f", eye[0], eye[1], eye[2], eh); }
}

// The view matrix: row vectors, v_view = (v - origin) * R, translation row = -origin * R; columns of R: right, up, forward.
bool apply(float view[4][4], float unitsPerMetre, const vr::HeadPose& head) {
    if(pairViewValid) { memcpy(view,&pairView,sizeof(pairView)); return pairFollowsHead; }
    if (!eyeValid && !camValid) { pitchGateMs=0; return false; }
    float o[3];
    for (int i = 0; i < 3; ++i) o[i] = -(view[3][0] * view[i][0] + view[3][1] * view[i][1] + view[3][2] * view[i][2]);
    const float dx = eye[0] - o[0], dy = eye[1] - o[1], dz = eye[2] - o[2];
    // FPGATE (City): the main menu has a stand-in pawn far from the menu scene's camera; third person sits ~2-4 m behind him.
    const bool closeCamera=sqrtf(dx * dx + dy * dy + dz * dz) < 12.0f * unitsPerMetre;
    if (!closeCamera) {
        if (eyeValid) { pitchGateMs=0; release_aim(); gate("game camera far from Batman"); aim_state("camera far from pawn"); }
        return false;
    }
    if (camValid) { cam_offset(view, o, unitsPerMetre); return false; } // CAMPOS: third person
    gate(nullptr);
    if(pairContext.native && !pairContext.panel && pairMode.mode!=menu_state::Mode::menu) pitchGateMs=GetTickCount64();
    if(pairAimCandidate && head_aim::permits(pairContext.evaluate(eyeValid&&pairPawn!=0,closeCamera))) {
        math::M body{};
        pairFollowsHead=drive_aim(body,head);
        if(pairFollowsHead) memcpy(view,&body,sizeof(body));
    }
    float fx = view[0][2], fy = view[1][2];
    const float len = sqrtf(fx * fx + fy * fy);
    if (len > 1e-3f) { fx /= len; fy /= len; } else { fx = fy = 0.0f; } // straight down / up: no forward push
    const float cm = unitsPerMetre * 0.01f, fwd = forwardCm.load(std::memory_order_relaxed) * cm;
    const float n[3] = {eye[0] + fx * fwd, eye[1] + fy * fwd, eye[2] + upCm.load(std::memory_order_relaxed) * cm};
    for (int j = 0; j < 3; ++j) view[3][j] = -(n[0] * view[0][j] + n[1] * view[1][j] + n[2] * view[2][j]);
    pairLevel[0] = fx; pairLevel[1] = fy; pairLevelValid = len > 1e-3f; fpPair = true; // LEANBACK
    frames.fetch_add(1, std::memory_order_relaxed);
    memcpy(&pairView,view,sizeof(pairView)); pairViewValid=pairFollowsHead;
    return pairFollowsHead;
}

bool clamp_eye(float o[3], float unitsPerMetre) {
    if (!fpPair || !eyeValid || !pairLevelValid) return false;
    const float minF = fminf(leanMinCm.load(std::memory_order_relaxed), forwardCm.load(std::memory_order_relaxed)) * unitsPerMetre * 0.01f;
    const float s = (o[0] - eye[0]) * pairLevel[0] + (o[1] - eye[1]) * pairLevel[1];
    if (!(s < minF)) return false;
    o[0] += (minF - s) * pairLevel[0]; o[1] += (minF - s) * pairLevel[1];
    leanClamps.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool on() { return enabled.load(); }
void set_on(bool v) {
    if(!v) pitchGateMs=0;
    if (enabled.exchange(v) != v) { BVR_LOG("[aa] first person %s", v ? "ON" : "off"); save(); }
}
void toggle() { set_on(!enabled.load()); } // both sticks clicked twice quickly (overlay thread)

void note_hud(uint32_t generation, bool followsHead, bool success) {
    hudGen=generation; hudFollowing=followsHead;
    if(success) ++hudFrames; else ++hudFailures;
}
bool save_trace(const wchar_t* directory) {
    std::vector<TraceSample> samples; samples.reserve(aimTrace.size());
    AcquireSRWLockShared(&aimReportLock);
    for(unsigned n=0;n<traceCount;++n) samples.push_back(aimTrace[(traceWrite-traceCount+n)%aimTrace.size()]);
    ReleaseSRWLockShared(&aimReportLock);
    wchar_t path[MAX_PATH];swprintf_s(path,L"%s\\head_aim.csv",directory);
    FILE* file=nullptr;
    if(_wfopen_s(&file,path,L"wt")!=0 || !file){BVR_LOG("[aa head aim] trace open failed: %ls",path);return false;}
    bool ok=fprintf(file,"time_ms,generation,active,reason,menu_mode,menu_read,viewport_count,viewport_capacity,body_pitch,body_yaw,target_pitch,target_yaw,before_pitch,before_yaw,readback_pitch,readback_yaw,head_qx,head_qy,head_qz,head_qw,stick_rx,stick_ry,writes,failures,restores,raw_ry,move_lx,move_ly,pitch_input,block_stick_pitch\n")>=0;
    constexpr float deg=360.f/65536;
    for(const auto&sample:samples){const auto&r=sample.report;
        if(fprintf(file,"%llu,%u,%u,%s,%s,%s,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.6f,%.6f,%.6f,%.6f,%d,%d,%u,%u,%u,%d,%d,%d,%u,%u\n",
            sample.at,r.generation,unsigned(r.active),r.reason,
            r.mode.mode==menu_state::Mode::gameplay?"gameplay":r.mode.mode==menu_state::Mode::menu?"menu":"unknown",r.mode.reason,r.mode.count,r.mode.capacity,
            head_aim::wrap(r.body.pitch)*deg,head_aim::wrap(r.body.yaw)*deg,
            head_aim::wrap(r.requested.pitch)*deg,head_aim::wrap(r.requested.yaw)*deg,
            head_aim::wrap(r.before.pitch)*deg,head_aim::wrap(r.before.yaw)*deg,
            head_aim::wrap(r.observed.pitch)*deg,head_aim::wrap(r.observed.yaw)*deg,
            r.head[0],r.head[1],r.head[2],r.head[3],int(r.stickX),int(r.stickY),r.writes,r.failures,r.restores,int(r.rawY),int(r.moveX),int(r.moveY),unsigned(r.pitchInput),unsigned(r.pitchBlocked))<0)ok=false;
    }
    if(fclose(file)!=0)ok=false;
    BVR_LOG("[aa head aim] trace samples=%u saved=%u path=%ls",unsigned(samples.size()),unsigned(ok),path);
    return ok;
}
void status_line(char* out, size_t size) {
    AcquireSRWLockShared(&aimReportLock); const auto r=aimReport; ReleaseSRWLockShared(&aimReportLock);
    constexpr float degrees=360.f/65536;
    snprintf(out,size,"requested=%u enabled=%u active=%u reason=%s menuMode=%s menuRead=%s viewportCountCap=%u/%u gen=%u writes=%u failures=%u restores=%u ageMs=%llu "
        "bodyPitchYaw=%.2f/%.2f targetPitchYaw=%.2f/%.2f beforePitchYaw=%.2f/%.2f readbackPitchYaw=%.2f/%.2f "
        "headQuat=%.4f/%.4f/%.4f/%.4f stickRXRY=%d/%d hudGen=%u hudFollowing=%u hudComposites=%u hudFailures=%u "
        "fpFrames=%u headFrames=%u headMisses=%u blockStickPitch=%u rawRY=%d deliveredRY=%d moveXY=%d/%d pitchInput=%u",
        unsigned(lookAim.load()),unsigned(enabled.load()),unsigned(r.active),r.reason,
        r.mode.mode==menu_state::Mode::gameplay?"gameplay":r.mode.mode==menu_state::Mode::menu?"menu":"unknown",r.mode.reason,r.mode.count,r.mode.capacity,
        r.generation,r.writes,r.failures,r.restores,
        r.lastWriteMs?GetTickCount64()-r.lastWriteMs:0ULL,
        head_aim::wrap(r.body.pitch)*degrees,head_aim::wrap(r.body.yaw)*degrees,
        head_aim::wrap(r.requested.pitch)*degrees,head_aim::wrap(r.requested.yaw)*degrees,
        head_aim::wrap(r.before.pitch)*degrees,head_aim::wrap(r.before.yaw)*degrees,
        head_aim::wrap(r.observed.pitch)*degrees,head_aim::wrap(r.observed.yaw)*degrees,
        r.head[0],r.head[1],r.head[2],r.head[3],int(r.stickX),int(r.stickY),hudGen.load(),unsigned(hudFollowing.load()),hudFrames.load(),hudFailures.load(),
        frames.load(),headFrames.load(),headMisses.load(),unsigned(lookAim.load()),int(rawPitch.load()),int(deliveredPitch.load()),int(moveX.load()),int(moveY.load()),unsigned(pitch_input()));
}

void draw_ui() { // at the top of the panel: it changes the whole view (City)
    bool fp = enabled.load();
    if (ImGui::Checkbox("FIRST PERSON  (or click both sticks twice quickly)", &fp)) set_on(fp);
    if (!fp) return;
    bool aim=lookAim.load();
    if(ImGui::Checkbox("   aim and HUD follow the headset",&aim)) { lookAim=aim; save(); }
    if(aim)ImGui::TextDisabled("   Headset controls up / down; right stick turns left / right.");
    float f = forwardCm.load(), u = upCm.load();
    if (overlay::SliderStep("   camera in front of his face (cm)", &f, -20.0f, 80.0f, "%.0f")) { forwardCm.store(f); save(); }
    if (overlay::SliderStep("   camera up / down (cm)", &u, -40.0f, 40.0f, "%.0f")) { upCm.store(u); save(); }
    float lb = leanMinCm.load();
    if (overlay::SliderStep("   closest to his face when you lean back (cm)", &lb, 0.0f, 40.0f, "%.0f")) { leanMinCm.store(lb); save(); }
    bool hf = followHead.load();
    if (ImGui::Checkbox("   camera follows his head (gliding, animations)", &hf)) { followHead.store(hf); save(); }
    if (hf) {
        float sm = headSmoothS.load();
        if (overlay::SliderStep("   head smoothing (s) - higher = steadier, lags more", &sm, 0.0f, 0.5f, "%.2f")) { headSmoothS.store(sm); save(); }
    }
    const char* why=gateWhy.load();
    if (scene::test_options()) {
        ImGui::TextDisabled("      frames %u, head frames %u, steady eye point used %u, lean-back held %u%s%s", frames.load(), headFrames.load(),
                            headMisses.load(), leanClamps.load(), *why ? ", paused: " : "", why);
        char report[1024]; status_line(report,sizeof(report)); ImGui::TextWrapped("%s",report);
    }
}

void draw_camera_ui() { // CAMPOS: VIEW section, open the first time (AKVR "camera position")
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (!ImGui::TreeNode("camera position  (third person)")) return;
    if (enabled.load()) ImGui::TextDisabled("not used in first person");
    float r = camRightCm.load(), u = camUpCm.load(), f = camFwdCm.load();
    if (overlay::SliderStep("left / right (cm)", &r, -300.0f, 300.0f, "%.0f")) { camRightCm.store(r); save(); }
    if (overlay::SliderStep("down / up (cm)", &u, -300.0f, 300.0f, "%.0f")) { camUpCm.store(u); save(); }
    if (overlay::SliderStep("back / forward (cm)", &f, -300.0f, 300.0f, "%.0f")) { camFwdCm.store(f); save(); }
    if (ImGui::Button("Reset position")) { camRightCm.store(0.0f); camUpCm.store(0.0f); camFwdCm.store(0.0f); save(); }
    if (scene::test_options()) ImGui::TextDisabled("      moved frames %u", camFrames.load());
    ImGui::TreePop();
}

bool command(const char* a) {
    if (!strcmp(a, "on")) set_on(true);
    else if (!strcmp(a, "off")) set_on(false);
    else if (!strncmp(a, "fwd ", 4)) { forwardCm.store(float(atof(a + 4))); save(); }
    else if (!strncmp(a, "up ", 3)) { upCm.store(float(atof(a + 3))); save(); }
    else if (!strncmp(a, "lean ", 5)) { leanMinCm.store(fminf(fmaxf(float(atof(a + 5)), 0.0f), 40.0f)); save(); } // LEANBACK (cm)
    else if (!strncmp(a, "head ", 5)) { followHead.store(atoi(a + 5) != 0); save(); }
    else if (!strcmp(a, "aim on")) { lookAim=true; save(); }
    else if (!strcmp(a, "aim off")) { lookAim=false; save(); }
    else if (!strncmp(a, "smooth ", 7)) { headSmoothS.store(float(atof(a + 7))); save(); }
    else if (!strncmp(a, "cam ", 4)) { // CAMPOS: "fp cam <right> <up> <forward>" (cm)
        float r = 0, u = 0, f = 0;
        if (sscanf_s(a + 4, "%f %f %f", &r, &u, &f) != 3) return false;
        camRightCm.store(r); camUpCm.store(u); camFwdCm.store(f); save();
        BVR_LOG("[aa] camera position right %.0f, up %.0f, forward %.0f cm (moved frames %u)", r, u, f, camFrames.load());
        return true;
    }
    else if (strcmp(a, "status")) return false;
    char report[1024]; status_line(report,sizeof(report)); BVR_LOG("[aa head aim] %s",report);
    return true;
}

} // namespace bvr::aa::first_person
