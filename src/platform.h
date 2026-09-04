#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace platform {

struct Range {
    uint8_t* start = nullptr;
    size_t size = 0;
};

struct Module {
    void* handle = nullptr;
    std::vector<Range> code;
    bool valid() const { return handle != nullptr; }
};

Module FindModule(std::initializer_list<const char*> names);
void* FindSymbol(const Module& mod, const char* symbol);
void* FindPattern(const Module& mod, const std::string& pattern);

}
