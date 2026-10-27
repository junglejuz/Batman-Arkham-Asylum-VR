#include "d3d9_bridge.h"
#include "native_stereo.h"
#include "first_person.h"
#include "head_aim.h"
#include "hud9.h"
#include "dlss.h"
#include "afw.h"
#include "window.h"
#include "core/gfx/gfx_hud.h"
#include "core/hooks/d3d11_hook.h"
#include "core/gfx/hud_capture.h"
#include "core/gfx/stereo_history.h"
#include "detail9.h"
#include "pipeline_cache.h"
#include "dynamic_pipeline.h"
#include "managed9.h"
#include "native9_bridge.h"
#include "core/gfx/encoded_colour.h"
#include "billboard9.h"
#include "transitions9.h"
#include "markers9.h"
#include "transition_plane.h"
#include "core/gfx/blit.h"
#include "core/ui/overlay.h"
#include "core/util/log.h"
#include "core/vr/openxr_runtime.h"
#include "core/vr/flat_preview.h"
#include <d3d9on12.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <MinHook.h>
#include <imgui.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <psapi.h>
#include <algorithm>
#include <functional>
#include <d3dcompiler.h>
#include <cmath>

using Microsoft::WRL::ComPtr;
extern "C" DWORD WINAPI AA_Identity();
namespace bvr::aa::bridge {
namespace {
std::atomic<bool> ready{false};
std::atomic<unsigned> frames{0}, failures{0};
HMODULE system9=nullptr;
IDirect3DDevice9* ownDevice=nullptr;
using Present=HRESULT(WINAPI*)(IDirect3DDevice9*,const RECT*,const RECT*,HWND,const RGNDATA*);
using Reset=HRESULT(WINAPI*)(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*);
Present originalPresent=nullptr;Reset originalReset=nullptr;
ComPtr<IDirect3DDevice9On12> nine;
ComPtr<ID3D12Device> twelve;
ComPtr<ID3D12CommandQueue> queue;
ComPtr<ID3D12Fence> fence;
ComPtr<ID3D11Device> eleven;
ComPtr<ID3D11DeviceContext> context;
ComPtr<ID3D11On12Device> elevenOn12;
UINT64 serial=0;
HWND gameWindow=nullptr;
bool dead=false;
struct CpuSpan {unsigned count{};double total{},peak{};void add(double ms){++count;total+=ms;peak=std::fmax(peak,ms);}double mean()const{return count?total/count:0;}};
CpuSpan colourTime,hudTime,xrTime,depthTime,presentTime;
double cpu_now(){static const double scale=[](){LARGE_INTEGER f;QueryPerformanceFrequency(&f);return 1000./f.QuadPart;}();LARGE_INTEGER t;QueryPerformanceCounter(&t);return t.QuadPart*scale;}
void prepare_present(D3DPRESENT_PARAMETERS* pp){if(pp)pp->PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;}

// One wrapper per actual checked-out D3D12 object, bounded across rotating
// backbuffers/depth targets. Creating/destroying these every copy makes the
// 11On12 deferred-deletion queue do unnecessary work in a 32-bit process.
struct Wrapped {ComPtr<IUnknown> identity;ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;D3D12_RESOURCE_DESC desc{};uint64_t used{};bool readable{};};
Wrapped wrappedCache[8];uint64_t wrappedTick{};unsigned wrappedCreates{},wrappedReuses{},wrappedEvictions{};
bool same_desc(const D3D12_RESOURCE_DESC&a,const D3D12_RESOURCE_DESC&b){return a.Dimension==b.Dimension&&a.Alignment==b.Alignment&&a.Width==b.Width&&a.Height==b.Height&&a.DepthOrArraySize==b.DepthOrArraySize&&a.MipLevels==b.MipLevels&&a.Format==b.Format&&a.SampleDesc.Count==b.SampleDesc.Count&&a.SampleDesc.Quality==b.SampleDesc.Quality&&a.Layout==b.Layout&&a.Flags==b.Flags;}
HRESULT wrapped_texture(ID3D12Resource* resource,ID3D11Texture2D** out,bool readable=false){
 if(!resource||!out)return E_POINTER;*out=nullptr;ComPtr<IUnknown> identity;auto hr=resource->QueryInterface(IID_PPV_ARGS(&identity));if(FAILED(hr))return hr;
 const auto desc=resource->GetDesc();Wrapped* slot=&wrappedCache[0];
 for(auto& entry:wrappedCache){
  if(entry.identity.Get()==identity.Get()&&entry.texture&&entry.readable==readable&&same_desc(entry.desc,desc)){entry.used=++wrappedTick;*out=entry.texture.Get();(*out)->AddRef();++wrappedReuses;return S_OK;}
  if(entry.used<slot->used)slot=&entry;
 }
 ComPtr<ID3D11Texture2D> texture;D3D11_RESOURCE_FLAGS flags{};
 if(readable)flags.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
 hr=elevenOn12->CreateWrappedResource(resource,&flags,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COMMON,IID_PPV_ARGS(&texture));
 if(FAILED(hr))return hr;
 ComPtr<ID3D11ShaderResourceView> view;if(readable){hr=eleven->CreateShaderResourceView(texture.Get(),nullptr,&view);if(FAILED(hr))return hr;}
 if(slot->texture)++wrappedEvictions;
 *slot={};slot->identity=identity;slot->texture=texture;slot->view=view;slot->desc=desc;slot->readable=readable;slot->used=++wrappedTick;++wrappedCreates;
 *out=texture.Detach();return S_OK;
}
void clear_wrapped(){pipeline_cache::clear();dynamic_pipeline::clear_templates();for(auto& entry:wrappedCache)entry={};if(context)context->Flush();}
// Observe pipeline creation without changing descriptions or cache policy.
using CreateGraphicsPSO=HRESULT(WINAPI*)(ID3D12Device*,const D3D12_GRAPHICS_PIPELINE_STATE_DESC*,REFIID,void**);
using CreateStreamPSO=HRESULT(WINAPI*)(ID3D12Device2*,const D3D12_PIPELINE_STATE_STREAM_DESC*,REFIID,void**);
CreateGraphicsPSO graphicsPSOOriginal{};CreateStreamPSO streamPSOOriginal{};
std::atomic<unsigned> graphicsPSOCount{},streamPSOCount{},psoFailures{};
std::atomic<uint64_t> psoMicros{};
thread_local unsigned allocationActivity{};
HRESULT WINAPI graphics_pso(ID3D12Device* d,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc,REFIID iid,void** out){
 const auto prior=allocationActivity;allocationActivity=1;const double at=cpu_now();const auto hr=dynamic_pipeline::create(d,desc,iid,out,graphicsPSOOriginal);psoMicros.fetch_add(uint64_t((cpu_now()-at)*1000));++graphicsPSOCount;if(FAILED(hr))++psoFailures;allocationActivity=prior;return hr;
}
HRESULT WINAPI stream_pso(ID3D12Device2* d,const D3D12_PIPELINE_STATE_STREAM_DESC* desc,REFIID iid,void** out){
 const auto prior=allocationActivity;allocationActivity=2;const double at=cpu_now();const auto hr=streamPSOOriginal(d,desc,iid,out);psoMicros.fetch_add(uint64_t((cpu_now()-at)*1000));++streamPSOCount;if(FAILED(hr))++psoFailures;allocationActivity=prior;return hr;
}
bool install_pipeline_trace(){
 auto install=[](void* target,void* replacement,void** original){if(*original)return true;if(MH_CreateHook(target,replacement,original)!=MH_OK)return false;if(MH_EnableHook(target)==MH_OK)return true;MH_RemoveHook(target);*original=nullptr;return false;};
 auto** v=*reinterpret_cast<void***>(twelve.Get());bool ok=install(v[10],reinterpret_cast<void*>(graphics_pso),reinterpret_cast<void**>(&graphicsPSOOriginal));
 ComPtr<ID3D12Device2> d;if(SUCCEEDED(twelve.As(&d))){auto** v2=*reinterpret_cast<void***>(d.Get());ok=install(v2[47],reinterpret_cast<void*>(stream_pso),reinterpret_cast<void**>(&streamPSOOriginal))&&ok;}return ok;
}
// Bounded reservation provenance. No heap allocation, logging, or symbol work
// inside the allocator detour; only large current-process reservations qualify.
using NtAllocate=LONG(NTAPI*)(HANDLE,PVOID*,ULONG_PTR,PSIZE_T,ULONG,ULONG);
NtAllocate allocateOriginal{};void* allocateEntry{};
struct Reservation {void* base{};SIZE_T bytes{};unsigned activity{};USHORT depth{};void* stack[48]{};};
Reservation reservations[128];SRWLOCK reservationLock=SRWLOCK_INIT;
unsigned reservationWrite{},reservationRead{},reservationLost{};
thread_local bool tracingAllocation{};
LONG NTAPI allocate_trace(HANDLE process,PVOID* address,ULONG_PTR zero,PSIZE_T bytes,ULONG type,ULONG protect){
 const LONG status=allocateOriginal(process,address,zero,bytes,type,protect);
 if(status>=0&&process==GetCurrentProcess()&&(type&MEM_RESERVE)&&address&&bytes&&*bytes>=1024*1024&&!tracingAllocation){
  tracingAllocation=true;Reservation r{};r.base=*address;r.bytes=*bytes;r.activity=allocationActivity;r.depth=CaptureStackBackTrace(1,48,r.stack,nullptr);
  AcquireSRWLockExclusive(&reservationLock);
  if(reservationWrite-reservationRead<128)reservations[reservationWrite++%128]=r;else ++reservationLost;
  ReleaseSRWLockExclusive(&reservationLock);tracingAllocation=false;
 }return status;
}
bool install_allocation_trace(){
 if(allocateOriginal)return true;
 allocateEntry=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtAllocateVirtualMemory"));
 if(!allocateEntry||MH_CreateHook(allocateEntry,reinterpret_cast<void*>(allocate_trace),reinterpret_cast<void**>(&allocateOriginal))!=MH_OK)return false;
 if(MH_EnableHook(allocateEntry)!=MH_OK){MH_RemoveHook(allocateEntry);allocateOriginal=nullptr;return false;}return true;
}
void report_reservations(){
 for(unsigned i=0;i<128;++i){Reservation r{};
  AcquireSRWLockExclusive(&reservationLock);const bool have=reservationRead!=reservationWrite;if(have)r=reservations[reservationRead++%128];ReleaseSRWLockExclusive(&reservationLock);if(!have)break;
  MEMORY_BASIC_INFORMATION info{};const bool live=VirtualQuery(r.base,&info,sizeof(info))&&info.State!=MEM_FREE&&info.AllocationBase==r.base;
  BVR_LOG("[aa allocation] reserve %p %.1f MiB still mapped=%u pipelineActivity=%u (address reuse possible)",r.base,r.bytes/1048576.,live,r.activity);
  for(unsigned j=0;j<r.depth;++j){HMODULE module{};wchar_t path[MAX_PATH]{};
   if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(r.stack[j]),&module))GetModuleFileNameW(module,path,MAX_PATH);
   BVR_LOG("[aa allocation]   %u %ls + 0x%Ix",j,path,reinterpret_cast<uintptr_t>(r.stack[j])-reinterpret_cast<uintptr_t>(module));
  }
 }
 unsigned lost{};AcquireSRWLockExclusive(&reservationLock);lost=reservationLost;reservationLost=0;ReleaseSRWLockExclusive(&reservationLock);if(lost)BVR_LOG("[aa allocation] dropped %u reservation records (bounded queue)",lost);
}
struct MemoryBudget {uint64_t privateBytes{},freeBytes{},largestFree{},reservedBytes{};};
MemoryBudget memory_budget(){
 MemoryBudget result;PROCESS_MEMORY_COUNTERS_EX counters{};counters.cb=sizeof(counters);
 if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters)))result.privateBytes=counters.PrivateUsage;
 SYSTEM_INFO si{};GetSystemInfo(&si);uintptr_t at=0,limit=reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress);
 while(at<limit){MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(reinterpret_cast<void*>(at),&info,sizeof(info)))break;
  if(info.State==MEM_FREE){result.freeBytes+=info.RegionSize;result.largestFree=std::max(result.largestFree,uint64_t(info.RegionSize));}
  else if(info.State==MEM_RESERVE)result.reservedBytes+=info.RegionSize;
  const uintptr_t next=reinterpret_cast<uintptr_t>(info.BaseAddress)+info.RegionSize;if(next<=at)break;at=next;
 }return result;
}
void log_memory(const char* reason){
 report_reservations();
 const auto bias=dynamic_pipeline::stats();BVR_LOG("[aa dynamic bias] enabled=%u variants=%llu binds=%llu staticFallbacks=%llu blobFallbacks=%llu liveVariants=%u liveTemplates=%u",bias.enabled,bias.variants,bias.binds,bias.staticFallbacks,bias.blobFallbacks,bias.liveVariants,bias.liveTemplates);
 const auto cache=pipeline_cache::stats();BVR_LOG("[aa pipeline variants] nearest sampled=%llu changed root/shader/SO/blend/raster/depth/input/target %llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu; depthBias/slope/clamp %llu/%llu/%llu (diagnostic fingerprints only)",cache.nearSamples,cache.changed[0],cache.changed[1],cache.changed[2],cache.changed[3],cache.changed[4],cache.changed[5],cache.changed[6],cache.changed[7],cache.depthBiasChanges,cache.slopeBiasChanges,cache.clampChanges);BVR_LOG("[aa pipeline reuse] requests=%llu hits=%llu created=%llu bypassed=%llu evicted=%llu live=%u/512 keyMiB=%.3f/16",cache.requests,cache.hits,cache.created,cache.bypassed,cache.evicted,cache.live,cache.keyBytes/1048576.);
 BVR_LOG("[aa pipeline] process cumulative graphics=%u stream=%u failed=%u creationCPU=%.3f ms (nested API calls may overlap)",graphicsPSOCount.load(),streamPSOCount.load(),psoFailures.load(),psoMicros.load()/1000.);
 const auto m=memory_budget();constexpr double mb=1024.*1024.;unsigned live{};for(const auto& entry:wrappedCache)if(entry.texture)++live;
 BVR_LOG("[aa memory] %s: private %.1f MiB, VA free %.1f MiB largest %.1f MiB reserved %.1f MiB; wrapped live %u/8 created %u reused %u evicted %u",reason,m.privateBytes/mb,m.freeBytes/mb,m.largestFree/mb,m.reservedBytes/mb,live,wrappedCreates,wrappedReuses,wrappedEvictions);
 ComPtr<IDXGIAdapter3> adapter;ComPtr<IDXGIFactory4> factory;
 if(twelve&&SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))&&SUCCEEDED(factory->EnumAdapterByLuid(twelve->GetAdapterLuid(),IID_PPV_ARGS(&adapter)))){
  DXGI_QUERY_VIDEO_MEMORY_INFO video{};if(SUCCEEDED(adapter->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&video)))BVR_LOG("[aa memory] local GPU usage %.1f MiB / budget %.1f MiB (process segment, not system total)",video.CurrentUsage/mb,video.Budget/mb);
 }
}


// The frozen core needs a device, owned backbuffer and window descriptor.
// This facade has no presentation or resizing side effects; D3D9 owns those.
class Frame final : public IDXGISwapChain {
    std::atomic<ULONG> refs{1};
public:
    ComPtr<ID3D11Texture2D> image;
    DXGI_SWAP_CHAIN_DESC desc{};
    Frame(ID3D11Texture2D* tex,HWND window):image(tex) {
        D3D11_TEXTURE2D_DESC d{};tex->GetDesc(&d);
        desc.BufferDesc.Width=d.Width;desc.BufferDesc.Height=d.Height;desc.BufferDesc.Format=d.Format;
        desc.SampleDesc={1,0};desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount=1;desc.OutputWindow=window;desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    }
    HRESULT WINAPI QueryInterface(REFIID iid,void** p) override {
        if(!p) return E_POINTER;*p=nullptr;
        if(iid==__uuidof(IUnknown)||iid==__uuidof(IDXGIObject)||iid==__uuidof(IDXGIDeviceSubObject)||iid==__uuidof(IDXGISwapChain)) {
            *p=static_cast<IDXGISwapChain*>(this);AddRef();return S_OK;
        }return E_NOINTERFACE;
    }
    ULONG WINAPI AddRef() override {return ++refs;}
    ULONG WINAPI Release() override {const ULONG n=--refs;if(!n) delete this;return n;}
    HRESULT WINAPI SetPrivateData(REFGUID,UINT,const void*) override {return E_NOTIMPL;}
    HRESULT WINAPI SetPrivateDataInterface(REFGUID,const IUnknown*) override {return E_NOTIMPL;}
    HRESULT WINAPI GetPrivateData(REFGUID,UINT*,void*) override {return E_NOTIMPL;}
    HRESULT WINAPI GetParent(REFIID,void**) override {return E_NOINTERFACE;}
    HRESULT WINAPI GetDevice(REFIID iid,void** p) override {if(!image)return DXGI_ERROR_INVALID_CALL;ComPtr<ID3D11Device> d;image->GetDevice(&d);return d->QueryInterface(iid,p);}
    HRESULT WINAPI Present(UINT,UINT) override {return E_NOTIMPL;}
    HRESULT WINAPI GetBuffer(UINT index,REFIID iid,void** p) override {return index||!image ? DXGI_ERROR_INVALID_CALL:image->QueryInterface(iid,p);}
    HRESULT WINAPI SetFullscreenState(BOOL,IDXGIOutput*) override {return E_NOTIMPL;}
    HRESULT WINAPI GetFullscreenState(BOOL* b,IDXGIOutput** out) override {if(b)*b=FALSE;if(out)*out=nullptr;return S_OK;}
    HRESULT WINAPI GetDesc(DXGI_SWAP_CHAIN_DESC* d) override {if(!d)return E_POINTER;*d=desc;return S_OK;}
    HRESULT WINAPI ResizeBuffers(UINT,UINT,UINT,DXGI_FORMAT,UINT) override {return E_NOTIMPL;}
    HRESULT WINAPI ResizeTarget(const DXGI_MODE_DESC*) override {return E_NOTIMPL;}
    HRESULT WINAPI GetContainingOutput(IDXGIOutput**) override {return E_NOTIMPL;}
    HRESULT WINAPI GetFrameStatistics(DXGI_FRAME_STATISTICS*) override {return E_NOTIMPL;}
    HRESULT WINAPI GetLastPresentCount(UINT* p) override {if(p)*p=frames.load();return p ? S_OK:E_POINTER;}
};
Frame* frame=nullptr;
Frame* borrowedFrame=nullptr;
using OfflineBorrowConsumer=std::function<bool(Frame*)>;
OfflineBorrowConsumer offlineBorrowConsumer{};
unsigned borrowedFrames{},borrowFallbacks{};
DXGI_FORMAT sourceFormat=DXGI_FORMAT_UNKNOWN;
ComPtr<ID3D11Texture2D> colourSource;
ComPtr<ID3D11ShaderResourceView> colourSrv;
ComPtr<ID3D11RenderTargetView> frameRtv;
ComPtr<ID3D11Texture2D> sceneDepth;
ComPtr<ID3D11ShaderResourceView> sceneDepthSrv;
uint32_t depthGeneration{};int depthEye{};bool depthFresh{};
ComPtr<ID3D11Texture2D> dlssScene[2],stockLeft,leftTarget,stockRight,rightTarget;
ComPtr<ID3D11RenderTargetView> dlssSceneRtv[2],stockLeftRtv;
uint32_t stockLeftGeneration{};bool stockLeftReady{};
uint32_t stockRightGeneration{};
ComPtr<ID3D11Texture2D> hudSource,hudImage;
ComPtr<ID3D11ShaderResourceView> hudSourceSrv,hudSrv;
ComPtr<ID3D11RenderTargetView> hudRtv;
uint32_t hudGeneration{};bool hudFresh=false,hudPlaced=false;
bool hudHeadLocked{};uint32_t hudPlacedGeneration{};
float hudShape[3]{};
std::atomic<bool> hudRecentre{true};
std::atomic<float> hudDistance{4},hudScale{1};wchar_t hudSettings[MAX_PATH]{};
float hudWorld[4][3]{};
ComPtr<ID3D11Texture2D> batSource,batImage;
ComPtr<ID3D11ShaderResourceView> batSourceSrv,batSrv;
ComPtr<ID3D11RenderTargetView> batRtv;
uint32_t batGeneration{};bool batFresh{};
struct MarkerImage{ComPtr<ID3D11Texture2D>source,image;ComPtr<ID3D11ShaderResourceView>sourceSrv,srv;ComPtr<ID3D11RenderTargetView>rt;bool fresh{};uint32_t gen{};};MarkerImage markerImages[2];
ComPtr<ID3D11Texture2D> fadeMask;ComPtr<ID3D11ShaderResourceView> fadeMaskSrv;
float publishedFadeAlpha{};uint32_t publishedFadeGeneration{};unsigned fadeFrames{},fadeEpisodes{};
bool checked(HRESULT hr,const char* stage);

bool transport_sheet(IDirect3DSurface9* sheet,unsigned slot,
 ComPtr<ID3D11Texture2D>& hudSource,ComPtr<ID3D11Texture2D>& hudImage,
 ComPtr<ID3D11ShaderResourceView>& hudSourceSrv,ComPtr<ID3D11ShaderResourceView>& hudSrv,
 ComPtr<ID3D11RenderTargetView>& hudRtv) {
    if(!sheet)return true;
    if(native9::device()){
        auto* shared=native9::colour(sheet,slot);if(!shared)return false;
        D3D11_TEXTURE2D_DESC sd{},old{};shared->GetDesc(&sd);if(hudImage)hudImage->GetDesc(&old);
        if(!hudImage||old.Width!=sd.Width||old.Height!=sd.Height){hudSrv.Reset();hudRtv.Reset();hudImage.Reset();sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;sd.MiscFlags=0;
            if(FAILED(eleven->CreateTexture2D(&sd,nullptr,&hudImage))||FAILED(eleven->CreateRenderTargetView(hudImage.Get(),nullptr,&hudRtv))||FAILED(eleven->CreateShaderResourceView(hudImage.Get(),nullptr,&hudSrv)))return false;}
        ComPtr<ID3D11ShaderResourceView> view;if(FAILED(eleven->CreateShaderResourceView(shared,nullptr,&view))||!blit::shift_copy(context.Get(),hudRtv.Get(),sd.Width,sd.Height,view.Get(),0))return false;
        return true;
    }
    ComPtr<ID3D12Resource> resource;
    if(!checked(nine->UnwrapUnderlyingResource(sheet,queue.Get(),IID_PPV_ARGS(&resource)),"unwrap HUD sheet"))return false;
    const auto rd=resource->GetDesc();bool ok=true;
    D3D11_TEXTURE2D_DESC d{};if(hudImage)hudImage->GetDesc(&d);
    if(!hudImage||!hudSource||!hudRtv||!hudSrv||!hudSourceSrv||d.Width!=rd.Width||d.Height!=rd.Height){
        hudRtv.Reset();hudSrv.Reset();hudSourceSrv.Reset();hudImage.Reset();hudSource.Reset();
        d={};d.Width=UINT(rd.Width);d.Height=rd.Height;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=rd.Format;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        ok=SUCCEEDED(eleven->CreateTexture2D(&d,nullptr,&hudSource))&&SUCCEEDED(eleven->CreateShaderResourceView(hudSource.Get(),nullptr,&hudSourceSrv));
        d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags|=D3D11_BIND_RENDER_TARGET;
        ok=ok&&SUCCEEDED(eleven->CreateTexture2D(&d,nullptr,&hudImage))&&SUCCEEDED(eleven->CreateRenderTargetView(hudImage.Get(),nullptr,&hudRtv))&&SUCCEEDED(eleven->CreateShaderResourceView(hudImage.Get(),nullptr,&hudSrv));
        
    }
    ComPtr<ID3D11Texture2D> wrapped;
    if(ok)ok=checked(wrapped_texture(resource.Get(),&wrapped),"wrap HUD sheet");
    if(ok){ID3D11Resource* resources[]={wrapped.Get()};elevenOn12->AcquireWrappedResources(resources,1);context->CopyResource(hudSource.Get(),wrapped.Get());elevenOn12->ReleaseWrappedResources(resources,1);
        ok=blit::shift_copy(context.Get(),hudRtv.Get(),UINT(rd.Width),rd.Height,hudSourceSrv.Get(),0);context->Flush();}
    const UINT64 value=++serial;
    if(!checked(queue->Signal(fence.Get(),value),"HUD return signal")){dead=true;resource.Detach();wrapped.Detach();return false;}
    ID3D12Fence* fences[]={fence.Get()};UINT64 values[]={value};
    if(!checked(nine->ReturnUnderlyingResource(sheet,1,values,fences),"return HUD sheet")){dead=true;return false;}
    
    return ok;
}

bool publish_hud(){
 if(hud9::eye()<0)hudFresh=false;auto*sheet=hud9::sheet();if(!sheet)return true;
 auto*old=hudImage.Get();const bool ok=transport_sheet(sheet,2,hudSource,hudImage,hudSourceSrv,hudSrv,hudRtv);
 if(ok){hudGeneration=hud9::generation();hudFresh=true;if(old!=hudImage.Get())hudPlaced=false;}return ok;
}
bool publish_markers(){
 if(hud9::eye()>0)return true;bool ok=true;
 for(int e=0;e<2;++e){auto&m=markerImages[e];m.fresh=false;auto*s=markers9::sheet(e);if(!s)continue;m.fresh=transport_sheet(s,5+e,m.source,m.image,m.sourceSrv,m.srv,m.rt);m.gen=markers9::generation();ok=ok&&m.fresh;}
 return ok;
}
bool compose_markers(ID3D11DeviceContext*ctx,int eye,ID3D11Texture2D*dst){
 if(eye<0||eye>1||!dst)return false;auto&m=markerImages[eye];if(!m.fresh||m.gen!=hud9::generation())return true;
 ComPtr<ID3D11Device>d;ctx->GetDevice(&d);D3D11_TEXTURE2D_DESC out{};dst->GetDesc(&out);D3D11_RENDER_TARGET_VIEW_DESC rd{};rd.Format=out.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS?DXGI_FORMAT_R8G8B8A8_UNORM:out.Format;rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;ComPtr<ID3D11RenderTargetView>rt;
 return SUCCEEDED(d->CreateRenderTargetView(dst,&rd,&rt))&&blit::zoom_premul(ctx,rt.Get(),out.Width,out.Height,m.srv.Get(),1);
}
bool fixture_marker_transport(IDirect3DSurface9*left,IDirect3DSurface9*right){
 for(int e=0;e<2;++e){auto*sheet=e?right:left;auto&m=markerImages[e];if(!sheet||!transport_sheet(sheet,5+e,m.source,m.image,m.sourceSrv,m.srv,m.rt))return false;m.fresh=true;m.gen=hud9::generation();
  D3DSURFACE_DESC sd{};sheet->GetDesc(&sd);ComPtr<IDirect3DDevice9>device;if(FAILED(sheet->GetDevice(&device)))return false;ComPtr<IDirect3DSurface9>nineCpu;if(FAILED(device->CreateOffscreenPlainSurface(sd.Width,sd.Height,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&nineCpu,nullptr))||FAILED(device->GetRenderTargetData(sheet,nineCpu.Get())))return false;D3DLOCKED_RECT source{};if(FAILED(nineCpu->LockRect(&source,nullptr,D3DLOCK_READONLY)))return false;std::vector<BYTE>mask(sd.Width*sd.Height*4);for(UINT y=0;y<sd.Height;++y)memcpy(mask.data()+y*sd.Width*4,static_cast<BYTE*>(source.pBits)+y*source.Pitch,sd.Width*4);nineCpu->UnlockRect();
  D3D11_TEXTURE2D_DESC d{};m.image->GetDesc(&d);d.MiscFlags=0;d.BindFlags=D3D11_BIND_RENDER_TARGET;ComPtr<ID3D11Texture2D>dst,cpu;ComPtr<ID3D11RenderTargetView>rt;if(FAILED(eleven->CreateTexture2D(&d,nullptr,&dst))||FAILED(eleven->CreateRenderTargetView(dst.Get(),nullptr,&rt)))return false;auto stage=d;stage.BindFlags=0;stage.Usage=D3D11_USAGE_STAGING;stage.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(eleven->CreateTexture2D(&stage,nullptr,&cpu)))return false;
  const float background[]={.2f,.4f,.6f,1};context->ClearRenderTargetView(rt.Get(),background);if(!compose_markers(context.Get(),e,dst.Get()))return false;context->CopyResource(cpu.Get(),dst.Get());D3D11_MAPPED_SUBRESOURCE actual{};if(FAILED(context->Map(cpu.Get(),0,D3D11_MAP_READ,0,&actual)))return false;bool ok=true;unsigned lit=0;
  for(UINT y=0;y<d.Height;++y)for(UINT x=0;x<d.Width;++x){const BYTE*src=mask.data()+(y*d.Width+x)*4;const BYTE*p=static_cast<const BYTE*>(actual.pData)+y*actual.RowPitch+x*4;float alpha=src[3]/255.f;for(int c=0;c<3;++c)ok=ok&&std::abs(int(p[c])-int(std::lround(src[2-c]+255*background[c]*(1-alpha))))<=1;if(src[3])++lit;}
  context->Unmap(cpu.Get(),0);if(!ok||!lit)return false;
 }
 for(auto&m:markerImages)m=MarkerImage{};puts("World marker transport GPU: actual native/legacy slots5+6 -> own LEFT/RIGHT masks, final full-eye composition matches every source alpha/colour pixel; separate world-card disparity retained PASS");return !native9::device()||native9::finish_reads();
}
bool publish_bats(){
 if(hud9::eye()<=0){
  batFresh=false;const float alpha=transitions9::fade_alpha(transitions9::generation());
  if(alpha>0){++fadeFrames;if(publishedFadeAlpha<=0){++fadeEpisodes;BVR_LOG("[aa menu fade] authored envelope begins gen%u alpha%.4f; full field behind HUD",transitions9::generation(),alpha);}}
  else if(publishedFadeAlpha>0)BVR_LOG("[aa menu fade] authored envelope cleared gen%u; episodes%u frames%u",transitions9::generation(),fadeEpisodes,fadeFrames);
  publishedFadeAlpha=alpha;publishedFadeGeneration=transitions9::generation();
 }
 auto*sheet=transitions9::sheet();if(!sheet)return true;
 const bool ok=transport_sheet(sheet,4,batSource,batImage,batSourceSrv,batSrv,batRtv);
 if(ok){batGeneration=transitions9::generation();batFresh=true;}
 else {ComPtr<IDirect3DSurface9>bb;if(!hud9::sheet())ownDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb);transitions9::fallback(hud9::sheet()?hud9::sheet():bb.Get());BVR_LOG("[aa transitions] transfer failed; original HUD fallback retains animation");}
 return ok;
}
// Regression follows the actual separate-mask transport, near-plane shader,
// and XR-size image dimensions. Black edges must occlude; transparent centre
// must preserve the scene in BOTH asymmetric eyes, including head roll/IPD.
bool fixture_bat_transport(IDirect3DSurface9* mask){
 if(!transport_sheet(mask,4,batSource,batImage,batSourceSrv,batSrv,batRtv))return false;
 XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
 for(int e=0;e<2;++e){const float roll=.4f;auto&q=views[e].pose.orientation;q={0,0,std::sin(roll*.5f),std::cos(roll*.5f)};const float local[3]={e?.032f:-.032f,0,0};float moved[3];xrmath::quat_rotate(q.x,q.y,q.z,q.w,local,moved);views[e].pose.position={moved[0],moved[1],moved[2]};views[e].fov={e?-.65f:-.9f,e?.9f:.65f,.8f,-1.f};}
 D3D11_TEXTURE2D_DESC d{};d.Width=2284;d.Height=2448;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags=D3D11_BIND_RENDER_TARGET;
 ComPtr<ID3D11Texture2D>dst,cpu;ComPtr<ID3D11RenderTargetView>rt;
 if(FAILED(eleven->CreateTexture2D(&d,nullptr,&dst))||FAILED(eleven->CreateRenderTargetView(dst.Get(),nullptr,&rt)))return false;d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(eleven->CreateTexture2D(&d,nullptr,&cpu)))return false;
 float clips[2][4][4]{};if(!transition_plane(views,192.f/208.f,clips))return false;
 const float background[]={.8f,.4f,.2f,1};
 for(int e=0;e<2;++e){context->ClearRenderTargetView(rt.Get(),background);if(!blit::premul_in_eye(context.Get(),rt.Get(),d.Width,d.Height,batSrv.Get(),clips[e]))return false;context->CopyResource(cpu.Get(),dst.Get());D3D11_MAPPED_SUBRESOURCE m{};if(FAILED(context->Map(cpu.Get(),0,D3D11_MAP_READ,0,&m)))return false;bool ok=true;
  for(unsigned y:{8u,d.Height-9})for(unsigned x:{8u,d.Width-9}){const auto*p=static_cast<const BYTE*>(m.pData)+y*m.RowPitch+x*4;ok=ok&&p[0]<2&&p[1]<2&&p[2]<2&&p[3]>253;}
  const auto*p=static_cast<const BYTE*>(m.pData)+(d.Height/2)*m.RowPitch+(d.Width/2)*4;ok=ok&&std::abs(int(p[0])-204)<=1&&std::abs(int(p[1])-102)<=1&&std::abs(int(p[2])-51)<=1&&p[3]>253;context->Unmap(cpu.Get(),0);if(!ok)return false;
 }
 const bool finished=!native9::device()||native9::finish_reads();
 batFresh=false;batGeneration=0;batRtv.Reset();batSrv.Reset();batSourceSrv.Reset();batImage.Reset();batSource.Reset();
 puts("Bat transport GPU: separate slot4 -> BOTH2284x2448 eyes; asymmetric FOV/head roll/IPD; near0.8m plane, black full-view corners occlude and transparent centre preserves scene PASS");return finished;
}
ID3D11Texture2D* flat_bats(ID3D11DeviceContext*,bool projection){return !projection&&batFresh?batImage.Get():nullptr;}

bool render_scene(ID3D11DeviceContext* ctx,ID3D11RenderTargetView* rt,UINT w,UINT h,ID3D11ShaderResourceView* src,const native::Camera* camera){
 if(camera&&camera->centred){
  // Use unjittered lens bounds for resampling. Jitter is already in pixels;
  // sampling through a jittered crop would cancel the engine's jitter.
  math::M base=camera->projection;base.v[2][0]-=2*camera->jitterX/camera->inputWidth;base.v[2][1]+=2*camera->jitterY/camera->inputHeight;
  const auto centred=math::centred_projection(base,camera->angles);const float x=1/centred.v[0][0],y=1/centred.v[1][1];float corners[4][4]{};
  for(int i=0;i<4;++i){corners[i][0]=(i&1?x:-x)*base.v[0][0]+base.v[2][0];corners[i][1]=(i&2?-y:y)*base.v[1][1]+base.v[2][1];corners[i][3]=1;}
  return blit::screen_in_eye(ctx,rt,w,h,src,corners);
 }
 return blit::shift_copy(ctx,rt,w,h,src,0);
}
ComPtr<ID3D11VertexShader>connectorVS;ComPtr<ID3D11PixelShader>connectorPS;ComPtr<ID3D11InputLayout>connectorLayout;ComPtr<ID3D11Buffer>connectorVB;ComPtr<ID3D11BlendState>connectorBlend;ComPtr<ID3D11RasterizerState>connectorRaster;ComPtr<ID3D11DepthStencilState>connectorDepth;
bool draw_connector_mesh(ID3D11DeviceContext*ctx,ID3D11Texture2D*dst,const std::vector<markers9::ConnectorVertex>&vertices){
 if(vertices.empty())return true;if(vertices.size()>8192||!ctx||!dst)return false;ComPtr<ID3D11Device>device;ctx->GetDevice(&device);
 if(!connectorVS){const char*shader="struct V{float4 p:POSITION;float4 c:COLOR0;};struct O{float4 p:SV_POSITION;float4 c:COLOR0;};O vs(V v){O o;o.p=v.p;o.c=v.c;return o;}float4 ps(O v):SV_Target{return v.c;}";ComPtr<ID3DBlob>vs,ps,error;if(FAILED(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"vs","vs_4_0",0,0,&vs,&error))||FAILED(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"ps","ps_4_0",0,0,&ps,&error)))return false;
  const D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0}};D3D11_BUFFER_DESC bd{};bd.ByteWidth=8192*sizeof(markers9::ConnectorVertex);bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;D3D11_BLEND_DESC blend{};auto&b=blend.RenderTarget[0];b.BlendEnable=TRUE;b.SrcBlend=D3D11_BLEND_ONE;b.DestBlend=D3D11_BLEND_INV_SRC_ALPHA;b.BlendOp=D3D11_BLEND_OP_ADD;b.SrcBlendAlpha=D3D11_BLEND_ONE;b.DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=15;D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;D3D11_DEPTH_STENCIL_DESC depth{};depth.DepthFunc=D3D11_COMPARISON_ALWAYS;
  ComPtr<ID3D11VertexShader>v;ComPtr<ID3D11PixelShader>p;ComPtr<ID3D11InputLayout>il;ComPtr<ID3D11Buffer>vb;ComPtr<ID3D11BlendState>bs;ComPtr<ID3D11RasterizerState>rs;ComPtr<ID3D11DepthStencilState>ds;if(FAILED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&v))||FAILED(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&p))||FAILED(device->CreateInputLayout(input,2,vs->GetBufferPointer(),vs->GetBufferSize(),&il))||FAILED(device->CreateBuffer(&bd,nullptr,&vb))||FAILED(device->CreateBlendState(&blend,&bs))||FAILED(device->CreateRasterizerState(&raster,&rs))||FAILED(device->CreateDepthStencilState(&depth,&ds)))return false;connectorVS=v;connectorPS=p;connectorLayout=il;connectorVB=vb;connectorBlend=bs;connectorRaster=rs;connectorDepth=ds;
 }
 D3D11_MAPPED_SUBRESOURCE map{};if(FAILED(ctx->Map(connectorVB.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map)))return false;memcpy(map.pData,vertices.data(),vertices.size()*sizeof(vertices[0]));ctx->Unmap(connectorVB.Get(),0);
 D3D11_TEXTURE2D_DESC d{};dst->GetDesc(&d);D3D11_RENDER_TARGET_VIEW_DESC rd{};rd.Format=d.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS?DXGI_FORMAT_R8G8B8A8_UNORM:d.Format;rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;ComPtr<ID3D11RenderTargetView>rt;if(FAILED(device->CreateRenderTargetView(dst,&rd,&rt)))return false;
 ID3D11RenderTargetView*targets[8]{};ComPtr<ID3D11DepthStencilView>dsv;ctx->OMGetRenderTargets(8,targets,&dsv);ComPtr<ID3D11VertexShader>vs;ComPtr<ID3D11PixelShader>ps;ComPtr<ID3D11GeometryShader>gs;ID3D11ClassInstance*vsClasses[256]{},*psClasses[256]{},*gsClasses[256]{};UINT vn=256,pn=256,gn=256;ctx->VSGetShader(&vs,vsClasses,&vn);ctx->PSGetShader(&ps,psClasses,&pn);ctx->GSGetShader(&gs,gsClasses,&gn);ComPtr<ID3D11InputLayout>layout;ctx->IAGetInputLayout(&layout);ComPtr<ID3D11Buffer>vb;UINT stride{},offset{};ctx->IAGetVertexBuffers(0,1,&vb,&stride,&offset);D3D11_PRIMITIVE_TOPOLOGY topology;ctx->IAGetPrimitiveTopology(&topology);ComPtr<ID3D11RasterizerState>raster;ctx->RSGetState(&raster);D3D11_VIEWPORT viewports[16]{};UINT viewportCount=16;ctx->RSGetViewports(&viewportCount,viewports);ComPtr<ID3D11BlendState>blend;float factors[4]{};UINT sampleMask{};ctx->OMGetBlendState(&blend,factors,&sampleMask);ComPtr<ID3D11DepthStencilState>depth;UINT ref{};ctx->OMGetDepthStencilState(&depth,&ref);
 ID3D11RenderTargetView*target=rt.Get();ctx->OMSetRenderTargets(1,&target,nullptr);const D3D11_VIEWPORT vp{0,0,float(d.Width),float(d.Height),0,1};ctx->RSSetViewports(1,&vp);ctx->RSSetState(connectorRaster.Get());ctx->OMSetBlendState(connectorBlend.Get(),nullptr,~0u);ctx->OMSetDepthStencilState(connectorDepth.Get(),0);ctx->VSSetShader(connectorVS.Get(),nullptr,0);ctx->PSSetShader(connectorPS.Get(),nullptr,0);ctx->GSSetShader(nullptr,nullptr,0);ctx->IASetInputLayout(connectorLayout.Get());ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11Buffer*buffer=connectorVB.Get();UINT vertexStride=sizeof(vertices[0]),zero=0;ctx->IASetVertexBuffers(0,1,&buffer,&vertexStride,&zero);ctx->Draw(UINT(vertices.size()),0);
 ctx->OMSetRenderTargets(8,targets,dsv.Get());ctx->RSSetViewports(viewportCount,viewports);ctx->RSSetState(raster.Get());ctx->OMSetBlendState(blend.Get(),factors,sampleMask);ctx->OMSetDepthStencilState(depth.Get(),ref);ctx->VSSetShader(vs.Get(),vsClasses,vn);ctx->PSSetShader(ps.Get(),psClasses,pn);ctx->GSSetShader(gs.Get(),gsClasses,gn);ctx->IASetInputLayout(layout.Get());ctx->IASetPrimitiveTopology(topology);buffer=vb.Get();ctx->IASetVertexBuffers(0,1,&buffer,&stride,&offset);for(auto*r:targets)if(r)r->Release();for(UINT i=0;i<vn;++i)vsClasses[i]->Release();for(UINT i=0;i<pn;++i)psClasses[i]->Release();for(UINT i=0;i<gn;++i)gsClasses[i]->Release();return true;
}
bool fixture_connector_gpu(){
 D3D11_TEXTURE2D_DESC d{};d.Width=37;d.Height=41;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags=D3D11_BIND_RENDER_TARGET;ComPtr<ID3D11Texture2D>dst,cpu;ComPtr<ID3D11RenderTargetView>rt;if(FAILED(eleven->CreateTexture2D(&d,nullptr,&dst))||FAILED(eleven->CreateRenderTargetView(dst.Get(),nullptr,&rt)))return false;auto sd=d;sd.BindFlags=0;sd.Usage=D3D11_USAGE_STAGING;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(eleven->CreateTexture2D(&sd,nullptr,&cpu)))return false;
 const float background[]={.2f,.4f,.6f,1};std::vector<markers9::ConnectorVertex>vertices(6);const float xy[6][2]={{-.5f,.5f},{.5f,.5f},{-.5f,-.5f},{.5f,.5f},{.5f,-.5f},{-.5f,-.5f}};for(int i=0;i<6;++i){auto&v=vertices[i];v.clip[0]=xy[i][0];v.clip[1]=xy[i][1];v.clip[2]=.5f;v.clip[3]=1;v.colour[0]=.4f;v.colour[1]=.2f;v.colour[3]=.5f;}
 for(int e=0;e<2;++e){context->ClearRenderTargetView(rt.Get(),background);ID3D11RenderTargetView*bound=rt.Get();context->OMSetRenderTargets(1,&bound,nullptr);const D3D11_VIEWPORT vp{3,4,5,6,0,1};context->RSSetViewports(1,&vp);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);if(!draw_connector_mesh(context.Get(),dst.Get(),vertices))return false;D3D11_VIEWPORT after{};UINT n=1;context->RSGetViewports(&n,&after);D3D11_PRIMITIVE_TOPOLOGY topology;context->IAGetPrimitiveTopology(&topology);ComPtr<ID3D11RenderTargetView>restored;context->OMGetRenderTargets(1,&restored,nullptr);if(memcmp(&after,&vp,sizeof(vp))||topology!=D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP||restored.Get()!=rt.Get())return false;context->CopyResource(cpu.Get(),dst.Get());D3D11_MAPPED_SUBRESOURCE m{};if(FAILED(context->Map(cpu.Get(),0,D3D11_MAP_READ,0,&m)))return false;const BYTE*centre=static_cast<const BYTE*>(m.pData)+(d.Height/2)*m.RowPitch+(d.Width/2)*4;const BYTE*corner=static_cast<const BYTE*>(m.pData);bool ok=std::abs(int(centre[0])-128)<=1&&std::abs(int(centre[1])-102)<=1&&std::abs(int(centre[2])-77)<=1&&centre[3]==255&&std::abs(int(corner[0])-51)<=1;context->Unmap(cpu.Get(),0);if(!ok)return false;}
 context->OMSetRenderTargets(0,nullptr,nullptr);puts("Vitals connector GPU: BOTH-eye dynamic triangle draw, independent premultiplied blend reference, world outside stroke unchanged; RT/viewport/topology restore PASS");return true;
}
bool compose_original_hud(ID3D11DeviceContext* ctx,int e,ID3D11Texture2D* dst){
 if(!hudFresh||hudGeneration!=hud9::generation()||!dst)return true;
 const bool follow=native::head_hud(hudGeneration);
 ComPtr<ID3D11Device> dev;ComPtr<ID3D11RenderTargetView> rt;ctx->GetDevice(&dev);D3D11_TEXTURE2D_DESC d{};dst->GetDesc(&d);
 D3D11_RENDER_TARGET_VIEW_DESC rd{};rd.Format=d.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS?DXGI_FORMAT_R8G8B8A8_UNORM:d.Format;rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
 if(FAILED(dev->CreateRenderTargetView(dst,&rd,&rt))){first_person::note_hud(hudGeneration,follow,false);return false;}
    vr::HeadPose poses[2]{};float angles[2][4]{};XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
    for(int i=0;i<2;++i){if(!vr::get_eye_pose_gen(hudGeneration,i,poses[i],angles[i])){first_person::note_hud(hudGeneration,follow,false);return false;}
        auto& v=views[i];const auto& p=poses[i];v.pose.orientation={p.qx,p.qy,p.qz,p.qw};v.pose.position={p.px,p.py,p.pz};v.fov={angles[i][0],angles[i][1],angles[i][2],angles[i][3]};}
    if(hudRecentre.exchange(false)){hudPlaced=false;markers9::invalidate_hud_plane();}
    if(!hudPlaced||hudHeadLocked!=follow||(follow&&hudPlacedGeneration!=hudGeneration)){
        const auto& p=poses[0];const float yaw=std::atan2(2*(p.qx*p.qz+p.qw*p.qy),1-2*(p.qx*p.qx+p.qy*p.qy));
        D3D11_TEXTURE2D_DESC hd{};hudImage->GetDesc(&hd);float inner=10;
        for(int i=0;i<2;++i)inner=std::fmin(inner,std::fmin(-std::tan(angles[i][0]),std::tan(angles[i][1])));
        const float distance=hudDistance.load();const float hw=distance*inner*.82f*hudScale.load(),hh=hw*hd.Height/hd.Width;
        const float centre[3]={(poses[0].px+poses[1].px)*.5f,(poses[0].py+poses[1].py)*.5f,(poses[0].pz+poses[1].pz)*.5f};
        if(follow){
            if(!head_aim::hud_plane(poses,distance,hw,hh,hudWorld)){first_person::note_hud(hudGeneration,true,false);return false;}
        } else {
            for(int c=0;c<4;++c){const float local[3]={c&1?hw:-hw,c&2?-hh:hh,-distance};float rotated[3];xrmath::quat_rotate(0,std::sin(yaw*.5f),0,std::cos(yaw*.5f),local,rotated);for(int j=0;j<3;++j)hudWorld[c][j]=centre[j]+rotated[j];}
        }
        if(!hudPlaced||hudHeadLocked!=follow) BVR_LOG("[aa HUD] mode=%s gen=%u distance=%.2fm width=%.2fm",follow?"headset":"room",hudGeneration,distance,2*hw);
        hudShape[0]=distance;hudShape[1]=hw;hudShape[2]=hh;
        hudPlaced=true;hudHeadLocked=follow;hudPlacedGeneration=hudGeneration;
    }
    markers9::publish_hud_plane(hudWorld,follow,hudShape);
    float clip[4][4];for(int c=0;c<4;++c)vr::preview::clip(views[e],hudWorld[c],clip[c]);
    const bool ok=blit::premul_in_eye(ctx,rt.Get(),d.Width,d.Height,hudSrv.Get(),clip);
    first_person::note_hud(hudGeneration,follow,ok);
 return ok;
}
// Dim only the final world; menu/HUD is drawn next and stays legible.
// Reuse the existing state-restoring full-viewport mask pass. A constant white
// fill uses the actual engine alpha, so faint fades never become black flashes.
bool compose_menu_fade(ID3D11DeviceContext*ctx,ID3D11Texture2D*dst,float alpha){
 if(alpha<=0)return true;if(!ctx||!dst||!std::isfinite(alpha))return false;
 ComPtr<ID3D11Device>dev;ctx->GetDevice(&dev);
 if(!fadeMaskSrv){D3D11_TEXTURE2D_DESC d{};d.Width=d.Height=d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8_UNORM;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;const BYTE zero=0;D3D11_SUBRESOURCE_DATA init{&zero,1,1};if(FAILED(dev->CreateTexture2D(&d,&init,&fadeMask))||FAILED(dev->CreateShaderResourceView(fadeMask.Get(),nullptr,&fadeMaskSrv)))return false;}
 D3D11_TEXTURE2D_DESC d{};dst->GetDesc(&d);D3D11_RENDER_TARGET_VIEW_DESC rd{};rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;rd.Format=d.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS?DXGI_FORMAT_R8G8B8A8_UNORM:d.Format;ComPtr<ID3D11RenderTargetView>rt;
 if(FAILED(dev->CreateRenderTargetView(dst,&rd,&rt)))return false;
 return blit::mask_black(ctx,rt.Get(),d.Width,d.Height,fadeMaskSrv.Get(),1,false,std::clamp(alpha,0.f,1.f),1);
}
// Exercise the exact world fade pass and the same HUD blit ordering, without
// relying on a simulated headset. Reference pixels are independent equations.
bool fixture_menu_fade(){
 if(!eleven||!context)return false;
 D3D11_TEXTURE2D_DESC d{};d.Width=37;d.Height=41;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags=D3D11_BIND_RENDER_TARGET;
 ComPtr<ID3D11Texture2D>dst,cpu,text;ComPtr<ID3D11RenderTargetView>rt;ComPtr<ID3D11ShaderResourceView>textSrv;
 if(FAILED(eleven->CreateTexture2D(&d,nullptr,&dst))||FAILED(eleven->CreateRenderTargetView(dst.Get(),nullptr,&rt)))return false;
 auto stage=d;stage.BindFlags=0;stage.Usage=D3D11_USAGE_STAGING;stage.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(eleven->CreateTexture2D(&stage,nullptr,&cpu)))return false;
 auto td=d;td.Width=td.Height=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;const BYTE white[]={255,255,255,255};D3D11_SUBRESOURCE_DATA init{white,4,4};if(FAILED(eleven->CreateTexture2D(&td,&init,&text))||FAILED(eleven->CreateShaderResourceView(text.Get(),nullptr,&textSrv)))return false;
 const float background[]={.8f,.4f,.2f,1};const float clip[4][4]={{-.25f,.25f,0,1},{.25f,.25f,0,1},{-.25f,-.25f,0,1},{.25f,-.25f,0,1}};
 unsigned checks=0;
 for(int eye=0;eye<2;++eye)for(float alpha:{0.f,.125f,.5f,1.f,.5f,.125f,0.f}){
  context->ClearRenderTargetView(rt.Get(),background);
  // Deliberately leave a tiny, offset viewport/scissor; the fade must cover
  // the complete output and must restore the caller's pipeline bindings.
  const D3D11_VIEWPORT vp{3,4,5,6,0,1};context->RSSetViewports(1,&vp);ID3D11RenderTargetView* bound=rt.Get();context->OMSetRenderTargets(1,&bound,nullptr);
  if(!compose_menu_fade(context.Get(),dst.Get(),alpha))return false;
  D3D11_VIEWPORT after{};UINT count=1;context->RSGetViewports(&count,&after);ComPtr<ID3D11RenderTargetView>afterRt;context->OMGetRenderTargets(1,&afterRt,nullptr);
  if(count!=1||memcmp(&vp,&after,sizeof(vp))||afterRt.Get()!=rt.Get())return false;
  context->CopyResource(cpu.Get(),dst.Get());D3D11_MAPPED_SUBRESOURCE m{};if(FAILED(context->Map(cpu.Get(),0,D3D11_MAP_READ,0,&m)))return false;bool ok=true;
  for(UINT y=0;y<d.Height;++y)for(UINT x=0;x<d.Width;++x){const BYTE*p=static_cast<const BYTE*>(m.pData)+y*m.RowPitch+x*4;for(unsigned c=0;c<3;++c)ok=ok&&std::abs(int(p[c])-int(std::lround(255*background[c]*(1-alpha))))<=1;ok=ok&&p[3]==255;}
  context->Unmap(cpu.Get(),0);if(!ok)return false;
  if(!blit::premul_in_eye(context.Get(),rt.Get(),d.Width,d.Height,textSrv.Get(),clip))return false;
  context->CopyResource(cpu.Get(),dst.Get());if(FAILED(context->Map(cpu.Get(),0,D3D11_MAP_READ,0,&m)))return false;const BYTE*p=static_cast<const BYTE*>(m.pData)+(d.Height/2)*m.RowPitch+(d.Width/2)*4;
  ok=p[0]==255&&p[1]==255&&p[2]==255&&p[3]==255;context->Unmap(cpu.Get(),0);if(!ok)return false;++checks;
 }
 context->OMSetRenderTargets(0,nullptr,nullptr);publishedFadeAlpha=0;publishedFadeGeneration=0;fadeMaskSrv.Reset();fadeMask.Reset();
 printf("Full-view menu fade GPU: %u BOTH-eye rising/black/falling/no-fade cases, every world pixel reference matches, menu text remains WHITE even at full black; caller RT/viewport restore PASS\n",checks);return true;
}
bool compose_hud(ID3D11DeviceContext*ctx,int e,ID3D11Texture2D*dst){
 const float alpha=publishedFadeGeneration==hud9::generation()?publishedFadeAlpha:0;
 const bool faded=compose_menu_fade(ctx,dst,alpha);
 const bool markers=compose_markers(ctx,e,dst);
 const bool hud=compose_original_hud(ctx,e,dst)&&faded&&markers;if(!batFresh||batGeneration!=hud9::generation()||!dst)return hud;
 vr::HeadPose poses[2]{};float angles[2][4]{};XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
 for(int i=0;i<2;++i){if(!vr::get_eye_pose_gen(batGeneration,i,poses[i],angles[i]))return hud;views[i].pose.orientation={poses[i].qx,poses[i].qy,poses[i].qz,poses[i].qw};views[i].pose.position={poses[i].px,poses[i].py,poses[i].pz};views[i].fov={angles[i][0],angles[i][1],angles[i][2],angles[i][3]};}
 D3D11_TEXTURE2D_DESC bd{},out{};batImage->GetDesc(&bd);dst->GetDesc(&out);float clips[2][4][4]{};if(!transition_plane(views,float(bd.Width)/bd.Height,clips))return false;const auto&clip=clips[e];ComPtr<ID3D11Device>dev;ComPtr<ID3D11RenderTargetView>rt;ctx->GetDevice(&dev);D3D11_RENDER_TARGET_VIEW_DESC rd{};rd.Format=out.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS?DXGI_FORMAT_R8G8B8A8_UNORM:out.Format;rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
 if(FAILED(dev->CreateRenderTargetView(dst,&rd,&rt)))return false;return blit::premul_in_eye(ctx,rt.Get(),out.Width,out.Height,batSrv.Get(),clip)&&hud;
}
bool synth_eye(ID3D11DeviceContext* ctx,int eye,ID3D11Texture2D* drawn,ID3D11Texture2D* other){return afw::synth(ctx,eye,drawn,other);}
void synth_hud(ID3D11DeviceContext* ctx,int eye,ID3D11Texture2D* drawn,ID3D11Texture2D* other){compose_hud(ctx,eye,drawn);compose_hud(ctx,eye^1,other);}
bool compose_eye(ID3D11DeviceContext* ctx,int e,ID3D11Texture2D* dst,ID3D11Texture2D* source,bool deferLeftAllowed){
    if(!ctx||!dst||!source)return false;
    ComPtr<ID3D11Device> dev;ComPtr<ID3D11RenderTargetView> rt;ComPtr<ID3D11ShaderResourceView> src;
    ctx->GetDevice(&dev);D3D11_TEXTURE2D_DESC d{};dst->GetDesc(&d);
    D3D11_RENDER_TARGET_VIEW_DESC rd{};rd.Format=d.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS?DXGI_FORMAT_R8G8B8A8_UNORM:d.Format;rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
    if(FAILED(dev->CreateRenderTargetView(dst,&rd,&rt)))return false;
    for(auto& entry:wrappedCache)if(entry.texture.Get()==source&&entry.view){src=entry.view;break;}
    if(!src&&FAILED(dev->CreateShaderResourceView(source,nullptr,&src)))return false;
    native::Camera camera{};const bool nativeFrame=native::camera(camera)&&camera.eye==e;
    if(!render_scene(ctx,rt.Get(),d.Width,d.Height,src.Get(),nativeFrame?&camera:nullptr))return false;
    if(nativeFrame){
        if(e==0){stockLeftReady=false;leftTarget=dst;rightTarget.Reset();stockRightGeneration=0;}
        if(e==0&&dlss::mode()&&!camera.afw){D3D11_TEXTURE2D_DESC old{};if(stockLeft)stockLeft->GetDesc(&old);
            if(!stockLeft||!stockLeftRtv||old.Width!=d.Width||old.Height!=d.Height){stockLeftRtv.Reset();stockLeft.Reset();auto sd=d;sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;sd.MiscFlags=0;if(SUCCEEDED(dev->CreateTexture2D(&sd,nullptr,&stockLeft)))dev->CreateRenderTargetView(stockLeft.Get(),nullptr,&stockLeftRtv);}
            if(stockLeft&&stockLeftRtv){ctx->CopyResource(stockLeft.Get(),dst);stockLeftGeneration=camera.generation;stockLeftReady=true;}
        }
        if(e==1&&dlss::left_pending()){
            D3D11_TEXTURE2D_DESC old{};if(stockRight)stockRight->GetDesc(&old);
            if(!stockRight||old.Width!=d.Width||old.Height!=d.Height){stockRight.Reset();auto sd=d;sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;sd.MiscFlags=0;dev->CreateTexture2D(&sd,nullptr,&stockRight);}
            if(stockRight){ctx->CopyResource(stockRight.Get(),dst);stockRightGeneration=camera.generation;rightTarget=dst;}
        }
        D3D11_TEXTURE2D_DESC input{};source->GetDesc(&input);input.Width=camera.inputWidth;input.Height=camera.inputHeight;D3D11_TEXTURE2D_DESC old{};if(dlssScene[e])dlssScene[e]->GetDesc(&old);
        if(dlss::mode()&&(!dlssScene[e]||!dlssSceneRtv[e]||old.Width!=input.Width||old.Height!=input.Height)){
            dlssScene[e].Reset();dlssSceneRtv[e].Reset();input.Format=DXGI_FORMAT_R8G8B8A8_UNORM;input.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;input.MiscFlags=0;
            if(SUCCEEDED(dev->CreateTexture2D(&input,nullptr,&dlssScene[e])))dev->CreateRenderTargetView(dlssScene[e].Get(),nullptr,&dlssSceneRtv[e]);
        }
        const bool prepared=dlss::mode()&&dlssSceneRtv[e]&&render_scene(ctx,dlssSceneRtv[e].Get(),input.Width,input.Height,src.Get(),&camera);
        const bool depthMatches=depthFresh&&depthGeneration==camera.generation&&depthEye==(camera.afw?-1:(e?1:-1));
        // Without a stock LEFT backup, do not start a half-rollbackable pair.
        const bool rollbackReady=e==0||camera.afw||!dlss::left_pending()||rightTarget&&stockRightGeneration==camera.generation;
        auto* depth=prepared&&depthMatches&&rollbackReady&&(camera.afw||stockLeftReady)?sceneDepthSrv.Get():nullptr;
        if((dlss::mode()||dlss::left_pending())&&!dlss::process(ctx,camera,dlssScene[e].Get(),depth,dst,deferLeftAllowed))render_scene(ctx,rt.Get(),d.Width,d.Height,src.Get(),&camera);
    }else dlss::reset();
    auto restoreLeft=[&]{if(e==1&&dlss::rollback_left()&&stockLeftReady&&stockLeftGeneration==camera.generation&&leftTarget){ctx->CopyResource(leftTarget.Get(),stockLeft.Get());stereo_history::replace_left(ctx,stockLeft.Get(),camera.generation);BVR_LOG("[aa DLSS] right failure restored BOTH eyes to the stock pair");}};
    if(nativeFrame&&camera.afw)return true; // Warp the world first; overlay hook adds both HUDs afterward.
    compose_hud(ctx,e,dst);
    if(e==0&&stockLeftReady)compose_hud(ctx,e,stockLeft.Get());
    restoreLeft();return true;
}
bool capture_eye(ID3D11DeviceContext* ctx,int e,ID3D11Texture2D* dst,ID3D11Texture2D* source){return compose_eye(ctx,e,dst,source,false);}
bool hold_left(ID3D11DeviceContext* ctx,ID3D11Texture2D* dst,ID3D11Texture2D* source){return compose_eye(ctx,0,dst,source,true);}
void finish_left(ID3D11DeviceContext* ctx,ID3D11Texture2D* dst){
    if(!dlss::left_pending())return;
    if(dlss::finish_left(ctx,dst)){compose_hud(ctx,0,dst);stereo_history::replace_left(ctx,dst,stockLeftGeneration);}
    else if(dlss::rollback_left()&&stockLeftReady){ctx->CopyResource(dst,stockLeft.Get());stereo_history::replace_left(ctx,dst,stockLeftGeneration);
        if(rightTarget&&stockRight&&stockRightGeneration==stockLeftGeneration){ctx->CopyResource(rightTarget.Get(),stockRight.Get());compose_hud(ctx,1,rightTarget.Get());}}
    rightTarget.Reset();
}

bool checked(HRESULT hr,const char* stage) {
    if(FAILED(hr)) {
        if(++failures<=8) BVR_LOG("[aa bridge] %s failed: 0x%08lx",stage,hr);
        return false;
    }return true;
}
HMODULE load_system9() {
    if(!system9) {wchar_t path[MAX_PATH]{};GetSystemDirectoryW(path,MAX_PATH);wcscat_s(path,L"\\d3d9.dll");system9=LoadLibraryW(path);}
    return system9;
}
bool drain() {
    if(native9::device())return native9::drain();
    if(!queue || !fence) return true;
    context->Flush();
    const UINT64 value=++serial;
    if(!checked(queue->Signal(fence.Get(),value),"drain signal")) return false;
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!event) return false;
    const bool done=checked(fence->SetEventOnCompletion(value,event),"drain fence") && WaitForSingleObject(event,5000)==WAIT_OBJECT_0;
    CloseHandle(event);return done;
}
void release_frame() {
    connectorVS.Reset();connectorPS.Reset();connectorLayout.Reset();connectorVB.Reset();connectorBlend.Reset();connectorRaster.Reset();connectorDepth.Reset();
    for(auto&m:markerImages)m=MarkerImage{};
    publishedFadeAlpha=0;publishedFadeGeneration=0;fadeMaskSrv.Reset();fadeMask.Reset();
    batFresh=false;batGeneration=0;batRtv.Reset();batSrv.Reset();batSourceSrv.Reset();batImage.Reset();batSource.Reset();
    if(borrowedFrame){borrowedFrame->Release();borrowedFrame=nullptr;}
    clear_wrapped();
    frameRtv.Reset();colourSrv.Reset();colourSource.Reset();sourceFormat=DXGI_FORMAT_UNKNOWN;
    if(frame) {frame->Release();frame=nullptr;}
}
bool adopt(IDirect3DDevice9* dev) {
    if(managed9::native_device(dev)){
        if(!native9::adopt(dev))return false;
        if(!eleven){eleven=native9::device();context=native9::context();D3DDEVICE_CREATION_PARAMETERS cp{};dev->GetCreationParameters(&cp);gameWindow=cp.hFocusWindow;
            BVR_LOG("[aa bridge] NATIVE9EX -> D3D11 same-adapter shared GPU textures active; NO 9On12 / 11On12 / D3D12 rendering translation");
            dlss::prewarm(eleven.Get());} // PIMAXSTART: the helper starts now, during logos/menus, never at the first 3D frame
        return true;
    }
    if(nine) return elevenOn12!=nullptr;
    if(!checked(dev->QueryInterface(IID_PPV_ARGS(&nine)),"9On12 interface") ||
       !checked(nine->GetD3D12Device(IID_PPV_ARGS(&twelve)),"adopt game D3D12 device")) return false;
    BVR_LOG("[aa pipeline] creation trace %s",install_pipeline_trace()?"active":"unavailable");
    D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    if(!checked(twelve->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"ordered bridge queue") ||
       !checked(twelve->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"bridge fence")) return false;
    BVR_LOG("[aa dynamic bias] install %s",dynamic_pipeline::install(twelve.Get(),graphicsPSOOriginal)?"exact dynamic state":"stock unsupported");
    IUnknown* queues[]={queue.Get()};D3D_FEATURE_LEVEL level{};
    if(!checked(D3D11On12CreateDevice(twelve.Get(),D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,
        queues,1,0,&eleven,&context,&level),"11On12 device") ||
       !checked(eleven.As(&elevenOn12),"11On12 interface")) return false;
    D3DDEVICE_CREATION_PARAMETERS cp{};dev->GetCreationParameters(&cp);gameWindow=cp.hFocusWindow;
    const LUID luid=twelve->GetAdapterLuid();
    BVR_LOG("[aa bridge] adopted game GPU LUID %08lx:%08lx, 11On12 feature level 0x%x, window %p",luid.HighPart,luid.LowPart,level,gameWindow);
    return true;
}

bool capture(IDirect3DDevice9* dev,bool submit) {
    const double colourAt=cpu_now();
    if(dead || !adopt(dev)) return false;
    ComPtr<IDirect3DSurface9> surface;
    if(!checked(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surface),"game backbuffer")) return false;
    if(submit){publish_bats();publish_markers();}
    if(native9::device()){
        const unsigned slot=native::present_eye()>0?1:0;
        auto* shared=native9::colour(surface.Get(),slot);if(!shared)return false;
        D3D11_TEXTURE2D_DESC d{};shared->GetDesc(&d);window::observe_source(d.Width,d.Height);
        if(frame)frame->Release();frame=new Frame(shared,gameWindow);colourTime.add(cpu_now()-colourAt);
        bool ok=true;if(submit){const double at=cpu_now();ok=publish_hud();hudTime.add(cpu_now()-at);++frames;const double xr=cpu_now();d3d11_hook::process_bridge_frame(frame);xrTime.add(cpu_now()-xr);}
        if(offlineBorrowConsumer)ok=offlineBorrowConsumer(frame)&&ok;
        return native9::finish_reads()&&ok;
    }
    ComPtr<ID3D12Resource> resource;
    if(!checked(nine->UnwrapUnderlyingResource(surface.Get(),queue.Get(),IID_PPV_ARGS(&resource)),"unwrap completed game image")) return false;
    // The resource is checked out in COMMON. The same queue performs every
    // 11On12 copy, releases to COMMON and signals the fence handed back to 9On12.
    const auto rd=resource->GetDesc();
    window::observe_source(UINT(rd.Width),rd.Height);
    // Sample the checked-out game resource directly during this one Present.
    // The existing lens/input shader converts BGRA into the RGBA XR/DLSS
    // destination; there is no full-resolution copy or conversion beforehand.
    const bool directFormat=rd.Format==DXGI_FORMAT_B8G8R8A8_UNORM||rd.Format==DXGI_FORMAT_B8G8R8X8_UNORM||rd.Format==DXGI_FORMAT_R8G8B8A8_UNORM;
    if((submit||offlineBorrowConsumer)&&rd.SampleDesc.Count==1&&directFormat&&!(rd.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)){
        ComPtr<ID3D11Texture2D> borrowed;
        if(SUCCEEDED(wrapped_texture(resource.Get(),&borrowed,true))){
            const bool resized=borrowedFrame&&(borrowedFrame->desc.BufferDesc.Width!=rd.Width||borrowedFrame->desc.BufferDesc.Height!=rd.Height||borrowedFrame->desc.BufferDesc.Format!=rd.Format);
            if(resized&&submit){vr::on_resize(UINT(rd.Width),rd.Height,rd.Format);overlay::on_resize();hud::release_resources();}
            if(!borrowedFrame)borrowedFrame=new Frame(borrowed.Get(),gameWindow);
            borrowedFrame->image=borrowed;borrowedFrame->desc.BufferDesc.Width=UINT(rd.Width);borrowedFrame->desc.BufferDesc.Height=rd.Height;borrowedFrame->desc.BufferDesc.Format=rd.Format;
            ID3D11Resource* resources[]={borrowed.Get()};elevenOn12->AcquireWrappedResources(resources,1);
            const double preparedAt=cpu_now();bool consumed=true;
            if(submit){const double hudAt=cpu_now();publish_hud();hudTime.add(cpu_now()-hudAt);++frames;const double xrAt=cpu_now();d3d11_hook::process_bridge_frame(borrowedFrame);xrTime.add(cpu_now()-xrAt);}
            else consumed=offlineBorrowConsumer(borrowedFrame);
            const double returnAt=cpu_now();
            elevenOn12->ReleaseWrappedResources(resources,1);context->Flush();
            const UINT64 value=++serial;
            if(!checked(queue->Signal(fence.Get(),value),"borrowed colour return fence")){dead=true;surface.Detach();resource.Detach();borrowed.Detach();return false;}
            ID3D12Fence* fences[]={fence.Get()};UINT64 values[]={value};
            if(!checked(nine->ReturnUnderlyingResource(surface.Get(),1,values,fences),"return borrowed game image")){dead=true;return false;}
            borrowedFrame->image.Reset(); // The facade exposes no returned game resource outside its lease.
            if(submit)colourTime.add(preparedAt-colourAt+cpu_now()-returnAt);
            if(++borrowedFrames<=4)BVR_LOG("[aa colour] directly sampled checked-out game image %ux%u format %u flags %u; release/flush/fenced return AFTER consumers, no source copy/conversion",UINT(rd.Width),rd.Height,rd.Format,unsigned(rd.Flags));
            return consumed;
        }
    }
    if(submit||offlineBorrowConsumer)++borrowFallbacks;
    bool ok=true;
    if(!frame || frame->desc.BufferDesc.Width!=rd.Width || frame->desc.BufferDesc.Height!=rd.Height || sourceFormat!=rd.Format) {
        if(submit) {vr::on_resize(UINT(rd.Width),rd.Height,rd.Format);overlay::on_resize();hud::release_resources();}
        release_frame();
        D3D11_TEXTURE2D_DESC d{};d.Width=UINT(rd.Width);d.Height=rd.Height;d.Format=rd.Format;
        d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        // Origins' XR images are RGBA; D3D9 backbuffers are commonly BGRA/XRGB.
        // Never CopyResource between those unrelated families in the core.
        if(rd.Format!=DXGI_FORMAT_R8G8B8A8_UNORM) {
            ok=checked(eleven->CreateTexture2D(&d,nullptr,&colourSource),"source-format intermediate") &&
               checked(eleven->CreateShaderResourceView(colourSource.Get(),nullptr,&colourSrv),"source-format view");
        }
        d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        ComPtr<ID3D11Texture2D> image;
        ok=ok && checked(eleven->CreateTexture2D(&d,nullptr,&image),"owned RGBA game-frame texture") &&
            checked(eleven->CreateRenderTargetView(image.Get(),nullptr,&frameRtv),"RGBA frame view");
        if(ok) {sourceFormat=rd.Format;frame=new Frame(image.Get(),gameWindow);BVR_LOG("[aa bridge] actual game image %ux%u source format %u samples %u -> RGBA core format %u",d.Width,d.Height,rd.Format,rd.SampleDesc.Count,d.Format);
            RECT client{};if(GetClientRect(gameWindow,&client))BVR_LOG("[aa bridge] actual window client %ldx%ld; backbuffer %ux%u",client.right-client.left,client.bottom-client.top,d.Width,d.Height);
        }
    }
    ComPtr<ID3D11Texture2D> wrapped;
    if(ok) ok=checked(wrapped_texture(resource.Get(),&wrapped),"wrap game resource for frozen core");
    if(ok) {
        ID3D11Resource* resources[]={wrapped.Get()};elevenOn12->AcquireWrappedResources(resources,1);
        auto* destination=colourSource ? colourSource.Get():frame->image.Get();
        if(rd.SampleDesc.Count>1) context->ResolveSubresource(destination,0,wrapped.Get(),0,rd.Format);
        else context->CopyResource(destination,wrapped.Get());
        elevenOn12->ReleaseWrappedResources(resources,1);
        if(colourSource) ok=blit::shift_copy(context.Get(),frameRtv.Get(),UINT(rd.Width),rd.Height,colourSrv.Get(),0);
        context->Flush();
    }
    const UINT64 value=++serial;
    if(!checked(queue->Signal(fence.Get(),value),"resource return fence signal")) {
        // Device failure makes completion untrackable. Do not falsely return
        // or release a still checked-out resource; retain it until process exit.
        dead=true;surface.Detach();resource.Detach();wrapped.Detach();return false;
    }
    ID3D12Fence* fences[]={fence.Get()};UINT64 values[]={value};
    if(!checked(nine->ReturnUnderlyingResource(surface.Get(),1,values,fences),"return game image ownership")) {dead=true;return false;}
    if(ok && submit) {colourTime.add(cpu_now()-colourAt);const double hudAt=cpu_now();publish_hud();hudTime.add(cpu_now()-hudAt);++frames;const double xrAt=cpu_now();d3d11_hook::process_bridge_frame(frame);xrTime.add(cpu_now()-xrAt);}
    return ok;
}
HRESULT WINAPI present(IDirect3DDevice9* dev,const RECT* src,const RECT* dst,HWND wnd,const RGNDATA* dirty) {
    if(dev==ownDevice)hud9::end_render();
    const bool captured=dev==ownDevice&&capture(dev,true);
    if(dev==ownDevice&&!captured)native::discard_present();
    // Both world passes reach the XR bridge; only the completed pair swaps
    // the desktop image. Keep one physical desktop Present per stereo pair.
    const bool mirrorHud=dev==ownDevice&&captured&&native::present_eye()>0&&hudFresh&&hudGeneration==hud9::generation();
    if(dev==ownDevice)hud9::end_present();
    if(mirrorHud)hud9::mirror();
    if(captured && native::present_eye()<0&&!native::afw_present())return S_OK;
    if(dev!=ownDevice)return originalPresent(dev,src,dst,wnd,dirty);
    const double at=cpu_now();const auto hr=originalPresent(dev,src,dst,wnd,dirty);presentTime.add(cpu_now()-at);if(FAILED(hr)){BVR_LOG("[aa presentation] game Present failed 0x%08lx",hr);log_memory("Present failure");}return hr;
}
HRESULT WINAPI reset(IDirect3DDevice9* dev,D3DPRESENT_PARAMETERS* pp) {
    if(dev==ownDevice) {
        native::discard_present();
        billboard9::reset();
        if(!drain()) {dead=true;return D3DERR_DEVICELOST;}
        rightTarget.Reset();stockRight.Reset();stockRightGeneration=0;
        stereo_history::reset_gpu();hud9::reset();dlss::reset();afw::release();for(int e=0;e<2;++e){dlssSceneRtv[e].Reset();dlssScene[e].Reset();}stockLeftRtv.Reset();stockLeft.Reset();leftTarget.Reset();stockLeftReady=false;sceneDepthSrv.Reset();sceneDepth.Reset();depthFresh=false;hudRtv.Reset();hudSrv.Reset();hudSourceSrv.Reset();hudImage.Reset();hudSource.Reset();hudFresh=hudPlaced=false;markers9::invalidate_hud_plane();
        vr::on_resize(pp->BackBufferWidth,pp->BackBufferHeight,0);overlay::on_resize();hud::release_resources();release_frame();if(native9::device())native9::reset();
    }
    const UINT requestedInterval=pp?pp->PresentationInterval:0;
    if(dev==ownDevice)prepare_present(pp);
    const HRESULT hr=originalReset(dev,pp);
    if(dev==ownDevice)BVR_LOG("[aa presentation] Reset interval requested 0x%08x -> effective 0x%08x (desktop VSync disabled)",requestedInterval,pp?pp->PresentationInterval:0);
    BVR_LOG("[aa bridge] game Reset %ux%u -> 0x%08lx",pp->BackBufferWidth,pp->BackBufferHeight,hr);
    return hr;
}
HRESULT create_device(IDirect3D9* d,UINT adapter,D3DDEVTYPE type,HWND wnd,DWORD behavior,D3DPRESENT_PARAMETERS* pp,IDirect3DDevice9** out,bool hookDevice,IDirect3D9* logicalFactory=nullptr) {
    const UINT requestedInterval=pp?pp->PresentationInterval:0;
    if(hookDevice&&!ownDevice)prepare_present(pp);
    ComPtr<IDirect3D9Ex> exFactory;
    HRESULT hr;
    if(SUCCEEDED(d->QueryInterface(IID_PPV_ARGS(&exFactory)))){
        ComPtr<IDirect3DDevice9Ex> exDevice;hr=exFactory->CreateDeviceEx(adapter,type,wnd,behavior,pp,nullptr,&exDevice);
        if(SUCCEEDED(hr)&&out)*out=managed9::wrap(exDevice.Get(),logicalFactory);
    }else hr=d->CreateDevice(adapter,type,wnd,behavior,pp,out);
    if(FAILED(hr)) {
        BVR_LOG("[aa bridge] game CreateDevice adapter %u flags 0x%x %ux%u windowed %u format %u -> 0x%08lx",adapter,behavior,pp?pp->BackBufferWidth:0,pp?pp->BackBufferHeight:0,pp?pp->Windowed:0,pp?pp->BackBufferFormat:0,hr);
        return hr;
    }
    if(!hookDevice || !out || !*out) return hr;
    if(ownDevice && ownDevice!=*out) {BVR_LOG("[aa bridge] additional game device left untouched");return hr;}
    ownDevice=*out;
    if(managed9::native_device(ownDevice)&&!adopt(ownDevice))BVR_LOG("[aa bridge] native GPU sharing initialization failed");
    BVR_LOG("[aa allocation] large-reservation tracing %s",install_allocation_trace()?"active":"unavailable");
    BVR_LOG("[aa presentation] CreateDevice interval requested 0x%08x -> effective 0x%08x, windowed %u (desktop VSync disabled)",requestedInterval,pp?pp->PresentationInterval:0,pp?pp->Windowed:0);
    BVR_LOG("[aa bridge] LIVE game device, adapter %u flags 0x%x FPU-preserve %u",adapter,behavior,!!(behavior&D3DCREATE_FPU_PRESERVE));
    auto** v=*reinterpret_cast<void***>(ownDevice);
    if(MH_CreateHook(v[17],reinterpret_cast<void*>(present),reinterpret_cast<void**>(&originalPresent))!=MH_OK ||
       MH_CreateHook(v[16],reinterpret_cast<void*>(reset),reinterpret_cast<void**>(&originalReset))!=MH_OK) {
        BVR_LOG("[aa bridge] device hooks unavailable; game remains flat");return hr;
    }
    MH_QueueEnableHook(v[17]);MH_QueueEnableHook(v[16]);MH_ApplyQueued();
    hud9::install(ownDevice);detail9::install(ownDevice);billboard9::install(ownDevice);vr::set_eye_capture_hook(capture_eye);vr::set_eye_synth_hook(synth_eye);vr::set_eye_synth_overlay_hook(synth_hud);vr::set_transition_texture_provider(flat_bats);vr::set_transition_distance(.8f);
    vr::set_eye_defer_hooks(hold_left,finish_left); // LEFT NGX runs while the engine renders RIGHT
    return hr;
}

// Scope interception to each returned factory. System D3D9/9On12 factories
// can dispatch internally through different implementations of CreateDevice;
// replacing a process-global trampoline on a second factory recurses into our
// first hook. A COM wrapper forwards directly to its own unmodified factory.
class Factory final : public IDirect3D9 {
    std::atomic<ULONG> refs{1};
    ComPtr<IDirect3D9> real;
    bool hookDevice;
public:
    Factory(IDirect3D9* owned,bool hook):hookDevice(hook) {real.Attach(owned);}
    HRESULT WINAPI QueryInterface(REFIID iid,void** out) override {
        if(!out)return E_POINTER;
        *out=nullptr;
        if(iid==__uuidof(IUnknown)||iid==__uuidof(IDirect3D9)) {
            *out=static_cast<IDirect3D9*>(this);AddRef();return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG WINAPI AddRef() override {return ++refs;}
    ULONG WINAPI Release() override {const ULONG n=--refs;if(!n)delete this;return n;}
    HRESULT WINAPI RegisterSoftwareDevice(void* p) override {return real->RegisterSoftwareDevice(p);}
    UINT WINAPI GetAdapterCount() override {return real->GetAdapterCount();}
    HRESULT WINAPI GetAdapterIdentifier(UINT a,DWORD f,D3DADAPTER_IDENTIFIER9* p) override {return real->GetAdapterIdentifier(a,f,p);}
    UINT WINAPI GetAdapterModeCount(UINT a,D3DFORMAT f) override {return real->GetAdapterModeCount(a,f);}
    HRESULT WINAPI EnumAdapterModes(UINT a,D3DFORMAT f,UINT m,D3DDISPLAYMODE* p) override {return real->EnumAdapterModes(a,f,m,p);}
    HRESULT WINAPI GetAdapterDisplayMode(UINT a,D3DDISPLAYMODE* p) override {return real->GetAdapterDisplayMode(a,p);}
    HRESULT WINAPI CheckDeviceType(UINT a,D3DDEVTYPE t,D3DFORMAT f,D3DFORMAT b,BOOL w) override {return real->CheckDeviceType(a,t,f,b,w);}
    HRESULT WINAPI CheckDeviceFormat(UINT a,D3DDEVTYPE t,D3DFORMAT f,DWORD u,D3DRESOURCETYPE r,D3DFORMAT c) override {return real->CheckDeviceFormat(a,t,f,u,r,c);}
    HRESULT WINAPI CheckDeviceMultiSampleType(UINT a,D3DDEVTYPE t,D3DFORMAT f,BOOL w,D3DMULTISAMPLE_TYPE m,DWORD* q) override {return real->CheckDeviceMultiSampleType(a,t,f,w,m,q);}
    HRESULT WINAPI CheckDepthStencilMatch(UINT a,D3DDEVTYPE t,D3DFORMAT f,D3DFORMAT r,D3DFORMAT z) override {return real->CheckDepthStencilMatch(a,t,f,r,z);}
    HRESULT WINAPI CheckDeviceFormatConversion(UINT a,D3DDEVTYPE t,D3DFORMAT s,D3DFORMAT d) override {return real->CheckDeviceFormatConversion(a,t,s,d);}
    HRESULT WINAPI GetDeviceCaps(UINT a,D3DDEVTYPE t,D3DCAPS9* p) override {return real->GetDeviceCaps(a,t,p);}
    HMONITOR WINAPI GetAdapterMonitor(UINT a) override {return real->GetAdapterMonitor(a);}
    HRESULT WINAPI CreateDevice(UINT a,D3DDEVTYPE t,HWND w,DWORD b,D3DPRESENT_PARAMETERS* p,IDirect3DDevice9** d) override {
        return create_device(real.Get(),a,t,w,b,p,d,hookDevice,this);
    }
};
}
bool install() {ready.store(true);return load_system9()!=nullptr;}
void status(){
 log_memory("heartbeat");
 BVR_LOG("[aa colour] route %s; borrowed frames %u copy fallback %u",native9::device()?"native9Ex shared DX11":"9On12 / 11On12",borrowedFrames,borrowFallbacks);
 BVR_LOG("[aa budget] CPU wall ms avg/max: colour transport %.3f/%.3f (%u), HUD transport %.3f/%.3f (%u), XR bridge incl DLSS waits %.3f/%.3f (%u), depth transport %.3f/%.3f (%u), physical desktop Present %.3f/%.3f (%u); spans reset each heartbeat, CPU submission/waits not GPU execution",colourTime.mean(),colourTime.peak,colourTime.count,hudTime.mean(),hudTime.peak,hudTime.count,xrTime.mean(),xrTime.peak,xrTime.count,depthTime.mean(),depthTime.peak,depthTime.count,presentTime.mean(),presentTime.peak,presentTime.count);
 colourTime={};hudTime={};xrTime={};depthTime={};presentTime={};
}
void draw_ui() {ImGui::Text("Game frames %u; bridge failures %u",frames.load(),failures.load());ImGui::Text("Graphics: %s",native9::device()?"Native DX9Ex -> DX11":"DX9 -> DX12 -> DX11");}
void init_hud_settings(){
 swprintf_s(hudSettings,L"%s\\aa_rendering.ini",log::data_dir());
 wchar_t v[32];GetPrivateProfileStringW(L"AsylumVR",L"HUDDistance",L"4",v,32,hudSettings);const float d=float(wcstod(v,nullptr));hudDistance=std::isfinite(d)?std::clamp(d,1.f,16.f):4;
 GetPrivateProfileStringW(L"AsylumVR",L"HUDScale",L"1",v,32,hudSettings);const float s=float(wcstod(v,nullptr));hudScale=std::isfinite(s)?std::clamp(s,.3f,1.2f):1;
}
void draw_hud_ui(){
 float d=hudDistance.load(),s=hudScale.load();bool changed=false;
 if(overlay::SliderStep("HUD distance (metres)",&d,1,16,"%.1f")){hudDistance=d;changed=true;}
 if(overlay::SliderStep("HUD size",&s,.3f,1.2f)){hudScale=s;changed=true;}
 if(changed){wchar_t v[32];swprintf_s(v,L"%.2f",d);WritePrivateProfileStringW(L"AsylumVR",L"HUDDistance",v,hudSettings);swprintf_s(v,L"%.2f",s);WritePrivateProfileStringW(L"AsylumVR",L"HUDScale",v,hudSettings);hudRecentre=true;markers9::invalidate_hud_plane();}
 ImGui::TextWrapped("In first person, enable 'aim and HUD follow the headset' to keep the HUD in front of you. Otherwise it stays in the room.");
}
void recenter_hud(){hudRecentre=true;markers9::invalidate_hud_plane();dlss::reset();}
bool save_hud(const wchar_t* path){return hudFresh&&hudImage&&gfx_hud::save_texture_bmp(context.Get(),hudImage.Get(),path);}
bool save_depth(const wchar_t* path){
 if(!depthFresh||!sceneDepthSrv)return false;
 // Keep raw physical DS values: an 8-bit preview cannot distinguish a
 // cleared plane from distant geometry or establish its depth convention.
 D3D11_TEXTURE2D_DESC rawDesc{};sceneDepth->GetDesc(&rawDesc);auto read=rawDesc;
 read.BindFlags=read.MiscFlags=0;read.Usage=D3D11_USAGE_STAGING;read.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> staging;
 if(SUCCEEDED(eleven->CreateTexture2D(&read,nullptr,&staging))){
  context->CopyResource(staging.Get(),sceneDepth.Get());D3D11_MAPPED_SUBRESOURCE m{};
  if(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&m))){
   wchar_t rawPath[MAX_PATH]{};swprintf_s(rawPath,L"%s.raw",path);FILE* f=_wfopen(rawPath,L"wb");
   const unsigned bytes=rawDesc.Format==DXGI_FORMAT_R16_TYPELESS?2:4;
   const uint32_t header[]={0x31445041,rawDesc.Width,rawDesc.Height,unsigned(rawDesc.Format)};
   if(f){fwrite(header,sizeof(header),1,f);for(unsigned y=0;y<read.Height;++y)fwrite(static_cast<const BYTE*>(m.pData)+y*m.RowPitch,bytes,read.Width,f);fclose(f);}
   double minimum=1,maximum=0;uint64_t zero{},one{},pixels{};
   for(unsigned y=0;y<read.Height;++y)for(unsigned x=0;x<read.Width;++x){const auto*p=static_cast<const BYTE*>(m.pData)+y*m.RowPitch+x*bytes;double z{};
    if(rawDesc.Format==DXGI_FORMAT_R24G8_TYPELESS){uint32_t v;memcpy(&v,p,4);z=(v&0xffffff)/16777215.;}
    else if(bytes==2){uint16_t v;memcpy(&v,p,2);z=v/65535.;}
    else{float v;memcpy(&v,p,4);z=v;}
    minimum=std::min(minimum,z);maximum=std::max(maximum,z);zero+=z==0;one+=z==1;++pixels;
   }
   BVR_LOG("[aa depth raw] range %.9f..%.9f, clear0 %llu clear1 %llu / %llu, eye %d generation %u",minimum,maximum,zero,one,pixels,depthEye,depthGeneration);
   swprintf_s(rawPath,L"%s.txt",path);if(FILE* info=_wfopen(rawPath,L"wt")){fprintf(info,"Physical D3D9 depth; header APD1,width,height,DXGI_FORMAT (uint32), then tightly packed rows.\nrange %.9f..%.9f; exact0 %llu exact1 %llu pixels %llu; eye %d generation %u\n",minimum,maximum,zero,one,pixels,depthEye,depthGeneration);fclose(info);}
   context->Unmap(staging.Get(),0);
  }
 }
 D3D11_TEXTURE2D_DESC d=rawDesc;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
 ComPtr<ID3D11Texture2D> image;ComPtr<ID3D11RenderTargetView> rt;
 return SUCCEEDED(eleven->CreateTexture2D(&d,nullptr,&image))&&SUCCEEDED(eleven->CreateRenderTargetView(image.Get(),nullptr,&rt))&&blit::shift_copy(context.Get(),rt.Get(),d.Width,d.Height,sceneDepthSrv.Get(),0)&&gfx_hud::save_texture_bmp(context.Get(),image.Get(),path);
}
ID3D11ShaderResourceView* afw_depth(){native::Camera c{};return native::camera(c)&&c.afw&&depthFresh&&depthGeneration==c.generation&&depthEye==-1?sceneDepthSrv.Get():nullptr;}
void invalidate_depth(){depthFresh=false;}
namespace {std::atomic<bool> captureDepthRequested{};}
bool depth_required(){return dlss::mode()!=0||afw::on()||captureDepthRequested.load();}
bool depth_ready(){return depthFresh&&sceneDepthSrv;}
void request_depth_capture(bool enabled){captureDepthRequested=enabled;}
bool capture_scene_depth(IDirect3DSurface9* surface,int eye,unsigned generation){
    const double at=cpu_now();
    if(native9::device()){
        depthFresh=false;if(dead||!native9::depth(surface,sceneDepth.ReleaseAndGetAddressOf(),sceneDepthSrv.ReleaseAndGetAddressOf()))return false;
        depthEye=eye;depthGeneration=generation;depthFresh=true;depthTime.add(cpu_now()-at);return true;
    }
    depthFresh=false;if(!surface||!nine||!elevenOn12||dead)return false;
    ComPtr<ID3D12Resource> resource;if(!checked(nine->UnwrapUnderlyingResource(surface,queue.Get(),IID_PPV_ARGS(&resource)),"unwrap scene depth"))return false;
    const auto rd=resource->GetDesc();DXGI_FORMAT resourceFormat{},srvFormat{};
    if(rd.Format==DXGI_FORMAT_R24G8_TYPELESS||rd.Format==DXGI_FORMAT_D24_UNORM_S8_UINT){resourceFormat=DXGI_FORMAT_R24G8_TYPELESS;srvFormat=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;}
    else if(rd.Format==DXGI_FORMAT_R32_TYPELESS||rd.Format==DXGI_FORMAT_D32_FLOAT){resourceFormat=DXGI_FORMAT_R32_TYPELESS;srvFormat=DXGI_FORMAT_R32_FLOAT;}
    else if(rd.Format==DXGI_FORMAT_R16_TYPELESS||rd.Format==DXGI_FORMAT_D16_UNORM){resourceFormat=DXGI_FORMAT_R16_TYPELESS;srvFormat=DXGI_FORMAT_R16_UNORM;}
    bool ok=resourceFormat!=DXGI_FORMAT_UNKNOWN&&rd.SampleDesc.Count==1;
    D3D11_TEXTURE2D_DESC d{};if(sceneDepth)sceneDepth->GetDesc(&d);
    if(ok&&(!sceneDepth||!sceneDepthSrv||d.Width!=rd.Width||d.Height!=rd.Height||d.Format!=resourceFormat)){
        sceneDepthSrv.Reset();sceneDepth.Reset();d={};d.Width=UINT(rd.Width);d.Height=rd.Height;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=resourceFormat;d.BindFlags=D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
        D3D11_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=srvFormat;sd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sd.Texture2D.MipLevels=1;
        ok=checked(eleven->CreateTexture2D(&d,nullptr,&sceneDepth),"readable owned scene depth")&&checked(eleven->CreateShaderResourceView(sceneDepth.Get(),&sd,&sceneDepthSrv),"scene depth SRV");
    }
    ComPtr<ID3D11Texture2D> wrapped;
    if(ok)ok=checked(wrapped_texture(resource.Get(),&wrapped),"wrap scene depth");
    if(ok){ID3D11Resource* resources[]={wrapped.Get()};elevenOn12->AcquireWrappedResources(resources,1);context->CopyResource(sceneDepth.Get(),wrapped.Get());elevenOn12->ReleaseWrappedResources(resources,1);context->Flush();}
    const UINT64 value=++serial;if(!checked(queue->Signal(fence.Get(),value),"depth return fence")){dead=true;resource.Detach();wrapped.Detach();return false;}
    ID3D12Fence* fences[]={fence.Get()};UINT64 values[]={value};if(!checked(nine->ReturnUnderlyingResource(surface,1,values,fences),"return scene depth")){dead=true;return false;}
    if(ok){depthEye=eye;depthGeneration=generation;depthFresh=true;static unsigned logs{};if(logs++<4)BVR_LOG("[aa depth] captured actual scene D3D9 depth %ux%u format %u, eye %d generation %u",d.Width,d.Height,rd.Format,eye,generation);}
    if(ok)depthTime.add(cpu_now()-at);return ok;
}
}

extern "C" __declspec(dllexport) IDirect3D9* WINAPI AA_CreateD3D9(UINT sdk) {
    using namespace bvr::aa::bridge;
    const ULONGLONG deadline=GetTickCount64()+10000;
    while(!ready.load() && AA_Identity()!=2 && GetTickCount64()<deadline) Sleep(1);
    HMODULE sys=load_system9();if(!sys) return nullptr;
    const auto native=reinterpret_cast<IDirect3D9*(WINAPI*)(UINT)>(GetProcAddress(sys,"Direct3DCreate9"));
    if(!ready.load() || AA_Identity()!=1) return native ? native(sdk):nullptr;
    wchar_t route[32]{};GetEnvironmentVariableW(L"AA_GRAPHICS_BRIDGE",route,32);
    if(_wcsicmp(route,L"9on12")!=0){
        auto createEx=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(sys,"Direct3DCreate9Ex"));IDirect3D9Ex* ex{};
        if(createEx&&SUCCEEDED(createEx(sdk,&ex))){BVR_LOG("[aa bridge] selected native Direct3D9Ex / direct Direct3D11 sharing; managed-resource compatibility active");return new Factory(ex,true);}
        BVR_LOG("[aa bridge] native9Ex factory unavailable; retaining 9On12 rollback route");
    }
    auto factory=reinterpret_cast<PFN_Direct3DCreate9On12>(GetProcAddress(sys,"Direct3DCreate9On12"));
    if(!factory) {BVR_LOG("[aa bridge] plain 9On12 factory unavailable; game remains flat");return native(sdk);}
    D3D9ON12_ARGS args{};args.Enable9On12=TRUE;
    IDirect3D9* d=factory(sdk,&args,1);
    if(!d) {BVR_LOG("[aa bridge] 9On12 creation failed; game remains flat");return native(sdk);}
    BVR_LOG("[aa bridge] plain Direct3DCreate9On12 active; factory-scoped forwarding, behavior flags preserved");
    return new Factory(d,true);
}

extern "C" __declspec(dllexport) DWORD WINAPI AA_NativeBridgeSelftest(HWND window) {
    using namespace bvr;using namespace bvr::aa;using namespace bvr::aa::bridge;
    setvbuf(stdout,nullptr,_IONBF,0);
    struct Cleanup{~Cleanup(){hud9::reset();release_frame();dlss::shutdown();afw::release();blit::release();sceneDepthSrv.Reset();sceneDepth.Reset();hudSrv.Reset();hudRtv.Reset();hudImage.Reset();hudSourceSrv.Reset();hudSource.Reset();native9::shutdown();context.Reset();eleven.Reset();MH_DisableHook(MH_ALL_HOOKS);MH_Uninitialize();}} cleanup;
    puts("Native assembled fixture: create native factory/device");
    auto createEx=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(load_system9(),"Direct3DCreate9Ex"));IDirect3D9Ex* raw{};
    if(!createEx||FAILED(createEx(D3D_SDK_VERSION,&raw)))return 1;
    ComPtr<IDirect3D9> factory;factory.Attach(new Factory(raw,false));
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.hDeviceWindow=window;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    ComPtr<IDirect3DDevice9> dev;if(FAILED(factory->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_FPU_PRESERVE|D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&dev))||!managed9::native_device(dev.Get())||!adopt(dev.Get()))return 2;
    ComPtr<IDirect3D9> returned;if(FAILED(dev->GetDirect3D(&returned))||returned.Get()!=factory.Get())return 3;returned.Reset();
    // Finished movie/loading bytes must survive sampling into an sRGB XR RTV.
    // Include actual native9 BGRA sharing plus RGBA/BGRX fallback families.
    {
        const BYTE levels[]={0,1,2,5,10,16,32,64,96,128,160,192,224,240,254,255};
        ComPtr<IDirect3DSurface9> source9;if(FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&source9)))return 24;
        for(int i=0;i<16;++i){const RECT r{i*4,0,i*4+4,64};if(FAILED(dev->ColorFill(source9.Get(),&r,D3DCOLOR_ARGB(255,levels[i],levels[(i+3)%16],255-levels[i]))))return 25;}
        ComPtr<ID3D11Texture2D> shared=native9::colour(source9.Get(),0);if(!shared)return 26;
        const float corners[4][4]={{-1,1,0,1},{1,1,0,1},{-1,-1,0,1},{1,-1,0,1}};
        unsigned checks{};BYTE oldGrey{};
        for(auto sourceFormat:{DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8X8_UNORM}){
            ComPtr<ID3D11Texture2D> source=shared;
            if(sourceFormat!=DXGI_FORMAT_B8G8R8A8_UNORM){std::vector<BYTE> data(64*64*4);for(int y=0;y<64;++y)for(int x=0;x<64;++x){const int i=x/4;const BYTE r=levels[i],g=levels[(i+3)%16],b=255-r;const size_t at=(y*64+x)*4;data[at]=sourceFormat==DXGI_FORMAT_R8G8B8A8_UNORM?r:b;data[at+1]=g;data[at+2]=sourceFormat==DXGI_FORMAT_R8G8B8A8_UNORM?b:r;data[at+3]=255;}D3D11_TEXTURE2D_DESC d{};shared->GetDesc(&d);d.Format=sourceFormat;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;d.MiscFlags=0;D3D11_SUBRESOURCE_DATA bytes{data.data(),256,0};source.Reset();if(FAILED(eleven->CreateTexture2D(&d,&bytes,&source)))return 27;}
            for(auto targetFormat:{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_R8G8B8A8_UNORM}){
                D3D11_TEXTURE2D_DESC d{};source->GetDesc(&d);const auto policy=gfx::encoded_screen_formats(d.Format,targetFormat);d.Format=policy.resource;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;d.MiscFlags=0;
                ComPtr<ID3D11Texture2D> copy,target,read;ComPtr<ID3D11ShaderResourceView> view;ComPtr<ID3D11RenderTargetView> rt;
                D3D11_SHADER_RESOURCE_VIEW_DESC sd{};sd.Format=policy.view;sd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sd.Texture2D.MipLevels=1;
                if(FAILED(eleven->CreateTexture2D(&d,nullptr,&copy))||FAILED(eleven->CreateShaderResourceView(copy.Get(),&sd,&view)))return 28;context->CopyResource(copy.Get(),source.Get());
                d.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS;d.BindFlags=D3D11_BIND_RENDER_TARGET;D3D11_RENDER_TARGET_VIEW_DESC rd{};rd.Format=targetFormat;rd.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
                if(FAILED(eleven->CreateTexture2D(&d,nullptr,&target))||FAILED(eleven->CreateRenderTargetView(target.Get(),&rd,&rt))||!blit::screen_in_eye(context.Get(),rt.Get(),64,64,view.Get(),corners))return 29;
                d.BindFlags=0;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(eleven->CreateTexture2D(&d,nullptr,&read)))return 30;context->CopyResource(read.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE mapped{};if(FAILED(context->Map(read.Get(),0,D3D11_MAP_READ,0,&mapped)))return 31;
                bool exact=true;for(unsigned i=0;i<16;++i){const BYTE* p=static_cast<const BYTE*>(mapped.pData)+32*mapped.RowPitch+(i*4+2)*4;const int expected[]={levels[i],levels[(i+3)%16],255-levels[i],255};for(int channel=0;channel<4;++channel)exact=exact&&std::abs(int(p[channel])-expected[channel])<=1;}context->Unmap(read.Get(),0);if(!exact)return 32;++checks;
                if(sourceFormat==DXGI_FORMAT_B8G8R8A8_UNORM&&targetFormat==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB){view.Reset();sd.Format=DXGI_FORMAT_B8G8R8A8_UNORM;if(FAILED(eleven->CreateShaderResourceView(copy.Get(),&sd,&view))||!blit::screen_in_eye(context.Get(),rt.Get(),64,64,view.Get(),corners))return 33;context->CopyResource(read.Get(),target.Get());if(FAILED(context->Map(read.Get(),0,D3D11_MAP_READ,0,&mapped)))return 34;oldGrey=*(static_cast<const BYTE*>(mapped.pData)+32*mapped.RowPitch+38*4);context->Unmap(read.Get(),0);if(std::abs(int(oldGrey)-188)>1)return 35;}
            }
        }
        if(!native9::finish_reads())return 36;
        printf("Flat movie/loading gamma GPU: %u RGBA/BGRA/BGRX x SRGB/UNORM projected screens preserve 16 colour bands including blacks/alpha within1LSB; old midgrey128->%u, corrected128->128 PASS\n",checks,oldGrey);
    }
    puts("Native assembled fixture: install production HUD hooks");
    if(MH_Initialize()!=MH_OK||!hud9::install(dev.Get()))return 4;
    wchar_t particleShader[MAX_PATH]{};GetModuleFileNameW(nullptr,particleShader,MAX_PATH);*wcsrchr(particleShader,L'\\')=0;wcscat_s(particleShader,L"\\..\\..\\re\\evidence\\particle-vs-original.bin");
    if(!billboard9::fixture(dev.Get(),particleShader))return 37;
    *wcsrchr(particleShader,L'\\')=0;if(!transitions9::fixture(dev.Get(),particleShader,fixture_bat_transport))return 38;if(!hud9::fixture_alpha(dev.Get(),particleShader))return 39;if(!fixture_menu_fade())return 41;if(!markers9::fixture(dev.Get(),particleShader,fixture_marker_transport))return 42;if(!fixture_connector_gpu())return 43;
    hud9::reset();
    puts("Native assembled fixture: create scene/depth targets");
    ComPtr<IDirect3DTexture9> scene;ComPtr<IDirect3DSurface9> sceneSurface,bb,depth;
    if(FAILED(dev->CreateTexture(64,64,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&scene,nullptr))||FAILED(scene->GetSurfaceLevel(0,&sceneSurface))||FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb))||FAILED(dev->CreateDepthStencilSurface(64,64,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr)))return 5;
    dev->ColorFill(sceneSurface.Get(),nullptr,0xff0000ff);
    struct Q{float x,y,z,w,u,v;};const Q full[]={{-.5f,-.5f,.5f,1,0,0},{63.5f,-.5f,.5f,1,1,0},{-.5f,63.5f,.5f,1,0,1},{63.5f,63.5f,.5f,1,1,1}};
    struct W{float x,y,z,w;DWORD colour;};const W widget[]={{15.5f,15.5f,0,1,0x80ff0000},{47.5f,15.5f,0,1,0x80ff0000},{15.5f,47.5f,0,1,0x80ff0000},{47.5f,47.5f,0,1,0x80ff0000}};
    request_depth_capture(true);dlss::set_mode(0,false);afw::set_on(false);
    for(int eye:{-1,1}){
        printf("Native assembled fixture: begin eye %d\n",eye);
        hud9::begin_eye(eye,321);dev->SetRenderTarget(0,sceneSurface.Get());dev->SetDepthStencilSurface(depth.Get());
        dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);dev->SetRenderState(D3DRS_COLORWRITEENABLE,0);
        if(FAILED(dev->BeginScene())||FAILED(dev->Clear(0,nullptr,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,eye<0?.25f:.75f,0)))return 6;
        Q geometry[4];memcpy(geometry,full,sizeof(full));for(auto& v:geometry)v.z=eye<0?.25f:.75f;
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,geometry,sizeof(Q)))||FAILED(dev->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,1,0)))return 7;
        dev->SetDepthStencilSurface(nullptr);dev->SetRenderTarget(0,bb.Get());dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);
        dev->SetTexture(0,scene.Get());dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,full,sizeof(Q)))||!depthFresh||depthEye!=eye||depthGeneration!=321)return 8;
        D3D11_TEXTURE2D_DESC rd{};sceneDepth->GetDesc(&rd);rd.BindFlags=rd.MiscFlags=0;rd.Usage=D3D11_USAGE_STAGING;rd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> read;
        if(FAILED(eleven->CreateTexture2D(&rd,nullptr,&read)))return 9;context->CopyResource(read.Get(),sceneDepth.Get());D3D11_MAPPED_SUBRESOURCE m{};if(FAILED(context->Map(read.Get(),0,D3D11_MAP_READ,0,&m)))return 10;
        float z{};memcpy(&z,static_cast<const BYTE*>(m.pData)+32*m.RowPitch+128,4);context->Unmap(read.Get(),0);if(std::fabs(z-(eye<0?.25f:.75f))>2.f/16777215)return 11;
        printf("Native production depth eye %d actual geometry %.9f survived later clear1/rebind PASS\n",eye,z);
        dev->SetTexture(0,nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
        dev->SetRenderState(D3DRS_STENCILENABLE,TRUE);dev->SetRenderState(D3DRS_STENCILREF,1);dev->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_ALWAYS);dev->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_REPLACE);dev->SetRenderState(D3DRS_COLORWRITEENABLE,0);
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,widget,sizeof(W))))return 12;
        dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);dev->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_EQUAL);dev->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_KEEP);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,widget,sizeof(W)))||FAILED(dev->EndScene()))return 13;hud9::end_render();
        if(eye<0&&(!hud9::sheet()||!publish_hud()))return 14;if(eye>0&&hud9::sheet())return 15;
        if(!capture(dev.Get(),false))return 16;
        D3D11_TEXTURE2D_DESC out{};frame->image->GetDesc(&out);out.Format=DXGI_FORMAT_R8G8B8A8_UNORM;out.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;out.MiscFlags=0;
        ComPtr<ID3D11Texture2D> image;ComPtr<ID3D11RenderTargetView> rt;ComPtr<ID3D11ShaderResourceView> src;if(FAILED(eleven->CreateTexture2D(&out,nullptr,&image))||FAILED(eleven->CreateRenderTargetView(image.Get(),nullptr,&rt))||FAILED(eleven->CreateShaderResourceView(frame->image.Get(),nullptr,&src))||!render_scene(context.Get(),rt.Get(),64,64,src.Get(),nullptr))return 17;
        auto getCenter=[&](ID3D11Texture2D* t,unsigned char (&pixel)[4]){D3D11_TEXTURE2D_DESC d{};t->GetDesc(&d);d.BindFlags=d.MiscFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;if(FAILED(eleven->CreateTexture2D(&d,nullptr,&staging)))return false;context->CopyResource(staging.Get(),t);D3D11_MAPPED_SUBRESOURCE map{};if(FAILED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map)))return false;memcpy(pixel,static_cast<const BYTE*>(map.pData)+32*map.RowPitch+128,4);context->Unmap(staging.Get(),0);return true;};
        unsigned char pixel[4]{};if(!getCenter(image.Get(),pixel)||pixel[0]!=0||pixel[1]!=0||pixel[2]!=255)return 18;
        if(!getCenter(hudImage.Get(),pixel)||pixel[0]<120||pixel[1]!=0||pixel[2]!=0||pixel[3]<120)return 19;
        // Exercise the exact asymmetric eye lens on the native DX11 device.
        for(int e=0;e<2;++e){native::Camera c{};c.centred=true;c.inputWidth=c.inputHeight=64;const float angles[]={e?-.62f:-.86f,e?.86f:.62f,.69f,-.88f};memcpy(c.angles,angles,sizeof(angles));c.projection.v[2][3]=1;c.projection=math::projection(c.projection,angles);if(!render_scene(context.Get(),rt.Get(),64,64,src.Get(),&c)||!getCenter(image.Get(),pixel)||pixel[2]!=255)return 20;}
        printf("Native production eye %d: clean world, private-stencil HUD alpha, BGRA -> RGBA and asymmetric lens PASS\n",eye);
        if(eye>0){if(!afw::fixture(context.Get(),sceneDepthSrv.Get()))return 23;wchar_t helper[MAX_PATH]{};GetModuleFileNameW(nullptr,helper,MAX_PATH);*wcsrchr(helper,L'\\')=0;wcscat_s(helper,L"\\..\\..\\build64\\Release\\aa_dlss64.exe");if(!dlss::fixture(context.Get(),image.Get(),sceneDepthSrv.Get(),helper))return 21;}
        native9::finish_reads();hud9::end_present();
    }
    request_depth_capture(false);hud9::reset();dev->SetTexture(0,nullptr);dev->SetDepthStencilSurface(nullptr);depth.Reset();sceneSurface.Reset();scene.Reset();bb.Reset();sceneDepthSrv.Reset();sceneDepth.Reset();hudSrv.Reset();hudRtv.Reset();hudImage.Reset();dlss::reset();release_frame();native9::reset();
    pp.BackBufferWidth=80;pp.BackBufferHeight=72;if(FAILED(dev->Reset(&pp))||!capture(dev.Get(),false)||frame->desc.BufferDesc.Width!=80||frame->desc.BufferDesc.Height!=72||nine||elevenOn12||twelve||queue)return 22;
    printf("Native assembled mod: resize/reset, no9On12/11On12/D3D12 device/queue; retail/headset not launched PASS\n");fflush(nullptr);
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI AA_BridgeSelftest(HWND window) {
    using namespace bvr;using namespace bvr::aa;
    using namespace bvr::aa::bridge;
    auto factory=reinterpret_cast<PFN_Direct3DCreate9On12>(GetProcAddress(load_system9(),"Direct3DCreate9On12"));
    if(!factory) return 1;
    D3D9ON12_ARGS args{};args.Enable9On12=TRUE;
    IDirect3D9* raw=factory(D3D_SDK_VERSION,&args,1);if(!raw)return 2;
    ComPtr<IDirect3D9> d;d.Attach(new Factory(raw,false));
    // Asylum creates two factories during startup. Keep both alive and create
    // a real device through each wrapper, rather than testing a raw factory.
    raw=factory(D3D_SDK_VERSION,&args,1);if(!raw)return 12;
    ComPtr<IDirect3D9> second;second.Attach(new Factory(raw,false));
    ComPtr<IUnknown> identity;ComPtr<IDirect3D9> identity9;
    if(FAILED(d.As(&identity)) || FAILED(identity.As(&identity9)) || identity9.Get()!=d.Get())return 13;
    D3DCAPS9 caps{};if(FAILED(second->GetDeviceCaps(0,D3DDEVTYPE_HAL,&caps)))return 14;
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.hDeviceWindow=window;pp.BackBufferWidth=64;pp.BackBufferHeight=64;
    pp.BackBufferFormat=D3DFMT_UNKNOWN;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    pp.PresentationInterval=D3DPRESENT_INTERVAL_ONE;prepare_present(&pp);
    ComPtr<IDirect3DDevice9> dev;
    if(FAILED(d->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,window,D3DCREATE_FPU_PRESERVE|D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&dev)))return 3;
    ComPtr<IDirect3DSwapChain9> actualSwap;D3DPRESENT_PARAMETERS actualPresent{};
    if(FAILED(dev->GetSwapChain(0,&actualSwap))||FAILED(actualSwap->GetPresentParameters(&actualPresent))||actualPresent.PresentationInterval!=D3DPRESENT_INTERVAL_IMMEDIATE)return 79;
    actualSwap.Reset();printf("D3D9 presentation: requested interval ONE normalized to IMMEDIATE on actual9On12 swapchain PASS\n");
    ComPtr<IDirect3DDevice9> secondDevice;
    auto secondParameters=pp;
    if(FAILED(second->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_FPU_PRESERVE|D3DCREATE_HARDWARE_VERTEXPROCESSING,&secondParameters,&secondDevice)))return 15;
    secondDevice.Reset();second.Reset();identity9.Reset();identity.Reset();
    ComPtr<IDirect3DTexture9> managed;
    if(FAILED(dev->CreateTexture(16,16,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&managed,nullptr)))return 4;
    for(unsigned i=0;i<8;++i) {
        ComPtr<IDirect3DSurface9> bb;dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb);
        const unsigned char r=51+i,g=128-i,b=204-i;
        for(int y=0;y<2;++y)for(int x=0;x<2;++x) {
            const RECT rect{x*32,y*32,(x+1)*32,(y+1)*32};
            if(FAILED(dev->ColorFill(bb.Get(),&rect,D3DCOLOR_ARGB(255,r+30*x,g+25*y,b+10*x+20*y))))return 5;
        }
        if(!capture(dev.Get(),false))return 5;
        D3D11_TEXTURE2D_DESC desc{};frame->image->GetDesc(&desc);
        desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> stage;
        if(FAILED(eleven->CreateTexture2D(&desc,nullptr,&stage)))return 6;
        context->CopyResource(stage.Get(),frame->image.Get());context->Flush();
        D3D11_MAPPED_SUBRESOURCE map{};if(FAILED(context->Map(stage.Get(),0,D3D11_MAP_READ,0,&map)))return 7;
        const bool bgra=desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM || desc.Format==DXGI_FORMAT_B8G8R8X8_UNORM;
        bool exact=true;
        for(int y:{3,32,60})for(int x:{3,32,60}) {
            const auto* p=static_cast<const unsigned char*>(map.pData)+y*map.RowPitch+x*4;
            const auto red=r+30*(x>=32),green=g+25*(y>=32),blue=b+10*(x>=32)+20*(y>=32);
            exact=exact && p[0]==(bgra?blue:red) && p[1]==green && p[2]==(bgra?red:blue);
        }
        context->Unmap(stage.Get(),0);if(!exact)return 8;
    }
    // Exercise thousands of production unwrap/acquire/copy/release/return
    // operations. Cache size and wrapper creation must stay bounded, and the
    // CPU memory trend must not depend on one wrapper allocation per frame.
    if(!drain())return 82;
    const auto createsBefore=wrappedCreates,reusesBefore=wrappedReuses;
    const auto memoryBefore=memory_budget();
    for(unsigned i=0;i<2048;++i)if(!capture(dev.Get(),false))return 83;
    if(!drain())return 84;
    const auto memoryAfter=memory_budget();
    const bool reused=wrappedCreates==createsBefore&&wrappedReuses-reusesBefore==2048;
    printf("D3D9 bridge reuse: 2048 real transfers, new wrappers %u, reused %u, private before/after %.1f/%.1f MiB, VA free %.1f/%.1f MiB %s\n",wrappedCreates-createsBefore,wrappedReuses-reusesBefore,memoryBefore.privateBytes/1048576.,memoryAfter.privateBytes/1048576.,memoryBefore.freeBytes/1048576.,memoryAfter.freeBytes/1048576.,reused?"PASS":"FAIL");
    if(!reused)return 85;
    for(unsigned i=0;i<2048;++i)if(!capture(dev.Get(),false))return 86;
    if(!drain())return 87;
    const auto memoryWarm=memory_budget();
    printf("D3D9 bridge second batch: 2048 transfers, additional wrappers %u, private %.1f MiB, VA free %.1f MiB (memory observation only)\n",wrappedCreates-createsBefore,memoryWarm.privateBytes/1048576.,memoryWarm.freeBytes/1048576.);
    if(wrappedCreates!=createsBefore)return 88;
    if(GetEnvironmentVariableW(L"AA_DLSS_SOAK",nullptr,0)){
        for(unsigned i=0;i<8192;++i){
            if(FAILED(dev->Present(nullptr,nullptr,nullptr,nullptr))||!capture(dev.Get(),false))return 89;
            if(i%1024==0){const auto m=memory_budget();printf("Present soak %u VA free %.1f MiB private %.1f MiB\n",i,m.freeBytes/1048576.,m.privateBytes/1048576.);}
        }
    }
    // Exercise the actual preview draw on this 11On12 device, including all eye
    // corners. The old centre-only solid-colour test could miss an inset/flip.
    D3D11_TEXTURE2D_DESC pd{};frame->image->GetDesc(&pd);
    ComPtr<ID3D11Texture2D> projected;ComPtr<ID3D11RenderTargetView> projectedRtv;
    ComPtr<ID3D11ShaderResourceView> sourceSrv;
    if(FAILED(eleven->CreateTexture2D(&pd,nullptr,&projected)) ||
       FAILED(eleven->CreateRenderTargetView(projected.Get(),nullptr,&projectedRtv)) ||
       FAILED(eleven->CreateShaderResourceView(frame->image.Get(),nullptr,&sourceSrv)))return 16;
    pd.BindFlags=0;pd.Usage=D3D11_USAGE_STAGING;pd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> projectedReadback;
    if(FAILED(eleven->CreateTexture2D(&pd,nullptr,&projectedReadback)))return 17;
    for(unsigned pose=0;pose<3;++pose) {
        XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
        constexpr float rad=3.141592654f/180.f;
        float q[4]={0,0,0,1};
        if(pose)bvr::xrmath::xr_local_trim_quat(21*rad,73*rad,-18*rad,q);
        for(int e=0;e<2;++e) {
            const float offset[3]={e?.032f:-.032f,0,0};float rotated[3];
            bvr::xrmath::quat_rotate(q[0],q[1],q[2],q[3],offset,rotated);
            views[e].pose.position={rotated[0]+1.3f,rotated[1]-1.1f,rotated[2]+2.2f};
            const float sign=pose==2 && e==1?-1.f:1.f;
            views[e].pose.orientation={sign*q[0],sign*q[1],sign*q[2],sign*q[3]};
            views[e].fov={-(e?35.5f:49.5f)*rad,(e?49.5f:35.5f)*rad,39.4f*rad,-50.5f*rad};
        }
        float world[4][3];
        if(!bvr::vr::preview::plane(views,1.f,true,world))return 18;
        // Even when placed from a pitched/rolled headset, vertical edges
        // stay parallel to world UP and horizontal edges stay level.
        for(int c=0;c<2;++c)if(std::fabs(world[c][0]-world[c+2][0])>.0001f ||
            std::fabs(world[c][2]-world[c+2][2])>.0001f)return 28;
        if(std::fabs(world[0][1]-world[1][1])>.0001f ||
            std::fabs(world[2][1]-world[3][1])>.0001f)return 29;
        if(std::fabs((world[0][1]+world[3][1])*.5f-(views[0].pose.position.y+views[1].pose.position.y)*.5f)>.0001f)return 30;
        for(int e=0;e<2;++e) {
            float corners[4][4];for(int c=0;c<4;++c)bvr::vr::preview::clip(views[e],world[c],corners[c]);
            if(!bvr::blit::screen_in_eye(context.Get(),projectedRtv.Get(),64,64,sourceSrv.Get(),corners))return 19;
            context->CopyResource(projectedReadback.Get(),projected.Get());context->Flush();
            D3D11_MAPPED_SUBRESOURCE map{};
            if(FAILED(context->Map(projectedReadback.Get(),0,D3D11_MAP_READ,0,&map)))return 20;
            bool covered=true;
            for(int y:{1,32,62})for(int x:{1,32,62}) {
                const auto* pixel=static_cast<const unsigned char*>(map.pData)+y*map.RowPitch+x*4;
                covered=covered && pixel[0]>0 && pixel[1]>0 && pixel[2]>0;
            }
            context->Unmap(projectedReadback.Get(),0);if(!covered)return 21;
        }
        if(!bvr::vr::preview::plane(views,.933f,false,world))return 22;
        for(int e=0;e<2;++e)for(int c=0;c<4;++c) {
            float clip[4];bvr::vr::preview::clip(views[e],world[c],clip);
            if(clip[3]<=0 || std::fabs(clip[0])>clip[3] || std::fabs(clip[1])>clip[3])return 23;
        }
        if(pose==0) {
            bvr::vr::preview::RoomPlane room;
            if(!room.place(views,64,64,false))return 24;
            float anchored[4][3];memcpy(anchored,room.world,sizeof(anchored));
            float before[4];bvr::vr::preview::clip(views[0],anchored[0],before);
            // Move/turn the viewer after placing. The world corners stay
            // identical while the image's position in the lens changes.
            XrView moved[2]={views[0],views[1]};float turned[4];
            bvr::xrmath::xr_local_trim_quat(0,30*rad,0,turned);
            for(auto& v:moved){v.pose.orientation={turned[0],turned[1],turned[2],turned[3]};v.pose.position.x+=.4f;}
            if(!room.place(moved,64,64,false)||memcmp(anchored,room.world,sizeof(anchored)))return 25;
            float after[4];bvr::vr::preview::clip(moved[0],room.world[0],after);
            if(std::fabs(before[0]/before[3]-after[0]/after[3])<.1f)return 26;
            room.reset();if(!room.place(moved,64,64,false)||!memcmp(anchored,room.world,sizeof(anchored)))return 27;
        }
    }
    // Exercise the production legacy-lens remap against independent ray/UV
    // expectations and a patterned texture (not just a solid centre pixel).
    for(int e=0;e<2;++e){native::Camera c{};c.centred=true;c.inputWidth=c.inputHeight=64;
        constexpr float rad=3.141592654f/180.f;const float angles[]={-(e?35.5f:49.5f)*rad,(e?49.5f:35.5f)*rad,39.4f*rad,-50.5f*rad};memcpy(c.angles,angles,sizeof(angles));c.projection.v[2][3]=1;c.projection=math::projection(c.projection,angles);
        if(!render_scene(context.Get(),projectedRtv.Get(),64,64,sourceSrv.Get(),&c))return 62;
        context->CopyResource(projectedReadback.Get(),projected.Get());context->Flush();D3D11_MAPPED_SUBRESOURCE m{};
        if(FAILED(context->Map(projectedReadback.Get(),0,D3D11_MAP_READ,0,&m)))return 63;
        bool correct=true;const float l=std::tan(angles[0]),r=std::tan(angles[1]),u=std::tan(angles[2]),down=std::tan(angles[3]);
        for(int y:{1,20,62})for(int x:{1,20,62}){
            const float su=.5f+(l+(x+.5f)/64*(r-l))/(2*std::fmax(-l,r));const float sv=.5f-(u+(y+.5f)/64*(down-u))/(2*std::fmax(u,-down));
            if(std::fabs(su-.5f)<.03f||std::fabs(sv-.5f)<.03f)continue;
            const auto*p=static_cast<const BYTE*>(m.pData)+y*m.RowPitch+x*4;
            correct=correct&&std::abs(int(p[0])-(58+30*(su>=.5f)))<=1&&std::abs(int(p[1])-(121+25*(sv>=.5f)))<=1&&std::abs(int(p[2])-(197+10*(su>=.5f)+20*(sv>=.5f)))<=1;
        }
        context->Unmap(projectedReadback.Get(),0);if(!correct)return 64;
    }
    // Real zero-copy lease: compare every final RGBA pixel against the old
    // copy+conversion path in both lenses, then rewrite the D3D9 backbuffer
    // and repeat to prove fresh content and safe fenced return.
    unsigned directChecks{};std::vector<unsigned char> firstBorrowed;
    auto pixels=[&](std::vector<unsigned char>& bytes){context->CopyResource(projectedReadback.Get(),projected.Get());D3D11_MAPPED_SUBRESOURCE m{};
        if(FAILED(context->Map(projectedReadback.Get(),0,D3D11_MAP_READ,0,&m)))return false;
        bytes.resize(64*64*4);for(unsigned y=0;y<64;++y)memcpy(bytes.data()+y*64*4,static_cast<const BYTE*>(m.pData)+y*m.RowPitch,64*4);context->Unmap(projectedReadback.Get(),0);return true;};
    for(unsigned rewrite=0;rewrite<2;++rewrite){
        if(rewrite){ComPtr<IDirect3DSurface9> bb;dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb);if(FAILED(dev->ColorFill(bb.Get(),nullptr,D3DCOLOR_ARGB(255,188,87,39))))return 96;}
        if(!capture(dev.Get(),false))return 97;
        ComPtr<ID3D11ShaderResourceView> stock;if(FAILED(eleven->CreateShaderResourceView(frame->image.Get(),nullptr,&stock)))return 98;
        for(int eye=0;eye<2;++eye){native::Camera c{};c.centred=true;c.inputWidth=c.inputHeight=64;
            constexpr float rad=3.141592654f/180;const float angles[]={-(eye?35.5f:49.5f)*rad,(eye?49.5f:35.5f)*rad,39.4f*rad,-50.5f*rad};memcpy(c.angles,angles,sizeof(angles));c.projection.v[2][3]=1;c.projection=math::projection(c.projection,angles);
            std::vector<unsigned char> reference,actual;
            if(!render_scene(context.Get(),projectedRtv.Get(),64,64,stock.Get(),&c)||!pixels(reference))return 99;
            offlineBorrowConsumer=[&](Frame* image){ComPtr<ID3D11ShaderResourceView> srv;
                return SUCCEEDED(eleven->CreateShaderResourceView(image->image.Get(),nullptr,&srv))&&render_scene(context.Get(),projectedRtv.Get(),64,64,srv.Get(),&c)&&pixels(actual);};
            const auto directBefore=borrowedFrames;const bool direct=capture(dev.Get(),false);offlineBorrowConsumer={};
            if(!direct||borrowedFrames!=directBefore+1||actual!=reference)return 100;
            if(firstBorrowed.empty())firstBorrowed=actual;else if(rewrite&&actual==firstBorrowed)return 101;
            ++directChecks;
        }
    }
    // Restore the patterned image expected by the remaining regression cases.
    {ComPtr<IDirect3DSurface9> bb;dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb);
     for(int y=0;y<2;++y)for(int x=0;x<2;++x){const RECT rect{x*32,y*32,(x+1)*32,(y+1)*32};dev->ColorFill(bb.Get(),&rect,D3DCOLOR_ARGB(255,58+30*x,121+25*y,197+10*x+20*y));}}
    if(!capture(dev.Get(),false))return 102;
    printf("Direct colour lease GPU: %u BOTH-eye/full-image RGBA equality checks vs stock conversion; rewritten next D3D9 source stays fresh; acquire/release/fenced return PASS\n",directChecks);
    // Exercise the production D3D9 draw hooks, not a model of extraction:
    // scene blit, stencil-only mask, premultiplied widget, private filter target,
    // same target restored and right-eye UI skipped. Transport the actual sheet.
    if(MH_Initialize()!=MH_OK||!hud9::install(dev.Get()))return 31;
    wchar_t particleShader[MAX_PATH]{};GetModuleFileNameW(nullptr,particleShader,MAX_PATH);*wcsrchr(particleShader,L'\\')=0;wcscat_s(particleShader,L"\\..\\..\\re\\evidence\\particle-vs-original.bin");
    if(!billboard9::fixture(dev.Get(),particleShader))return 119;
    *wcsrchr(particleShader,L'\\')=0;if(!transitions9::fixture(dev.Get(),particleShader,fixture_bat_transport))return 38;if(!hud9::fixture_alpha(dev.Get(),particleShader))return 39;if(!fixture_menu_fade())return 41;if(!markers9::fixture(dev.Get(),particleShader,fixture_marker_transport))return 42;if(!fixture_connector_gpu())return 43;
    hud9::reset();
    if(!install_allocation_trace()||!install_pipeline_trace())return 90;
    if(!pipeline_cache::fixture(twelve.Get(),queue.Get(),graphicsPSOOriginal))return 93;
    const bool dynamicReady=dynamic_pipeline::install(twelve.Get(),graphicsPSOOriginal);printf("Dynamic bias driver capability %s\n",dynamicReady?"SUPPORTED":"UNSUPPORTED");
    if(!dynamic_pipeline::fixture(twelve.Get(),queue.Get(),graphicsPSOOriginal))return 94;
    auto* reserved=VirtualAlloc(nullptr,2*1024*1024,MEM_RESERVE,PAGE_NOACCESS);
    bool traced=false;AcquireSRWLockExclusive(&reservationLock);for(unsigned i=reservationRead;i<reservationWrite;++i)if(reservations[i%128].base==reserved&&reservations[i%128].bytes==2*1024*1024&&reservations[i%128].depth)traced=true;ReleaseSRWLockExclusive(&reservationLock);
    const bool freed=reserved&&VirtualFree(reserved,0,MEM_RELEASE);
    printf("Allocation trace: actual 2 MiB reservation with stack, then released %s\n",traced&&freed?"PASS":"FAIL");
    if(!traced||!freed)return 91;
    ComPtr<IDirect3DTexture9> sceneTex,privateTex;ComPtr<IDirect3DSurface9> sceneSurface,privateSurface,bb;
    if(FAILED(dev->CreateTexture(64,64,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&sceneTex,nullptr))||
       FAILED(sceneTex->GetSurfaceLevel(0,&sceneSurface))||FAILED(dev->CreateTexture(16,16,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&privateTex,nullptr))||
       FAILED(privateTex->GetSurfaceLevel(0,&privateSurface))||FAILED(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb)))return 32;
    dev->ColorFill(sceneSurface.Get(),nullptr,D3DCOLOR_ARGB(255,0,0,255));
    struct Quad{float x,y,z,rhw,u,v;};const Quad full[]={{-.5f,-.5f,0,1,0,0},{63.5f,-.5f,0,1,1,0},{-.5f,63.5f,0,1,0,1},{63.5f,63.5f,0,1,1,1}};
    struct Widget{float x,y,z,rhw;DWORD colour;};const Widget widget[]={{15.5f,15.5f,0,1,0x80ff0000},{47.5f,15.5f,0,1,0x80ff0000},{15.5f,47.5f,0,1,0x80ff0000},{47.5f,47.5f,0,1,0x80ff0000}};
    // With no depth consumer, real scene writes/clear/rebind must make zero
    // cross-API depth transfers. Explicit still requests re-enable the exact
    // preservation path tested for both eyes immediately below.
    dlss::set_mode(0,false);afw::set_on(false);request_depth_capture(false);
    hud9::begin_eye(-1,122);dev->SetRenderTarget(0,sceneSurface.Get());
    ComPtr<IDirect3DSurface9> unusedDepth;
    if(FAILED(dev->CreateDepthStencilSurface(64,64,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&unusedDepth,nullptr)))return 82;
    dev->SetDepthStencilSurface(unusedDepth.Get());dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
    dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE,0);
    const auto depthCopiesBefore=depthTime.count;
    if(FAILED(dev->BeginScene())||FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,full,sizeof(Quad)))||FAILED(dev->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,1,0))||FAILED(dev->EndScene()))return 83;
    dev->SetDepthStencilSurface(nullptr);hud9::end_present();
    if(depthFresh||depthTime.count!=depthCopiesBefore||depth_required())return 84;
    dlss::set_mode(3,false);if(!depth_required())return 85;dlss::set_mode(0,false);
    afw::set_on(true);if(!depth_required())return 86;afw::set_on(false);
    printf("Native stock GPU: scene geometry/DS clear/rebind with ZERO unused depth copies; DLSS/AFW/still depth consumers preserved PASS\n");
    unusedDepth.Reset();
    hud9::reset(); // Give the timestamp regression its own complete sampled pair.
    for(int sign:{-1,1}){
        request_depth_capture(true);
        hud9::begin_eye(sign,123);dev->SetRenderTarget(0,bb.Get());
        if(FAILED(dev->BeginScene()))return 33;
        ComPtr<IDirect3DSurface9> fixtureDepth;
        if(FAILED(dev->CreateDepthStencilSurface(64,64,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&fixtureDepth,nullptr)))return 57;
        dev->SetDepthStencilSurface(fixtureDepth.Get());dev->SetRenderTarget(0,sceneSurface.Get());
        dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE,0);dev->SetRenderState(D3DRS_ZENABLE,TRUE);dev->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);dev->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
        dev->Clear(0,nullptr,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,sign<0?.25f:.75f,0);
        // Track the real scene-depth candidate through the production draw hook.
        Quad geometry[4];memcpy(geometry,full,sizeof(full));for(auto& v:geometry)v.z=sign<0?.25f:.75f;
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,geometry,sizeof(Quad))))return 58;
        if(!hud9::fixture_tracking(dev.Get(),D3DPT_TRIANGLESTRIP,geometry,sizeof(Quad),privateSurface.Get()))return 80;
        // Reproduce the retail hazard: the same hardware DS is cleared for
        // later passes BEFORE the final scene/UI blit. The transported scene
        // depth must still contain geometry (.25 LEFT / .75 RIGHT), not1.
        if(FAILED(dev->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,1.f,0)))return 75;
        dev->SetDepthStencilSurface(nullptr);
        dev->SetRenderTarget(0,bb.Get());
        dev->SetVertexShader(nullptr);dev->SetPixelShader(nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
        dev->SetRenderState(D3DRS_ZENABLE,FALSE);dev->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);
        dev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);dev->SetTexture(0,sceneTex.Get());
        dev->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
        dev->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,full,sizeof(Quad))))return 34;
        if(!depthFresh||depthEye!=sign||depthGeneration!=123)return 59;
        {D3D11_TEXTURE2D_DESC rd{};sceneDepth->GetDesc(&rd);rd.BindFlags=rd.MiscFlags=0;rd.Usage=D3D11_USAGE_STAGING;rd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
         ComPtr<ID3D11Texture2D> rb;if(FAILED(eleven->CreateTexture2D(&rd,nullptr,&rb)))return 76;
         context->CopyResource(rb.Get(),sceneDepth.Get());D3D11_MAPPED_SUBRESOURCE m{};if(FAILED(context->Map(rb.Get(),0,D3D11_MAP_READ,0,&m)))return 77;
         uint32_t value{};memcpy(&value,static_cast<const BYTE*>(m.pData)+32*m.RowPitch+32*4,4);context->Unmap(rb.Get(),0);
         const float actual=(value&0xffffff)/16777215.f;if(std::fabs(actual-(sign<0?.25f:.75f))>.00001f)return 78;
         printf("Scene-depth preservation: eye %d geometry %.5f survived clear1/rebind before final blit PASS\n",sign,actual);}
        dev->SetTexture(0,nullptr);dev->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
        dev->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);dev->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
        dev->SetRenderState(D3DRS_STENCILENABLE,TRUE);dev->SetRenderState(D3DRS_STENCILREF,1);dev->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_ALWAYS);
        dev->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_REPLACE);dev->SetRenderState(D3DRS_COLORWRITEENABLE,0);
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,widget,sizeof(Widget))))return 35;
        dev->SetRenderState(D3DRS_COLORWRITEENABLE,15);dev->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_EQUAL);dev->SetRenderState(D3DRS_STENCILPASS,D3DSTENCILOP_KEEP);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);dev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);dev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,widget,sizeof(Widget))))return 36;
        ComPtr<IDirect3DSurface9> restored;dev->GetRenderTarget(0,&restored);if(restored.Get()!=bb.Get())return 37;
        dev->SetRenderTarget(0,privateSurface.Get());dev->SetRenderState(D3DRS_STENCILENABLE,FALSE);
        const Widget privateWidget[]={{1.5f,1.5f,0,1,0x80ff0000},{13.5f,1.5f,0,1,0x80ff0000},{1.5f,13.5f,0,1,0x80ff0000},{13.5f,13.5f,0,1,0x80ff0000}};
        dev->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_ARGB(255,0,0,255),1,0);
        if(FAILED(dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,privateWidget,sizeof(Widget))))return 38;
        dev->SetRenderTarget(0,bb.Get());if(FAILED(dev->EndScene()))return 39;hud9::end_render();
        if(sign<0){
            ComPtr<IDirect3DSurface9> privateReadback;
            if(FAILED(dev->CreateOffscreenPlainSurface(16,16,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&privateReadback,nullptr))||FAILED(dev->GetRenderTargetData(privateSurface.Get(),privateReadback.Get())))return 50;
            D3DLOCKED_RECT locked{};if(FAILED(privateReadback->LockRect(&locked,nullptr,D3DLOCK_READONLY)))return 51;
            const auto*pixel=static_cast<const BYTE*>(locked.pBits)+8*locked.Pitch+8*4;
            const bool filterKept=pixel[0]>120&&pixel[1]==0&&pixel[2]>120;
            privateReadback->UnlockRect();if(!filterKept)return 52;
            if(!hud9::sheet()||!publish_hud())return 40;
        }
        else if(hud9::sheet())return 41;
        D3D11_TEXTURE2D_DESC depthRead{};sceneDepth->GetDesc(&depthRead);depthRead.BindFlags=0;depthRead.Usage=D3D11_USAGE_STAGING;depthRead.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> depthReadback;
        if(FAILED(eleven->CreateTexture2D(&depthRead,nullptr,&depthReadback)))return 65;
        context->CopyResource(depthReadback.Get(),sceneDepth.Get());context->Flush();D3D11_MAPPED_SUBRESOURCE depthMap{};
        if(FAILED(context->Map(depthReadback.Get(),0,D3D11_MAP_READ,0,&depthMap)))return 66;
        uint32_t packed{};memcpy(&packed,static_cast<const BYTE*>(depthMap.pData)+32*depthMap.RowPitch+32*4,4);
        const float actualDepth=float(packed&0xffffff)/16777215.f;context->Unmap(depthReadback.Get(),0);
        if(std::fabs(actualDepth-(sign<0?.25f:.75f))>.00001f)return 67;
        if(!capture(dev.Get(),false))return 42;
        context->CopyResource(projectedReadback.Get(),frame->image.Get());context->Flush();D3D11_MAPPED_SUBRESOURCE map{};
        if(FAILED(context->Map(projectedReadback.Get(),0,D3D11_MAP_READ,0,&map)))return 43;
        const auto*p=static_cast<const BYTE*>(map.pData)+32*map.RowPitch+32*4;const bool clean=p[0]==0&&p[1]==0&&p[2]==255;
        context->Unmap(projectedReadback.Get(),0);if(!clean)return 44;
        hud9::end_present();
    }
    if(!hud9::fixture_budget())return 81;
    request_depth_capture(false);
    // The post-capture desktop mirror must also retain the HUD and restore
    // the game's stencil/blend state rather than contaminating the next eye.
    DWORD beforeStencil{},afterStencil{},beforeSrc{},afterSrc{};
    dev->GetRenderState(D3DRS_STENCILENABLE,&beforeStencil);dev->GetRenderState(D3DRS_SRCBLEND,&beforeSrc);
    hud9::mirror();dev->GetRenderState(D3DRS_STENCILENABLE,&afterStencil);dev->GetRenderState(D3DRS_SRCBLEND,&afterSrc);
    if(beforeStencil!=afterStencil||beforeSrc!=afterSrc)return 53;
    if(!capture(dev.Get(),false))return 54;
    context->CopyResource(projectedReadback.Get(),frame->image.Get());context->Flush();D3D11_MAPPED_SUBRESOURCE mirrorMap{};
    if(FAILED(context->Map(projectedReadback.Get(),0,D3D11_MAP_READ,0,&mirrorMap)))return 55;
    const auto*mirrorPixel=static_cast<const BYTE*>(mirrorMap.pData)+32*mirrorMap.RowPitch+32*4;
    const bool desktopHud=mirrorPixel[0]>120&&mirrorPixel[1]==0&&mirrorPixel[2]>120;
    context->Unmap(projectedReadback.Get(),0);if(!desktopHud)return 56;
    context->CopyResource(projectedReadback.Get(),hudImage.Get());context->Flush();D3D11_MAPPED_SUBRESOURCE hm{};
    if(FAILED(context->Map(projectedReadback.Get(),0,D3D11_MAP_READ,0,&hm)))return 45;
    const auto*inside=static_cast<const BYTE*>(hm.pData)+32*hm.RowPitch+32*4;const auto*outside=static_cast<const BYTE*>(hm.pData)+3*hm.RowPitch+3*4;
    const bool isolated=std::abs(int(inside[0])-128)<=1&&inside[1]==0&&inside[2]==0&&std::abs(int(inside[3])-128)<=1&&outside[0]==0&&outside[1]==0&&outside[2]==0&&outside[3]==0;
    context->Unmap(projectedReadback.Get(),0);if(!isolated)return 46;
    // The production overlay shader must preserve the scene outside the HUD,
    // including additive colour at zero alpha, under both asymmetric lenses.
    XrView fixtureViews[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
    const float worldHud[4][3]={{-2,2,-4},{2,2,-4},{-2,-2,-4},{2,-2,-4}};
    for(int e=0;e<2;++e){auto&v=fixtureViews[e];v.pose.orientation.w=1;v.pose.position.x=e?.032f:-.032f;
        constexpr float rad=3.141592654f/180;v.fov={-(e?35.5f:49.5f)*rad,(e?49.5f:35.5f)*rad,39.4f*rad,-50.5f*rad};
        const float green[4]={0,1,0,1};context->ClearRenderTargetView(projectedRtv.Get(),green);
        float clip[4][4];for(int c=0;c<4;++c)vr::preview::clip(v,worldHud[c],clip[c]);
        if(!blit::premul_in_eye(context.Get(),projectedRtv.Get(),64,64,hudSrv.Get(),clip))return 47;
        float origin[4];const float centre[3]={0,0,-4};vr::preview::clip(v,centre,origin);
        const int x=int((origin[0]/origin[3]+1)*32),y=int((1-origin[1]/origin[3])*32);
        context->CopyResource(projectedReadback.Get(),projected.Get());context->Flush();
        if(FAILED(context->Map(projectedReadback.Get(),0,D3D11_MAP_READ,0,&hm)))return 48;
        const auto*mid=static_cast<const BYTE*>(hm.pData)+y*hm.RowPitch+x*4;const auto*edge=static_cast<const BYTE*>(hm.pData)+hm.RowPitch+4;
        const bool blended=mid[0]>120&&mid[1]>120&&mid[2]==0&&edge[0]==0&&edge[1]==255&&edge[2]==0;
        context->Unmap(projectedReadback.Get(),0);if(!blended)return 49;
    }
    // Feed the transported D24S8 surface to the real x64 NGX helper (DLAA).
    // No game/VR launch: this is the owned offline device's actual GPU path.
    wchar_t helper[MAX_PATH]{};HMODULE self{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&AA_BridgeSelftest),&self);
    GetModuleFileNameW(self,helper,MAX_PATH);auto* slash=wcsrchr(helper,L'\\');if(!slash)return 60;
    wcscpy_s(slash+1,MAX_PATH-(slash+1-helper),L"..\\..\\build64\\Release\\aa_dlss64.exe");
    if(!dlss::fixture(context.Get(),frame->image.Get(),sceneDepthSrv.Get(),helper))return 61;
    if(!afw::fixture(context.Get(),sceneDepthSrv.Get()))return 72;
    if(!stereo_history::fixture(context.Get()))return 73;
    if(!detail9::fixture(dev.Get()))return 74;
    hud9::reset();dev->SetTexture(0,nullptr);dev->SetDepthStencilSurface(nullptr);sceneDepthSrv.Reset();sceneDepth.Reset();privateSurface.Reset();privateTex.Reset();sceneSurface.Reset();sceneTex.Reset();bb.Reset();
    hudRtv.Reset();hudSrv.Reset();hudSourceSrv.Reset();hudImage.Reset();hudSource.Reset();hudFresh=hudPlaced=false;markers9::invalidate_hud_plane();
    projectedReadback.Reset();sourceSrv.Reset();projectedRtv.Reset();projected.Reset();
    managed.Reset();
    if(!drain())return 9;
    release_frame();pp.BackBufferWidth=80;pp.BackBufferHeight=72;
    if(FAILED(dev->Reset(&pp)) || !capture(dev.Get(),false) || frame->desc.BufferDesc.Width!=80 || frame->desc.BufferDesc.Height!=72)return 10;
    if(!drain())return 11;
    if(!window::fixture())return 68;
    release_frame();pp.BackBufferWidth=2284;pp.BackBufferHeight=2448;
    if(FAILED(dev->Reset(&pp))||!capture(dev.Get(),false)||frame->desc.BufferDesc.Width!=2284||frame->desc.BufferDesc.Height!=2448)return 69;
    RECT desktop{};GetClientRect(window,&desktop);if(desktop.right>=2284||desktop.bottom>=2448)return 70;
    if(!drain())return 71;
    // Production-size bridge comparison, without a game or headset. Completion
    // timing covers transfer+lens draws only; it is not a retail scene FPS test.
    release_frame();pp.BackBufferWidth=2836;pp.BackBufferHeight=3040;
    if(FAILED(dev->Reset(&pp)))return 103;
    {ComPtr<IDirect3DSurface9> source;dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&source);
     for(int y=0;y<2;++y)for(int x=0;x<2;++x){const RECT rect{x*1418,y*1520,(x+1)*1418,(y+1)*1520};if(FAILED(dev->ColorFill(source.Get(),&rect,D3DCOLOR_ARGB(255,42+53*x,71+67*y,109+37*x+31*y))))return 104;}}
    if(!capture(dev.Get(),false))return 105;
    {
        D3D11_TEXTURE2D_DESC d{};d.Width=2284;d.Height=2448;d.ArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        ComPtr<ID3D11Texture2D> target,read;ComPtr<ID3D11RenderTargetView> rt;
        if(FAILED(eleven->CreateTexture2D(&d,nullptr,&target))||FAILED(eleven->CreateRenderTargetView(target.Get(),nullptr,&rt)))return 106;
        d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(eleven->CreateTexture2D(&d,nullptr,&read)))return 107;
        native::Camera lenses[2]{};for(int e=0;e<2;++e){auto& c=lenses[e];c.centred=true;c.inputWidth=2284;c.inputHeight=2448;constexpr float rad=3.141592654f/180;const float angles[]={-(e?35.5f:49.5f)*rad,(e?49.5f:35.5f)*rad,39.4f*rad,-50.5f*rad};memcpy(c.angles,angles,sizeof(angles));c.projection.v[2][3]=1;c.projection=math::projection(c.projection,angles);}
        auto render=[&](Frame* input,int e){ComPtr<ID3D11ShaderResourceView> view;
            for(auto& entry:wrappedCache)if(entry.texture.Get()==input->image.Get()&&entry.view){view=entry.view;break;}
            return (view||SUCCEEDED(eleven->CreateShaderResourceView(input->image.Get(),nullptr,&view)))&&render_scene(context.Get(),rt.Get(),2284,2448,view.Get(),&lenses[e]);};
        auto readPixels=[&](std::vector<unsigned char>& bytes){context->CopyResource(read.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE m{};
            if(FAILED(context->Map(read.Get(),0,D3D11_MAP_READ,0,&m)))return false;bytes.resize(2284*2448*4);
            for(unsigned y=0;y<2448;++y)memcpy(bytes.data()+y*2284*4,static_cast<const BYTE*>(m.pData)+y*m.RowPitch,2284*4);context->Unmap(read.Get(),0);return true;};
        for(int e=0;e<2;++e){std::vector<unsigned char> expected,actual;if(!capture(dev.Get(),false)||!render(frame,e)||!readPixels(expected))return 108;
            offlineBorrowConsumer=[&](Frame* image){return render(image,e)&&readPixels(actual);};const auto before=borrowedFrames;
            const bool ok=capture(dev.Get(),false);offlineBorrowConsumer={};if(!ok||borrowedFrames!=before+1||actual!=expected||borrowedFrame->image)return 109;
        }
        printf("Direct colour full-size GPU: source2836x3040 -> BOTH2284x2448 eyes, all finalRGBA pixels match copied reference; returned facade hides source PASS\n");
        double gpuPair[2]{},completedPair[2]{};
        for(unsigned route=0;route<2;++route){
            auto pair=[&]{for(int e=0;e<2;++e){bool ok;
                if(route){offlineBorrowConsumer=[&](Frame* image){return render(image,e);};ok=capture(dev.Get(),false);offlineBorrowConsumer={};}
                else ok=capture(dev.Get(),false)&&render(frame,e);
                if(!ok)return false;}return true;};
            for(unsigned warm=0;warm<8;++warm)if(!pair())return 110;
            context->Flush();if(!drain())return 111;
            ComPtr<ID3D11Query> start,end,disjoint;D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP,0};
            if(FAILED(eleven->CreateQuery(&q,&start))||FAILED(eleven->CreateQuery(&q,&end)))return 112;
            q.Query=D3D11_QUERY_TIMESTAMP_DISJOINT;if(FAILED(eleven->CreateQuery(&q,&disjoint)))return 113;
            context->Begin(disjoint.Get());context->End(start.Get());const double at=cpu_now();
            for(unsigned pairIndex=0;pairIndex<32;++pairIndex)if(!pair())return 114;
            context->End(end.Get());context->End(disjoint.Get());context->Flush();if(!drain())return 115;
            completedPair[route]=(cpu_now()-at)/32;
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT clock{};UINT64 begin{},finish{};
            const auto until=GetTickCount64()+1000;HRESULT result;
            while((result=context->GetData(disjoint.Get(),&clock,sizeof(clock),D3D11_ASYNC_GETDATA_DONOTFLUSH))==S_FALSE&&GetTickCount64()<until)SwitchToThread();
            if(result!=S_OK||clock.Disjoint||!clock.Frequency||context->GetData(start.Get(),&begin,sizeof(begin),0)!=S_OK||context->GetData(end.Get(),&finish,sizeof(finish),0)!=S_OK||finish<begin)return 116;
            gpuPair[route]=1000.*double(finish-begin)/clock.Frequency/32;
        }
        printf("Colour bridge isolated completed32pairs/source2836x3040/eyes2284x2448: stock CPUcompletion/GPUelapsed %.3f/%.3f ms perpair; direct %.3f/%.3f. Includes transfer+lens only, no retailscene/VRpacing/headset.\n",completedPair[0],gpuPair[0],completedPair[1],gpuPair[1]);
    }
    if(!drain())return 117;release_frame();pp.BackBufferWidth=80;pp.BackBufferHeight=72;
    if(FAILED(dev->Reset(&pp))||!capture(dev.Get(),false))return 118;
    printf("Pipeline trace: graphics %u stream %u failures %u, actual colour/depth/DLSS checks passed\n",graphicsPSOCount.load(),streamPSOCount.load(),psoFailures.load());
    if(!graphicsPSOCount&&!streamPSOCount)return 92;
    release_frame();elevenOn12.Reset();context.Reset();eleven.Reset();fence.Reset();queue.Reset();twelve.Reset();nine.Reset();
    return 0;
}
