#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

struct OpusDecoder;

namespace decoder {

constexpr int kSampleRate = 24000;
constexpr int kFrameSamples = 480;
constexpr int kMaxFrameSamples = 5760;

class Opus {
public:
    Opus() = default;
    ~Opus();
    Opus(const Opus&) = delete;
    Opus& operator=(const Opus&) = delete;

    bool Decode(const uint8_t* data, size_t size, std::vector<int16_t>& out);
    void Reset();
    void Release();

private:
    bool Ensure();

    OpusDecoder* m_decoder = nullptr;
};

}
