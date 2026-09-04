#include "platform.h"
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <link.h>
#endif

namespace platform {

#ifdef _WIN32

Module FindModule(std::initializer_list<const char*> names) {
    Module m;
    for (const char* name : names) {
        HMODULE h = GetModuleHandleA(name);
        if (!h) continue;
        auto base = reinterpret_cast<uint8_t*>(h);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
            if (!(section->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
            m.code.push_back({base + section->VirtualAddress, section->Misc.VirtualSize});
        }
        m.handle = h;
        break;
    }
    return m;
}

void* FindSymbol(const Module& mod, const char* symbol) {
    if (!mod.handle) return nullptr;
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(mod.handle), symbol));
}

#else

struct Query {
    const std::initializer_list<const char*>* names;
    Module result;
};

static const char* BaseName(const char* path) {
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int Visit(dl_phdr_info* info, size_t, void* data) {
    auto q = static_cast<Query*>(data);
    if (!info->dlpi_name || !*info->dlpi_name) return 0;
    const char* base = BaseName(info->dlpi_name);
    bool match = false;
    for (const char* name : *q->names) {
        if (strcmp(base, name) == 0) { match = true; break; }
    }
    if (!match) return 0;
    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const auto& ph = info->dlpi_phdr[i];
        if (ph.p_type != PT_LOAD || !(ph.p_flags & PF_X)) continue;
        q->result.code.push_back({reinterpret_cast<uint8_t*>(info->dlpi_addr + ph.p_vaddr), static_cast<size_t>(ph.p_memsz)});
    }
    q->result.handle = dlopen(info->dlpi_name, RTLD_NOW | RTLD_NOLOAD);
    return q->result.handle ? 1 : 0;
}

Module FindModule(std::initializer_list<const char*> names) {
    Query q{&names, Module{}};
    dl_iterate_phdr(Visit, &q);
    return q.result;
}

void* FindSymbol(const Module& mod, const char* symbol) {
    if (!mod.handle) return nullptr;
    return dlsym(mod.handle, symbol);
}

#endif

static int HexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool ParsePattern(const std::string& text, std::vector<int>& out) {
    out.clear();
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == ' ') { ++i; continue; }
        if (text[i] == '?') {
            out.push_back(-1);
            while (i < text.size() && text[i] == '?') ++i;
            continue;
        }
        if (i + 1 >= text.size()) return false;
        int a = HexValue(text[i]), b = HexValue(text[i + 1]);
        if (a < 0 || b < 0) return false;
        out.push_back((a << 4) | b);
        i += 2;
    }
    return !out.empty();
}

static void* Scan(const Range& r, const std::vector<int>& bytes) {
    size_t n = bytes.size();
    if (n > r.size) return nullptr;
    for (size_t i = 0; i <= r.size - n; ++i) {
        size_t j = 0;
        for (; j < n; ++j) {
            if (bytes[j] != -1 && r.start[i + j] != static_cast<uint8_t>(bytes[j])) break;
        }
        if (j == n) return r.start + i;
    }
    return nullptr;
}

void* FindPattern(const Module& mod, const std::string& pattern) {
    std::vector<int> bytes;
    if (!mod.valid() || !ParsePattern(pattern, bytes)) return nullptr;
    for (const Range& r : mod.code) {
        if (void* hit = Scan(r, bytes)) return hit;
    }
    return nullptr;
}

}
