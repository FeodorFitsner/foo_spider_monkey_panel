#pragma once

#include <stdint.h>

struct SmpPluginApiVersion
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

extern "C" SmpPluginApiVersion __cdecl Smp_GetApiVersion();
extern "C" void* __cdecl Smp_GetInterface( const SmpPluginApiVersion* pVersion );
