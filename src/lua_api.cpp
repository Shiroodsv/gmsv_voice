#include "lua_api.h"
#include "capture.h"
#include "hook.h"
#include "decoder.h"
#include "wav.h"
#include <GarrysMod/Lua/Interface.h>
#include <string>

using namespace GarrysMod::Lua;

namespace lua_api {

static const char* kVersion = "1.0.0";

static int EntityIndex(ILuaBase* lua, int stackPos) {
    lua->CheckType(stackPos, Type::Entity);
    lua->Push(stackPos);
    lua->GetField(-1, "EntIndex");
    lua->Push(-2);
    lua->Call(1, 1);
    int index = static_cast<int>(lua->GetNumber(-1));
    lua->Pop(2);
    return index;
}

static int ClampInt(double v, int lo, int hi) {
    if (!(v >= lo)) return lo;
    if (v > hi) return hi;
    return static_cast<int>(v);
}

static bool SafeDataPath(const std::string& name) {
    if (name.empty() || name.size() > 200) return false;
    if (name.find("..") != std::string::npos) return false;
    if (name[0] == '/' || name[0] == '\\') return false;
    if (name.find(':') != std::string::npos) return false;
    return true;
}

static void PushStats(ILuaBase* lua, const capture::Stats& st) {
    lua->CreateTable();
    lua->PushNumber(static_cast<double>(st.packets));
    lua->SetField(-2, "packets");
    lua->PushNumber(static_cast<double>(st.bytes));
    lua->SetField(-2, "bytes");
    lua->PushNumber(st.talkSeconds);
    lua->SetField(-2, "seconds");
    lua->PushNumber(st.idleSeconds);
    lua->SetField(-2, "idle");
}

LUA_FUNCTION(Enable) {
    bool enable = LUA->IsType(1, Type::Bool) ? LUA->GetBool(1) : true;
    if (!enable) {
        hook::Remove();
        LUA->PushBool(true);
        return 1;
    }
    hook::Status s = hook::Install();
    LUA->PushBool(s.installed);
    if (!s.installed) {
        LUA->PushString(s.error.c_str());
        return 2;
    }
    return 1;
}

LUA_FUNCTION(IsEnabled) {
    LUA->PushBool(hook::IsInstalled());
    return 1;
}

LUA_FUNCTION(SetSignature) {
    LUA->CheckType(1, Type::String);
    hook::SetSignature(LUA->GetString(1));
    return 0;
}

LUA_FUNCTION(SetBufferSeconds) {
    LUA->CheckType(1, Type::Number);
    capture::SetBufferSeconds(ClampInt(LUA->GetNumber(1), 1, 600));
    return 0;
}

LUA_FUNCTION(GetBufferSeconds) {
    LUA->PushNumber(capture::GetBufferSeconds());
    return 1;
}

LUA_FUNCTION(SetDecode) {
    LUA->CheckType(1, Type::Bool);
    capture::SetDecode(LUA->GetBool(1));
    return 0;
}

LUA_FUNCTION(GetDecode) {
    LUA->PushBool(capture::GetDecode());
    return 1;
}

LUA_FUNCTION(SetFillSilence) {
    LUA->CheckType(1, Type::Bool);
    capture::SetFillSilence(LUA->GetBool(1));
    return 0;
}

LUA_FUNCTION(GetFillSilence) {
    LUA->PushBool(capture::GetFillSilence());
    return 1;
}

LUA_FUNCTION(Track) {
    int index = EntityIndex(LUA, 1);
    bool tracked = LUA->IsType(2, Type::Bool) ? LUA->GetBool(2) : true;
    capture::SetTracked(index, tracked);
    return 0;
}

LUA_FUNCTION(IsTracked) {
    LUA->PushBool(capture::IsTracked(EntityIndex(LUA, 1)));
    return 1;
}

LUA_FUNCTION(GetBuffered) {
    LUA->PushNumber(capture::GetBufferedSeconds(EntityIndex(LUA, 1)));
    return 1;
}

LUA_FUNCTION(IsTalking) {
    LUA->PushBool(capture::IsTalking(EntityIndex(LUA, 1)));
    return 1;
}

LUA_FUNCTION(GetLevel) {
    LUA->PushNumber(capture::GetLevel(EntityIndex(LUA, 1)));
    return 1;
}

LUA_FUNCTION(GetStats) {
    PushStats(LUA, capture::GetStats(EntityIndex(LUA, 1)));
    return 1;
}

LUA_FUNCTION(GetPCM) {
    int index = EntityIndex(LUA, 1);
    LUA->CheckType(2, Type::Number);
    std::vector<int16_t> samples = capture::GetSamples(index, LUA->GetNumber(2));
    LUA->PushString(reinterpret_cast<const char*>(samples.data()), static_cast<unsigned>(samples.size() * sizeof(int16_t)));
    return 1;
}

LUA_FUNCTION(GetWav) {
    int index = EntityIndex(LUA, 1);
    LUA->CheckType(2, Type::Number);
    std::string bytes = wav::Encode(capture::GetSamples(index, LUA->GetNumber(2)), decoder::kSampleRate);
    LUA->PushString(bytes.data(), static_cast<unsigned>(bytes.size()));
    return 1;
}

LUA_FUNCTION(SaveWav) {
    int index = EntityIndex(LUA, 1);
    LUA->CheckType(2, Type::Number);
    LUA->CheckType(3, Type::String);
    std::string name = LUA->GetString(3);
    if (!SafeDataPath(name)) {
        LUA->PushBool(false);
        LUA->PushString("invalid path");
        return 2;
    }
    if (name.size() < 4 || name.compare(name.size() - 4, 4, ".wav") != 0) name += ".wav";
    std::vector<int16_t> samples = capture::GetSamples(index, LUA->GetNumber(2));
    bool ok = wav::Write("garrysmod/data/" + name, samples, decoder::kSampleRate);
    LUA->PushBool(ok);
    if (!ok) {
        LUA->PushString("write failed");
        return 2;
    }
    return 1;
}

LUA_FUNCTION(Clear) {
    capture::Clear(EntityIndex(LUA, 1));
    return 0;
}

LUA_FUNCTION(OnDisconnect) {
    if (LUA->IsType(1, Type::Entity)) capture::Clear(EntityIndex(LUA, 1));
    return 0;
}

static void AddHook(ILuaBase* lua) {
    lua->PushSpecial(SPECIAL_GLOB);
    lua->GetField(-1, "hook");
    if (lua->IsType(-1, Type::Table)) {
        lua->GetField(-1, "Add");
        lua->PushString("PlayerDisconnected");
        lua->PushString("voice");
        lua->PushCFunction(OnDisconnect);
        lua->Call(3, 0);
    }
    lua->Pop(2);
}

struct Entry {
    const char* name;
    CFunc fn;
};

static const Entry kEntries[] = {
    {"Enable", Enable},
    {"IsEnabled", IsEnabled},
    {"SetSignature", SetSignature},
    {"SetBufferSeconds", SetBufferSeconds},
    {"GetBufferSeconds", GetBufferSeconds},
    {"SetDecode", SetDecode},
    {"GetDecode", GetDecode},
    {"SetFillSilence", SetFillSilence},
    {"GetFillSilence", GetFillSilence},
    {"Track", Track},
    {"IsTracked", IsTracked},
    {"GetBuffered", GetBuffered},
    {"IsTalking", IsTalking},
    {"GetLevel", GetLevel},
    {"GetStats", GetStats},
    {"GetPCM", GetPCM},
    {"GetWav", GetWav},
    {"SaveWav", SaveWav},
    {"Clear", Clear},
};

void Register(ILuaBase* lua) {
    lua->PushSpecial(SPECIAL_GLOB);
    lua->CreateTable();
    lua->PushString(kVersion);
    lua->SetField(-2, "Version");
    lua->PushNumber(decoder::kSampleRate);
    lua->SetField(-2, "SampleRate");
    for (const Entry& e : kEntries) {
        lua->PushCFunction(e.fn);
        lua->SetField(-2, e.name);
    }
    lua->SetField(-2, "voice");
    lua->Pop();
    AddHook(lua);
}

}
