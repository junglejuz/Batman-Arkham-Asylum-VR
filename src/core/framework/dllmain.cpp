#include "framework.h"

#include "core/util/log.h"

#include <windows.h>
#include <atomic>
#include <string>
namespace aa { std::string HashFile(const std::wstring& path); }
namespace { std::atomic<DWORD> identity{0}; }
extern "C" __declspec(dllexport) DWORD WINAPI AA_Identity() { return identity.load(); }

namespace {

DWORD WINAPI InitThread(LPVOID) {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr,path,MAX_PATH);
    auto* name=wcsrchr(path,L'\\');
    // Steam GOTY, or the same build after the Joker DLC Patcher: it changes two bytes in the DLC flag routine at
    // 0x16153C0, which nothing of ours hooks (Godoy301 JOKERCOMPAT, 2026-10-20). Any other exe stays flat.
    const std::string hash=_wcsicmp(name ? name+1 : path,L"ShippingPC-BmGame.exe")==0 ? aa::HashFile(path) : std::string();
    const bool joker=hash=="ed65ce3c52baca7adaea00cb0e5def3d8a78bd05f7ceacd68a68462e92d8dab2";
    if(!joker && hash!="4dac1f5e2ac6710b7378fdce74601f616f4753e3756cb5fda63c7519cc2eb028") {
        OutputDebugStringA(("[Batman Asylum VR] unsupported game exe (sha256 "+(hash.empty()?std::string("-"):hash)+"); VR not started\n").c_str());
        identity.store(2); return 0;
    }
    identity.store(1);
    bvr::framework::init();
    BVR_LOG("[aa identity] game exe: %s",joker ? "Steam GOTY + Joker DLC Patcher" : "Steam GOTY");
    return 0;
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        // All real work happens off the loader lock.
        HANDLE thread = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        // Breadcrumb that separates an ORDERLY shutdown from a hard kill. A log
        // that just stops mid-heartbeat with no line at all (session 23's
        // external report) means neither ran - i.e. TerminateProcess or a
        // fail-fast, not an exception our filter could ever have seen.
        BVR_LOG("shutdown: DLL_PROCESS_DETACH (orderly process exit)");
    }
    return TRUE;
}
