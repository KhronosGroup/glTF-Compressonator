#ifndef _PLUGIN_IMAGE_HDR_H
#define _PLUGIN_IMAGE_HDR_H

#pragma once

#define WIN32_LEAN_AND_MEAN        // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
// TODO: reference additional headers your program requires here
#include <assert.h>

#ifdef _WIN32
#include <tchar.h>
#endif

#include "PluginInterface.h"

// ---------------- HDR Plugin ------------------------
#ifdef _WIN32
static const GUID g_GUID = { 0x7603D7F2, 0x7823, 0x4C60, { 0x8B, 0x09, 0xF9, 0xE8, 0xBE, 0xEA, 0xE3, 0xA7 } };
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


// ---------------- HDR Definitions ------------------------
static const CMP_WORD HDR_HEADER = ((CMP_WORD)(CMP_BYTE)('B') | ((CMP_WORD)(CMP_BYTE)('M') << 8));

#pragma pack(push)
#pragma pack(1)

typedef struct
{                                       //Byte    Comments
    CMP_BYTE cIDFieldLength;                //[0]     length of a string located after the header
    CMP_BYTE cColorMapType;                 //[1]     0  -  Image type (Supported)
                                        //        1  -  Data type (Not supported)
    CMP_BYTE cImageType;                    //[2]     0  -  No image data included.
                                        //        1  -  Uncompressed, color-mapped images.
                                        //        2  -  Uncompressed, RGB images.
                                        //        3  -  Uncompressed, black and white images.
                                        //        9  -  Runlength encoded color-mapped images.
                                        //        10  - Runlength encoded RGB images.
                                        //        11  - Compressed, black and white images.
                                        //        32  - Compressed color-mapped data, using Huffman, Delta, and
                                        //              runlength encoding.
                                        //        33  - Compressed color-mapped data, using Huffman, Delta, and
                                        //              runlength encoding.  4-pass quadtree-type process.

    short nFirstColorMapEntry;            // [3:4]
    short nColorMapLength;                // [5:6]
    CMP_BYTE cColorMapEntrySize;            // [7]
    short nXOrigin;                        // [8:9]    
    short nYOrigin;                        // [10:11]
    short nWidth;                        // [12:13]
    short nHeight;                        // [14:15]
    CMP_BYTE cColorDepth;                    // [16]
    CMP_BYTE cFormatFlags;                    // [17]
} HDRHeader;


// Image Types Not supported
//    1 = Raw indexed image
//  9 = RLE indexed
//  32 & 33 Other compression, indexed

static const CMP_BYTE ImageType_ARGB8888     =  2;        // Raw RGB
static const CMP_BYTE ImageType_G8             =  3;        // Raw greyscale
static const CMP_BYTE ImageType_ARGB8888_RLE = 10;        // RLE RGB
static const CMP_BYTE ImageType_G8_RLE         = 11;        // RLE greyscale

typedef struct
{
    CMP_DWORD dwSize;

    bool bRLECompressed;
}    HDR_FileSaveParams;

#pragma pack(pop)

#define IDS_STRING1                     1
#define IDS_ERROR_FILE_OPEN             1
#define IDS_ERROR_REGISTER_FILETYPE     2
#define IDS_ERROR_NOT_HDR               3
#define IDS_ERROR_UNSUPPORTED_TYPE      4
#define IDD_FILE_SAVE_PARAMETERS        101
#define IDC_RLE_COMPRESSED              1000

TC_PluginError LoadHDR_ARGB8888(FILE* pFile, MipSet* pMipSet, HDRHeader& Header);
TC_PluginError LoadHDR_ARGB8888_RLE(FILE* pFile, MipSet* pMipSet, HDRHeader& Header);
TC_PluginError LoadHDR_RGB888(FILE* pFile, MipSet* pMipSet, HDRHeader& Header);
TC_PluginError LoadHDR_RGB888_RLE(FILE* pFile, MipSet* pMipSet, HDRHeader& Header);
TC_PluginError LoadHDR_G8(FILE* pFile, MipSet* pMipSet, HDRHeader& Header);
TC_PluginError LoadHDR_G8_RLE(FILE* pFile, MipSet* pMipSet, HDRHeader& Header);

TC_PluginError SaveHDR_ARGB8888(FILE* pFile, const MipSet* pMipSet);
TC_PluginError SaveHDR_ARGB8888_RLE(FILE* pFile, const MipSet* pMipSet);
TC_PluginError SaveHDR_RGB888(FILE* pFile, const MipSet* pMipSet);
TC_PluginError SaveHDR_RGB888_RLE(FILE* pFile, const MipSet* pMipSet);
TC_PluginError SaveHDR_G8(FILE* pFile, const MipSet* pMipSet);
TC_PluginError SaveHDR_G8_RLE(FILE* pFile, const MipSet* pMipSet);

void LoadRegistryKeys(HDR_FileSaveParams* pParams);
void LoadRegistryKeyDefaults(HDR_FileSaveParams* pParams);
void SaveRegistryKeys(const HDR_FileSaveParams* pParams);
void ParseTextParams(HDR_FileSaveParams* pParams, const TCHAR* pszTextParams);

#endif
