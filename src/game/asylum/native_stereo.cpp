// First Asylum-specific native integration. See RE_NOTES and ABI evidence.
// UE3 FViewport::Draw: ECX receiver, zero incoming stack words (bare ret).
// ULocalPlayer::CalcSceneView: five stack words (ret 14h).
// FSceneView ctor: TWENTY-FOUR stack words (ret 60h), unlike Origins.
#include "native_stereo.h"
#include "present_sequence.h"
#include "stereo_math.h"
#include "hud9.h"
#include "dlss.h"
#include "afw.h"
#include "window.h"
#include "detail9.h"
#include "billboard9.h"
#include "markers9.h"
#include "first_person.h"
#include "core/vr/openxr_runtime.h"
#include "core/util/log.h"
#include "core/ui/overlay.h"
#include <cstdlib>
#include <windows.h>
#include <dxgi.h>
#include <MinHook.h>
#include <intrin.h>
#include <imgui.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
namespace bvr::aa::native {
namespace {
using W=uint32_t;
using math::M;using math::mul;
#define A24 W a1,W a2,W a3,W a4,W a5,W a6,W a7,W a8,W a9,W a10,W a11,W a12,W a13,W a14,W a15,W a16,W a17,W a18,W a19,W a20,W a21,W a22,W a23,W a24
#define P24 a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24
using Draw=void(__fastcall*)(void*,void*);
using Calc=void*(__fastcall*)(void*,void*,void*,float*,int32_t*,void*,W);
using Ctor=void*(__fastcall*)(void*,void*,A24);
using Alloc=void*(__cdecl*)();
Draw drawOriginal{};Calc calcOriginal{};Ctor ctorOriginal{};
uintptr_t base{};
std::atomic<bool> ready{false},wantNative{true},failed{false},recentre{true};
std::atomic<bool> centredLens{true};
std::atomic<float> worldScale{1.f};
thread_local float pairUnits=90.f;
thread_local bool pairCentred{};
thread_local bool pairAFW{};thread_local int afwNext{};
thread_local float pairJitterX{},pairJitterY{};
thread_local unsigned pairInputWidth{},pairInputHeight{};
Camera renderedCamera{};
std::atomic<W> width{},height{},owner{};
std::atomic<unsigned> draws{},pairs{},edits{},invalid{},misses{},baseChanged{};
std::atomic<unsigned> completed{}, poseUnavailable{}, flatDraws{};
std::atomic<ULONGLONG> lastPresent{};
std::atomic<const char*> failureReason{"No validation failure"};
struct Caller {W rva{};unsigned prepass{},ordinary{},views{};} callers[16]{};
struct State {W left{};void* right{};} states[8]{};
struct Tag {void* viewport{};W generation{},serial{};int eye{};bool viewValid{},afw{};Camera camera{};};
SRWLOCK tagLock=SRWLOCK_INIT;
Tag tags[64]{};unsigned writeTag{},readTag{};
thread_local unsigned depth{},calcDepth{};
thread_local void* viewportNow{};
thread_local bool poseValid{},found{};
thread_local W generation{};
thread_local W gameSlot{};
thread_local bool haveSlot{};
thread_local int eye{};
thread_local vr::HeadPose head{},eyes[2]{};
thread_local float angles[2][4]{};
thread_local M firstView{},firstProjection{};
thread_local menu_state::Mode pairMode{menu_state::Mode::unknown};
thread_local W firstRect[4]{};
std::atomic<int> renderEye{};
W renderSlot{};
bool haveRenderSlot=false;
PresentSequence presentSequence;
float yawRef{},posRef[3]{};
bool read(const void* p,void* out,size_t n) {__try{memcpy(out,p,n);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
W bits(float f){W w;memcpy(&w,&f,4);return w;}
float value(W w){float f;memcpy(&f,&w,4);return f;}
bool matrices(const M&v,const M&p) {
 for(int n=0;n<16;n++)if(!std::isfinite((&v.v[0][0])[n])||!std::isfinite((&p.v[0][0])[n]))return false;
 if(std::fabs(v.v[3][3]-1)>.001f||std::fabs(p.v[2][3]-1)>.001f||std::fabs(p.v[3][3])>.001f||p.v[0][0]<.01f||p.v[1][1]<.01f)return false;
 for(int j=0;j<3;j++){float n=0;for(int i=0;i<3;i++)n+=v.v[i][j]*v.v[i][j];if(std::fabs(n-1)>.02f)return false;}
 return true;
}
void relative(vr::HeadPose&p){
 float ay=std::sin(-yawRef*.5f),aw=std::cos(-yawRef*.5f),x=p.qx,y=p.qy,z=p.qz,w=p.qw;
 p.qw=aw*w-ay*y;p.qx=aw*x+ay*z;p.qy=aw*y+ay*w;p.qz=aw*z-ay*x;
 float c=std::cos(-yawRef),s=std::sin(-yawRef),dx=p.px-posRef[0],dz=p.pz-posRef[2];
 p.px=c*dx+s*dz;p.py-=posRef[1];p.pz=-s*dx+c*dz;
}
M delta(const vr::HeadPose&p){return math::pose_view(p,pairUnits);}
// LEANBACK (JJ 2026-10-27: "When in first person mode and I move my physical head back my virtual camera is moving through Batman's
// head"): a final eye view (headset position included) whose origin leaning back brought into his head moves forward along the level
// push; both eyes, markers and AFW's other eye alike. shift = how far its world origin moved (for the particle centre delta).
bool lean_clamp(M&view,float shift[3]){
 float o[3];for(int i=0;i<3;++i)o[i]=-(view.v[3][0]*view.v[i][0]+view.v[3][1]*view.v[i][1]+view.v[3][2]*view.v[i][2]);
 const float before[3]={o[0],o[1],o[2]};
 if(!first_person::clamp_eye(o,90.f))return false; // 90 = apply's units, so the limit matches the forward push
 for(int j=0;j<3;++j)view.v[3][j]=-(o[0]*view.v[0][j]+o[1]*view.v[1][j]+o[2]*view.v[2][j]);
 if(shift)for(int i=0;i<3;++i)shift[i]=o[i]-before[i];
 return true;
}
M lens(M p,int e){return math::projection(p,angles[e]);}
void stop(const char*why){failureReason=why;if(!failed.exchange(true))BVR_LOG("[aa native] disabled: %s; restart required",why);}
bool tag(void*p,int e,W gen){
 AcquireSRWLockExclusive(&tagLock);
 if(writeTag-readTag==64){ReleaseSRWLockExclusive(&tagLock);stop("render tag overflow");return false;}
 gameSlot=writeTag;haveSlot=true;tags[writeTag%64]={p,gen,writeTag,e,false,pairAFW};++writeTag;ReleaseSRWLockExclusive(&tagLock);return true;
}
bool second(void*self,void*edx){__try{drawOriginal(self,edx);return true;}__except(EXCEPTION_EXECUTE_HANDLER){stop("second render fault");return false;}}
bool main_viewport(void*self){
 W vt=0,client=0,cvt=0;
 return read(self,&vt,4)&&vt==base+0x1d2c380&&read(static_cast<BYTE*>(self)+0x20,&client,4)&&read(reinterpret_cast<void*>(client),&cvt,4)&&cvt==base+0x1fa5f88;
}
void __fastcall draw(void*self,void*edx){
 markers9::end_game_pass();
 if(depth||!main_viewport(self)){drawOriginal(self,edx);return;}
 const W ra=W(reinterpret_cast<uintptr_t>(_ReturnAddress())-base);++draws;
 vr::StereoPose stereo{};poseValid=vr::get_stereo_pose(stereo);
 if(poseValid){head=stereo.head;generation=stereo.generation;
  memcpy(eyes,stereo.eyes,sizeof(eyes));memcpy(angles,stereo.angles,sizeof(angles));}
 else generation=0;
 if(poseValid){
  if(recentre.exchange(false)){yawRef=std::atan2(2*(head.qx*head.qz+head.qw*head.qy),1-2*(head.qx*head.qx+head.qy*head.qy));posRef[0]=head.px;posRef[1]=head.py;posRef[2]=head.pz;}
  relative(head);relative(eyes[0]);relative(eyes[1]);
 }
 W disabled=1,ordinary=0,surface=0;
 read(reinterpret_cast<void*>(base+0x22c5638),&disabled,4);read(reinterpret_cast<void*>(base+0x21df37c),&ordinary,4);
 read(static_cast<BYTE*>(self)+0x44,&surface,4);
 const bool renders=!disabled&&ordinary&&surface;
 // Select only this target's observed ordinary viewport and an owner whose
 // actual CalcSceneView->constructor has already passed matrix validation.
 const bool active=ready&&wantNative&&!failed&&poseValid&&width&&height&&owner==ra&&renders;
 if(renders && wantNative && !active){
  const auto n=++flatDraws;if(!poseValid)++poseUnavailable;
  if(n<=8 || n%120==0)BVR_LOG("[aa native fallback] flatDraw=%u coherentPose=%u ready=%u failed=%u size=%ux%u owner=%08X caller=%08X",n,poseValid,ready.load(),failed.load(),width.load(),height.load(),owner.load(),ra);
 }
 const auto mode=menu_state::observe();pairMode=mode.mode; // One snapshot shared by both eyes.
 if(renders)first_person::begin_frame(active,mode,generation);
 pairUnits=math::world_units(worldScale.load());
 pairCentred=centredLens.load();pairAFW=active&&afw::on();
 pairInputWidth=width;pairInputHeight=height;
 if(active&&pairCentred){float crop[2][4];for(int e=0;e<2;++e)math::lens_crop(angles[e],crop[e]);
  pairInputWidth=std::max(64u,unsigned(width.load()*std::fmin(crop[0][0],crop[1][0]))&~1u);
  pairInputHeight=std::max(64u,unsigned(height.load()*std::fmin(crop[0][1],crop[1][1]))&~1u);
  window::observe_crop(std::fmin(std::fmin(crop[0][0],crop[1][0]),std::fmin(crop[0][1],crop[1][1])));
 }
 pairJitterX=pairJitterY=0;if(active)dlss::jitter(pairAFW?generation/2:generation,width,height,pairJitterX,pairJitterY);
 detail9::input_width(pairInputWidth);
 viewportNow=self;++depth;found=false;haveSlot=false;eye=active?(pairAFW&&afwNext?1:-1):0;
 if(renders&&!tag(self,eye,generation))eye=0;
 drawOriginal(self,edx);
 // CalcSceneView also runs in the streaming/prepass branch. Its camera is
 // valid but its caller is NOT evidence of a scene draw reaching Present.
 for(auto& c:callers)if(c.rva==ra||!c.rva){
  const bool first=!c.rva;c.rva=ra;if(renders)++c.ordinary;else ++c.prepass;if(renders&&found)++c.views;
  if(first||(renders&&c.ordinary==1))BVR_LOG("[aa native] caller RVA %08X: ordinary=%u validMainView=%u surface=%08X prepass=%u",ra,renders,found,surface,!renders);
  break;
 }
 if(renders&&found&&!owner){owner=ra;BVR_LOG("[aa native] observed ordinary camera owner RVA %08X; native intent selected",ra);}
 if(renders&&!found&&owner==ra){owner=0;BVR_LOG("[aa native] ordinary caller RVA %08X has no main view; waiting for next scene",ra);}
 if(active&&found&&!failed){if(pairAFW){afw::note_drawn(eye<0?0:1);afwNext^=1;++pairs;}else{eye=1;if(tag(self,eye,generation)&&second(self,edx))++pairs;}}
 markers9::end_game_pass();eye=0;haveSlot=false;--depth;viewportNow=nullptr;
}
void* __fastcall calc(void*self,void*edx,void*family,float*loc,int32_t*rot,void*viewport,W playerIndex){
 bool ours=depth&&viewport==viewportNow;
 // FIRSTPERSON: this pair's eye point from the first eye's pass (the second reuses it); self = the ULocalPlayer.
 if(ours&&eye&&(eye<0||pairAFW))first_person::update(self,pairMode==menu_state::Mode::menu?"main menu":nullptr);
 if(ours)++calcDepth;
 void*out=calcOriginal(self,edx,family,loc,rot,viewport,playerIndex);
 if(ours)--calcDepth;return out;
}
void* __fastcall ctor(void*self,void*edx,A24){
 if(!depth||!calcDepth||reinterpret_cast<uintptr_t>(_ReturnAddress())!=base+0x49f017)return ctorOriginal(self,edx,P24);
 M v{},p{};
 if(!read(reinterpret_cast<void*>(a15),&v,64)||!read(reinterpret_cast<void*>(a16),&p,64)||!matrices(v,p)||value(a13)<=0||value(a14)<=0){++invalid;if(eye)stop("camera input rejected");return ctorOriginal(self,edx,P24);}
 found=true;if(!eye||!poseValid)return ctorOriginal(self,edx,P24);
 // FIRSTPERSON: the game camera at his eyes; both eyes move alike, so the between-eyes check below still holds.
 const bool headHud=first_person::apply(v.v,90.f,head);
 if(eye<0||pairAFW){firstView=v;firstProjection=p;firstRect[0]=a11;firstRect[1]=a12;firstRect[2]=a13;firstRect[3]=a14;
  static unsigned rectLogs=0;if(rectLogs++<3)BVR_LOG("[aa native] raw main rect %.0f,%.0f %.0fx%.0f; actual source %ux%u; pose generation %u",value(a11),value(a12),value(a13),value(a14),width.load(),height.load(),generation);
 }
 else if(memcmp(&firstView,&v,64)||memcmp(&firstProjection,&p,64)||firstRect[0]!=a11||firstRect[1]!=a12||firstRect[2]!=a13||firstRect[3]!=a14){++baseChanged;stop("engine camera changed between eyes");return ctorOriginal(self,edx,P24);}
 if(eye>0&&a2){
  bool have=false;
  for(auto&s:states)if(s.left==a2||!s.left){if(!s.left){s.left=a2;s.right=reinterpret_cast<Alloc>(base+0x6f25a0)();BVR_LOG("[aa native] independent right history %08X -> %p",a2,s.right);}if(s.right){a2=W(s.right);have=true;}break;}
  if(!have){stop("right history allocation failed");return ctorOriginal(self,edx,P24);}
 }
 if(haveSlot){M markerEyes[2]{},markerViews[2]{};for(int i=0;i<2;++i){markerViews[i]=mul(firstView,delta(eyes[i]));lean_clamp(markerViews[i],nullptr);markerEyes[i]=mul(markerViews[i],lens(firstProjection,i));}markers9::publish(gameSlot,draws.load(),generation,self,firstView,firstProjection,markerEyes,yawRef,posRef,markerViews,headHud,pairUnits);}
 const int e=eye<0?0:1;v=mul(v,delta(eyes[e]));
 float leanShift[3]{};lean_clamp(v,leanShift); // LEANBACK: the lens never goes back into his head
 // Particle expansion gets the game camera's orientation/centre, while its
 // projection keeps this real eye. Compute centre delta in the game frame,
 // avoiding subtraction of large rounded world-space camera positions.
 if(haveSlot){M inverseGame{};if(math::inverse(firstView,inverseGame)){
  const double localEye[3]={eyes[e].px*double(pairUnits),eyes[e].py*double(pairUnits),-eyes[e].pz*double(pairUnits)};float centreDelta[3]{};
  for(int i=0;i<3;++i){double d{};for(int j=0;j<3;++j)d-=localEye[j]*inverseGame.v[j][i];centreDelta[i]=float(d)-leanShift[i];}
  billboard9::publish(gameSlot,&firstView.v[0][0],&v.v[0][0],centreDelta,pairMode);
 }}
 M outputProjection=lens(p,e);p=pairCentred?math::centred_projection(p,angles[e]):outputProjection;
 Camera camera{};camera.view=v;camera.projection=outputProjection;camera.generation=generation;camera.width=width;camera.height=height;camera.eye=e;camera.valid=true;camera.centred=pairCentred;camera.afw=pairAFW;camera.headHud=headHud;camera.unitsPerMetre=pairUnits;
 camera.inputWidth=pairInputWidth;camera.inputHeight=pairInputHeight;
 memcpy(camera.angles,angles[e],sizeof(camera.angles));if(pairCentred)math::lens_crop(angles[e],camera.crop);
 camera.jitterX=pairJitterX;camera.jitterY=pairJitterY;
 camera.projection.v[2][0]+=2*pairJitterX/camera.inputWidth;camera.projection.v[2][1]-=2*pairJitterY/camera.inputHeight;
 // The enclosing input is cropped later: scale its jitter so the final
 // DLSS input has exactly the declared pixel offset, equal in both eyes.
 p.v[2][0]+=2*pairJitterX/camera.inputWidth*camera.crop[0];p.v[2][1]-=2*pairJitterY/camera.inputHeight*camera.crop[1];
 // The ctor's RHI switch changes shader depth-unpack constants (+0x620),
 // NOT the projection stored at +0x90. We copy the hardware D3D9 DS, not
 // that shader's encoded scene-depth texture: retain the physical projection.
 W invertedZ{};read(reinterpret_cast<void*>(base+0x22b21f0),&invertedZ,4);
 if(pairAFW&&haveSlot){
  M unjittered=camera.projection;unjittered.v[2][0]-=2*pairJitterX/camera.inputWidth;unjittered.v[2][1]+=2*pairJitterY/camera.inputHeight;
  M otherProjection=math::projection(unjittered,angles[e^1]);
  M otherView=mul(firstView,delta(eyes[e^1]));lean_clamp(otherView,nullptr);
  afw::set_camera_pass(gameSlot,e,&v.v[0][0],&camera.projection.v[0][0],&unjittered.v[0][0],&otherView.v[0][0],&otherProjection.v[0][0],camera.inputWidth,camera.inputHeight,camera.crop,camera.width,camera.height);
 }

 static unsigned cameraLogs{};if(cameraLogs++<4)BVR_LOG("[aa camera] eye %d generation %u centred %u source %ux%u DLSS input %ux%u jitter %.4f/%.4f physical-DS projection %.6f/%.6f shader-unpack inverted %u (not raw DS)",e,generation,pairCentred,camera.width,camera.height,camera.inputWidth,camera.inputHeight,camera.jitterX,camera.jitterY,camera.projection.v[2][2],camera.projection.v[3][2],invertedZ);
 a11=a12=bits(0);a13=bits(float(width.load()));a14=bits(float(height.load()));a15=W(&v);a16=W(&p);
 ++edits;vr::set_rendered_hfov(2*std::atan(1/p.v[0][0])*57.29578f);vr::publish_gameplay_view(true);
 // BeginDrawing can run before game-thread view construction. Publish the
 // verdict separately; Present sees it only AFTER those draws have completed.
 if(haveSlot){AcquireSRWLockExclusive(&tagLock);auto&t=tags[gameSlot%64];if(t.serial==gameSlot){t.viewValid=true;t.camera=camera;}ReleaseSRWLockExclusive(&tagLock);}
 return ctorOriginal(self,edx,P24);
}
struct Seam{W rva;const char*sig;unsigned relocation;void*hook;void**orig;};
}
bool install(){
 base=uintptr_t(GetModuleHandleW(nullptr));
 menu_state::install(base);
 wchar_t settings[MAX_PATH];swprintf_s(settings,L"%s\\aa_rendering.ini",log::data_dir());centredLens=GetPrivateProfileIntW(L"AsylumVR",L"LightingCompatibility",1,settings)!=0;first_person::install(base,settings);
 wchar_t scaleText[32]{};GetPrivateProfileStringW(L"AsylumVR",L"WorldScale",L"1",scaleText,32,settings);
 wchar_t* scaleEnd=nullptr;const float savedScale=float(wcstod(scaleText,&scaleEnd));
 worldScale=(scaleEnd!=scaleText && !*scaleEnd && std::isfinite(savedScale) && savedScale>=.5f && savedScale<=2.f)?savedScale:1.f;
 BVR_LOG("[aa world scale] loaded %.2fx (%.2f units/metre)",worldScale.load(),math::world_units(worldScale.load()));
 const Seam seams[]={{0x8cae50,"6aff689225cd0164a1000000005081ec",3,(void*)draw,(void**)&drawOriginal},{0x49e620,"558bec83e4f06aff686930c90164a100",9,(void*)calc,(void**)&calcOriginal},{0x6ecc60,"558bec83e4f06aff68da53cb0164a100",9,(void*)ctor,(void**)&ctorOriginal},{0x6f25a0,"6aff68cb5acb0164a1000000005051a1",3,nullptr,nullptr}};
 for(const auto&s:seams){BYTE expected[16]{},actual[16]{};for(int n=0;n<16;n++){unsigned v;sscanf_s(s.sig+2*n,"%2x",&v);expected[n]=BYTE(v);}if(s.relocation){W ptr;memcpy(&ptr,expected+s.relocation,4);ptr+=W(base-0x400000);memcpy(expected+s.relocation,&ptr,4);}if(!read((void*)(base+s.rva),actual,16)||memcmp(expected,actual,16)){BVR_LOG("[aa native] Asylum seam guard failed at RVA %08X",s.rva);return false;}}
 bool ok=markers9::install_engine(base);for(const auto&s:seams)if(s.hook){if(MH_CreateHook((void*)(base+s.rva),s.hook,s.orig)!=MH_OK){ok=false;break;}MH_QueueEnableHook((void*)(base+s.rva));}
 if(ok)ok=MH_ApplyQueued()==MH_OK;
 if(!ok){for(const auto&s:seams)if(s.hook)MH_QueueDisableHook((void*)(base+s.rva));MH_ApplyQueued();return false;}
 ready=true;vr::set_asym_fov(true);vr::set_exact_pose_tag(true);vr::sr_use_render_thread_eyes(true);vr::set_sr_pair_pacing(true);vr::set_pace_sync(true);vr::set_camera_mode(false);
 BVR_LOG("[aa native] Asylum 0/5/24-word camera hooks installed; native selected, waiting for validated ordinary camera");return true;
}
void begin_render(void*command){
 void*p=nullptr;read(static_cast<BYTE*>(command)+4,&p,4);Tag t{};
 AcquireSRWLockExclusive(&tagLock);
 haveRenderSlot=readTag!=writeTag&&tags[readTag%64].viewport==p;
 if(haveRenderSlot)t=tags[readTag++%64];else if(ready)++misses;
 ReleaseSRWLockExclusive(&tagLock);
 renderSlot=t.serial;
 presentSequence.begin(failed?0:t.eye,t.generation,t.afw);afw::set_render_slot(renderSlot);
 billboard9::begin_render(renderSlot,!failed&&t.eye!=0);markers9::begin_render(renderSlot,!failed&&t.eye!=0);
 hud9::begin_eye(failed?0:t.afw&&t.eye?-1:t.eye,t.generation);
 // Publish only at the matching Present, after the ctor verdict and bridge
 // copy are known. Begin can precede game-thread view construction.
}
void on_present(IDXGISwapChain*sc){
 DXGI_SWAP_CHAIN_DESC d{};if(SUCCEEDED(sc->GetDesc(&d))){width=d.BufferDesc.Width;height=d.BufferDesc.Height;}
 bool valid=false;AcquireSRWLockShared(&tagLock);
 renderedCamera={};if(haveRenderSlot){const auto&t=tags[renderSlot%64];valid=t.serial==renderSlot&&t.viewValid;if(valid)renderedCamera=t.camera;}
 ReleaseSRWLockShared(&tagLock);
 // Reset temporal history on the actual rendered scale transition, not while
 // older queued pairs still use the previous UI value.
 static float renderedUnits=90.f;
 if(valid && renderedCamera.unitsPerMetre!=renderedUnits){renderedUnits=renderedCamera.unitsPerMetre;dlss::reset();afw::reset();}
 haveRenderSlot=false; // An extra movie/loading Present cannot reuse this eye.
 const auto result=presentSequence.present(!failed&&valid);
 switch(result.fault){
 case PresentSequence::Fault::RepeatedLeft:++misses;stop("left Present repeated before its right eye");break;
 case PresentSequence::Fault::MissingLeft:++misses;stop("right Present has no matching left");break;
 case PresentSequence::Fault::GenerationMismatch:++misses;stop("right Present pose generation differs from left");break;
 default:break;
 }
 if(result.completedPair){const auto n=++completed;if(n<=3)BVR_LOG("[aa native] completed paired scene %u, pose generation %u",n,result.generation);}
 renderEye=result.eye;lastPresent=GetTickCount64();
 vr::set_camera_mode(result.eye!=0);vr::sr_set_present_eye(result.eye);vr::sr_set_present_gen(result.generation);vr::sr_set_present_afw(result.afw);
}
void discard_present(){renderedCamera={};haveRenderSlot=false;presentSequence.reset();renderEye=0;billboard9::begin_render(0,false);vr::set_camera_mode(false);vr::sr_set_present_eye(0);vr::sr_set_present_gen(0);}
bool camera(Camera& out){out=renderedCamera;return renderEye!=0&&out.valid&&out.eye==(renderEye<0?0:1);}
bool head_hud(uint32_t gen){
 bool result=false;AcquireSRWLockShared(&tagLock);
 // DLSS may finish the left after another eye was submitted. Match its exact
 // pose generation, never a live toggle or the newest render-thread camera.
 for(unsigned n=0;n<64&&n<writeTag;++n){const auto&t=tags[(writeTag-1-n)%64];
  if(t.viewValid&&t.camera.generation==gen){result=t.camera.headHud;break;}}
 ReleaseSRWLockShared(&tagLock);return result;
}
bool afw_present(){return renderEye!=0&&renderedCamera.valid&&renderedCamera.afw;}
void set_centred(bool on){centredLens=on;dlss::reset();afw::reset();wchar_t settings[MAX_PATH];swprintf_s(settings,L"%s\\aa_rendering.ini",log::data_dir());WritePrivateProfileStringW(L"AsylumVR",L"LightingCompatibility",on?L"1":L"0",settings);BVR_LOG("[aa native] scene lens %s; XR retains actual asymmetric lens",on?"centred enclosure":"direct asymmetric");}
bool centred(){return centredLens;}
const char* rendering_status(){
 if(failed)return "Floating screen - native validation failed";
 if(!ready)return "Floating screen - native hooks unavailable";
 if(!wantNative)return "Floating screen - native disabled";
 if(renderEye&&GetTickCount64()-lastPresent.load()<500){if(afw::on())return "AFW - one rendered eye plus depth warp";return completed?"Native stereo - paired scene":"Native stereo - first pair in progress";}
 return owner?"Floating screen - no stereo scene in this Present":"Floating screen - waiting for a 3D scene render";
}
unsigned completed_pairs(){return completed;}
void status(){BVR_LOG("[aa native] selected %u ready %u failed %u owner %08X | draws %u queuedPairs %u completedPairs %u edits %u invalid %u tagMiss %u baseCameraChanged %u coherentPoseUnavailable %u flatDraws %u | %s",wantNative.load(),ready.load(),failed.load(),owner.load(),draws.load(),pairs.load(),completed.load(),edits.load(),invalid.load(),misses.load(),baseChanged.load(),poseUnavailable.load(),flatDraws.load(),rendering_status());billboard9::status();}
bool selected(){return wantNative;}
void select(bool on){wantNative=on;dlss::reset();afw::reset();if(on)recentre=true;}
void recenter(){recentre=true;dlss::reset();afw::reset();}
int present_eye(){return renderEye;}
float world_scale(){return worldScale.load();}
void draw_world_scale_ui(){
 float scale=worldScale.load();
 bool changed=overlay::SliderStep("world scale  (higher = the world looks smaller)",&scale,.5f,2.f,"%.2fx");
 ImGui::SameLine();if(ImGui::Button("Reset##world-scale")){scale=1.f;changed=true;}
 ImGui::TextDisabled("1.00x = original size.");
 if(changed && std::isfinite(scale)){
  scale=std::clamp(scale,.5f,2.f);worldScale=scale;
  wchar_t path[MAX_PATH],text[32];swprintf_s(path,L"%s\\aa_rendering.ini",log::data_dir());swprintf_s(text,L"%.3f",scale);
  WritePrivateProfileStringW(L"AsylumVR",L"WorldScale",text,path);
  BVR_LOG("[aa world scale] %.2fx (%.2f units/metre); applies from next stereo pair",scale,math::world_units(scale));
 }
}
void draw_ui(){bool on=wantNative;if(ImGui::Checkbox("Use native stereo",&on))select(on);ImGui::TextWrapped("Rendering: %s",rendering_status());if(failed)ImGui::TextWrapped("%s. Restart required.",failureReason.load());ImGui::Text("Completed pairs %u / camera edits %u",completed.load(),edits.load());billboard9::draw_ui();hud9::draw_ui();}
}
