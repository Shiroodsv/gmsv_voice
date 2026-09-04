#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace wav {

std::string Encode(const std::vector<int16_t>& samples, int sampleRate);
bool Write(const std::string& path, const std::vector<int16_t>& samples, int sampleRate);

}
