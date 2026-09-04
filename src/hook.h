#pragma once
#include <string>

namespace hook {

struct Status {
    bool installed = false;
    std::string error;
};

void SetSignature(const std::string& pattern);
Status Install();
void Remove();
bool IsInstalled();

}
