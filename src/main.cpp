#include <GarrysMod/Lua/Interface.h>
#include "capture.h"
#include "hook.h"
#include "lua_api.h"

GMOD_MODULE_OPEN() {
    capture::Init(LUA);
    lua_api::Register(LUA);
    return 0;
}

GMOD_MODULE_CLOSE() {
    hook::Remove();
    capture::Shutdown();
    return 0;
}
