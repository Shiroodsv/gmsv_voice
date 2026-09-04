#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace GarrysMod { namespace Lua { class ILuaBase; } }

namespace capture {

constexpr int kMaxPlayers = 128;

struct Stats {
    uint64_t packets = 0;
    uint64_t bytes = 0;
    double talkSeconds = 0;
    double idleSeconds = -1;
};

void Init(GarrysMod::Lua::ILuaBase* lua);
void Shutdown();

void OnVoice(int entIndex, int userId, const uint8_t* data, size_t size);

void SetBufferSeconds(int seconds);
int GetBufferSeconds();
void SetDecode(bool enabled);
bool GetDecode();
void SetFillSilence(bool enabled);
bool GetFillSilence();
void SetTracked(int entIndex, bool tracked);
bool IsTracked(int entIndex);

std::vector<int16_t> GetSamples(int entIndex, double seconds);
double GetBufferedSeconds(int entIndex);
bool IsTalking(int entIndex);
double GetLevel(int entIndex);
Stats GetStats(int entIndex);
void Clear(int entIndex);

}
