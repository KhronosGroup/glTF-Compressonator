#include <stdio.h>
#include <stdlib.h>
#include "TC_PluginAPI.h"
#include "TC_PluginInternal.h"
#include "MIPS.h"
#include "HDR.h"

//

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define STBI_NO_BMP
#define STBI_NO_GIF
#define STBI_NO_JPEG
#define STBI_NO_PIC
#define STBI_NO_PNG
#define STBI_NO_PNM
#define STBI_NO_PSD
#define STBI_NO_TGA

#define STBI_ONLY_HDR

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

//

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

    printf("Loading Texture\n");

    return PE_Unknown;
}

int Plugin_HDR::TC_PluginFileSaveTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
    // TODO: Implement.

    printf("Saving Texture\n");

    return PE_Unknown;
}

int Plugin_HDR::TC_PluginFileLoadTexture(const char* pszFilename, MipSet* pMipSet)
{
    int x = 0;
    int y = 0;
    int channels_in_file = 0;

    float* pixelData = stbi_loadf(pszFilename, &x, &y, &channels_in_file, 0);
    if (!pixelData)
    {
        return PE_Unknown;
    }

    //

    pMipSet->m_ChannelFormat = CF_Float32;
    pMipSet->m_TextureDataType = TDT_ARGB;
    pMipSet->m_TextureType = TT_2D;
    pMipSet->m_nMipLevels = 1;
    pMipSet->m_nWidth = x;
    pMipSet->m_nHeight = y;
    pMipSet->m_nDepth = 1;
    pMipSet->m_format = CMP_FORMAT_ARGB_32F;

    if (!HDR_CMips->AllocateMipSet(pMipSet, pMipSet->m_ChannelFormat, pMipSet->m_TextureDataType, pMipSet->m_TextureType, pMipSet->m_nWidth, pMipSet->m_nHeight, pMipSet->m_nDepth))
    {
        free(pixelData);

        return PE_Unknown;
    }

    if (!HDR_CMips->AllocateMipLevelData(HDR_CMips->GetMipLevel(pMipSet, 0), pMipSet->m_nWidth, pMipSet->m_nHeight, pMipSet->m_ChannelFormat, pMipSet->m_TextureDataType))
    {
        free(pixelData);

        return PE_Unknown;
    }

    //

    CMP_FLOAT* pData = HDR_CMips->GetMipLevel(pMipSet, 0)->m_pfData;

    int py = 0;
    for (py = 0; py < pMipSet->m_nHeight; py++)
    {
        int px = 0;
        for (px = 0; px < pMipSet->m_nWidth; px++)
        {
            pData[4 * px + 0 + py * 4 * pMipSet->m_nWidth] = pixelData[channels_in_file * px + 0 + py * channels_in_file * pMipSet->m_nWidth];
            pData[4 * px + 1 + py * 4 * pMipSet->m_nWidth] = pixelData[channels_in_file * px + 1 + py * channels_in_file * pMipSet->m_nWidth];
            pData[4 * px + 2 + py * 4 * pMipSet->m_nWidth] = pixelData[channels_in_file * px + 2 + py * channels_in_file * pMipSet->m_nWidth];
            pData[4 * px + 3 + py * 4 * pMipSet->m_nWidth] = 1.0f;
        }
    }

    //

    free(pixelData);

    return PE_OK;
}

int Plugin_HDR::TC_PluginFileSaveTexture(const char* pszFilename, MipSet* pMipSet)
{
    // TODO: Implement.

    printf("Saving Texture with MipMap\n");

    return PE_Unknown;
}
