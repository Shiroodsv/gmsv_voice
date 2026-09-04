#include "hook.h"
#include "iclient.h"
#include "platform.h"
#include "capture.h"
#include <subhook.h>
#include <cstdint>

namespace hook {

using BroadcastFn = void (*)(IClient*, int, char*, int64_t);

static subhook_t g_hook = nullptr;
static BroadcastFn g_original = nullptr;
static std::string g_signature;

static void Detour(IClient* client, int bytes, char* data, int64_t xuid) {
    if (client && data && bytes > 0) {
        capture::OnVoice(client->GetPlayerSlot() + 1, client->GetUserID(), reinterpret_cast<const uint8_t*>(data), static_cast<size_t>(bytes));
    }
    g_original(client, bytes, data, xuid);
}

void SetSignature(const std::string& pattern) {
    g_signature = pattern;
}

static void* Locate(std::string& error) {
#ifdef _WIN32
    platform::Module engine = platform::FindModule({"engine.dll"});
    if (!engine.valid()) { error = "engine.dll not found"; return nullptr; }
    if (g_signature.empty()) { error = "no signature set, call voice.SetSignature first"; return nullptr; }
    void* target = platform::FindPattern(engine, g_signature);
    if (!target) error = "signature not found in engine.dll";
    return target;
#else
    platform::Module engine = platform::FindModule({"engine_srv.so", "engine.so"});
    if (!engine.valid()) { error = "engine library not found"; return nullptr; }
    void* target = platform::FindSymbol(engine, "_Z20SV_BroadcastVoiceDataP7IClientiPcx");
    if (!target && !g_signature.empty()) target = platform::FindPattern(engine, g_signature);
    if (!target) error = "SV_BroadcastVoiceData not found";
    return target;
#endif
}

static void Drop() {
    subhook_remove(g_hook);
    subhook_free(g_hook);
    g_hook = nullptr;
    g_original = nullptr;
}

Status Install() {
    Status s;
    if (g_hook) { s.installed = true; return s; }
    void* target = Locate(s.error);
    if (!target) return s;
#if defined(__x86_64__) || defined(_M_X64)
    subhook_flags_t flags = SUBHOOK_64BIT_OFFSET;
#else
    subhook_flags_t flags = static_cast<subhook_flags_t>(0);
#endif
    g_hook = subhook_new(target, reinterpret_cast<void*>(&Detour), flags);
    if (!g_hook) { s.error = "failed to create hook"; return s; }
    if (subhook_install(g_hook) != 0) {
        subhook_free(g_hook);
        g_hook = nullptr;
        s.error = "failed to install hook";
        return s;
    }
    g_original = reinterpret_cast<BroadcastFn>(subhook_get_trampoline(g_hook));
    if (!g_original) {
        Drop();
        s.error = "failed to build trampoline";
        return s;
    }
    s.installed = true;
    return s;
}

void Remove() {
    if (g_hook) Drop();
}

bool IsInstalled() {
    return g_hook != nullptr;
}

}
