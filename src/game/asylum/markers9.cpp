#include "markers9.h"
#include "head_aim.h"
#include "core/util/log.h"
#include <MinHook.h>
#include <wrl/client.h>
#include <intrin.h>
#include <imgui.h>
#include <array>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
using Microsoft::WRL::ComPtr;
namespace bvr::aa::markers9 {
namespace {
// off: the game camera does not see the point (beside/behind); dir = the screen
// direction the game points its edge ring, from the head-free view-space x/y.
struct Anchor{float world[3]{},ref[4]{};bool off{},gameRef{};float dir[2]{};};
struct Set{uint32_t frame{};bool valid{};unsigned n{};std::array<Anchor,64> points;};
struct Pass{uint32_t serial{},frame{},gen{};bool valid{},haveEyeView{},headHud{};void* view{};math::M baseView{},baseP{},baseVP{},eyeVP[2],eyeView[2];float referenceYaw{},referencePosition[3]{},unitsPerMetre{90.f};};
std::array<Pass,64> passes;std::array<Set,16> sets;
// A2k: the HUD keeps drawing (and animating) a ring on frames with no Canvas.Project;
// the last projected points stay usable for a quarter second.
Set held;ULONGLONG heldAt{};constexpr ULONGLONG kHoldMs=250;std::atomic<unsigned> heldUsed{};SRWLOCK lock=SRWLOCK_INIT;
thread_local Pass gamePass;Pass renderPass;bool renderActive{};std::atomic<bool> on{true};thread_local bool internal{};uint32_t currentGen{};int currentEye{};
std::atomic<unsigned> projected{};unsigned routed{},misses{},variants{},shaderMisses{},failed{},stale{},maskRejects{};uint32_t runSlot=~0u;unsigned runLeft{},runAnchor=64;
std::atomic<bool> connectorsOn{true};std::atomic<unsigned> vitalsAdjusted{},vitalsFallback{};bool vitalsOwnerGuard{};
struct HudPlane{bool valid{},headFollowing{};float world[4][3]{},shape[3]{};};HudPlane hudPlane;
wchar_t settings[MAX_PATH]{};uintptr_t imageBase{};
using Project=float*(__fastcall*)(void*,void*,float*,const float*);Project projectOriginal{};
using WorldPixel=BOOL(__fastcall*)(void*,void*,const float*,float*);WorldPixel pixelOriginal{};bool pixelGuard{};std::atomic<unsigned>pixelCalls{},pixelPlaced{},pixelRejected{};
struct Layer{ComPtr<IDirect3DTexture9> texture;ComPtr<IDirect3DSurface9> rt,ds;bool painted{};};Layer layers[2];UINT fixtureSize{};
struct Variant{ComPtr<IDirect3DVertexShader9> shader;};std::unordered_map<void*,Variant> shaders;SRWLOCK shaderLock=SRWLOCK_INIT;
bool read(const void*p,void*out,size_t n){__try{memcpy(out,p,n);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
// A2h census: EVERY caller of each projection route, owned pass or not. A2g's
// counters sat behind the owned-view/caller filter, so zero proved nothing.
struct CallerCount{uintptr_t rva{};unsigned owned{},other{};};
struct Census{const char*name;std::array<CallerCount,24> callers{};unsigned overflow{},total{},reported{};SRWLOCK lock=SRWLOCK_INIT;};
Census projectCensus{"Project helper6bc2a0"},pixelCensus{"WorldToPixel helper6b36c0"},uiCensus{"UIScreenObject.Project9c3550"};
void count(Census&c,uintptr_t rva,bool owned,const float*world,const float*out){
 float w[3]{},o[3]{};const bool sample=read(world,w,12)&&read(out,o,12);bool first=false;
 AcquireSRWLockExclusive(&c.lock);++c.total;unsigned i=0;for(;i<c.callers.size()&&c.callers[i].rva&&c.callers[i].rva!=rva;++i){}
 if(i==c.callers.size())++c.overflow;else{auto&k=c.callers[i];first=!k.rva;k.rva=rva;++(owned?k.owned:k.other);}ReleaseSRWLockExclusive(&c.lock);
 if(first)BVR_LOG("[aa markers census] %s NEW caller%06zx ownedPass%u world%.1f/%.1f/%.1f out%.4f/%.4f/%.4f",c.name,rva,owned,sample?w[0]:0.f,w[1],w[2],o[0],o[1],o[2]);
}
void census_log(Census&c,bool force){
 AcquireSRWLockShared(&c.lock);const Census s=c;ReleaseSRWLockShared(&c.lock);if(!force&&s.total==s.reported)return;c.reported=s.total;
 char line[768];int n=snprintf(line,sizeof(line),"[aa markers census] %s total%u overflow%u:",s.name,s.total,s.overflow);
 for(const auto&k:s.callers){if(!k.rva||n>=int(sizeof(line))-40)break;n+=snprintf(line+n,sizeof(line)-n," %06zx=%u/%u",k.rva,k.owned,k.other);}BVR_LOG("%s",line);
}
// A2h grapple target: the grapple gun's own finders (script natives, thiscall
// Stack/Result, ret8, *Result = chosen grapple point or null). Their answer is
// the game's "is there a target"; the HUD's projection route no longer matters.
using Finder=void(__fastcall*)(void*,void*,void*,void**);Finder finderOriginal[2]{};bool finderGuard[2]{};
struct Grapple{std::atomic<void*> object{};std::atomic<ULONGLONG> at{};std::atomic<unsigned> calls{},found{},changes{};};Grapple grapple[2];const char*finderName[2]={"NoAim","WithAim"};
constexpr ULONGLONG kGrappleFreshMs=250;constexpr unsigned kScanBytes=0x1000;
std::array<unsigned,kScanBytes/4> offsetVotes{};unsigned offsetScans{},offsetScanLogs{};std::atomic<int> learnedOffset{-1};
std::atomic<unsigned> grappleAnchors{},orphans{},orphansWithTarget{},orphansNoTarget{},orphansHidden{};std::atomic<bool> orphanHide{false};
std::array<void*,32> grappleTextures{};unsigned grappleTextureCount{},orphanLogs{},orphanRunLeft{};
void* fresh_target(int k,ULONGLONG now){void*o=grapple[k].object.load();return o&&now-grapple[k].at.load()<=kGrappleFreshMs?o:nullptr;}
void note_finder(int k,void**result){void*o=nullptr;if(result)read(result,&o,4);++grapple[k].calls;if(o)++grapple[k].found;
 if(grapple[k].object.exchange(o)!=o&&++grapple[k].changes<=12){void*vt=nullptr;if(o)read(o,&vt,4);BVR_LOG("[aa grapple] %s target %p vtable%06zx (change%u)",finderName[k],o,vt?uintptr_t(vt)-imageBase:0,grapple[k].changes.load());}
 grapple[k].at=GetTickCount64();}
void __fastcall no_aim_hook(void*self,void*edx,void*stack,void**result){finderOriginal[0](self,edx,stack,result);note_finder(0,result);}
void __fastcall with_aim_hook(void*self,void*edx,void*stack,void**result){finderOriginal[1](self,edx,stack,result);note_finder(1,result);}
// Learn where a grapple point stores the world point the accepted Canvas.Project
// path uses: search the current finder object for that exact float triple.
void learn_offset(const float*world){
 const ULONGLONG now=GetTickCount64();void*objects[2]={fresh_target(0,now),fresh_target(1,now)};if(!objects[0]&&!objects[1])return;
 bool hit[kScanBytes/4]{};float bestError=1e30f;int bestOffset=-1;
 for(void*o:objects){if(!o)continue;float block[64];for(unsigned base=0;base+sizeof(block)<=kScanBytes;base+=sizeof(block)-8){/* blocks overlap two floats: no triple straddles */if(!read(static_cast<BYTE*>(o)+base,block,sizeof(block)))break;
  for(unsigned i=0;i+2<64;++i){float e=0;for(int j=0;j<3;++j)e=std::max(e,std::fabs(block[i+j]-world[j]));if(!std::isfinite(e))continue;if(e<bestError){bestError=e;bestOffset=int(base+i*4);}if(e<=2.f)hit[(base+i*4)/4]=true;}}}
 ++offsetScans;unsigned best=0;int bestVoted=-1;char found[96]{};int n=0;
 for(unsigned i=0;i<kScanBytes/4;++i){if(hit[i]){++offsetVotes[i];if(n<int(sizeof(found))-8)n+=snprintf(found+n,sizeof(found)-n," +%x",i*4);}if(offsetVotes[i]>best){best=offsetVotes[i];bestVoted=int(i*4);}}
 if(++offsetScanLogs<=8)BVR_LOG("[aa grapple] learn scan%u world%.1f/%.1f/%.1f noAim%p withAim%p matches%s best+%x error%.2f",offsetScans,world[0],world[1],world[2],objects[0],objects[1],n?found:" none",bestOffset,bestError);
 if(learnedOffset<0&&best>=6&&best*4>=offsetScans*3){learnedOffset=bestVoted;BVR_LOG("[aa grapple] LEARNED grapple point world offset +%x (%u of %u scans)",bestVoted,best,offsetScans);}
}
bool project(const math::M&vp,const float*w,float*out);
std::atomic<unsigned> offAnchors{},offRejected{},offMatched{},offOnTarget{},offPointer{},offHidden{},offUnlearned{};unsigned offLogs{};float runRef[2]{};float pointerDistance=4*90.f;
float pointerDegrees=26;constexpr float kOffMatchCos=.866f,kOffMinRadius=.25f;
// Screen direction the game uses for its edge ring: head-free view-space x/y
// scaled by the projection, valid in front of AND behind the camera.
bool off_direction(const Pass&p,Anchor&a){
 float v[2]{};for(int c=0;c<2;++c)v[c]=a.world[0]*p.baseView.v[0][c]+a.world[1]*p.baseView.v[1][c]+a.world[2]*p.baseView.v[2][c]+p.baseView.v[3][c];
 const float x=v[0]*p.baseP.v[0][0],y=v[1]*p.baseP.v[1][1],len=std::hypot(x,y);if(!std::isfinite(len)||len<1e-6f)return false;a.dir[0]=x/len;a.dir[1]=y/len;return true;
}
// Both eyes see the point inside their lens (a margin keeps edge rings off the rim).
bool in_view(const Pass&p,const float*world){for(int e=0;e<2;++e){float c[4]{};if(!project(p.eyeVP[e],world,c)||std::fabs(c[0])>.92f||std::fabs(c[1])>.92f)return false;}return true;}
// Head-locked edge pointer: a point kPointerDegrees off the LEFT eye's forward,
// toward the target, at HUD distance; rebuilt every pass from that pass's own
// eye pose, so it stays put in the view. q carries the eye as its camera.
// The LEFT eye as the card camera (on-point placement when the head-free camera cannot).
bool eye_pass(const Pass&p,Pass&q){if(!p.haveEyeView)return false;q=p;q.baseView=p.eyeView[0];q.baseVP=p.eyeVP[0];return true;}
bool pointer(const Pass&p,const Anchor&a,Pass&q,Anchor&out){
 if(!p.haveEyeView)return false;math::M inv;if(!math::inverse(p.eyeView[0],inv))return false;
 float t[2]{};for(int c=0;c<2;++c)t[c]=a.world[0]*p.eyeView[0].v[0][c]+a.world[1]*p.eyeView[0].v[1][c]+a.world[2]*p.eyeView[0].v[2][c]+p.eyeView[0].v[3][c];
 float len=std::hypot(t[0],t[1]);if(!std::isfinite(len))return false;if(len<1e-3f){t[0]=0;t[1]=-1;len=1;}
 const float r=pointerDegrees*3.14159265f/180,v[3]={std::sin(r)*t[0]/len,std::sin(r)*t[1]/len,std::cos(r)};
 float w[3]{},n=0;for(int j=0;j<3;++j){w[j]=v[0]*inv.v[0][j]+v[1]*inv.v[1][j]+v[2]*inv.v[2][j];n+=w[j]*w[j];}n=std::sqrt(n);if(n<1e-6f)return false;
 out=a;out.off=false;for(int j=0;j<3;++j)out.world[j]=inv.v[3][j]+w[j]/n*pointerDistance*(p.unitsPerMetre/90.f);
 q=p;q.baseView=p.eyeView[0];q.baseVP=p.eyeVP[0];return true;
}
bool anchor(const float*world,float*out,bool fromGrapple=false);
// Per game pass: the game's own current grapple point(s) become marker anchors
// even when the HUD placed its ring without calling Canvas.Project.
void grapple_anchors(){
 const int offset=learnedOffset;if(offset<0)return;const ULONGLONG now=GetTickCount64();
 for(int k=0;k<2;++k){void*o=fresh_target(k,now);float w[3]{},ref[4]{};if(o&&read(static_cast<BYTE*>(o)+offset,w,12)&&std::isfinite(w[0]+w[1]+w[2])&&anchor(w,ref,true))++grappleAnchors;}
}
bool learned_texture(void*t){for(unsigned i=0;i<grappleTextureCount&&i<grappleTextures.size();++i)if(grappleTextures[i]==t)return true;return false;}
bool project(const math::M&vp,const float*w,float*out){float c[4]{};for(int j=0;j<4;++j)c[j]=w[0]*vp.v[0][j]+w[1]*vp.v[1][j]+w[2]*vp.v[2][j]+vp.v[3][j];if(!std::isfinite(c[3])||c[3]<=.001f)return false;for(int j=0;j<3;++j){out[j]=c[j]/c[3];if(!std::isfinite(out[j]))return false;}out[3]=c[3];return true;}
bool anchor(const float*world,float*out,bool fromGrapple){
 if(!on||!gamePass.valid)return false;Anchor a{};if(!read(world,a.world,12))return false;
 const bool seen=project(gamePass.baseVP,a.world,a.ref)&&std::fabs(a.ref[0])<=1.1f&&std::fabs(a.ref[1])<=1.1f;
 // A2i/A2k: a point the head-free camera does not see is KEPT. The game's own
 // projection includes the head (the HUD pass view), so its output is where the
 // ring is drawn: keep it as the match position, and leave it unchanged.
 if(!seen){if(!off_direction(gamePass,a)){++offRejected;return false;}a.off=true;memset(a.ref,0,16);float g[4]{};
  if(!fromGrapple&&out&&read(out,g,16)&&std::isfinite(g[0])&&std::isfinite(g[1])&&std::fabs(g[0])<=1.2f&&std::fabs(g[1])<=1.2f){a.ref[0]=g[0];a.ref[1]=g[1];a.gameRef=true;}}
 AcquireSRWLockExclusive(&lock);auto&s=sets[gamePass.frame%sets.size()];if(!s.valid||s.frame!=gamePass.frame){s={};s.valid=true;s.frame=gamePass.frame;}
 bool found=false;for(unsigned i=0;i<s.n;++i){const auto&w=s.points[i].world;float d=0;for(int j=0;j<3;++j)d+=std::fabs(w[j]-a.world[j]);if(d<.01f){found=true;break;}}
 if(!found&&s.n<s.points.size())s.points[s.n++]=a;held=s;heldAt=GetTickCount64();ReleaseSRWLockExclusive(&lock);
 if(a.off){if(++offAnchors<=6)BVR_LOG("[aa grapple] off-screen point frame%u world %.1f %.1f %.1f game-screen direction %.3f %.3f grapple%u gameXY%u %.3f %.3f",gamePass.frame,a.world[0],a.world[1],a.world[2],a.dir[0],a.dir[1],fromGrapple,a.gameRef,a.ref[0],a.ref[1]);return false;}
 memcpy(out,a.ref,16);if(fromGrapple){if(grappleAnchors<4)BVR_LOG("[aa grapple] anchor from grapple point frame%u world %.2f %.2f %.2f reference %.4f %.4f",gamePass.frame,a.world[0],a.world[1],a.world[2],a.ref[0],a.ref[1]);return true;}
 if(++projected<=4)BVR_LOG("[aa markers] Canvas target frame%u world %.2f %.2f %.2f reference %.4f %.4f depth%.2f",gamePass.frame,a.world[0],a.world[1],a.world[2],a.ref[0],a.ref[1],a.ref[3]);return true;
}
// Project the shared head-centre ray onto the actual upright HUD plane. Head
// orientation is deliberately absent: only translation changes this layout.
bool plane_target(const Pass&p,const Anchor&a,const float world[4][3],const float head[3],float*xy){
 float local[3]{};for(int j=0;j<3;++j)local[j]=(a.world[0]*p.baseView.v[0][j]+a.world[1]*p.baseView.v[1][j]+a.world[2]*p.baseView.v[2][j]+p.baseView.v[3][j])/p.unitsPerMetre;
 local[2]=-local[2];const float c=std::cos(p.referenceYaw),s=std::sin(p.referenceYaw);
 const float target[3]={p.referencePosition[0]+c*local[0]+s*local[2],p.referencePosition[1]+local[1],p.referencePosition[2]-s*local[0]+c*local[2]};
 float right[3],down[3],ray[3];for(int j=0;j<3;++j){right[j]=world[1][j]-world[0][j];down[j]=world[2][j]-world[0][j];ray[j]=target[j]-head[j];}
 const float normal[3]={right[1]*down[2]-right[2]*down[1],right[2]*down[0]-right[0]*down[2],right[0]*down[1]-right[1]*down[0]};
 float numerator=0,denominator=0,rr=0,dd=0;for(int j=0;j<3;++j){numerator+=(world[0][j]-head[j])*normal[j];denominator+=ray[j]*normal[j];rr+=right[j]*right[j];dd+=down[j]*down[j];}
 if(std::fabs(denominator)<1e-6f||rr<1e-6f||dd<1e-6f)return false;const float t=numerator/denominator;if(!std::isfinite(t)||t<=0)return false;
 float u=0,v=0;for(int j=0;j<3;++j){const float offset=head[j]+t*ray[j]-world[0][j];u+=offset*right[j]/rr;v+=offset*down[j]/dd;}xy[0]=2*u-1;xy[1]=1-2*v;return std::isfinite(xy[0])&&std::isfinite(xy[1]);
}
// The native detective-callout owner creates every ring/line piece from one
// projected actor position. Correct that position BEFORE original layout.
bool vitals_projection(const Pass&p,void*view,uintptr_t caller,const Anchor&a,const HudPlane&plane,const float head[3],float*out){
 if(!connectorsOn||!p.valid||view!=p.view||caller!=0x118a36b||!plane.valid||!head||!out)return false;
 for(int j=0;j<4;++j)if(!std::isfinite(out[j]))return false;
 if(out[3]<=.001f||std::fabs(out[0])>1.1f||std::fabs(out[1])>1.1f)return false;
 float xy[2]{};if(!plane_target(p,a,plane.world,head,xy))return false;
 out[0]=xy[0];out[1]=xy[1];return true; // Original Z/W remain bit-exact.
}
float* __fastcall project_hook(void*view,void*edx,float*out,const float*world){auto*r=projectOriginal(view,edx,out,world);const auto caller=uintptr_t(_ReturnAddress())-imageBase;const bool owned=gamePass.valid&&view==gamePass.view;count(projectCensus,caller,owned,world,out);if(!owned)return r;if(caller==0x658947){if(anchor(world,out)){float w[3]{};if(read(world,w,12))learn_offset(w);}return r;}
 if(!connectorsOn||!vitalsOwnerGuard||caller!=0x118a36b)return r;
 Anchor a{};float original[4]{};if(!read(world,a.world,12)||!read(out,original,16))return r;
 AcquireSRWLockShared(&lock);HudPlane plane=hudPlane;ReleaseSRWLockShared(&lock);
 vr::HeadPose poses[2]{};float angles[2][4]{};
 if(!plane.valid||!vr::get_eye_pose_gen(gamePass.gen,0,poses[0],angles[0])||!vr::get_eye_pose_gen(gamePass.gen,1,poses[1],angles[1])){++vitalsFallback;return r;}
 // A moving HUD needs this game pass's pose, not the previous compositor
 // frame's corners. Returning to room mode waits for the new fixed plane.
 if(gamePass.headHud){if(!head_aim::hud_plane(poses,plane.shape[0],plane.shape[1],plane.shape[2],plane.world)){++vitalsFallback;return r;}}
 else if(plane.headFollowing){++vitalsFallback;return r;}
 const float head[3]={(poses[0].px+poses[1].px)*.5f,(poses[0].py+poses[1].py)*.5f,(poses[0].pz+poses[1].pz)*.5f};
 if(vitals_projection(gamePass,view,caller,a,plane,head,out)){if(++vitalsAdjusted<=6)BVR_LOG("[aa vitals] original callout owner118a36b frame%u gen%u world%.1f/%.1f/%.1f originalXY%.4f/%.4f sharedHudXY%.4f/%.4f preservedZW%.6f/%.2f",gamePass.frame,gamePass.gen,a.world[0],a.world[1],a.world[2],original[0],original[1],out[0],out[1],out[2],out[3]);}else ++vitalsFallback;
 return r;
}
// Canvas.WorldToPixel has a separate inline projection helper, not Project.
// Only its original-success branch can supply a marker. Preserve its validity.
bool pixel_projection(const Pass&p,void*view,uintptr_t caller,BOOL success,const float*world,const float rect[4],float pixels[2]){
 if(!on||!p.valid||p.view!=view||caller!=0x658b73||!success)return false;
 float ref[4]{};if(!project(p.baseVP,world,ref)||std::fabs(ref[0])>1.1f||std::fabs(ref[1])>1.1f)return false;
 for(int j=0;j<4;++j)if(!std::isfinite(rect[j]))return false;if(rect[2]<=0||rect[3]<=0)return false;
 pixels[0]=rect[0]+(ref[0]+1)*.5f*rect[2];pixels[1]=rect[1]+(1-ref[1])*.5f*rect[3];return std::isfinite(pixels[0])&&std::isfinite(pixels[1]);
}
BOOL __fastcall pixel_hook(void*view,void*edx,const float*world,float*out){const BOOL success=pixelOriginal(view,edx,world,out);const auto caller=uintptr_t(_ReturnAddress())-imageBase;
 count(pixelCensus,caller,gamePass.valid&&view==gamePass.view,world,out);
 if(!on||!gamePass.valid||view!=gamePass.view||caller!=0x658b73)return success;++pixelCalls;
 float point[3]{},rect[4]{},pixels[2]{};if(!success||!read(world,point,12)||!read(static_cast<BYTE*>(view)+0x28,rect,16)||!pixel_projection(gamePass,view,caller,success,point,rect,pixels)){++pixelRejected;return success;}
 float ref[4]{};if(!anchor(point,ref)){++pixelRejected;return success;}const float before[2]={out[0],out[1]};memcpy(out,pixels,8);
 if(++pixelPlaced<=6)BVR_LOG("[aa markers pixel] Canvas.WorldToPixel frame%u gen%u world%.2f/%.2f/%.2f originalXY%.2f/%.2f headFreeXY%.2f/%.2f rect%.0f/%.0f/%.0f/%.0f originalSuccess%u",gamePass.frame,gamePass.gen,point[0],point[1],point[2],before[0],before[1],out[0],out[1],rect[0],rect[1],rect[2],rect[3],success);return success;
}
// UIScreenObject.Project: its own scene-client matrix, no shared helper. Census only.
using UiProject=float*(__fastcall*)(void*,void*,float*,const float*);UiProject uiProjectOriginal{};bool uiProjectGuard{};
float* __fastcall ui_project_hook(void*self,void*edx,float*out,const float*world){auto*r=uiProjectOriginal(self,edx,out,world);count(uiCensus,uintptr_t(_ReturnAddress())-imageBase,gamePass.valid,world,out);return r;}
unsigned regtype(DWORD t){return ((t>>28)&7)|((t>>8)&24);}
// Exact target Scaleform position sequence, not a shader name/hash guess.
// Expand MUL+three MAD XY to XYZW; remove the following MOV input ZW.
// Colour, UV, output signature and ViewSpaceDepth multiplication stay original.
bool perspective_code(const std::vector<DWORD>&in,std::vector<DWORD>&out){
 if(in.empty()||in[0]!=0xfffe0300)return false;out=in;unsigned matches=0;
 for(size_t at=1;at<in.size();){unsigned op=in[at]&65535;if(op==D3DSIO_END)break;unsigned n=op==D3DSIO_COMMENT?(in[at]>>16)&32767:(in[at]>>24)&15;if(n>in.size()-at-1)return false;
  if(op==D3DSIO_MUL&&n==3&&regtype(in[at+1])==D3DSPR_TEMP&&((in[at+1]>>16)&15)==3&&in[at+2]==0xa0e40006&&in[at+3]==0x90550000){
   size_t q=at;const DWORD dst=in[q+1];bool ok=true;
   for(unsigned k=0;k<3;++k){q+=k?5:4;if(q+4>=in.size()||(in[q]&65535)!=D3DSIO_MAD||in[q+1]!=dst||in[q+2]!=(0xa0e40005+(k?k+1:0))||in[q+3]!=(k==0?0x90000000:k==1?0x90aa0000:0x90ff0000)||in[q+4]!=(0x80e40000|(dst&2047))){ok=false;break;}}
   q+=5;if(!ok||q+2>=in.size()||(in[q]&65535)!=D3DSIO_MOV||in[q+1]!=((dst&~0xf0000u)|0xc0000u)||in[q+2]!=0x90e40000){at+=n+1;continue;}
   size_t pos=at;for(unsigned k=0;k<4;++k){out[pos+1]=(out[pos+1]&~0xf0000u)|0xf0000u;pos+=k?5:4;}out.erase(out.begin()+q,out.begin()+q+3);++matches;break;
  }at+=n+1;
 }
 return matches==1;
}
ComPtr<IDirect3DVertexShader9> variant(IDirect3DDevice9*d,IDirect3DVertexShader9*v){
 AcquireSRWLockShared(&shaderLock);const auto it=shaders.find(v);const bool found=it!=shaders.end();ComPtr<IDirect3DVertexShader9> known;if(found)known=it->second.shader;ReleaseSRWLockShared(&shaderLock);if(found)return known;UINT n{};Variant value;
 if(SUCCEEDED(v->GetFunction(nullptr,&n))&&n>=8&&n<=32768&&n%4==0){std::vector<DWORD>code(n/4),patched;if(SUCCEEDED(v->GetFunction(code.data(),&n))&&perspective_code(code,patched)){if(SUCCEEDED(d->CreateVertexShader(patched.data(),&value.shader)))++variants;else ++failed;}}
 AcquireSRWLockExclusive(&shaderLock);if(shaders.size()>=128)shaders.erase(shaders.begin());known=shaders.emplace(v,std::move(value)).first->second.shader;ReleaseSRWLockExclusive(&shaderLock);return known;
}
bool card(const Pass&p,const Anchor&a,int e,math::M&map){
 math::M inverse;if(!math::inverse(p.baseView,inverse))return false;
 float normal[3],right[3],up[3];float length=0;for(int j=0;j<3;++j){normal[j]=inverse.v[3][j]-a.world[j];length+=normal[j]*normal[j];}length=std::sqrt(length);if(length<.01f)return false;for(float&v:normal)v/=length;
 const float h=std::hypot(normal[0],normal[1]);if(h<.05f)return false;right[0]=-normal[1]/h;right[1]=normal[0]/h;right[2]=0;
 float alignment=0;for(int j=0;j<3;++j)alignment+=right[j]*p.baseView.v[j][0];if(alignment<0)for(float&v:right)v=-v;
 up[0]=normal[1]*right[2]-normal[2]*right[1];up[1]=normal[2]*right[0]-normal[0]*right[2];up[2]=normal[0]*right[1]-normal[1]*right[0];if(up[2]<0)for(float&v:up)v=-v;
 float clip[4]{};for(int j=0;j<4;++j)clip[j]=a.world[0]*p.baseVP.v[0][j]+a.world[1]*p.baseVP.v[1][j]+a.world[2]*p.baseVP.v[2][j]+p.baseVP.v[3][j];const float w=clip[3];if(w<.001f)return false;
 auto derivative=[&](const float*axis,int c){float dc=0,dw=0;for(int j=0;j<3;++j){dc+=axis[j]*p.baseVP.v[j][c];dw+=axis[j]*p.baseVP.v[j][3];}return (dc*w-clip[c]*dw)/(w*w);};
 const float dx=derivative(right,0),dy=derivative(up,1);if(dx<=1e-7f||dy<=1e-7f)return false;
 float centre[4]{};for(int j=0;j<4;++j){centre[j]=a.world[0]*p.eyeVP[e].v[0][j]+a.world[1]*p.eyeVP[e].v[1][j]+a.world[2]*p.eyeVP[e].v[2][j]+p.eyeVP[e].v[3][j];for(int k=0;k<3;++k){map.v[0][j]+=right[k]*p.eyeVP[e].v[k][j]/dx;map.v[1][j]+=up[k]*p.eyeVP[e].v[k][j]/dy;}map.v[3][j]=centre[j]-a.ref[0]*map.v[0][j]-a.ref[1]*map.v[1][j];}
 if(centre[3]<=.001f)return false;for(auto&row:map.v)for(float&v:row)v/=centre[3];for(auto&row:map.v)row[2]=.5f*row[3];return true;
}
bool ensure(IDirect3DDevice9*d){UINT w=fixtureSize,h=fixtureSize;if(!w&&!vr::output_eye_size(&w,&h))return false;
 for(auto&l:layers){D3DSURFACE_DESC old{};if(l.rt)l.rt->GetDesc(&old);if(l.rt&&old.Width==w&&old.Height==h)continue;l={};if(FAILED(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&l.texture,nullptr))||FAILED(l.texture->GetSurfaceLevel(0,&l.rt))||FAILED(d->CreateDepthStencilSurface(w,h,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&l.ds,nullptr))){++failed;return false;}}
 return true;
}
// A UI piece drawn with a placed-grapple texture but with no anchor this frame.
// Counted against the grapple gun's own answer; hidden ONLY when the switch is on
// AND neither finder currently has a grapple point (the game's "no target").
float orphanXY[2]{};
bool orphan(IDirect3DDevice9*d,HRESULT&result){
 ComPtr<IDirect3DBaseTexture9>tex;d->GetTexture(0,&tex);ComPtr<IDirect3DVertexShader9>vs;d->GetVertexShader(&vs);
 float saved[16]{};if(!vs||FAILED(d->GetVertexShaderConstantF(5,saved,4))||std::fabs(saved[15]-1)>.002f){orphanRunLeft=0;return false;}
 if(!tex){if(!orphanRunLeft)return false;--orphanRunLeft;const float x=saved[12]-orphanXY[0],y=saved[13]-orphanXY[1];if(x*x+y*y>.12f*.12f)return false;result=S_OK;return true;}
 orphanRunLeft=0;if(!learned_texture(tex.Get())||!variant(d,vs.Get()))return false;
 const ULONGLONG now=GetTickCount64();void*targets[2]={fresh_target(0,now),fresh_target(1,now)};const bool target=targets[0]||targets[1];
 if(currentEye<0){++orphans;++(target?orphansWithTarget:orphansNoTarget);if(++orphanLogs<=12)BVR_LOG("[aa grapple] UNANCHORED grapple piece frame%u tex%p screen%.3f/%.3f noAim%p withAim%p learnedOffset%d hide%u",renderPass.frame,tex.Get(),saved[12],saved[13],targets[0],targets[1],learnedOffset.load(),orphanHide&&!target);}
 if(!orphanHide||target)return false;
 if(currentEye<0)++orphansHidden;orphanXY[0]=saved[12];orphanXY[1]=saved[13];orphanRunLeft=8;result=S_OK;return true;
}
}
bool install_engine(uintptr_t base){
 BYTE actual[16]{};const BYTE expected[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x8b,0x45,0x0c,0xf3,0x0f,0x10,0x70,0x08,0xf3,0x0f};
 BYTE call[5]{};if(!read((void*)(base+0x6bc2a0),actual,16)||memcmp(actual,expected,16)||!read((void*)(base+0x658942),call,5)||memcmp(call,"\xe8\x59\x39\x06\x00",5))return false;
 const BYTE vitalsCall[]={0xe8,0x35,0x1f,0x53,0xff},consumer[]={0x8b,0x08,0x8b,0x50,0x04,0x8b,0x40,0x08};BYTE vc[5]{},consumerBytes[8]{};vitalsOwnerGuard=read((void*)(base+0x118a366),vc,5)&&!memcmp(vc,vitalsCall,5)&&read((void*)(base+0x118a36b),consumerBytes,8)&&!memcmp(consumerBytes,consumer,8);BVR_LOG("[aa vitals] native callout projection owner118a36b exact call/consumer guard%u; original line/ring layout retained",vitalsOwnerGuard);
 imageBase=base;if(MH_CreateHook((void*)(base+0x6bc2a0),(void*)project_hook,(void**)&projectOriginal)!=MH_OK)return false;MH_QueueEnableHook((void*)(base+0x6bc2a0));
 const BYTE pixelEntry[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x10,0x8b,0x45,0x08,0xf3,0x0f,0x10,0x60};BYTE pe[16]{},pc[5]{};
 pixelGuard=read((void*)(base+0x6b36c0),pe,16)&&!memcmp(pe,pixelEntry,16)&&read((void*)(base+0x658b6e),pc,5)&&!memcmp(pc,"\xe8\x4d\xab\x05\x00",5);
 if(pixelGuard){pixelGuard=MH_CreateHook((void*)(base+0x6b36c0),(void*)pixel_hook,(void**)&pixelOriginal)==MH_OK;if(pixelGuard)pixelGuard=MH_QueueEnableHook((void*)(base+0x6b36c0))==MH_OK;}
 BVR_LOG("[aa markers pixel] exact Canvas.WorldToPixel helper/caller guard%u; original valid-result flag retained",pixelGuard);
 // A2h: grapple finders (VA17075e0/1707620) and UIScreenObject.Project (VA dc3550), exact entry bytes.
 const BYTE noAim[]={0x8b,0x44,0x24,0x04,0x83,0x40,0x18,0x01,0x56,0x8b,0xf1,0x8b,0x48,0x18,0x80,0x39},withAim[]={0x83,0xec,0x1c,0x56,0x8b,0x74,0x24,0x24,0x8b,0x46,0x18,0x0f,0xb6,0x10,0x83,0xc0};
 const BYTE uiEntry[]={0x55,0x8b,0xec,0x83,0xe4,0xf0,0x83,0xec,0x6c,0x56,0x8d,0x44,0x24,0x30,0x50,0x8b};
 const struct{uintptr_t rva;const BYTE*bytes;void*hook;void**orig;bool*guard;const char*name;}extra[]={{0x13075e0,noAim,(void*)no_aim_hook,(void**)&finderOriginal[0],&finderGuard[0],"FindGrapplePointNoAim"},{0x1307620,withAim,(void*)with_aim_hook,(void**)&finderOriginal[1],&finderGuard[1],"FindGrapplePointWithAim"},{0x9c3550,uiEntry,(void*)ui_project_hook,(void**)&uiProjectOriginal,&uiProjectGuard,"UIScreenObject.Project"}};
 for(const auto&x:extra){BYTE got[16]{};*x.guard=read((void*)(base+x.rva),got,16)&&!memcmp(got,x.bytes,16)&&MH_CreateHook((void*)(base+x.rva),x.hook,x.orig)==MH_OK&&MH_QueueEnableHook((void*)(base+x.rva))==MH_OK;BVR_LOG("[aa grapple] %s exact entry guard%u (observe only, original runs first)",x.name,*x.guard);}
 swprintf_s(settings,L"%s\\aa_rendering.ini",log::data_dir());on=GetPrivateProfileIntW(L"AsylumVR",L"WorldGrappleMarkers",1,settings)!=0;orphanHide=GetPrivateProfileIntW(L"AsylumVR",L"HideTargetlessGrapple",0,settings)!=0;{wchar_t v[32]{};GetPrivateProfileStringW(L"AsylumVR",L"HUDDistance",L"4",v,32,settings);const float m=float(wcstod(v,nullptr));pointerDistance=(std::isfinite(m)?std::clamp(m,1.f,16.f):4.f)*90.f;pointerDegrees=std::clamp(float(GetPrivateProfileIntW(L"AsylumVR",L"GrapplePointerDegrees",26,settings)),10.f,40.f);}connectorsOn=GetPrivateProfileIntW(L"AsylumVR",L"WorldVitalsConnectors",1,settings)!=0;BVR_LOG("[aa markers] exact Canvas.Project caller/ret8 ABI; head-free target and per-vertex world card, default%u",on.load());return true;
}
void publish(uint32_t slot,uint32_t frame,uint32_t gen,void*view,const math::M&bv,const math::M&bp,const math::M eyeVP[2],float referenceYaw,const float*referencePosition,const math::M*eyeView,bool headHud,float unitsPerMetre){Pass p{};p.unitsPerMetre=unitsPerMetre;p.headHud=headHud;if(eyeView){p.eyeView[0]=eyeView[0];p.eyeView[1]=eyeView[1];p.haveEyeView=true;}p.serial=slot;p.frame=frame;p.gen=gen;p.view=view;p.baseView=bv;p.baseP=bp;p.baseVP=math::mul(bv,bp);p.eyeVP[0]=eyeVP[0];p.eyeVP[1]=eyeVP[1];p.referenceYaw=referenceYaw;if(referencePosition)memcpy(p.referencePosition,referencePosition,12);p.valid=true;gamePass=p;AcquireSRWLockExclusive(&lock);passes[slot%passes.size()]=p;ReleaseSRWLockExclusive(&lock);grapple_anchors();}
void end_game_pass(){gamePass.valid=false;}
void begin_render(uint32_t slot,bool active){AcquireSRWLockShared(&lock);renderPass=passes[slot%passes.size()];ReleaseSRWLockShared(&lock);renderPass.serial=slot;renderActive=active;}
void begin_eye(int eye,uint32_t gen){currentEye=eye;currentGen=gen;if(eye<=0){for(auto&l:layers)l.painted=false;}}
void vertex_created(IDirect3DVertexShader9*v){AcquireSRWLockExclusive(&shaderLock);shaders.erase(v);ReleaseSRWLockExclusive(&shaderLock);}
bool draw(IDirect3DDevice9*d,bool uiLane,UINT primitives,const std::function<HRESULT()>&original,HRESULT&result){
 if(internal||!on||!currentEye||!uiLane||!renderActive||primitives>512)return false;
 // BeginDrawing may beat constructor publication: refresh this exact slot.
AcquireSRWLockShared(&lock);Pass p=passes[renderPass.serial%passes.size()];Set set=sets[p.frame%sets.size()];const Set last=held;const ULONGLONG lastAt=heldAt;ReleaseSRWLockShared(&lock);
 const bool passOk=p.valid&&p.serial==renderPass.serial&&p.gen==currentGen;if(!passOk){++stale;return false;}
 if(!set.valid||set.frame!=p.frame||!set.n){if(last.n&&GetTickCount64()-lastAt<=kHoldMs){set=last;++heldUsed;}else{++stale;return orphan(d,result);}}
 ComPtr<IDirect3DVertexShader9>vs;d->GetVertexShader(&vs);if(!vs)return false;auto patched=variant(d,vs.Get());if(!patched){++shaderMisses;return false;}
 float saved[16]{};if(FAILED(d->GetVertexShaderConstantF(5,saved,4))||std::fabs(saved[15]-1)>.002f||std::fabs(saved[14])>.002f||std::fabs(saved[3])>.002f||std::fabs(saved[7])>.002f)return false;
 unsigned best=set.n;float nearest=.12f*.12f;for(unsigned i=0;i<set.n;++i){if(set.points[i].off&&!set.points[i].gameRef)continue;float x=saved[12]-set.points[i].ref[0],y=saved[13]-set.points[i].ref[1],dist=x*x+y*y;if(dist<nearest){nearest=dist;best=i;}}
 // A2i: no on-screen match -> the game's EDGE ring for an off-screen point lies
 // along that point's screen direction (any radius the game clamps it to).
 bool off=best<set.n&&set.points[best].off,byDirection=false;if(best==set.n){byDirection=true;const float radius=std::hypot(saved[12],saved[13]);float bestCos=kOffMatchCos;
  if(radius>kOffMinRadius)for(unsigned i=0;i<set.n;++i){if(!set.points[i].off)continue;const float c=(saved[12]*set.points[i].dir[0]+saved[13]*set.points[i].dir[1])/radius;if(c>bestCos){bestCos=c;best=i;}}
  if(best==set.n){++misses;return orphan(d,result);}off=true;}
 DWORD writes{};d->GetRenderState(D3DRS_COLORWRITEENABLE,&writes);if(!(writes&7)){runLeft=0;++maskRejects;return false;}
 ComPtr<IDirect3DBaseTexture9>tex;d->GetTexture(0,&tex);ComPtr<IDirect3DTexture9>texture;D3DSURFACE_DESC td{};if(runSlot!=p.serial){runSlot=p.serial;runLeft=0;runAnchor=64;}
 const bool spriteTexture=tex&&SUCCEEDED(tex.As(&texture))&&SUCCEEDED(texture->GetLevelDesc(0,&td))&&td.Width<=256&&td.Height<=256&&td.Width>=8&&td.Height>=8;
 DWORD blending{},sourceBlend{},destinationBlend{};d->GetRenderState(D3DRS_ALPHABLENDENABLE,&blending);d->GetRenderState(D3DRS_SRCBLEND,&sourceBlend);d->GetRenderState(D3DRS_DESTBLEND,&destinationBlend);
 const bool multiply=blending&&((sourceBlend==D3DBLEND_DESTCOLOR&&destinationBlend==D3DBLEND_ZERO)||(sourceBlend==D3DBLEND_ZERO&&destinationBlend==D3DBLEND_SRCCOLOR));
 if(multiply||(blending&&sourceBlend==D3DBLEND_ZERO)){runLeft=0;return false;}
 if(byDirection&&spriteTexture&&!learned_texture(tex.Get())){++offUnlearned;if(++offLogs<=12)BVR_LOG("[aa grapple] edge-ring candidate NOT a learned grapple texture tex%p %ux%u screen%.3f/%.3f dir%.3f/%.3f",tex.Get(),td.Width,td.Height,saved[12],saved[13],set.points[best].dir[0],set.points[best].dir[1]);runLeft=0;return orphan(d,result);}
 if(!byDirection&&spriteTexture&&!learned_texture(tex.Get())){if(grappleTextureCount<grappleTextures.size()){grappleTextures[grappleTextureCount++]=tex.Get();BVR_LOG("[aa grapple] placed-marker texture%u %p %ux%u format%u",grappleTextureCount,tex.Get(),td.Width,td.Height,td.Format);}else if(grappleTextureCount++==grappleTextures.size())BVR_LOG("[aa grapple] placed-marker texture table FULL; later textures not tracked");}
 if(spriteTexture){runAnchor=best;runLeft=8;runRef[0]=saved[12];runRef[1]=saved[13];}else if(!runLeft||runAnchor!=best)return false;else --runLeft;
 if(currentEye>0){result=S_OK;return true;} // LEFT captured both, including AFW's other eye.
 math::M mapped[2]{};const auto&a=set.points[best];math::M placement{};memcpy(&placement,saved,64);
 if(!off){for(int e=0;e<2;++e){math::M k{};if(!card(p,a,e,k))return false;mapped[e]=math::mul(placement,k);}}
 else{
  // Off the game screen: on the point itself when both eyes see it, else the
  // head-locked edge pointer. The run's first piece is the reference, so the
  // ring/RB pieces keep their own layout around it.
  Anchor t=a;t.off=false;t.ref[0]=runRef[0];t.ref[1]=runRef[1];math::M k[2]{};const bool view=in_view(p,a.world);bool placed=view&&card(p,t,0,k[0])&&card(p,t,1,k[1]);
  if(!placed&&view){Pass q;placed=eye_pass(p,q)&&card(q,t,0,k[0])&&card(q,t,1,k[1]);}
  if(placed){if(spriteTexture)++offOnTarget;}else{Pass q;Anchor e;placed=pointer(p,a,q,e);if(placed){e.ref[0]=runRef[0];e.ref[1]=runRef[1];placed=card(q,e,0,k[0])&&card(q,e,1,k[1]);}if(placed&&spriteTexture)++offPointer;}
  if(spriteTexture){++offMatched;if(++offLogs<=12)BVR_LOG("[aa grapple] edge ring frame%u screen%.3f/%.3f dir%.3f/%.3f -> %s",p.frame,saved[12],saved[13],a.dir[0],a.dir[1],placed?(in_view(p,a.world)?"on point":"edge pointer"):"hidden");}
  if(!placed){if(spriteTexture)++offHidden;result=S_OK;return true;} // never left floating on the HUD sheet
  for(int e=0;e<2;++e)mapped[e]=math::mul(placement,k[e]);
 }
 if(!ensure(d))return false;ComPtr<IDirect3DSurface9>rt,ds;D3DVIEWPORT9 vp{};if(FAILED(d->GetRenderTarget(0,&rt))||FAILED(d->GetViewport(&vp)))return false;d->GetDepthStencilSurface(&ds);
 constexpr D3DRENDERSTATETYPE keys[]={D3DRS_SCISSORTESTENABLE,D3DRS_STENCILENABLE,D3DRS_ZENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_SRCBLENDALPHA,D3DRS_DESTBLENDALPHA,D3DRS_BLENDOPALPHA,D3DRS_COLORWRITEENABLE};DWORD originalStates[9]{};for(int i=0;i<9;++i)d->GetRenderState(keys[i],originalStates+i);
 internal=true;result=S_OK;for(int e=0;e<2;++e){auto&l=layers[e];D3DSURFACE_DESC desc{};l.rt->GetDesc(&desc);d->SetDepthStencilSurface(nullptr);if(FAILED(d->SetRenderTarget(0,l.rt.Get()))||FAILED(d->SetDepthStencilSurface(l.ds.Get()))){++failed;result=E_FAIL;break;}D3DVIEWPORT9 full{0,0,desc.Width,desc.Height,0,1};d->SetViewport(&full);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLENDALPHA,!blending||destinationBlend==D3DBLEND_INVSRCALPHA?D3DBLEND_ONE:D3DBLEND_ZERO);d->SetRenderState(D3DRS_DESTBLENDALPHA,!blending||destinationBlend==D3DBLEND_INVSRCALPHA?D3DBLEND_INVSRCALPHA:D3DBLEND_ONE);d->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD);d->SetRenderState(D3DRS_COLORWRITEENABLE,writes|8);
  if(!l.painted)d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,1,0);
  d->SetVertexShader(patched.Get());d->SetVertexShaderConstantF(5,&mapped[e].v[0][0],4);result=original();if(FAILED(result)){++failed;break;}l.painted=true;++routed;
 }
 d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,rt.Get());d->SetDepthStencilSurface(ds.Get());d->SetVertexShader(vs.Get());d->SetVertexShaderConstantF(5,saved,4);for(int i=0;i<9;++i)d->SetRenderState(keys[i],originalStates[i]);d->SetViewport(&vp);internal=false;if(FAILED(result)){for(auto&l:layers)l.painted=false;return false;}return true;
}
IDirect3DSurface9* sheet(int eye){return on&&eye>=0&&eye<2&&layers[eye].painted?layers[eye].rt.Get():nullptr;}
uint32_t generation(){return currentGen;}
void reset(){grappleTextureCount=0;held={};heldAt=0;orphanRunLeft=0;internal=true;AcquireSRWLockExclusive(&shaderLock);shaders.clear();ReleaseSRWLockExclusive(&shaderLock);for(auto&l:layers)l={};internal=false;renderActive=false;renderPass={};gamePass={};AcquireSRWLockExclusive(&lock);for(auto&p:passes)p={};for(auto&s:sets)s={};hudPlane={};ReleaseSRWLockExclusive(&lock);}
void draw_ui(){bool connected=connectorsOn.load();if(ImGui::Checkbox("Vital-statistics line and circle follow the person",&connected)){connectorsOn=connected;WritePrivateProfileStringW(L"AsylumVR",L"WorldVitalsConnectors",connected?L"1":L"0",settings);}bool selected=on.load();if(ImGui::Checkbox("Grapple reticle sits on its world target",&selected)){on=selected;WritePrivateProfileStringW(L"AsylumVR",L"WorldGrappleMarkers",selected?L"1":L"0",settings);}}
void publish_hud_plane(const float world[4][3],bool headFollowing,const float*shape){HudPlane plane{};plane.headFollowing=headFollowing;if(shape)memcpy(plane.shape,shape,sizeof(plane.shape));if(world){plane.valid=true;for(int i=0;i<4;++i)for(int j=0;j<3;++j){plane.world[i][j]=world[i][j];if(!std::isfinite(world[i][j]))plane.valid=false;}}AcquireSRWLockExclusive(&lock);hudPlane=plane;ReleaseSRWLockExclusive(&lock);}
void invalidate_hud_plane(){publish_hud_plane(nullptr);}
void status_line(char*out,size_t n){snprintf(out,n,"enabled%u projected%u placed%u variants%u shaderMiss%u noMatch%u stale%u masks%u failed%u vitalsOwner%u vitalsAdjusted%u vitalsFallback%u pixelGuard%u pixelCalls%u pixelPlaced%u pixelRejected%u | grappleGuard%u%u%u noAim%u/%u withAim%u/%u learnedOffset%d grappleAnchors%u unanchored%u withTarget%u noTarget%u hidden%u hideSwitch%u textures%u | offPoints%u offRejected%u edgeRings%u onPoint%u pointer%u edgeHidden%u edgeUnlearned%u heldFrames%u",on.load(),projected.load(),routed,variants,shaderMisses,misses,stale,maskRejects,failed,vitalsOwnerGuard,vitalsAdjusted.load(),vitalsFallback.load(),pixelGuard,pixelCalls.load(),pixelPlaced.load(),pixelRejected.load(),finderGuard[0],finderGuard[1],uiProjectGuard,grapple[0].found.load(),grapple[0].calls.load(),grapple[1].found.load(),grapple[1].calls.load(),learnedOffset.load(),grappleAnchors.load(),orphans.load(),orphansWithTarget.load(),orphansNoTarget.load(),orphansHidden.load(),orphanHide.load(),grappleTextureCount,offAnchors.load(),offRejected.load(),offMatched.load(),offOnTarget.load(),offPointer.load(),offHidden.load(),offUnlearned.load(),heldUsed.load());census_log(projectCensus,false);census_log(pixelCensus,false);census_log(uiCensus,false);}
bool eye_origins(uint32_t generation,float left[3],float right[3]){
 AcquireSRWLockShared(&lock);Pass p{};bool found=false;for(const auto&q:passes)if(q.valid&&q.gen==generation&&q.haveEyeView){p=q;found=true;break;}ReleaseSRWLockShared(&lock);if(!found)return false;
 math::M a,b;if(!math::inverse(p.eyeView[0],a)||!math::inverse(p.eyeView[1],b))return false;for(int j=0;j<3;++j){left[j]=a.v[3][j];right[j]=b.v[3][j];}return true;
}
bool command(const char*args){
 if(!_stricmp(args,"hide on")||!_stricmp(args,"hide off")){orphanHide=!_stricmp(args,"hide on");WritePrivateProfileStringW(L"AsylumVR",L"HideTargetlessGrapple",orphanHide?L"1":L"0",settings);BVR_LOG("[aa grapple] hide targetless rings %u",orphanHide.load());return true;}
 if(!_stricmp(args,"census")){census_log(projectCensus,true);census_log(pixelCensus,true);census_log(uiCensus,true);return true;}
 return false;
}
bool fixture(IDirect3DDevice9*d,const wchar_t*directory,bool(*completed)(IDirect3DSurface9*,IDirect3DSurface9*)){
 ComPtr<IDirect3DStateBlock9>state;ComPtr<IDirect3DSurface9>savedRt,savedDs;D3DVIEWPORT9 savedVp{};if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&state))||FAILED(d->GetRenderTarget(0,&savedRt)))return false;d->GetDepthStencilSurface(&savedDs);d->GetViewport(&savedVp);
 const bool oldOn=on;struct Restore{IDirect3DDevice9*d;IDirect3DStateBlock9*s;IDirect3DSurface9*rt,*ds;D3DVIEWPORT9 vp;bool on;~Restore(){reset();fixtureSize=0;markers9::on=on;d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,rt);d->SetDepthStencilSurface(ds);s->Apply();d->SetViewport(&vp);}}restore{d,state.Get(),savedRt.Get(),savedDs.Get(),savedVp,oldOn};on=true;fixtureSize=128;reset();
 auto load=[&](const wchar_t*name,std::vector<DWORD>&out){wchar_t path[MAX_PATH]{};swprintf_s(path,L"%s\\%s.bin",directory,name);FILE*f=_wfopen(path,L"rb");if(!f)return false;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<8||n%4||n>32768){fclose(f);return false;}out.resize(n/4);bool ok=fread(out.data(),1,n,f)==n;fclose(f);return ok;};
 std::vector<DWORD>vc,cc,pc;if(!load(L"ui-live-complex-vs",vc)||!load(L"ui-clip-vs",cc)||!load(L"ui-constant-ps",pc))return false;ComPtr<IDirect3DVertexShader9>vs,clipVS;ComPtr<IDirect3DPixelShader9>ps;if(FAILED(d->CreateVertexShader(vc.data(),&vs))||FAILED(d->CreateVertexShader(cc.data(),&clipVS))||FAILED(d->CreatePixelShader(pc.data(),&ps)))return false;
 std::vector<DWORD>modified;if(!perspective_code(vc,modified)||perspective_code(cc,modified))return false;
 struct Vertex{float x,y,z,w;DWORD colour,colour1;float u,v;};const D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,16,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},{0,20,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,1},{0,24,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};ComPtr<IDirect3DVertexDeclaration9>decl;ComPtr<IDirect3DTexture9>scene,spriteTexture;ComPtr<IDirect3DSurface9>rt;
 if(FAILED(d->CreateVertexDeclaration(elements,&decl))||FAILED(d->CreateTexture(128,128,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&scene,nullptr))||FAILED(scene->GetSurfaceLevel(0,&rt))||FAILED(d->CreateTexture(32,32,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&spriteTexture,nullptr)))return false;
 auto readPixels=[&](IDirect3DSurface9*surface,std::vector<BYTE>&out){ComPtr<IDirect3DSurface9>cpu;if(FAILED(d->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&cpu,nullptr))||FAILED(d->GetRenderTargetData(surface,cpu.Get())))return false;D3DLOCKED_RECT m{};if(FAILED(cpu->LockRect(&m,nullptr,D3DLOCK_READONLY)))return false;out.resize(128*128*4);for(int y=0;y<128;++y)memcpy(out.data()+y*128*4,static_cast<BYTE*>(m.pBits)+y*m.Pitch,128*4);cpu->UnlockRect();return true;};
 const Vertex source[]={{-1,1,0,1,~0u,~0u,0,0},{1,1,0,1,~0u,~0u,1,0},{-1,-1,0,1,~0u,~0u,0,1},{1,-1,0,1,~0u,~0u,1,1}};const float white[]={1,1,1,1},scale[]={.25f,0,0,0};d->SetVertexDeclaration(decl.Get());d->SetPixelShader(ps.Get());d->SetPixelShaderConstantF(0,white,1);d->SetVertexShaderConstantF(13,scale,1);d->SetTexture(0,spriteTexture.Get());d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
 unsigned cases=0,oldDiff=0;uint32_t frame=3000;
 for(float distance:{450.f,1800.f})for(float heading:{0.f,.35f})for(int motion=0;motion<4;++motion){
  math::M bv{};const float c=std::cos(heading),s=std::sin(heading);bv.v[0][0]=-s;bv.v[1][0]=c;bv.v[2][1]=1;bv.v[0][2]=c;bv.v[1][2]=s;bv.v[3][3]=1;math::M bp{};bp.v[0][0]=1;bp.v[1][1]=1.1f;bp.v[2][2]=1.001f;bp.v[2][3]=1;bp.v[3][2]=-.1f;
  math::M eyeVP[2]{};for(int e=0;e<2;++e){vr::HeadPose pose{};pose.qw=1;pose.px=e?.032f:-.032f;if(motion==1){pose.qy=std::sin(.35f*.5f);pose.qw=std::cos(.35f*.5f);}if(motion==2){pose.qz=std::sin(.4f*.5f);pose.qw=std::cos(.4f*.5f);}if(motion==3){pose.px+=.12f;pose.py=.06f;pose.pz=.08f;}const float angles[]={e?-.65f:-.9f,e?.9f:.65f,.8f,-.85f};eyeVP[e]=math::mul(math::mul(bv,math::pose_view(pose)),math::projection(bp,angles));}
  float world[]={distance*c,distance*s,0},reference[]={9,9,9,9};publish(frame%64,frame,frame,nullptr,bv,bp,eyeVP);anchor(world,reference);if(std::fabs(reference[0])>.0001f||std::fabs(reference[1])>.0001f||std::fabs(reference[3]-distance)>.01f)return false;begin_render(frame%64,true);begin_eye(-1,frame);
  math::M placement{};placement.v[0][0]=placement.v[1][1]=.06f;placement.v[2][2]=placement.v[3][3]=1;d->SetVertexShader(vs.Get());d->SetVertexShaderConstantF(5,&placement.v[0][0],4);d->SetDepthStencilSurface(nullptr);d->SetRenderTarget(0,rt.Get());D3DVIEWPORT9 vp{0,0,128,128,0,1};d->SetViewport(&vp);d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff203040,1,0);std::vector<BYTE>before,after;if(!readPixels(rt.Get(),before)||FAILED(d->BeginScene()))return false;HRESULT result{};
  if(!draw(d,true,2,[&]{return d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,source,sizeof(Vertex));},result)||FAILED(result))return false;d->EndScene();if(!readPixels(rt.Get(),after)||after!=before)return false;float restored[16]{};d->GetVertexShaderConstantF(5,restored,4);ComPtr<IDirect3DVertexShader9>restoredVS;d->GetVertexShader(&restoredVS);if(memcmp(restored,&placement,64)||restoredVS.Get()!=vs.Get())return false;
  for(int e=0;e<2;++e){std::vector<BYTE>actual,expected,old;if(!sheet(e)||!readPixels(sheet(e),actual))return false;
   Vertex projectedVertices[4];for(int i=0;i<4;++i){auto&v=projectedVertices[i];v=source[i];const float horizontal=source[i].x*.06f*distance,vertical=source[i].y*.06f*distance/1.1f;const float corner[]={world[0]-s*horizontal,world[1]+c*horizontal,vertical};float clip[4]{};for(int j=0;j<4;++j)clip[j]=corner[0]*eyeVP[e].v[0][j]+corner[1]*eyeVP[e].v[1][j]+corner[2]*eyeVP[e].v[2][j]+eyeVP[e].v[3][j];v.x=clip[0];v.y=clip[1];v.z=.5f*clip[3];v.w=clip[3];}
   d->SetVertexShader(clipVS.Get());d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);d->BeginScene();d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,projectedVertices,sizeof(Vertex));d->EndScene();if(!readPixels(rt.Get(),expected))return false;
   unsigned lit=0;for(size_t n=0;n<actual.size();++n){if(std::abs(int(actual[n])-int(expected[n]))>1){printf("Marker reference mismatch case%u eye%d byte%zu got%u want%u\n",cases,e,n,actual[n],expected[n]);return false;}if(n%4==3&&actual[n])++lit;}if(!lit)return false;
   math::M k{},oldPlacement{};if(!card(gamePass,sets[frame%16].points[0],e,k))return false;oldPlacement=math::mul(placement,k);d->SetVertexShader(vs.Get());d->SetVertexShaderConstantF(5,&oldPlacement.v[0][0],4);d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);d->BeginScene();d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,source,sizeof(Vertex));d->EndScene();if(!readPixels(rt.Get(),old))return false;if(old!=expected)++oldDiff;++cases;
  }
  ++frame;
 }

 if(oldDiff<8)return false;if(completed&&!completed(sheet(0),sheet(1)))return false;
 // Real candidate state negatives: mask/erase/text atlas/unknown shader must
 // remain on the original HUD path, and never execute the routed draw.
 begin_eye(-1,frame-1);math::M originalPlacement{};originalPlacement.v[0][0]=originalPlacement.v[1][1]=.06f;originalPlacement.v[2][2]=originalPlacement.v[3][3]=1;d->SetVertexShader(vs.Get());d->SetVertexShaderConstantF(5,&originalPlacement.v[0][0],4);d->SetTexture(0,spriteTexture.Get());HRESULT rejected{};
 d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);if(draw(d,true,2,[&]{return E_FAIL;},rejected))return false;d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
 d->SetRenderState(D3DRS_COLORWRITEENABLE,0);if(draw(d,true,2,[&]{return E_FAIL;},rejected))return false;d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
 ComPtr<IDirect3DTexture9>atlas;if(FAILED(d->CreateTexture(512,512,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&atlas,nullptr)))return false;d->SetTexture(0,atlas.Get());if(draw(d,true,2,[&]{return E_FAIL;},rejected))return false;
 d->SetTexture(0,spriteTexture.Get());d->SetVertexShader(clipVS.Get());if(draw(d,true,2,[&]{return E_FAIL;},rejected))return false;

 // Empty/new/mismatched passes and unknown perspective programs don't route.
 begin_eye(-1,frame+1);HRESULT result{};if(draw(d,true,2,[&]{return S_OK;},result)||sheet(0)||sheet(1))return false;begin_render(999,true);if(draw(d,true,2,[&]{return S_OK;},result))return false;

 // Correct the native OWNER's position, not independently matched UI draws.
 // Independent analytic plane hits verify camera turn, translation and exact
 // recenter inversion; LEFT/RIGHT original XY differ but corrected XY coincide.
 const bool previousConnectors=connectorsOn;connectorsOn=true;
 struct RestoreVitals{bool value;~RestoreVitals(){connectorsOn=value;}}restoreVitals{previousConnectors};
 unsigned ownerChecks=0;void*ownerView=reinterpret_cast<void*>(0x1234);
 for(float distance:{450.f,1800.f})for(float heading:{0.f,.35f})for(int reference=0;reference<2;++reference)for(int motion=0;motion<4;++motion){
  const float yaw=reference?.6f:0.f,origin[3]={reference?2.f:0.f,reference?1.5f:0.f,reference?-3.f:0.f},cosine=std::cos(yaw),sine=std::sin(yaw);
  auto absolute=[&](const float*local,float*out){out[0]=origin[0]+cosine*local[0]+sine*local[2];out[1]=origin[1]+local[1];out[2]=origin[2]-sine*local[0]+cosine*local[2];};
  Pass p{};p.valid=true;p.view=ownerView;p.gen=61000+ownerChecks;p.referenceYaw=yaw;memcpy(p.referencePosition,origin,12);
  const float c=std::cos(heading),ss=std::sin(heading);p.baseView.v[0][0]=-ss;p.baseView.v[1][0]=c;p.baseView.v[2][1]=1;p.baseView.v[0][2]=c;p.baseView.v[1][2]=ss;p.baseView.v[3][3]=1;
  math::M bp{};bp.v[0][0]=bp.v[1][1]=1;bp.v[2][2]=1.001f;bp.v[2][3]=1;bp.v[3][2]=-.1f;p.baseVP=math::mul(p.baseView,bp);
  Anchor actor{};const float lateral=45.f,vertical=90.f;actor.world[0]=distance*c-lateral*ss;actor.world[1]=distance*ss+lateral*c;actor.world[2]=vertical;
  HudPlane plane{};plane.valid=true;for(int i=0;i<4;++i){const float corner[3]={i&1?3.f:-3.f,i&2?-3.f:3.f,-4.f};absolute(corner,plane.world[i]);}
  const float localHead[3]={motion==3?.12f:0.f,motion==3?.06f:0.f,motion==3?.08f:0.f};float head[3]{};absolute(localHead,head);
  const float hitFraction=(-4-localHead[2])/(-distance/90-localHead[2]);const float expected[2]={(localHead[0]+hitFraction*(lateral/90-localHead[0]))/3,(localHead[1]+hitFraction*(vertical/90-localHead[1]))/3};
  float corrected[2][4]{};for(int e=0;e<2;++e){vr::HeadPose pose{};pose.qw=1;pose.px=localHead[0]+(e?.032f:-.032f);pose.py=localHead[1];pose.pz=localHead[2];if(motion==1){pose.qy=std::sin(.35f*.5f);pose.qw=std::cos(.35f*.5f);}if(motion==2){pose.qz=std::sin(.4f*.5f);pose.qw=std::cos(.4f*.5f);}const float fov[]={e?-.65f:-.9f,e?.9f:.65f,.8f,-.85f};const auto vp=math::mul(math::mul(p.baseView,math::pose_view(pose)),math::projection(bp,fov));float original[4]{};if(!project(vp,actor.world,original))return false;memcpy(corrected[e],original,16);
   if(!vitals_projection(p,ownerView,0x118a36b,actor,plane,head,corrected[e])||memcmp(corrected[e]+2,original+2,8))return false;
   for(int j=0;j<2;++j)if(std::fabs(corrected[e][j]-expected[j])>.00001f)return false;
   // Wrong caller/view, disabled setting, invalid plane/pass, behind and edge
   // arrows must retain original output. No piece classification is involved.
   auto rejects=[&](const Pass&pass,void*view,uintptr_t caller,const HudPlane&hud,float*value){float before[4];memcpy(before,value,16);return !vitals_projection(pass,view,caller,actor,hud,head,value)&&!memcmp(before,value,16);};
   float probe[4];memcpy(probe,original,16);if(!rejects(p,ownerView,0x658947,plane,probe)||!rejects(p,nullptr,0x118a36b,plane,probe))return false;Pass inactive=p;inactive.valid=false;if(!rejects(inactive,ownerView,0x118a36b,plane,probe))return false;HudPlane absent{};if(!rejects(p,ownerView,0x118a36b,absent,probe))return false;connectorsOn=false;if(!rejects(p,ownerView,0x118a36b,plane,probe))return false;connectorsOn=true;probe[3]=-1;if(!rejects(p,ownerView,0x118a36b,plane,probe))return false;probe[3]=original[3];probe[0]=1.2f;if(!rejects(p,ownerView,0x118a36b,plane,probe))return false;
   ++ownerChecks;
  }
  if(memcmp(corrected[0],corrected[1],8))return false;
 }
 float planeCorners[4][3]={{-3,3,-4},{3,3,-4},{-3,-3,-4},{3,-3,-4}};publish_hud_plane(planeCorners);if(!hudPlane.valid)return false;invalidate_hud_plane();if(hudPlane.valid)return false;publish_hud_plane(planeCorners);reset();if(hudPlane.valid)return false;
 printf("Vitals native owner: %u BOTH-eye analytic shared-head-ray/HUD-plane positions; near/far actor, game camera turn, head yaw/roll/lean, asymmetric lenses and recenter; LEFT/RIGHT XY identical, original Z/W bit-exact; unrelated caller/view/pass/plane/behind/edge/toggle unchanged; plane recenter/reset invalidation PASS\n",ownerChecks);
 // The second native API retains its Boolean and outputs on all negatives;
 // valid points share the same anchor/card route as Canvas.Project.
 unsigned pixelChecks=0;void*pixelView=reinterpret_cast<void*>(0x2345);
 for(float distance:{450.f,1800.f})for(float heading:{0.f,.35f})for(int motion=0;motion<4;++motion){
  const float c=std::cos(heading),ss=std::sin(heading);math::M bv{};bv.v[0][0]=-ss;bv.v[1][0]=c;bv.v[2][1]=1;bv.v[0][2]=c;bv.v[1][2]=ss;bv.v[3][3]=1;math::M bp{};bp.v[0][0]=1;bp.v[1][1]=1.1f;bp.v[2][2]=1.001f;bp.v[2][3]=1;bp.v[3][2]=-.1f;
  math::M ep[2]{};for(int e=0;e<2;++e){vr::HeadPose pose{};pose.qw=1;pose.px=e?.032f:-.032f;if(motion==1){pose.qy=std::sin(.35f*.5f);pose.qw=std::cos(.35f*.5f);}if(motion==2){pose.qz=std::sin(.4f*.5f);pose.qw=std::cos(.4f*.5f);}if(motion==3){pose.px+=.12f;pose.py=.06f;pose.pz=.08f;}ep[e]=math::mul(math::mul(bv,math::pose_view(pose)),bp);}
  float point[]={distance*c-45*ss,distance*ss+45*c,90};const uint32_t frame=71000+pixelChecks;publish(frame%64,frame,frame,pixelView,bv,bp,ep);const float rect[]={17,23,1890,2026};float xy[2]{};
  if(!pixel_projection(gamePass,pixelView,0x658b73,TRUE,point,rect,xy))return false;
  const float expected[2]={17+(45/distance+1)*.5f*1890,23+(1-90*1.1f/distance)*.5f*2026};for(int j=0;j<2;++j)if(std::fabs(xy[j]-expected[j])>.001f)return false;
  float ref[4]{};if(!anchor(point,ref))return false;AcquireSRWLockShared(&lock);const Set registered=sets[frame%sets.size()];ReleaseSRWLockShared(&lock);if(!registered.valid||registered.frame!=frame||registered.n!=1||memcmp(registered.points[0].world,point,12)||std::fabs(registered.points[0].ref[0]-45/distance)>.00001f)return false;
  for(int e=0;e<2;++e){math::M mapped{};if(!card(gamePass,registered.points[0],e,mapped))return false;float projectedPoint[4]{};if(!project(ep[e],point,projectedPoint))return false;for(int j=0;j<2;++j){float placed=mapped.v[3][j]+ref[0]*mapped.v[0][j]+ref[1]*mapped.v[1][j];if(std::fabs(placed-projectedPoint[j])>.00001f)return false;}++pixelChecks;}
  float sentinel[]={-123,456};auto untouched=[&](void*view,uintptr_t caller,BOOL success,const float*target,const float*viewport){float out[2];memcpy(out,sentinel,8);return !pixel_projection(gamePass,view,caller,success,target,viewport,out)&&!memcmp(out,sentinel,8);};
  if(!untouched(pixelView,0x658b73,FALSE,point,rect)||!untouched(nullptr,0x658b73,TRUE,point,rect)||!untouched(pixelView,0x658947,TRUE,point,rect))return false;
  float behind[]={-distance*c,-distance*ss,0};if(!untouched(pixelView,0x658b73,TRUE,behind,rect))return false;float outside[]={distance*c-2*distance*ss,distance*ss+2*distance*c,0};if(!untouched(pixelView,0x658b73,TRUE,outside,rect))return false;const float zeroRect[]={0,0,0,2026};if(!untouched(pixelView,0x658b73,TRUE,point,zeroRect))return false;on=false;if(!untouched(pixelView,0x658b73,TRUE,point,rect))return false;on=true;gamePass.valid=false;if(!untouched(pixelView,0x658b73,TRUE,point,rect))return false;
 }
 printf("Canvas WorldToPixel: %u BOTH-eye head-free pixel/world-card cases, native rectangle offsets, near/far/camera turn/head yaw/roll/lean; original-false output/behind/offscreen/foreign caller/view/empty rect/disabled/stale preserved PASS\n",pixelChecks);
 // A2h: the grapple point's stored world point is learned from accepted anchors,
 // then anchored from the finder alone; stale/null finders and decoys add nothing.
 {struct RestoreGrapple{~RestoreGrapple(){learnedOffset=-1;offsetVotes={};offsetScans=offsetScanLogs=0;for(auto&g:grapple){g.object=nullptr;g.at=0;}grappleAnchors=0;}}restoreGrapple;
  static float object[kScanBytes/4];for(auto&v:object)v=-7777.f;const float target[3]={1234.5f,-321.f,88.f};memcpy(object+0x41,target,12);
  math::M bv{};bv.v[1][0]=1;bv.v[2][1]=1;bv.v[0][2]=1;bv.v[3][3]=1;math::M bp{};bp.v[0][0]=1;bp.v[1][1]=1.1f;bp.v[2][2]=1.001f;bp.v[2][3]=1;bp.v[3][2]=-.1f;math::M ep[2]{bv,bv};
  grapple[0].object=object;grapple[0].at=GetTickCount64();const float decoy[3]={10,20,30};memcpy(object+0x200,decoy,12);learn_offset(decoy);
  for(int i=0;i<6;++i){if(learnedOffset>=0){puts("grapple: learned early");fflush(stdout);return false;}learn_offset(target);}if(learnedOffset!=0x104){printf("grapple: learned %d scans%u votes%u\n",learnedOffset.load(),offsetScans,offsetVotes[0x104/4]);fflush(stdout);return false;}
  uint32_t frame=81000;publish(frame%64,frame,frame,nullptr,bv,bp,ep);AcquireSRWLockShared(&lock);Set got=sets[frame%sets.size()];ReleaseSRWLockShared(&lock);
  if(!got.valid||got.frame!=frame||got.n!=1||memcmp(got.points[0].world,target,12)||grappleAnchors!=1){printf("grapple: anchor valid%u frame%u n%u anchors%u\n",got.valid,got.frame,got.n,grappleAnchors.load());fflush(stdout);return false;}
  grapple[0].at=GetTickCount64()-kGrappleFreshMs-50;++frame;publish(frame%64,frame,frame,nullptr,bv,bp,ep);AcquireSRWLockShared(&lock);got=sets[frame%sets.size()];ReleaseSRWLockShared(&lock);if(got.valid&&got.frame==frame&&got.n){puts("grapple: stale/null anchored");fflush(stdout);return false;}
  grapple[0].object=nullptr;grapple[0].at=GetTickCount64();++frame;publish(frame%64,frame,frame,nullptr,bv,bp,ep);AcquireSRWLockShared(&lock);got=sets[frame%sets.size()];ReleaseSRWLockShared(&lock);if(got.valid&&got.frame==frame&&got.n){puts("grapple: stale/null anchored");fflush(stdout);return false;}
  end_game_pass();puts("Grapple finder target: offset learned from accepted anchors (+104, decoy rejected), fresh finder point anchored, stale/null finder adds nothing PASS");}
 // A2i: off-screen points are kept with the game's screen direction (front AND
 // behind), output untouched; the edge pointer is head-locked: same eye-view
 // direction (32 deg toward the target) at HUD distance for any head pose.
 {unsigned checks=0;math::M bv{};bv.v[1][0]=1;bv.v[2][1]=1;bv.v[0][2]=1;bv.v[3][3]=1;math::M bp{};bp.v[0][0]=1;bp.v[1][1]=1.1f;bp.v[2][2]=1.001f;bp.v[2][3]=1;bp.v[3][2]=-.1f;
  for(float yaw:{0.f,.5f,-.8f})for(float pitch:{0.f,.3f})for(int side=0;side<4;++side){
   vr::HeadPose pose{};const float cy=std::cos(yaw*.5f),sy=std::sin(yaw*.5f),cp=std::cos(pitch*.5f),sp=std::sin(pitch*.5f);pose.qw=cy*cp;pose.qx=cy*sp;pose.qy=sy*cp;pose.qz=-sy*sp;
   math::M views[2],eyes[2];const float angles[]={-.9f,.8f,.85f,-.85f};for(int e=0;e<2;++e){vr::HeadPose ep=pose;ep.px=e?.032f:-.032f;views[e]=math::mul(bv,math::pose_view(ep));eyes[e]=math::mul(views[e],math::projection(bp,angles));}
   const uint32_t frame=91000+checks;publish(frame%64,frame,frame,nullptr,bv,bp,eyes,0,nullptr,views);
   const float targets[4][3]={{-500,-400,0},{-300,600,200},{100,2000,0},{50,-30,-1500}};const float*w=targets[side];float out[4]={7,7,7,7};
   if(anchor(w,out,true)||out[0]!=7||out[3]!=7){puts("edge: off-screen output changed");fflush(stdout);return false;}
   AcquireSRWLockShared(&lock);const Set got=sets[frame%sets.size()];ReleaseSRWLockShared(&lock);if(!got.valid||got.n!=1||!got.points[0].off){puts("edge: off anchor missing");fflush(stdout);return false;}
   {const float w2[3]={w[0]+1,w[1],w[2]};float g[4]={.3f,.5f,.9f,100};if(anchor(w2,g,false)||g[0]!=.3f||g[3]!=100){puts("edge: game XY changed");fflush(stdout);return false;}
    AcquireSRWLockShared(&lock);const Set two=sets[frame%sets.size()];const Set last=held;ReleaseSRWLockShared(&lock);
    if(two.n!=2||!two.points[1].off||!two.points[1].gameRef||two.points[1].ref[0]!=.3f||two.points[1].ref[1]!=.5f||last.n!=2||got.points[0].gameRef){puts("edge: game XY not kept for matching");fflush(stdout);return false;}}
   const auto&a=got.points[0];const float vx=w[1],vy=w[2]*1.1f,len=std::hypot(vx,vy);if(std::fabs(a.dir[0]-vx/len)>1e-4f||std::fabs(a.dir[1]-vy/len)>1e-4f){puts("edge: direction wrong");fflush(stdout);return false;}
   Pass q;Anchor e;if(!pointer(gamePass,a,q,e)){puts("edge: no pointer");fflush(stdout);return false;}
   float t[3]{},pv[3]{};for(int c=0;c<3;++c){t[c]=w[0]*views[0].v[0][c]+w[1]*views[0].v[1][c]+w[2]*views[0].v[2][c]+views[0].v[3][c];pv[c]=e.world[0]*views[0].v[0][c]+e.world[1]*views[0].v[1][c]+e.world[2]*views[0].v[2][c]+views[0].v[3][c];}
   const float pl=std::sqrt(pv[0]*pv[0]+pv[1]*pv[1]+pv[2]*pv[2]),angle=std::acos(pv[2]/pl)*180/3.14159265f,tl=std::hypot(t[0],t[1]);
   if(std::fabs(pl-pointerDistance)>.5f||std::fabs(angle-pointerDegrees)>.05f||(tl>1e-3f&&(pv[0]*t[0]+pv[1]*t[1])/(std::hypot(pv[0],pv[1])*tl)<.9999f)){printf("edge: pointer distance%.2f angle%.3f\n",pl,angle);fflush(stdout);return false;}
   for(int k=0;k<2;++k){float c[4]{};if(!project(eyes[k],e.world,c)||std::fabs(c[0])>.92f||std::fabs(c[1])>.92f){puts("edge: pointer outside lens");fflush(stdout);return false;}}
   math::M k0;if(!card(q,Anchor{e},0,k0)){puts("edge: pointer card");fflush(stdout);return false;}++checks;}
  end_game_pass();printf("Edge grapple pointer: %u off-screen/behind points kept with game direction, the game drawn XY kept for matching, untouched output, held for skipped frames; head-locked pointer %.0fdeg toward target at HUD distance inside both lenses for head yaw/pitch PASS\n",checks,pointerDegrees);}
 printf("World grapple GPU: %u BOTH-eye original target shader vs independent world-card vertices; near/farthestIndex target, game-camera turn, head yaw/roll/lean/asymmetric lenses; %u old affine controls differ; source clean and original VS/Transform restore; stale/empty/erase/mask/text-atlas/unknown shader guards PASS\n",cases,oldDiff);return true;
}

}
