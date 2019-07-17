//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "TC_PluginAPI.h"
#include "TC_PluginInternal.h"
#include "MIPS.h"
#include "HDR.h"

CMIPS *HDR_CMips;
HDR_FileSaveParams g_FileSaveParams;

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
/*
..\.\..\..\..\..\Common\Lib\Ext\OpenEXR\v1.4.0\lib_MT\$(Configuration)\$(Platform);
zlibstatic_d.lib
*/

#pragma comment(lib,"advapi32.lib")        // for RegCloseKey and other Reg calls ...

Plugin_HDR::Plugin_HDR()
{ 
    //MessageBox(0,"Plugin_HDR","Plugin_HDR",MB_OK);  
}

Plugin_HDR::~Plugin_HDR()
{ 
    //MessageBox(0,"Plugin_HDR","~Plugin_HDR",MB_OK);  
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
    //MessageBox(0,"TC_PluginGetVersion","Plugin_HDR",MB_OK);  
#ifdef _WIN32
    pPluginVersion->guid                    = g_GUID;
#endif
    pPluginVersion->dwAPIVersionMajor        = TC_API_VERSION_MAJOR;
    pPluginVersion->dwAPIVersionMinor        = TC_API_VERSION_MINOR;
    pPluginVersion->dwPluginVersionMajor    = TC_PLUGIN_VERSION_MAJOR;
    pPluginVersion->dwPluginVersionMinor    = TC_PLUGIN_VERSION_MINOR;
    return 0;
}

int Plugin_HDR::TC_PluginFileLoadTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
   //MessageBox(0,"TC_PluginFileLoadTexture srcTexture","Plugin_HDR",MB_OK);  
    return 0;
 
}

int Plugin_HDR::TC_PluginFileSaveTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
   //MessageBox(0,"TC_PluginFileSaveTexture srcTexture","Plugin_HDR",MB_OK);  
    return 0;
}

// #include "LoadHDR.h"

int Plugin_HDR::TC_PluginFileLoadTexture(const char* pszFilename, MipSet* pMipSet)
{

   // ATI code
   FILE* pFile = NULL;
   pFile = fopen(pszFilename, ("rb"));
   if(pFile == NULL)
    {
        if (HDR_CMips)
            HDR_CMips->PrintError(("Error(%d): HDR Plugin ID(%d) opening file = %s "), EL_Error, IDS_ERROR_FILE_OPEN, pszFilename);
        return -1;
    }

    // Read the header
    HDRHeader header;
   if(fread(&header, sizeof(HDRHeader), 1, pFile) != 1)
   {
      if (HDR_CMips)
            HDR_CMips->PrintError(("Error(%d): HDR Plugin ID(%d) invalid HDR header. Filename = %s "), EL_Error, IDS_ERROR_NOT_HDR, pszFilename);
      fclose(pFile);
      return -1;
   }

    // Skip the ID field
    if(header.cIDFieldLength)
        fseek(pFile, header.cIDFieldLength, SEEK_CUR);

    if(!HDR_CMips->AllocateMipSet(pMipSet, CF_8bit, TDT_ARGB, TT_2D, header.nWidth, header.nHeight, 1))
    {
        fclose(pFile);
        return PE_Unknown;
    }

    if(header.cColorMapType == 0)
    {
        if(header.cImageType == ImageType_ARGB8888 && header.cColorDepth == 32)
            return LoadHDR_ARGB8888(pFile, pMipSet, header);
        else if(header.cImageType == ImageType_ARGB8888_RLE && header.cColorDepth == 32)
            return LoadHDR_ARGB8888_RLE(pFile, pMipSet, header);
        else if(header.cImageType == ImageType_ARGB8888 && header.cColorDepth == 24)
            return LoadHDR_RGB888(pFile, pMipSet, header);
        else if(header.cImageType == ImageType_ARGB8888_RLE && header.cColorDepth == 24)
            return LoadHDR_RGB888_RLE(pFile, pMipSet, header);
        else if(header.cImageType == ImageType_G8 && header.cColorDepth == 8)
            return LoadHDR_G8(pFile, pMipSet, header);
        else if(header.cImageType == ImageType_G8_RLE && header.cColorDepth == 8)
            return LoadHDR_G8_RLE(pFile, pMipSet, header);
    }

   if (HDR_CMips)
            HDR_CMips->PrintError(("Error(%d): HDR Plugin ID(%d) unsupported type Filename = %s "), EL_Error, IDS_ERROR_UNSUPPORTED_TYPE, pszFilename);
   fclose(pFile);

   return -1;
}


int Plugin_HDR::TC_PluginFileSaveTexture(const char* pszFilename, MipSet* pMipSet)
{
    assert(pszFilename);
    assert(pMipSet);

   FILE* pFile = NULL;
   pFile = fopen(pszFilename, ("wb"));
   if(pFile == NULL)
   {
        if (HDR_CMips)
            HDR_CMips->PrintError(("Error(%d): HDR Plugin ID(%d) saving file = %s "), EL_Error, IDS_ERROR_FILE_OPEN, pszFilename);
        return -1;
    }

    HDRHeader header;
    memset(&header, 0, sizeof(header));
    switch(pMipSet->m_dwFourCC)
    {
        case MAKEFOURCC('G', '8', ' ', ' '):
            header.cImageType = ImageType_G8_RLE; 
            header.cColorDepth = 8;
            break;
        case 0: // Not a FourCC texture
        default:
            if(pMipSet->m_TextureDataType == TDT_ARGB)
            {
                header.cColorDepth = 32;
                header.cFormatFlags = 0x8;
            }
            else
                header.cColorDepth = 24;
            header.cImageType = ImageType_ARGB8888;
            break;
    }

    // napatel need to add this somehow!
    //if(pHDRFileSaveParams->bRLECompressed)
    //    header.cImageType |= 0x8;

    header.nWidth = static_cast<short>(pMipSet->m_nWidth);
    header.nHeight = static_cast<short>(pMipSet->m_nHeight);

    fwrite(&header, sizeof(header), 1, pFile);

    if(header.cImageType == ImageType_G8)
        return SaveHDR_G8(pFile, pMipSet);
    else if(header.cImageType == ImageType_G8_RLE)
        return SaveHDR_G8_RLE(pFile, pMipSet);
    else if(header.cImageType == ImageType_ARGB8888 && pMipSet->m_TextureDataType == TDT_ARGB)
        return SaveHDR_ARGB8888(pFile, pMipSet);
    else if(header.cImageType == ImageType_ARGB8888_RLE && pMipSet->m_TextureDataType == TDT_ARGB)
        return SaveHDR_ARGB8888_RLE(pFile, pMipSet);
    else if(header.cImageType == ImageType_ARGB8888 && pMipSet->m_TextureDataType == TDT_XRGB)
        return SaveHDR_RGB888(pFile, pMipSet);
    else if(header.cImageType == ImageType_ARGB8888_RLE && pMipSet->m_TextureDataType == TDT_XRGB)
        return SaveHDR_RGB888_RLE(pFile, pMipSet);

   if (HDR_CMips)
            HDR_CMips->PrintError(("Error(%d): HDR Plugin ID(%d) unsupported type Filename = %s "), EL_Error, IDS_ERROR_UNSUPPORTED_TYPE, pszFilename);
   fclose(pFile);

   return -1;
}


//---------------- HDR Code -----------------------------------


TC_PluginError LoadHDR_ARGB8888(FILE* pFile, MipSet* pMipSet, HDRHeader& Header)
{
    if(!HDR_CMips->AllocateMipLevelData(HDR_CMips->GetMipLevel(pMipSet, 0), Header.nWidth, Header.nHeight, CF_8bit, TDT_ARGB))
        return PE_Unknown;

    pMipSet->m_ChannelFormat = CF_8bit;
    pMipSet->m_TextureDataType = TDT_ARGB;
    pMipSet->m_dwFourCC = 0;
    pMipSet->m_dwFourCC2 = 0;
    pMipSet->m_format = CMP_FORMAT_ARGB_8888;
    pMipSet->m_nMipLevels = 1;

    // Allocate a temporary buffer and read the bitmap data into it
    CMP_DWORD dwSize = pMipSet->m_nWidth *  pMipSet->m_nHeight * sizeof(CMP_COLOR);
    unsigned char* pTempData = static_cast<unsigned char*>(malloc(dwSize));
    fread(pTempData, dwSize, 1, pFile);
    fclose(pFile);

    CMP_BYTE* pTempPtr = pTempData;

    int nStart, nEnd, nIncrement;
    // Bottom up ?
    if(Header.cFormatFlags & 0x20)
    {
        nStart = 0;
        nEnd = pMipSet->m_nHeight;
        nIncrement = 1;
    }
    else
    {
        nStart = pMipSet->m_nHeight-1;
        nEnd = -1;
        nIncrement = -1;
    }

    char nBlue ;
    char nGreen;
    char nRed  ;
    char nAlpha;

    for(int j = nStart; j != nEnd; j+= nIncrement)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * pMipSet->m_nWidth * sizeof(CMP_COLOR)));
        for(int i=0; i < pMipSet->m_nWidth; i++)
        {
            // Note MIPSet is  RGBA
            // HDR is saved as BGRA
            nBlue  = *pTempPtr++;
            nGreen = *pTempPtr++;
            nRed   = *pTempPtr++;
            nAlpha = *pTempPtr++;

            *pData++ = nRed;
            *pData++ = nGreen;
            *pData++ = nBlue;
            *pData++ = nAlpha;


        }
    }

    free(pTempData);

    return PE_OK;
}

TC_PluginError LoadHDR_ARGB8888_RLE(FILE* pFile, MipSet* pMipSet, HDRHeader& Header)
{
    if(!HDR_CMips->AllocateMipLevelData(HDR_CMips->GetMipLevel(pMipSet, 0), Header.nWidth, Header.nHeight, CF_8bit, TDT_ARGB))
        return PE_Unknown;

    pMipSet->m_ChannelFormat = CF_8bit;
    pMipSet->m_TextureDataType = TDT_ARGB;
    pMipSet->m_dwFourCC = 0;
    pMipSet->m_dwFourCC2 = 0;
    pMipSet->m_format = CMP_FORMAT_ARGB_8888;
    pMipSet->m_nMipLevels = 1;

    // Allocate a temporary buffer and read the bitmap data into it
    long lCurrPos = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    long lEndPos = ftell(pFile);
    fseek(pFile, lCurrPos, SEEK_SET);
    CMP_DWORD dwTempSize = lEndPos - lCurrPos;
    unsigned char* pTempData = static_cast<unsigned char*>(malloc(dwTempSize));
    fread(pTempData, dwTempSize, 1, pFile);
    fclose(pFile);

    CMP_DWORD dwPitch = pMipSet->m_nWidth * sizeof(CMP_COLOR);
    CMP_BYTE* pTempPtr = pTempData;

    int nStart, nEnd, nIncrement;
    // Bottom up ?
    if(Header.cFormatFlags & 0x20)
    {
        nStart = 0;
        nEnd = pMipSet->m_nHeight;
        nIncrement = 1;
    }
    else
    {
        nStart = pMipSet->m_nHeight-1;
        nEnd = -1;
        nIncrement = -1;
    }

    for(int j = nStart; j != nEnd; j+= nIncrement)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * dwPitch));

        int nColumn =0;
        while(nColumn < pMipSet->m_nWidth)
        {
            unsigned char nLength = *pTempPtr++;
            if(nLength & 0x80)
            {
                int nRepeat = nLength - 0x7f;
                char nBlue = *pTempPtr++;
                char nGreen = *pTempPtr++;
                char nRed = *pTempPtr++;
                char nAlpha = *pTempPtr++;

                while(nRepeat && nColumn < pMipSet->m_nWidth)
                {
                    *pData++ = nRed;
                    *pData++ = nGreen;
                    *pData++ = nBlue;
                    *pData++ = nAlpha;
                    nColumn++;
                    nRepeat--;
                }

            }
            else
            {
                int nRepeat = nLength+1;
                while(nRepeat && nColumn < pMipSet->m_nWidth)
                {
                    char nBlue = *pTempPtr++;
                    char nGreen = *pTempPtr++;
                    char nRed = *pTempPtr++;
                    char nAlpha = *pTempPtr++;
                    *pData++ = nRed;
                    *pData++ = nGreen;
                    *pData++ = nBlue;
                    *pData++ = nAlpha;
                    nColumn++;
                    nRepeat--;
                }
            }
        }
    }

    free(pTempData);

    return PE_OK;
}

TC_PluginError LoadHDR_RGB888(FILE* pFile, MipSet* pMipSet, HDRHeader& Header)
{
    if(!HDR_CMips->AllocateMipLevelData(HDR_CMips->GetMipLevel(pMipSet, 0), Header.nWidth, Header.nHeight, CF_8bit, TDT_ARGB))
        return PE_Unknown;

    pMipSet->m_ChannelFormat    = CF_8bit;
    pMipSet->m_TextureDataType  = TDT_ARGB;
    pMipSet->m_dwFourCC         = 0;
    pMipSet->m_dwFourCC2        = 0;
    pMipSet->m_nMipLevels       = 1;
    pMipSet->m_format = CMP_FORMAT_ARGB_8888;

    // Allocate a temporary buffer and read the bitmap data into it
    CMP_DWORD dwSize = pMipSet->m_nWidth *  pMipSet->m_nHeight * 3;
    unsigned char* pTempData = static_cast<unsigned char*>(malloc(dwSize));
    fread(pTempData, dwSize, 1, pFile);
    fclose(pFile);

    CMP_BYTE* pTempPtr = pTempData;

    int nStart, nEnd, nIncrement;
    // Bottom up ?
    if(Header.cFormatFlags & 0x20)
    {
        nStart = 0;
        nEnd = pMipSet->m_nHeight;
        nIncrement = 1;
    }
    else
    {
        nStart = pMipSet->m_nHeight-1;
        nEnd = -1;
        nIncrement = -1;
    }

    char nBlue;
    char nGreen;
    char nRed;

    for(int j = nStart; j != nEnd; j+= nIncrement)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * pMipSet->m_nWidth * sizeof(CMP_COLOR)));
        for(int i=0; i < pMipSet->m_nWidth; i++)
        {
            nBlue    = *pTempPtr++;
            nGreen   = *pTempPtr++;
            nRed     = *pTempPtr++;

            *pData++ = nRed;
            *pData++ = nGreen;
            *pData++ = nBlue;
            *pData++ = 0xff;
        }
    }

    free(pTempData);

    return PE_OK;
}

TC_PluginError LoadHDR_RGB888_RLE(FILE* pFile, MipSet* pMipSet, HDRHeader& Header)
{
    if(!HDR_CMips->AllocateMipLevelData(HDR_CMips->GetMipLevel(pMipSet, 0), Header.nWidth, Header.nHeight, CF_8bit, TDT_ARGB))
        return PE_Unknown;

    pMipSet->m_ChannelFormat    = CF_8bit;
    pMipSet->m_TextureDataType  = TDT_ARGB;
    pMipSet->m_dwFourCC         = 0;
    pMipSet->m_dwFourCC2        = 0;
    pMipSet->m_nMipLevels       = 1;
    pMipSet->m_format = CMP_FORMAT_ARGB_8888;

    // Allocate a temporary buffer and read the bitmap data into it
    long lCurrPos = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    long lEndPos = ftell(pFile);
    fseek(pFile, lCurrPos, SEEK_SET);
    CMP_DWORD dwTempSize = lEndPos - lCurrPos;
    unsigned char* pTempData = static_cast<unsigned char*>(malloc(dwTempSize));
    fread(pTempData, dwTempSize, 1, pFile);
    fclose(pFile);

    CMP_BYTE* pTempPtr = pTempData;
    CMP_DWORD dwPitchOut = pMipSet->m_nWidth * sizeof(CMP_COLOR);

    int nStart, nEnd, nIncrement;
    // Bottom up ?
    if(Header.cFormatFlags & 0x20)
    {
        nStart = 0;
        nEnd = pMipSet->m_nHeight;
        nIncrement = 1;
    }
    else
    {
        nStart = pMipSet->m_nHeight-1;
        nEnd = -1;
        nIncrement = -1;
    }

    unsigned char nRepeat = 0;
    unsigned char nLength = 0;
    for(int j = nStart; j != nEnd; j+= nIncrement)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * dwPitchOut));

        int nColumn =0;
        while(nColumn < pMipSet->m_nWidth)
        {
            if(nRepeat == 0)
            {
                nLength = *pTempPtr++;
                nRepeat = (nLength & 0x7f) + 1;
            }

            if(nLength & 0x80)
            {
                char nBlue  = *pTempPtr++;
                char nGreen = *pTempPtr++;
                char nRed   = *pTempPtr++;
                while(nRepeat && nColumn < pMipSet->m_nWidth)
                {
                    *pData++ = nRed;
                    *pData++ = nGreen;
                    *pData++ = nBlue;
                    *pData++ = 0xff;
                    nColumn++;
                    nRepeat--;
                }
            }
            else
            {
                while(nRepeat && nColumn < pMipSet->m_nWidth)
                {
                    char nBlue  = *pTempPtr++;
                    char nGreen = *pTempPtr++;
                    char nRed   = *pTempPtr++;
                    *pData++    = nRed;
                    *pData++    = nGreen;
                    *pData++    = nBlue;
                    *pData++    = 0xff;
                    nColumn++;
                    nRepeat--;
                }
            }
        }
    }

    free(pTempData);

    return PE_OK;
}

TC_PluginError LoadHDR_G8(FILE* pFile, MipSet* pMipSet, HDRHeader& Header)
{
    if(!HDR_CMips->AllocateCompressedMipLevelData(HDR_CMips->GetMipLevel(pMipSet, 0), Header.nWidth, Header.nHeight, Header.nWidth * Header.nHeight))
        return PE_Unknown;

    pMipSet->m_ChannelFormat        = CF_Compressed;
    pMipSet->m_TextureDataType      = TDT_XRGB;
    pMipSet->m_dwFourCC             = MAKEFOURCC('G', '8', ' ', ' ');
    pMipSet->m_dwFourCC2            = 0;
    pMipSet->m_nMipLevels           = 1;

    // Allocate a temporary buffer and read the bitmap data into it
    CMP_DWORD dwSize = pMipSet->m_nWidth *  pMipSet->m_nHeight * sizeof(CMP_BYTE);
    unsigned char* pTempData = static_cast<unsigned char*>(malloc(dwSize));
    fread(pTempData, dwSize, 1, pFile);
    fclose(pFile);

    CMP_BYTE* pTempPtr = pTempData;

    int nStart, nEnd, nIncrement;
    // Bottom up ?
    if(Header.cFormatFlags & 0x20)
    {
        nStart = 0;
        nEnd = pMipSet->m_nHeight;
        nIncrement = 1;
    }
    else
    {
        nStart = pMipSet->m_nHeight-1;
        nEnd = -1;
        nIncrement = -1;
    }

    for(int j = nStart; j != nEnd; j+= nIncrement)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * pMipSet->m_nWidth));
        memcpy(pData, pTempPtr, pMipSet->m_nWidth);
        pTempPtr += pMipSet->m_nWidth;
    }

    free(pTempData);

    return PE_OK;
}

TC_PluginError LoadHDR_G8_RLE(FILE* pFile, MipSet* pMipSet, HDRHeader& Header)
{
    if(!HDR_CMips->AllocateMipLevelData(HDR_CMips->GetMipLevel(pMipSet, 0), Header.nWidth, Header.nHeight, CF_8bit, TDT_ARGB))
        return PE_Unknown;

    pMipSet->m_ChannelFormat    = CF_8bit;
    pMipSet->m_TextureDataType  = TDT_ARGB;
    pMipSet->m_dwFourCC         = 0;
    pMipSet->m_dwFourCC2        = 0;
    pMipSet->m_nMipLevels       = 1;
    pMipSet->m_format           = CMP_FORMAT_ARGB_8888;

    // Allocate a temporary buffer and read the bitmap data into it
    long lCurrPos = ftell(pFile);
    fseek(pFile, 0, SEEK_END);
    long lEndPos = ftell(pFile);
    fseek(pFile, lCurrPos, SEEK_SET);
    CMP_DWORD dwTempSize = lEndPos - lCurrPos;
    unsigned char* pTempData = static_cast<unsigned char*>(malloc(dwTempSize));
    fread(pTempData, dwTempSize, 1, pFile);
    fclose(pFile);

    CMP_BYTE* pTempPtr = pTempData;

    int nStart, nEnd, nIncrement;
    // Bottom up ?
    if(Header.cFormatFlags & 0x20)
    {
        nStart = 0;
        nEnd = pMipSet->m_nHeight;
        nIncrement = 1;
    }
    else
    {
        nStart = pMipSet->m_nHeight-1;
        nEnd = -1;
        nIncrement = -1;
    }

    for(int j = nStart; j != nEnd; j+= nIncrement)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * pMipSet->m_nWidth * sizeof(CMP_COLOR)));

        int nColumn =0;
        while(nColumn < pMipSet->m_nWidth)
        {
            unsigned char nLength = *pTempPtr++;
            if(nLength & 0x80)
            {
                int nRepeat = nLength - 0x7f;
                CMP_BYTE gray = *pTempPtr++;
                while(nRepeat && nColumn < pMipSet->m_nWidth)
                {
                    *pData++ = gray;
                    *pData++ = gray;
                    *pData++ = gray;
                    *pData++ = 0xff;
                    nColumn++;
                    nRepeat--;
                }
            }
            else
            {
                int nRepeat = nLength+1;
                CMP_BYTE gray = *pTempPtr++;
                while(nRepeat && nColumn < pMipSet->m_nWidth)
                {
                    *pData++ = gray;
                    *pData++ = gray;
                    *pData++ = gray;
                    *pData++ = 0xff;
                    nColumn++;
                    nRepeat--;
                }
            }
        }
    }

    free(pTempData);

    return PE_OK;
}

CMP_BYTE CalcRawLength2(CMP_BYTE* pThis, CMP_BYTE* pEnd, int nSize, int nOffset)
{
    CMP_BYTE* pNext = pThis + nOffset;
    CMP_BYTE cRawLength = 1;

    while(pNext < pEnd && memcmp(pThis, pNext, nSize) != 0 && cRawLength < 0x80)
    {
        cRawLength++;
        pThis += nOffset;
        pNext += nOffset;
    }

    return cRawLength;
}


// *** NOTE ****
// The Save data code needs to be rewored as 
// HDR is saved as BGRA and our MipSet is using RGBA
//
// Current implementation is saving in wronge format as RGBA 
//
// Need to Fix this

void SaveLineRLE2(FILE* pFile, CMP_BYTE* pThis, CMP_BYTE* pEnd, int nSize, int nOffset)
{
    while (pThis < pEnd)
    {
        CMP_BYTE* pNext = pThis + nOffset;
        // Are the next two pixel the same ?
        if(pNext < pEnd && memcmp(pThis, pNext, nSize) == 0)
        {
            CMP_BYTE cRunLength = 0;
            CMP_BYTE cRepetitionCount = (cRunLength - 1) | 0x80;
            fwrite(&cRepetitionCount, sizeof(cRepetitionCount), 1, pFile);
            fwrite(pThis, nSize, 1, pFile);
            pThis += (cRunLength * nOffset);
        }
        else
        {
            CMP_BYTE cRawLength = CalcRawLength2(pThis, pEnd, nSize, nOffset);
            CMP_BYTE cRepetitionCount = (cRawLength - 1);
            fwrite(&cRepetitionCount, sizeof(cRepetitionCount), 1, pFile);
            if(nSize == nOffset)
            {
                fwrite(pThis, nSize*cRawLength, 1, pFile);
                pThis += (cRawLength * nOffset);
            }
            else
            {
                for(CMP_BYTE i = 0; i < cRawLength; i++)
                {
                    fwrite(pThis, nSize, 1, pFile);
                    pThis += nOffset;
                }
            }
        }
    }
}

TC_PluginError SaveRLE2(FILE* pFile, const MipSet* pMipSet, int nSize, int nOffset)
{
    CMP_DWORD dwPitch = pMipSet->m_nWidth * nOffset;
    for(int j=pMipSet->m_nHeight-1; j>=0; j--)
    {
        CMP_BYTE* pThis = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * dwPitch));
        CMP_BYTE* pEnd = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + ((j+1) * dwPitch));

        SaveLineRLE2(pFile, pThis, pEnd, nSize, nOffset);
    }

    fclose(pFile);

    return PE_OK;
}

TC_PluginError SaveHDR_ARGB8888(FILE* pFile, const MipSet* pMipSet)
{
    CMP_DWORD dwPitch = pMipSet->m_nWidth * 4;

    if (pMipSet->m_swizzle)
    {
        CMP_BYTE RGBA[4];
        for (int j = pMipSet->m_nHeight - 1; j >= 0; j--)
        {
            CMP_BYTE* pData = (CMP_BYTE*)(HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * dwPitch));
            for (int i = 0; i < pMipSet->m_nWidth; i++)
            {
                RGBA[2] = (CMP_BYTE)*pData++;
                RGBA[1] = (CMP_BYTE)*pData++;
                RGBA[0] = (CMP_BYTE)*pData++;
                RGBA[3] = (CMP_BYTE)*pData++;
                fwrite(RGBA, 4, 1, pFile);
            }
        }
    }
    else
    {
        for (int j = pMipSet->m_nHeight - 1; j >= 0; j--)
        {
            CMP_BYTE* pData = (CMP_BYTE*)(HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * dwPitch));
            fwrite(pData, dwPitch, 1, pFile);
        }
    }

    fclose(pFile);


    return PE_OK;
}

TC_PluginError SaveHDR_ARGB8888_RLE(FILE* pFile, const MipSet* pMipSet)
{
    return SaveRLE2(pFile, pMipSet, sizeof(CMP_COLOR), sizeof(CMP_COLOR));
}

TC_PluginError SaveHDR_RGB888(FILE* pFile, const MipSet* pMipSet)
{
    CMP_DWORD dwPitch = pMipSet->m_nWidth * sizeof(CMP_COLOR);
    for(int j=pMipSet->m_nHeight-1; j>=0; j--)
        //    for(int j=0; j < pMipSet->m_nHeight-1; j++)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * dwPitch));
        for(int i=0; i < pMipSet->m_nWidth; i++)
        {
            fwrite(pData, 3, 1, pFile);
            pData += sizeof(CMP_COLOR);
        }
    }

    fclose(pFile);

    return PE_OK;
}

TC_PluginError SaveHDR_RGB888_RLE(FILE* pFile, const MipSet* pMipSet)
{
    return SaveRLE2(pFile, pMipSet, 3, sizeof(CMP_COLOR));
}

TC_PluginError SaveHDR_G8(FILE* pFile, const MipSet* pMipSet)
{
    for(int j=pMipSet->m_nHeight-1; j>=0; j--)
    {
        CMP_BYTE* pData = (CMP_BYTE*) (HDR_CMips->GetMipLevel(pMipSet, 0)->m_pbData + (j * pMipSet->m_nWidth));
        fwrite(pData, pMipSet->m_nWidth, 1, pFile);
    }

    fclose(pFile);

    return PE_OK;
}

TC_PluginError SaveHDR_G8_RLE(FILE* pFile, const MipSet* pMipSet)
{
    return SaveRLE2(pFile, pMipSet, sizeof(CMP_BYTE), sizeof(CMP_BYTE));
}


// ------------ Registry!
#define DEFAULT_JPG_QUALITY 75
const char szKeyHDRPlugin[] = ("Software\\ATI Research, Inc.\\TextureAPI\\Plugins\\HDR");

void LoadRegistryKeyDefaults(HDR_FileSaveParams* pParams)
{
    memset(pParams, 0, sizeof(HDR_FileSaveParams));
    pParams->dwSize = sizeof(HDR_FileSaveParams);

    pParams->bRLECompressed = true;
}

void LoadRegistryKeys(HDR_FileSaveParams* pParams)
{
#ifdef _WIN32
    LoadRegistryKeyDefaults(pParams);

    HKEY hKey;
    CMP_DWORD dwSize;
    RegOpenKeyEx(HKEY_CURRENT_USER, szKeyHDRPlugin, 0, KEY_ALL_ACCESS, &hKey);

    CMP_DWORD dwTemp;
    if(RegQueryValueEx(hKey, ("RLECompressed"), NULL, NULL, (CMP_BYTE*)&dwTemp, (LPDWORD)&dwSize) != ERROR_SUCCESS)
        pParams->bRLECompressed = dwTemp ? true : false;

    RegCloseKey(hKey);
#endif
}

void SaveRegistryKeys(const HDR_FileSaveParams* pParams)
{
#ifdef _WIN32
    HKEY hKey;
    RegCreateKeyEx(HKEY_CURRENT_USER, szKeyHDRPlugin, 0, (""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    CMP_DWORD dwTemp;
    dwTemp = pParams->bRLECompressed ? 1 : 0;
    RegSetValueEx(hKey, ("RLECompressed"), NULL, REG_DWORD, (CMP_BYTE*) &dwTemp, sizeof(CMP_DWORD));

    RegCloseKey(hKey);
#endif
}
