cmake_minimum_required(VERSION 3.16)
project(gmsv_voice LANGUAGES C CXX)

option(VOICE_X86 "Build a 32-bit module" OFF)

if(VOICE_X86 AND NOT WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

FetchContent_Declare(gmod_module_base
    GIT_REPOSITORY https://github.com/Facepunch/gmod-module-base.git
    GIT_TAG development
    SOURCE_SUBDIR include)
FetchContent_Declare(opus
    GIT_REPOSITORY https://github.com/xiph/opus.git
    GIT_TAG v1.5.2)

set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(gmod_module_base opus)

add_library(subhook STATIC ext/subhook/subhook.c)
target_include_directories(subhook PUBLIC ext/subhook)
target_compile_definitions(subhook PUBLIC SUBHOOK_STATIC)

add_library(gmsv_voice MODULE
    src/main.cpp
    src/platform.cpp
    src/hook.cpp
    src/decoder.cpp
    src/buffer.cpp
    src/capture.cpp
    src/wav.cpp
    src/lua_api.cpp)

target_include_directories(gmsv_voice PRIVATE src)
target_link_libraries(gmsv_voice PRIVATE gmod-module-base subhook opus)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(VOICE_SUFFIX win64)
    else()
        set(VOICE_SUFFIX win32)
    endif()
elseif(APPLE)
    set(VOICE_SUFFIX osx)
else()
    if(VOICE_X86)
        set(VOICE_SUFFIX linux)
    else()
        set(VOICE_SUFFIX linux64)
    endif()
    target_link_options(gmsv_voice PRIVATE -static-libstdc++ -static-libgcc $<$<CONFIG:Release>:-s>)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(gmsv_voice PRIVATE -Wl,--exclude-libs,ALL)
    endif()
    target_link_libraries(gmsv_voice PRIVATE dl)
endif()

set_target_properties(gmsv_voice PROPERTIES
    PREFIX ""
    OUTPUT_NAME "gmsv_voice_${VOICE_SUFFIX}"
    SUFFIX ".dll")
