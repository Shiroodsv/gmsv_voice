#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace buffer {

class Ring {
public:
    void SetCapacity(size_t samples);
    size_t Capacity() const { return m_capacity; }
    size_t Count() const { return m_count; }
    void Push(const int16_t* samples, size_t n);
    void PushSilence(size_t n);
    std::vector<int16_t> Tail(size_t n) const;
    int16_t Peak(size_t n) const;
    void Clear();

private:
    void Ensure();

    std::vector<int16_t> m_data;
    size_t m_capacity = 0;
    size_t m_write = 0;
    size_t m_count = 0;
};

}
