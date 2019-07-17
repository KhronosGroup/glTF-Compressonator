#include <stdio.h>
#include <stdlib.h>
#include "TC_PluginAPI.h"
#include "TC_PluginInternal.h"
#include "MIPS.h"
#include "HDR.h"

CMIPS *HDR_CMips;

#ifdef BUILD_AS_PLUGIN_DLL
DECLARE_PLUGIN(Plugin_HDR)
SET_PLUGIN_TYPE("IMAGE")
SET_PLUGIN_NAME("HDR")
#else
void *make_Plugin_HDR() { return new Plugin_HDR; } 
#endif

#ifndef _WIN32
typedef CMP_DWORD* LPDWORD;
#endif

#pragma comment(lib,"advapi32.lib")        // for RegCloseKey and other Reg calls ...

Plugin_HDR::Plugin_HDR()
{ 
}

Plugin_HDR::~Plugin_HDR()
{ 
}

int Plugin_HDR::TC_PluginSetSharedIO(void* Shared)
{
    if (Shared)
    {
        HDR_CMips = static_cast<CMIPS *>(Shared);
        return 0;
    }
    return 1;
}


int Plugin_HDR::TC_PluginGetVersion(TC_PluginVersion* pPluginVersion)
{ 
#ifdef _WIN32
    pPluginVersion->guid                    = g_GUID;
#endif
    pPluginVersion->dwAPIVersionMajor       = TC_API_VERSION_MAJOR;
    pPluginVersion->dwAPIVersionMinor       = TC_API_VERSION_MINOR;
    pPluginVersion->dwPluginVersionMajor    = TC_PLUGIN_VERSION_MAJOR;
    pPluginVersion->dwPluginVersionMinor    = TC_PLUGIN_VERSION_MINOR;
    return 0;
}

int Plugin_HDR::TC_PluginFileLoadTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
    // TODO: Implement.

    return 0;
}

int Plugin_HDR::TC_PluginFileSaveTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
    // TODO: Implement.

    return 0;
}

int Plugin_HDR::TC_PluginFileLoadTexture(const char* pszFilename, MipSet* pMipSet)
{
    // TODO: Implement.

    return 0;
}

int Plugin_HDR::TC_PluginFileSaveTexture(const char* pszFilename, MipSet* pMipSet)
{
    // TODO: Implement.

    return 0;
}
