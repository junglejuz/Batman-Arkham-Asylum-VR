#include "game/igame_adapter.h"
#include "scene.h"
#include "d3d9_bridge.h"
#include "native_stereo.h"
#include "billboard9.h"
#include "transitions9.h"
#include "markers9.h"
#include "eyesame9.h"
#include "xform9.h"
#include "fxstep9.h"
#include "startup.h"
#include "hud9.h"
#include "dlss.h"
#include "afw.h"
#include "window.h"
#include "detail9.h"
#include "first_person.h"
#include "core/ui/overlay.h"
#include "core/input/xinput_bridge.h"
#include "core/framework/command.h"
#include "core/input/dinput_block.h"
#include "core/util/log.h"
#include "bvr_version.h"
#include "core/gfx/gfx_hud.h"
#include "core/gfx/stereo_history.h"
#include "core/vr/openxr_runtime.h"
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>
#include <cwchar>
#include <cstdio>
namespace bvr::aa::scene {
namespace {
bool captureRequested=false;
ULONGLONG captureDepthAt{};
float actualPairFps{};
int compareStep=-1,compareSavedMode{};bool compareSavedLens{},compareSavedAFW{},compareSavedOverlap{},compareSavedTracking{},compareWarp{},compareDLSS{},compareTransfer{},compareTracking{};ULONGLONG compareNext{},compareAt{};unsigned comparePairs{};
void compare_start(bool warp=false,bool upscale=false,bool transfer=false,bool tracking=false){
 if(compareStep>=0||(transfer&&(!dlss::mode()||!native::selected()))||(tracking&&!native::selected()))return;
 compareSavedMode=dlss::mode();compareSavedLens=native::centred();compareSavedAFW=afw::on();compareSavedOverlap=dlss::overlap();compareSavedTracking=hud9::fast_tracking();compareWarp=warp;compareDLSS=upscale;compareTransfer=transfer;compareTracking=tracking;compareStep=0;afw::set_on(false);
 dlss::set_mode(warp||transfer||tracking?compareSavedMode:0,false);if(transfer)dlss::set_overlap(false,false);if(tracking)hud9::set_fast_tracking(false,false);native::set_centred(true);
 compareAt=GetTickCount64();comparePairs=native::completed_pairs();compareNext=compareAt+(transfer||tracking?8000:4000);overlay::set_visible(false);
 BVR_LOG("[aa compare] step 1: native baseline; transfer=%u tracking=%u mode=%d; capture after %us",transfer,tracking,dlss::mode(),transfer||tracking?8:4);
}
void compare_tick(){
 if(compareStep<0||compareStep>=10||GetTickCount64()<compareNext||native::present_eye()>=0)return;
 captureRequested=true;
 // Advance on the next LEFT after its capture has actually completed.
}
void capture(IDXGISwapChain* sc) {
    using Microsoft::WRL::ComPtr;
    captureRequested=false;
    bridge::request_depth_capture(false);captureDepthAt=0;
    wchar_t root[MAX_PATH],dir[MAX_PATH],file[MAX_PATH];
    swprintf_s(root,L"%s\\captures",log::data_dir());CreateDirectoryW(root,nullptr);
    SYSTEMTIME now{};GetLocalTime(&now);
    swprintf_s(dir,L"%s\\A2p_%04u%02u%02u_%02u%02u%02u_%03u",root,now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,now.wMilliseconds);
    if(!CreateDirectoryW(dir,nullptr)) {BVR_LOG("[aa capture] directory failed %lu",GetLastError());return;}
    ComPtr<ID3D11Device> dev;ComPtr<ID3D11DeviceContext> ctx;ComPtr<ID3D11Texture2D> image;
    if(FAILED(sc->GetDevice(IID_PPV_ARGS(&dev))) || FAILED(sc->GetBuffer(0,IID_PPV_ARGS(&image))))return;
    dev->GetImmediateContext(&ctx);D3D11_TEXTURE2D_DESC d{};image->GetDesc(&d);
    swprintf_s(file,L"%s\\game_frame.bmp",dir);
    const bool saved=gfx_hud::save_texture_bmp(ctx.Get(),image.Get(),file);
    swprintf_s(file,L"%s\\hud_sheet.bmp",dir);bridge::save_hud(file);
    swprintf_s(file,L"%s\\depth_scene.bmp",dir);bridge::save_depth(file);
    swprintf_s(file,L"%s\\status.txt",dir);FILE* f=nullptr;
    if(_wfopen_s(&f,file,L"wt")==0 && f) {
        char dlssStatus[512]{};dlss::status_line(dlssStatus,sizeof(dlssStatus));
        fprintf(f,"Asylum A2p; nativeSelected=%u presentEye=%d completedPairs=%u; centredScene=%u; AFW=%u\nrendering=%s\nsource=%ux%u format=%u sourceSaved=%u\npreview=%s\ndlss=%s\ncompareStep=%d; transferComparison=%u; trackingComparison=%u; sceneTracking=%u; completedPairFps=%.2f\n",native::selected(),native::present_eye(),native::completed_pairs(),native::centred(),native::afw_present(),native::rendering_status(),d.Width,d.Height,d.Format,saved,vr::flat_preview()==2?"full":vr::flat_preview()==1?"fit":"window",dlssStatus,compareStep,compareTransfer,compareTracking,hud9::fast_tracking(),actualPairFps);
        char particleStatus[320]{};billboard9::status_line(particleStatus,sizeof(particleStatus));fprintf(f,"particles=%s\n",particleStatus);
        char transitionStatus[320]{};transitions9::status_line(transitionStatus,sizeof(transitionStatus));fprintf(f,"transitions=%s\n",transitionStatus);char markerStatus[640]{};markers9::status_line(markerStatus,sizeof(markerStatus));fprintf(f,"markers=%s\n",markerStatus);
        fclose(f);
    }
    vr::save_eye_pictures(dir);
    BVR_LOG("[aa capture] source %ux%u saved=%u; paired composed-eye capture armed in %ls",d.Width,d.Height,saved,dir);
}
}
bool install() {
    command::enable_present_pump();
    window::install();afw::install();bridge::init_hud_settings();
    input::dinput_block::install();
    install_render_anchors();
    native::install();
    dlss::init_settings();
    startup::install();
    input::set_enabled(true);
    vr::set_screen_room(true);
    vr::set_flat_preview(1);
    const auto* x=wcsstr(GetCommandLineW(),L"-AAOutputX=");
    const auto* y=wcsstr(GetCommandLineW(),L"-AAOutputY=");
    if(x && y) {
        const auto w=wcstoul(x+11,nullptr,10),h=wcstoul(y+11,nullptr,10);
        if(w>=512 && h>=512 && w<=8192 && h<=8192)vr::set_output_size(w,h);
    }
    BVR_LOG("[aa] A2p active; legacy lighting compatibility, exact XR lens remap and actual per-eye DLSS; Quality requested by default");
    return true;
}
void on_present(IDXGISwapChain* sc) {
    // Every capture (panel button, F2, command) also runs the eye-same draw finder.
    static const bool historyListener=(stereo_history::set_save_listener([]{eyesame9::request();}),true);(void)historyListener;
    native::on_present(sc);
    static ULONGLONG next=0;const auto now=GetTickCount64();
    static ULONGLONG fpsAt{};static unsigned fpsPairs{};
    if(now-fpsAt>=1000){const auto pairs=native::completed_pairs();if(fpsAt)actualPairFps=1000.f*(pairs-fpsPairs)/float(now-fpsAt);fpsAt=now;fpsPairs=pairs;}
    static bool wasF2{};const SHORT keyF2=GetAsyncKeyState(VK_F2);const bool f2=(keyF2&0x8000)!=0;
    if((f2&&!wasF2)||(keyF2&1)){DWORD foreground{};GetWindowThreadProcessId(GetForegroundWindow(),&foreground);const bool xrFocus=!strcmp(vr::session_state_name(),"FOCUSED");
        if(foreground==GetCurrentProcessId()||xrFocus){stereo_history::request_save();captureRequested=true;eyesame9::request();BVR_LOG("[aa capture] F2 accepted: desktopFocus=%u headsetFocus=%u; still AND preceding history",foreground==GetCurrentProcessId(),xrFocus);}
        else BVR_LOG("[aa capture] F2 ignored: game has neither desktop nor headset focus");}wasF2=f2;
    if(now>=next) {next=now+5000;render_anchor_status();bridge::status();native::status();hud9::status();transitions9::status();char markers[640]{};markers9::status_line(markers,sizeof(markers));BVR_LOG("[aa markers] %s",markers);char xform[160]{};xform9::status_line(xform,sizeof(xform));BVR_LOG("[aa xform] %s",xform);dlss::status();window::status();afw::command("status");}
    compare_tick();
    // Two bounded captures after ordinary rendering starts, by elapsed time.
    // No game launch automation, no VR-mode decision from frame counts.
    // Test tool, off in the package (PACKAGING_GUIDE 12): [AsylumVR] AutoCaptures=1 in aa_rendering.ini.
    static const bool autoCaptures=[]{wchar_t ini[MAX_PATH];swprintf_s(ini,L"%s\\aa_rendering.ini",log::data_dir());return GetPrivateProfileIntW(L"AsylumVR",L"AutoCaptures",0,ini)!=0;}();
    static ULONGLONG started=0;static unsigned captured=0;
    if(autoCaptures && vr::ever_focused() && ordinary_render_seen()) {
        if(!started)started=now;
        if(captured<2 && now-started>=(captured?30000:2000)) {++captured;captureRequested=true;}
    }
    // Also preserve the first real paired scene. A flat title capture alone
    // cannot demonstrate whether the native menu/world route activated.
    static bool nativeCaptured=false;
    if(autoCaptures&&!nativeCaptured&&native::completed_pairs()&&native::present_eye()<0){nativeCaptured=true;captureRequested=true;}
    // In native mode start on LEFT so the saved eyes belong to one pair.
    // A request arriving on RIGHT waits for the next left or untagged frame.
    if(captureRequested&&native::present_eye()<=0){
        // Still captures retain physical depth on demand. Native stock frames
        // otherwise never pay for an unused cross-API depth copy.
        if(native::present_eye()<0&&!bridge::depth_ready()){
            if(!captureDepthAt){captureDepthAt=now;bridge::request_depth_capture(true);}
            if(now-captureDepthAt<1500)return;
        }
        if((compareDLSS||compareWarp||compareTransfer||compareTracking)&&compareStep>=0&&compareStep<10&&now>=compareNext)stereo_history::request_save();
        if((compareTransfer||compareTracking)&&compareStep>=0&&compareStep<10&&now>=compareNext){
            const auto elapsed=now-compareAt;const auto pairs=native::completed_pairs()-comparePairs;
            BVR_LOG("[aa performance compare] phase %d transfer=%u tracking=%u sceneCache=%u overlap=%u DLSS mode=%d: %u pairs / %llu ms = %.2f fps",compareStep+1,compareTransfer,compareTracking,hud9::fast_tracking(),dlss::overlap(),dlss::mode(),pairs,elapsed,elapsed?1000.*pairs/elapsed:0);dlss::status();hud9::status();
        }
        capture(sc);
        if(compareStep>=0&&compareStep<10&&now>=compareNext){
            // Leave one pair for save_eye_pictures before switching lenses.
            compareNext=now+1000;compareStep+=10;
        }
    }
    if(compareStep>=10&&now>=compareNext&&native::present_eye()>0){
        if(compareStep==10){compareStep=1;if(compareTransfer)dlss::set_overlap(true,false);else if(compareTracking)hud9::set_fast_tracking(true,false);else if(compareWarp)afw::set_on(true);else if(compareDLSS)dlss::set_mode(compareSavedMode?compareSavedMode:3,false);else native::set_centred(false);compareAt=now;comparePairs=native::completed_pairs();compareNext=now+(compareTransfer||compareTracking?8000:4000);BVR_LOG("[aa compare] step 2: alternate path, AFW=%u DLSS comparison=%u transfer=%u tracking=%u",compareWarp,compareDLSS,compareTransfer,compareTracking);}
        else if(compareStep==11){compareStep=2;afw::set_on(false);native::set_centred(true);if(compareTransfer)dlss::set_overlap(false,false);else if(compareTracking)hud9::set_fast_tracking(false,false);else dlss::set_mode(compareDLSS?0:compareSavedMode,false);compareAt=now;comparePairs=native::completed_pairs();compareNext=now+8000;BVR_LOG("[aa compare] step 3: native baseline, DLSS mode %d",dlss::mode());}
        else{compareStep=-1;afw::set_on(compareSavedAFW);native::set_centred(compareSavedLens);dlss::set_mode(compareSavedMode,false);if(compareTransfer)dlss::set_overlap(compareSavedOverlap,false);if(compareTracking)hud9::set_fast_tracking(compareSavedTracking,false);overlay::set_visible(true);BVR_LOG("[aa compare] finished, settings restored");}
    }
}
bool testOptions=false;
bool test_options(){return testOptions;}
void draw_ui() {
    ImGui::TextUnformatted("Batman Asylum VR 0.1.5");
    ImGui::TextWrapped("F10 or hold both sticks to open/close. Settings save automatically.");
    stereo_history::draw_ui();
    if(ImGui::Button("Recentre view and screens")){vr::recenter_panel();native::recenter();bridge::recenter_hud();}
    first_person::draw_ui(); // FIRSTPERSON: near the top, it changes the whole view (City)
    if(ImGui::CollapsingHeader("VR RESOLUTION",ImGuiTreeNodeFlags_DefaultOpen))window::draw_ui();
    if(ImGui::CollapsingHeader("STEREO RENDERING",ImGuiTreeNodeFlags_DefaultOpen)){
        if(ImGui::RadioButton("Native (both eyes drawn every frame)",native::selected()&&!afw::on())){afw::set_on(false);native::select(true);}
        if(ImGui::RadioButton("AFW (one eye drawn, the other warped)",native::selected()&&afw::on())){native::select(true);afw::set_on(true);}
        ImGui::TextWrapped("Rendering: %s",native::rendering_status());
        if(afw::on())afw::draw_ui();
    }
    if(ImGui::CollapsingHeader("VIEW")){
        bool compatible=native::centred();
        if(ImGui::RadioButton("Full view, safe lighting (recommended)",compatible))native::set_centred(true);
        if(ImGui::RadioButton("Full view, exact lens (some lights may go missing in one eye)",!compatible))native::set_centred(false);
    }
    if(ImGui::CollapsingHeader("HUD"))bridge::draw_hud_ui();
    if(ImGui::CollapsingHeader("MENUS AND SCREENS")){
        const int m=vr::flat_preview();
        if(ImGui::RadioButton("Fit floating screen (whole image)",m==1))vr::set_flat_preview(1);
        if(ImGui::RadioButton("Fill headset view",m==2))vr::set_flat_preview(2);
    }
    if(ImGui::CollapsingHeader("DLSS (SMOOTHER EDGES)")){dlss::draw_ui();detail9::draw_ui();}
    if(ImGui::CollapsingHeader("FRAME RATE")){ImGui::Text("Completed stereo frames %.1f fps",actualPairFps);ImGui::TextWrapped("New stereo frames per second. Native draws both eyes; AFW draws one and warps the other.");}
    // PACKAGING_GUIDE 12: tests and diagnostics behind one switch, off at every start.
    ImGui::Separator();
    ImGui::Checkbox("Show test options (only when Claude asks)",&testOptions);
    if(testOptions&&ImGui::CollapsingHeader("TESTS (only when Claude asks)",ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::TextUnformatted("Build " BVR_BUILD_ID);
        ImGui::TextWrapped("Look at the problem object and keep still. Hides each kind of 3D drawing in turn for a quarter second (about 10 s), then saves the replay.");
        ImGui::BeginDisabled(fxstep9::running());if(ImGui::Button("Hide each effect in turn"))fxstep9::request();ImGui::EndDisabled();
        if(ImGui::Button("Capture full-resolution still"))captureRequested=true;
        ImGui::BeginDisabled(compareStep>=0);
        if(ImGui::Button("Compare native and AFW"))compare_start(true);
        if(ImGui::Button("Compare stock and DLSS"))compare_start(false,true);
        ImGui::BeginDisabled(!dlss::mode()||!native::selected());if(ImGui::Button("Compare DLSS transfer speed"))compare_start(false,false,true);ImGui::EndDisabled();
        ImGui::BeginDisabled(!native::selected());if(ImGui::Button("Compare scene tracking speed"))compare_start(false,false,false,true);ImGui::EndDisabled();
        if(ImGui::Button("Compare tower shading"))compare_start();
        ImGui::EndDisabled();
        if(compareStep>=0)ImGui::TextUnformatted("Comparison running; pictures save automatically.");
        bridge::draw_ui();native::draw_ui();transitions9::draw_ui();markers9::draw_ui();hud9::draw_ui();
    }
}

bool handle_command(const char* args) {
    if(!_stricmp(args,"preview full"))vr::set_flat_preview(2);
    else if(!_stricmp(args,"preview fit"))vr::set_flat_preview(1);
    else if(!_stricmp(args,"preview window")) {vr::set_flat_preview(0);vr::recenter_panel();}
    else if(!_stricmp(args,"capture"))captureRequested=true;
    else if(!_stricmp(args,"capture history"))stereo_history::request_save();
    else if(!_stricmp(args,"recenter")) {vr::recenter_panel();native::recenter();bridge::recenter_hud();}
    else if(!_stricmp(args,"native on"))native::select(true);
    else if(!_stricmp(args,"native off"))native::select(false);
    else if(!_stricmp(args,"lens centred"))native::set_centred(true);
    else if(!_stricmp(args,"lens asymmetric"))native::set_centred(false);
    else if(!_stricmp(args,"shadow compare"))compare_start();
    else if(!_strnicmp(args,"dlss mode ",10))dlss::set_mode(atoi(args+10));
    else if(!_stricmp(args,"dlss overlap on"))dlss::set_overlap(true);
    else if(!_stricmp(args,"dlss overlap off"))dlss::set_overlap(false);
    else if(!_stricmp(args,"dlss transfer compare"))compare_start(false,false,true);
    else if(!_stricmp(args,"tracking compare"))compare_start(false,false,false,true);
    else if(!_stricmp(args,"tracking on"))hud9::set_fast_tracking(true);
    else if(!_stricmp(args,"tracking off"))hud9::set_fast_tracking(false);
    else if(!_stricmp(args,"dlss reset"))dlss::reset();
    else if(!_strnicmp(args,"afw ",4))return afw::command(args+4);
    else if(!_strnicmp(args,"fp ",3))return first_person::command(args+3);
    else if(!_strnicmp(args,"grapple ",8))return markers9::command(args+8);
    else if(!_stricmp(args,"eyesame"))eyesame9::request();
    else if(!_strnicmp(args,"stale ",6))return xform9::command(args+6);
    else if(!_stricmp(args,"fxstep"))fxstep9::request();
    else if(!_strnicmp(args,"resolution ",11))return window::save_scale(float(atof(args+11)));
    else return false;
    return true;
}
}
namespace bvr::aa {
class AsylumAdapter final : public game::IGameAdapter {
public:
    uint32_t capabilities() const override { return 0; }
    bool init(const pattern_scan::ProcessImage&) override { return scene::install(); }
    void setFov(float) override {}
    void drawDebugUi() override { scene::draw_ui(); }
    bool handleCommand(const char* cmd,const char* args) override {
        if(_stricmp(cmd,"aa"))return false;
        if(scene::handle_command(args))return true;
        if(_stricmp(args,"status"))return false;
        native::status();scene::render_anchor_status();return true;
    }
};
game::IGameAdapter* create_adapter() {static AsylumAdapter a;return &a;}
}

extern "C" __declspec(dllexport) DWORD WINAPI AA_PanelSelftest(){
    // Actual production panel and ImGui navigation, without a window/GPU or
    // game input. Small viewport forces scrolling even with default drawers.
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.DisplaySize={640,450};io.DeltaTime=1.f/90;
    io.ConfigFlags=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
    unsigned char* pixels{};int w{},h{};io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    bool first=true;float scroll{},maximum{};ImGuiID firstId{},lastId{};
    auto frame=[&](ImGuiKey key=ImGuiKey_None,bool down=false){
        if(key!=ImGuiKey_None)io.AddKeyEvent(key,down);
        ImGui::NewFrame();ImGui::SetNextWindowSize(io.DisplaySize,ImGuiCond_Always);
        ImGui::Begin("Asylum navigation regression",nullptr,ImGuiWindowFlags_NoSavedSettings);
        if(first){ImGui::SetWindowFocus();ImGui::SetKeyboardFocusHere();first=false;}
        firstId=ImGui::GetID("Save last 10 seconds (stereo)");
        ImGui::PushTextWrapPos();bvr::aa::scene::draw_ui();ImGui::PopTextWrapPos();
        lastId=ImGui::GetID("Show test options (only when Claude asks)");scroll=ImGui::GetScrollY();maximum=ImGui::GetScrollMaxY();
        ImGui::End();ImGui::Render();
    };
    bool ok=true;
    for(bool afw:{false,true}){
        bvr::aa::afw::set_on(afw);first=true;
        for(int i=0;i<5;++i)frame();
        for(int i=0;i<70;++i){frame(ImGuiKey_GamepadDpadDown,true);frame(ImGuiKey_GamepadDpadDown,false);frame();}
        const bool bottom=ImGui::GetCurrentContext()->NavId==lastId&&scroll>0&&maximum>0;
        for(int i=0;i<70;++i){frame(ImGuiKey_GamepadDpadUp,true);frame(ImGuiKey_GamepadDpadUp,false);frame();}
        const bool top=ImGui::GetCurrentContext()->NavId==firstId&&scroll<maximum;
        const bool stable=bvr::aa::afw::on()==afw;
        printf("Asylum panel gamepad: %s reaches bottom %u, returns to first control %u, mode unchanged %u %s\n",afw?"AFW":"native",bottom,top,stable,bottom&&top&&stable?"PASS":"FAIL");
        ok=bottom&&top&&stable&&ok;
    }
    bvr::aa::afw::set_on(false);ImGui::DestroyContext();return ok?0:1;
}
