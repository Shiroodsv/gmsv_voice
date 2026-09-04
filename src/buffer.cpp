#include "buffer.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace buffer {

void Ring::SetCapacity(size_t samples) {
    if (samples == m_capacity) return;
    std::vector<int16_t> keep;
    if (!m_data.empty()) keep = Tail(std::min(samples, m_count));
    m_capacity = samples;
    m_data.clear();
    m_data.shrink_to_fit();
    m_write = 0;
    m_count = 0;
    if (!keep.empty()) Push(keep.data(), keep.size());
}

void Ring::Ensure() {
    if (m_data.size() != m_capacity) {
        m_data.assign(m_capacity, 0);
        m_write = 0;
        m_count = 0;
    }
}

void Ring::Push(const int16_t* samples, size_t n) {
    if (m_capacity == 0 || n == 0) return;
    Ensure();
    size_t cap = m_capacity;
    if (n >= cap) {
        std::memcpy(m_data.data(), samples + (n - cap), cap * sizeof(int16_t));
        m_write = 0;
        m_count = cap;
        return;
    }
    size_t first = std::min(n, cap - m_write);
    std::memcpy(m_data.data() + m_write, samples, first * sizeof(int16_t));
    if (first < n) std::memcpy(m_data.data(), samples + first, (n - first) * sizeof(int16_t));
    m_write = (m_write + n) % cap;
    m_count = std::min(cap, m_count + n);
}

void Ring::PushSilence(size_t n) {
    if (m_capacity == 0 || n == 0) return;
    Ensure();
    size_t cap = m_capacity;
    if (n >= cap) {
        std::memset(m_data.data(), 0, cap * sizeof(int16_t));
        m_write = 0;
        m_count = cap;
        return;
    }
    size_t first = std::min(n, cap - m_write);
    std::memset(m_data.data() + m_write, 0, first * sizeof(int16_t));
    if (first < n) std::memset(m_data.data(), 0, (n - first) * sizeof(int16_t));
    m_write = (m_write + n) % cap;
    m_count = std::min(cap, m_count + n);
}

std::vector<int16_t> Ring::Tail(size_t n) const {
    n = std::min(n, m_count);
    std::vector<int16_t> out(n);
    if (n == 0) return out;
    size_t cap = m_capacity;
    size_t start = (m_write + cap - n) % cap;
    size_t first = std::min(n, cap - start);
    std::memcpy(out.data(), m_data.data() + start, first * sizeof(int16_t));
    if (first < n) std::memcpy(out.data() + first, m_data.data(), (n - first) * sizeof(int16_t));
    return out;
}

int16_t Ring::Peak(size_t n) const {
    n = std::min(n, m_count);
    int peak = 0;
    size_t cap = m_capacity;
    for (size_t i = 0; i < n; ++i) {
        size_t idx = (m_write + cap - n + i) % cap;
        int v = std::abs(static_cast<int>(m_data[idx]));
        if (v > peak) peak = v;
    }
    return static_cast<int16_t>(std::min(peak, 32767));
}

void Ring::Clear() {
    m_data.clear();
    m_data.shrink_to_fit();
    m_write = 0;
    m_count = 0;
}

}
