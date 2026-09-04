#include "decoder.h"
#include <opus.h>
#include <cstring>

namespace decoder {

Opus::~Opus() {
    Release();
}

bool Opus::Ensure() {
    if (m_decoder) return true;
    int err = 0;
    m_decoder = opus_decoder_create(kSampleRate, 1, &err);
    if (err != OPUS_OK) m_decoder = nullptr;
    return m_decoder != nullptr;
}

void Opus::Reset() {
    if (m_decoder) opus_decoder_ctl(m_decoder, OPUS_RESET_STATE);
}

void Opus::Release() {
    if (m_decoder) opus_decoder_destroy(m_decoder);
    m_decoder = nullptr;
}

bool Opus::Decode(const uint8_t* data, size_t size, std::vector<int16_t>& out) {
    out.clear();
    if (!data || !Ensure()) return false;
    int16_t frame[kMaxFrameSamples];
    size_t pos = 0;
    while (pos + 4 <= size) {
        uint16_t length, sequence;
        std::memcpy(&length, data + pos, 2);
        std::memcpy(&sequence, data + pos + 2, 2);
        pos += 4;
        if (length == 0xFFFF) {
            Reset();
            break;
        }
        if (length == 0) {
            out.insert(out.end(), kFrameSamples, 0);
            continue;
        }
        if (pos + length > size) break;
        int n = opus_decode(m_decoder, data + pos, length, frame, kMaxFrameSamples, 0);
        pos += length;
        if (n <= 0) continue;
        out.insert(out.end(), frame, frame + n);
    }
    return !out.empty();
}

}
