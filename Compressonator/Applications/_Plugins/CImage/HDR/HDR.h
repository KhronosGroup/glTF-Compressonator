#ifndef _PLUGIN_IMAGE_HDR_H
#define _PLUGIN_IMAGE_HDR_H

#pragma once

#define WIN32_LEAN_AND_MEAN        // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <assert.h>

#ifdef _WIN32
#include <tchar.h>
#endif

#include "PluginInterface.h"

// ---------------- HDR Plugin ------------------------
// {f0863e13-1a18-404c-81ba-ae35b3b121c9}
#ifdef _WIN32
static const GUID g_GUID = { 0xf0863e13, 0x1a18, 0x404c, { 0x81, 0xba, 0xae, 0x35, 0xb3, 0xb1, 0x21, 0xc9 } };
#else
static const GUID g_GUID = {0};
#endif

#define TC_PLUGIN_VERSION_MAJOR    1
#define TC_PLUGIN_VERSION_MINOR    0


class Plugin_HDR : public PluginInterface_Image
{
    public: 
        Plugin_HDR();
        virtual ~Plugin_HDR();

        int TC_PluginSetSharedIO(void* Shared);
        int TC_PluginGetVersion(TC_PluginVersion* pPluginVersion);
        int TC_PluginFileLoadTexture(const char* pszFilename, MipSet* pMipSet);
        int TC_PluginFileSaveTexture(const char* pszFilename, MipSet* pMipSet);
        int TC_PluginFileLoadTexture(const char* pszFilename, CMP_Texture *srcTexture);
        int TC_PluginFileSaveTexture(const char* pszFilename, CMP_Texture *srcTexture);

};

extern void *make_Plugin_HDR();

#endif
