#include "wav.h"
#include <cstring>
#include <fstream>

namespace wav {

static void Put32(std::string& s, uint32_t v) { s.append(reinterpret_cast<const char*>(&v), 4); }
static void Put16(std::string& s, uint16_t v) { s.append(reinterpret_cast<const char*>(&v), 2); }

std::string Encode(const std::vector<int16_t>& samples, int sampleRate) {
    uint32_t dataBytes = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    std::string s;
    s.reserve(44 + dataBytes);
    s.append("RIFF", 4);
    Put32(s, 36 + dataBytes);
    s.append("WAVE", 4);
    s.append("fmt ", 4);
    Put32(s, 16);
    Put16(s, 1);
    Put16(s, 1);
    Put32(s, static_cast<uint32_t>(sampleRate));
    Put32(s, static_cast<uint32_t>(sampleRate * sizeof(int16_t)));
    Put16(s, sizeof(int16_t));
    Put16(s, 16);
    s.append("data", 4);
    Put32(s, dataBytes);
    if (dataBytes) s.append(reinterpret_cast<const char*>(samples.data()), dataBytes);
    return s;
}

bool Write(const std::string& path, const std::vector<int16_t>& samples, int sampleRate) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    std::string bytes = Encode(samples, sampleRate);
    f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(f);
}

}
