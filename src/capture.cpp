#include "capture.h"
#include "buffer.h"
#include "decoder.h"
#include <GarrysMod/Lua/Interface.h>
#include <algorithm>
#include <array>
#include <chrono>

namespace capture {

using Clock = std::chrono::steady_clock;

constexpr double kGapSeconds = 0.25;
constexpr double kTalkSeconds = 0.3;
constexpr double kLevelSeconds = 0.1;
constexpr double kLeadSeconds = 0.5;

struct Slot {
    buffer::Ring ring;
    decoder::Opus opus;
    bool tracked = true;
    bool active = false;
    int userId = -1;
    uint64_t packets = 0;
    uint64_t bytes = 0;
    uint64_t samples = 0;
    Clock::time_point lastPacket;
    Clock::time_point streamEnd;
};

static GarrysMod::Lua::ILuaBase* g_lua = nullptr;
static std::array<Slot, kMaxPlayers + 1> g_slots;
static int g_bufferSeconds = 60;
static bool g_decode = true;
static bool g_fill = true;
static std::vector<int16_t> g_scratch;

static bool ValidIndex(int i) { return i >= 1 && i <= kMaxPlayers; }

static double Seconds(Clock::duration d) {
    return std::chrono::duration<double>(d).count();
}

static size_t Capacity() {
    return static_cast<size_t>(g_bufferSeconds) * decoder::kSampleRate;
}

static void Reset(Slot& s) {
    s.ring.Clear();
    s.opus.Release();
    s.active = false;
    s.userId = -1;
    s.packets = 0;
    s.bytes = 0;
    s.samples = 0;
}

static double TrailingSeconds(const Slot& s, Clock::time_point now) {
    if (!g_fill || !s.active) return 0;
    double t = Seconds(now - s.streamEnd);
    return t > kGapSeconds ? t : 0;
}

void Init(GarrysMod::Lua::ILuaBase* lua) {
    g_lua = lua;
    for (auto& s : g_slots) s.ring.SetCapacity(Capacity());
    g_scratch.reserve(decoder::kMaxFrameSamples * 4);
}

void Shutdown() {
    g_lua = nullptr;
    for (auto& s : g_slots) Reset(s);
}

static void RunHook(int entIndex, const uint8_t* data, size_t size) {
    if (!g_lua) return;
    using namespace GarrysMod::Lua;
    g_lua->PushSpecial(SPECIAL_GLOB);
    g_lua->GetField(-1, "hook");
    if (!g_lua->IsType(-1, Type::Table)) { g_lua->Pop(2); return; }
    g_lua->GetField(-1, "Run");
    if (!g_lua->IsType(-1, Type::Function)) { g_lua->Pop(3); return; }
    g_lua->PushString("VoiceData");
    g_lua->GetField(-4, "Entity");
    g_lua->PushNumber(entIndex);
    g_lua->Call(1, 1);
    g_lua->PushNumber(static_cast<double>(size));
    g_lua->PushString(reinterpret_cast<const char*>(data), static_cast<unsigned>(size));
    if (g_lua->PCall(4, 0, 0) != 0) {
        g_lua->GetField(-3, "ErrorNoHalt");
        if (g_lua->IsType(-1, Type::Function)) {
            g_lua->Push(-2);
            g_lua->PushString("\n");
            if (g_lua->PCall(2, 0, 0) != 0) g_lua->Pop();
        } else {
            g_lua->Pop();
        }
        g_lua->Pop();
    }
    g_lua->Pop(2);
}

void OnVoice(int entIndex, int userId, const uint8_t* data, size_t size) {
    if (!ValidIndex(entIndex)) return;
    Slot& s = g_slots[entIndex];
    if (s.userId != userId) {
        bool tracked = s.tracked;
        Reset(s);
        s.tracked = tracked;
        s.userId = userId;
    }
    if (!s.tracked) return;
    Clock::time_point now = Clock::now();
    s.packets++;
    s.bytes += size;
    s.lastPacket = now;
    if (g_decode && s.opus.Decode(data, size, g_scratch)) {
        double gap = TrailingSeconds(s, now);
        if (gap > 0) {
            s.ring.PushSilence(std::min(static_cast<size_t>(gap * decoder::kSampleRate), Capacity()));
            s.streamEnd = now;
        }
        if (!s.active) {
            s.active = true;
            s.streamEnd = now;
        }
        s.ring.Push(g_scratch.data(), g_scratch.size());
        s.samples += g_scratch.size();
        s.streamEnd += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(static_cast<double>(g_scratch.size()) / decoder::kSampleRate));
        Clock::time_point lead = now + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(kLeadSeconds));
        if (s.streamEnd > lead) s.streamEnd = lead;
    }
    RunHook(entIndex, data, size);
}

void SetBufferSeconds(int seconds) {
    g_bufferSeconds = std::clamp(seconds, 1, 600);
    for (auto& s : g_slots) s.ring.SetCapacity(Capacity());
}

int GetBufferSeconds() { return g_bufferSeconds; }
void SetDecode(bool enabled) { g_decode = enabled; }
bool GetDecode() { return g_decode; }
void SetFillSilence(bool enabled) { g_fill = enabled; }
bool GetFillSilence() { return g_fill; }

void SetTracked(int entIndex, bool tracked) {
    if (!ValidIndex(entIndex)) return;
    Slot& s = g_slots[entIndex];
    s.tracked = tracked;
    if (!tracked) {
        int userId = s.userId;
        Reset(s);
        s.tracked = false;
        s.userId = userId;
    }
}

bool IsTracked(int entIndex) {
    return ValidIndex(entIndex) && g_slots[entIndex].tracked;
}

std::vector<int16_t> GetSamples(int entIndex, double seconds) {
    if (!ValidIndex(entIndex) || !(seconds > 0)) return {};
    const Slot& s = g_slots[entIndex];
    size_t want = std::min(static_cast<size_t>(seconds * decoder::kSampleRate), Capacity());
    size_t pad = std::min(want, static_cast<size_t>(TrailingSeconds(s, Clock::now()) * decoder::kSampleRate));
    std::vector<int16_t> out = s.ring.Tail(want - pad);
    out.resize(out.size() + pad, 0);
    return out;
}

double GetBufferedSeconds(int entIndex) {
    if (!ValidIndex(entIndex)) return 0;
    const Slot& s = g_slots[entIndex];
    double held = static_cast<double>(s.ring.Count()) / decoder::kSampleRate + TrailingSeconds(s, Clock::now());
    return std::min(held, static_cast<double>(g_bufferSeconds));
}

bool IsTalking(int entIndex) {
    if (!ValidIndex(entIndex)) return false;
    const Slot& s = g_slots[entIndex];
    return s.packets > 0 && Seconds(Clock::now() - s.lastPacket) < kTalkSeconds;
}

double GetLevel(int entIndex) {
    if (!IsTalking(entIndex)) return 0;
    const Slot& s = g_slots[entIndex];
    return s.ring.Peak(static_cast<size_t>(kLevelSeconds * decoder::kSampleRate)) / 32768.0;
}

Stats GetStats(int entIndex) {
    Stats st;
    if (!ValidIndex(entIndex)) return st;
    const Slot& s = g_slots[entIndex];
    st.packets = s.packets;
    st.bytes = s.bytes;
    st.talkSeconds = static_cast<double>(s.samples) / decoder::kSampleRate;
    if (s.packets > 0) st.idleSeconds = Seconds(Clock::now() - s.lastPacket);
    return st;
}

void Clear(int entIndex) {
    if (!ValidIndex(entIndex)) return;
    Slot& s = g_slots[entIndex];
    bool tracked = s.tracked;
    Reset(s);
    s.tracked = tracked;
}

}
