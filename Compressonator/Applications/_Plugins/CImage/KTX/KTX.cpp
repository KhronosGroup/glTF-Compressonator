//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//=====================================================================
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


#ifdef _WIN32
#include "stdafx.h"
#endif

#include <stdio.h>
#include <algorithm>
#include "cKTX.h"

#include <stdio.h>
#include <stdlib.h>
#include "TC_PluginAPI.h"
#include "TC_PluginInternal.h"
#include "MIPS.h"

#include "softfloat.h"
#ifndef _WIN32
#include "TextureIO.h"
#endif

// Feature is been removed as of Aug 2016
// #define CMP_Texture_IO_SUPPORTED

#pragma comment(lib, "opengl32.lib")        // Open GL
#pragma comment(lib, "Glu32.lib")           // Glu 
#pragma comment(lib, "glew32.lib")          // glew 1.13.0

using namespace std;

CMIPS *KTX_CMips;

#ifdef BUILD_AS_PLUGIN_DLL
DECLARE_PLUGIN(Plugin_KTX)
SET_PLUGIN_TYPE("IMAGE")
SET_PLUGIN_NAME("KTX")
#else
void *make_Plugin_KTX() { return new Plugin_KTX; } 
#endif

uint32_t Endian_Conversion(uint32_t dword)
{
    return (((dword>>24)&0x000000FF) | ((dword>>8)&0x0000FF00) | ((dword<<8)&0x00FF0000) | ((dword<<24)&0xFF000000));
}

Plugin_KTX::Plugin_KTX()
{ 
}

Plugin_KTX::~Plugin_KTX()
{ 
}

int Plugin_KTX::TC_PluginSetSharedIO(void* Shared)
{
    if (Shared)
    {
        KTX_CMips = static_cast<CMIPS *>(Shared);
        return 0;
    }
    return 1;
}

int Plugin_KTX::TC_PluginGetVersion(TC_PluginVersion* pPluginVersion)
{ 
#ifdef _WIN32
    pPluginVersion->guid                    = g_GUID;
#endif
    pPluginVersion->dwAPIVersionMajor        = TC_API_VERSION_MAJOR;
    pPluginVersion->dwAPIVersionMinor        = TC_API_VERSION_MINOR;
    pPluginVersion->dwPluginVersionMajor    = TC_PLUGIN_VERSION_MAJOR;
    pPluginVersion->dwPluginVersionMinor    = TC_PLUGIN_VERSION_MINOR;
    return 0;
}

int Plugin_KTX::TC_PluginFileLoadTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
    return -1;
}

int Plugin_KTX::TC_PluginFileSaveTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
    return -1;
}

int Plugin_KTX::TC_PluginFileLoadTexture(const char* pszFilename, MipSet* pMipSet)
{
    ktxTexture1* texture = nullptr;

    KTX_error_code loadStatus = ktxTexture1_CreateFromNamedFile(pszFilename, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);
    if (loadStatus != KTX_SUCCESS)
    {
        if (KTX_CMips)
        {
            KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) opening file = %s \n"), EL_Error, IDS_ERROR_FILE_OPEN, pszFilename);
        }
        return -1;
    }

    if (texture->isCompressed)
    {
        pMipSet->m_compressed   = true;
        pMipSet->m_nBlockHeight = 4;
        pMipSet->m_nBlockWidth  = 4;
        pMipSet->m_nBlockDepth  = 1;
        pMipSet->m_ChannelFormat = CF_Compressed;

        switch (texture->glInternalformat)
        {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            pMipSet->m_format = CMP_FORMAT_BC1;
            pMipSet->m_TextureDataType = TDT_XRGB;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            pMipSet->m_format = CMP_FORMAT_BC1;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            pMipSet->m_format = CMP_FORMAT_BC2;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            pMipSet->m_format = CMP_FORMAT_BC3;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case RGB_BP_UNorm:
            pMipSet->m_format = CMP_FORMAT_BC7;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case R_ATI1N_UNorm:
            pMipSet->m_format = CMP_FORMAT_ATI1N;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case R_ATI1N_SNorm:
            pMipSet->m_format = CMP_FORMAT_ATI2N;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case RG_ATI2N_UNorm:
            pMipSet->m_format = CMP_FORMAT_ATI2N_XY;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case RG_ATI2N_SNorm:
            pMipSet->m_format = CMP_FORMAT_ATI2N_DXT5;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case RGB_BP_UNSIGNED_FLOAT:
            pMipSet->m_format = CMP_FORMAT_BC6H;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case RGB_BP_SIGNED_FLOAT:
            pMipSet->m_format = CMP_FORMAT_BC6H_SF;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case ATC_RGB_AMD:
            pMipSet->m_format = CMP_FORMAT_ATC_RGB;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case ATC_RGBA_EXPLICIT_ALPHA_AMD:
            pMipSet->m_format = CMP_FORMAT_ATC_RGBA_Explicit;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case ATC_RGBA_INTERPOLATED_ALPHA_AMD: 
            pMipSet->m_format = CMP_FORMAT_ATC_RGBA_Interpolated;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 4;
            pMipSet->m_nBlockHeight = 4;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 5;
            pMipSet->m_nBlockHeight = 4;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 5;
            pMipSet->m_nBlockHeight = 5;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 6;
            pMipSet->m_nBlockHeight = 5;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 6;
            pMipSet->m_nBlockHeight = 6;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 8;
            pMipSet->m_nBlockHeight = 5;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 8;
            pMipSet->m_nBlockHeight = 6;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 10;
            pMipSet->m_nBlockHeight = 5;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 10;
            pMipSet->m_nBlockHeight = 6;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 8;
            pMipSet->m_nBlockHeight = 8;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 10;
            pMipSet->m_nBlockHeight = 8;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 10;
            pMipSet->m_nBlockHeight = 10;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 12;
            pMipSet->m_nBlockHeight = 10;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
            pMipSet->m_format = CMP_FORMAT_ASTC;
            pMipSet->m_nBlockWidth = 12;
            pMipSet->m_nBlockHeight = 12;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case ETC1_RGB8_OES: 
            pMipSet->m_format = CMP_FORMAT_ETC_RGB;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case GL_COMPRESSED_RGB8_ETC2:
            pMipSet->m_format = CMP_FORMAT_ETC2_RGB;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case COMPRESSED_FORMAT_DXT5_RxBG :
            pMipSet->m_format = CMP_FORMAT_DXT5_RxBG;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case COMPRESSED_FORMAT_DXT5_RBxG :
            pMipSet->m_format = CMP_FORMAT_DXT5_RBxG;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case COMPRESSED_FORMAT_DXT5_xRBG :
            pMipSet->m_format = CMP_FORMAT_DXT5_xRBG;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case COMPRESSED_FORMAT_DXT5_RGxB :
            pMipSet->m_format = CMP_FORMAT_DXT5_RGxB;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        case COMPRESSED_FORMAT_DXT5_xGxR :
            pMipSet->m_format = CMP_FORMAT_DXT5_xGxR;
            pMipSet->m_TextureDataType = TDT_ARGB;
            break;
        default:
            if (KTX_CMips)
            {
                KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) unsupported internl GL format %x\n"), EL_Error, IDS_ERROR_UNSUPPORTED_TYPE, texture->glInternalformat);
            }
            return -1;
        }
    }
    else
    {
        pMipSet->m_compressed = false;
        switch (texture->glType)
        {
        case GL_UNSIGNED_BYTE:
            pMipSet->m_ChannelFormat = CF_8bit;
            switch (texture->glFormat)
            {
            case GL_RED:
                pMipSet->m_format = CMP_FORMAT_R_8;
                pMipSet->m_TextureDataType = TDT_R;
                break;
            case GL_RG:
                pMipSet->m_format = CMP_FORMAT_RG_8;
                pMipSet->m_TextureDataType = TDT_RG;
                break;
            case GL_RGB:
                pMipSet->m_format = CMP_FORMAT_RGB_888;
                pMipSet->m_TextureDataType = TDT_XRGB;
                break;
            case GL_RGBA:
            case GL_RGBA8:
                pMipSet->m_format = CMP_FORMAT_ARGB_8888;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            case GL_BGR:
                pMipSet->m_swizzle = true;
                pMipSet->m_format = CMP_FORMAT_RGB_888;
                pMipSet->m_TextureDataType = TDT_XRGB;
                break;
            case GL_BGRA:
                pMipSet->m_swizzle = true;
                pMipSet->m_format = CMP_FORMAT_ARGB_8888;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            }
            break;
        case GL_UNSIGNED_SHORT:
            pMipSet->m_ChannelFormat = CF_16bit;
            switch (texture->glFormat)
            {
            case GL_RED:
                pMipSet->m_format = CMP_FORMAT_R_16;
                pMipSet->m_TextureDataType = TDT_R;
                break;
            case GL_RG:
                pMipSet->m_format = CMP_FORMAT_RG_16;
                pMipSet->m_TextureDataType = TDT_RG;
                break;
            case GL_RGBA:
                pMipSet->m_format = CMP_FORMAT_ARGB_16;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            case GL_BGRA:
                pMipSet->m_swizzle = true;
                pMipSet->m_format = CMP_FORMAT_ARGB_16;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            }
            break;
        case GL_HALF_FLOAT:
            pMipSet->m_ChannelFormat = CF_Float16;
            switch (texture->glFormat)
            {
            case GL_RED:
                pMipSet->m_format = CMP_FORMAT_R_16F;
                pMipSet->m_TextureDataType = TDT_R;
                break;
            case GL_RG:
                pMipSet->m_format = CMP_FORMAT_RG_16F;
                pMipSet->m_TextureDataType = TDT_RG;
                break;
            case GL_RGBA:
                pMipSet->m_format = CMP_FORMAT_ARGB_16F;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            case GL_BGRA:
                pMipSet->m_swizzle = true;
                pMipSet->m_format = CMP_FORMAT_ARGB_16F;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            }
            break;
        case GL_UNSIGNED_INT_2_10_10_10_REV:
            pMipSet->m_format = CMP_FORMAT_ARGB_2101010;
            pMipSet->m_TextureDataType = TDT_ARGB;
            pMipSet->m_ChannelFormat = CF_2101010;
            break;
        case GL_FLOAT:
            pMipSet->m_ChannelFormat = CF_Float32;
            switch (texture->glFormat)
            {
            case GL_RED:
                pMipSet->m_format = CMP_FORMAT_R_32F;
                pMipSet->m_TextureDataType = TDT_R;
                break;
            case GL_RG:
                pMipSet->m_format = CMP_FORMAT_RG_32F;
                pMipSet->m_TextureDataType = TDT_RG;
                break;
            case GL_RGBA:
                pMipSet->m_format = CMP_FORMAT_ARGB_32F;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            case GL_BGRA:
                pMipSet->m_swizzle = true;
                pMipSet->m_format = CMP_FORMAT_ARGB_32F;
                pMipSet->m_TextureDataType = TDT_ARGB;
                break;
            }
            break;
            break;
        default:
            if (KTX_CMips)
            {
                KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) unsupported GL format %x\n"), EL_Error, IDS_ERROR_UNSUPPORTED_TYPE, texture->glFormat);
            }
            return -1;
        }
    }

    if (texture->isCubemap)
    {
        pMipSet->m_TextureType = TT_CubeMap;
    }
    else if (texture->baseDepth > 0 && texture->numFaces == 1)
    {
        pMipSet->m_TextureType = TT_VolumeTexture;
    }
    else if (texture->baseDepth== 0 && texture->numFaces == 1)
    {
        pMipSet->m_TextureType = TT_2D;
    }
    else
    {
        if (KTX_CMips)
        {
            KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) unsupported texture format\n"), EL_Error, IDS_ERROR_UNSUPPORTED_TYPE);
        }
        return -1;
    }

    pMipSet->m_nMipLevels = texture->numLevels;

    // Allocate MipSet header
    KTX_CMips->AllocateMipSet(pMipSet,
        pMipSet->m_ChannelFormat,
        pMipSet->m_TextureDataType,
        pMipSet->m_TextureType,
        texture->baseWidth,
        texture->baseHeight,
        1
#ifdef USE_MIPSET_FACES
        , texture->numFaces
#endif
    );

    int w = pMipSet->m_nWidth;
    int h = pMipSet->m_nHeight;

    unsigned int totalByteRead=0;

    unsigned int faceSize = 0;
    unsigned int faceSizeRounded = 0;
    unsigned int numArrayElement = texture->numLayers;

    for (int nMipLevel = 0; nMipLevel < texture->numLevels; nMipLevel++)
    {
        if ((w <= 1) || (h <= 1))
        {
            break;
        }
        else
        {
            w = std::max(1, pMipSet->m_nWidth >> nMipLevel);
            h = std::max(1, pMipSet->m_nHeight >> nMipLevel);
        }

        faceSizeRounded = (w + 3) & ~(khronos_uint32_t)3;

        for (int face = 0; face < texture->numFaces; ++face)
        {
            // Determine buffer size and set Mip Set Levels 
            MipLevel *pMipLevel = KTX_CMips->GetMipLevel(pMipSet, nMipLevel, face);

            int channelSize = 0;
            int channelCount = 0;

            if (pMipSet->m_compressed)
            {
                KTX_CMips->AllocateCompressedMipLevelData(pMipLevel, w, h, faceSizeRounded);
            }
            else
            {
                switch (texture->glType)
                {
                    case GL_UNSIGNED_BYTE:
                        channelSize = 1;
                        break;
                    case GL_UNSIGNED_SHORT:
                        channelSize = 2;
                        break;
                    case GL_HALF_FLOAT:
                        channelSize = 2;
                        break;
                    case GL_FLOAT:
                        channelSize = 4;
                        break;
                    default:
                        return -1;
                }

                switch (texture->glFormat)
                {
                case GL_RED:
                    channelCount = 1;
                    break;
                case GL_RG:
                    channelCount = 2;
                    break;
                case GL_RGB:
                    channelCount = 3;
                    break;
                case GL_BGR:
                    channelCount = 3;
                    break;
                case GL_RGBA:
                    channelCount = 4;
                    break;
                case GL_BGRA:
                    channelCount = 4;
                    break;
                default:
                    return -1;
                }

                KTX_CMips->AllocateMipLevelData(pMipLevel, w, h, pMipSet->m_ChannelFormat, pMipSet->m_TextureDataType);
            }

            CMP_BYTE* pData = (CMP_BYTE*)(pMipLevel->m_pbData);

            if (!pData)
            {
                if (KTX_CMips)
                {
                    KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) Read image data failed, Out of Memory. Format %x\n"), EL_Error, IDS_ERROR_UNSUPPORTED_TYPE, texture->glFormat);
                }
                return -1;
            }
          
            //
            // Read image data into temporary buffer
            //

            ktx_size_t offset = 0;
            KTX_error_code dataStatus = ktxTexture_GetImageOffset(ktxTexture(texture), nMipLevel, 0, face, &offset);
            uint8_t* imageData = ktxTexture_GetData(ktxTexture(texture)) + offset;

            if (!pMipSet->m_compressed)
            {
                size_t readSize = channelSize * channelCount * w * h;
                std::vector<uint8_t> pixelData(readSize);

                memcpy(&pixelData[0], imageData, readSize);

                int pixelSize = channelCount * channelSize;
                int targetPixelSize = channelCount * channelSize;
                if (channelCount == 3)
                {
                    // XRGB conversion.
                    targetPixelSize = 4 * channelSize;
                }

                int py = 0;
                for (py = 0; py < h; py++)
                {
                    int px = 0;
                    for (px = 0; px < w; px++)
                    {
                        memcpy(&pData[targetPixelSize * px + py * targetPixelSize * w], &pixelData[pixelSize * px + py * pixelSize * w], pixelSize);
                    }
                }
            }
            else
            {
                memcpy(pData, imageData, faceSizeRounded);
            }
        }
    }

    return 0;
}

int Plugin_KTX::TC_PluginFileSaveTexture(const char* pszFilename, MipSet* pMipSet)
{
    assert(pszFilename);
    assert(pMipSet);

    if (pMipSet->m_pMipLevelTable == NULL)
    {
        if (KTX_CMips)
            KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) saving file = %s "), EL_Error, IDS_ERROR_ALLOCATEMIPSET, pszFilename);
        return -1;
    }

    if (KTX_CMips->GetMipLevel(pMipSet, 0) == NULL)
    {
        if (KTX_CMips)
            KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) saving file = %s "), EL_Error, IDS_ERROR_ALLOCATEMIPSET, pszFilename);
        return -1;
    }

    //

    ktxTextureCreateInfo textureCreateInfo;
                                                                                    /*!< Internal format for the texture, e.g., GL_RGB8. Ignored when creating a ktxTexture2. */
    textureCreateInfo.baseWidth = pMipSet->m_nWidth;                                /*!< Width of the base level of the texture. */
    textureCreateInfo.baseHeight = pMipSet->m_nWidth;                               /*!< Height of the base level of the texture. */
    textureCreateInfo.baseDepth = 1;                                                /*!< Depth of the base level of the texture. */
    textureCreateInfo.numDimensions = 2;                                            /*!< Number of dimensions in the texture, 1, 2 or 3. */
    textureCreateInfo.numLevels = pMipSet->m_nMipLevels;                            /*!< Number of mip levels in the texture. Should be 1 if @c generateMipmaps is KTX_TRUE; */
    textureCreateInfo.numLayers = 1;                                                /*!< Number of array layers in the texture. */
    textureCreateInfo.numFaces = (pMipSet->m_TextureType == TT_CubeMap) ? 6 : 1;    /*!< Number of faces: 6 for cube maps, 1 otherwise. */
    textureCreateInfo.isArray = KTX_FALSE;                                          /*!< Set to KTX_TRUE if the texture is to be an array texture. Means OpenGL will use a GL_TEXTURE_*_ARRAY target. */
    textureCreateInfo.generateMipmaps = KTX_FALSE;                                  /*!< Set to KTX_TRUE if mipmaps should be generated for the texture when loading into a 3D API. */

    bool isCompressed = false;

    switch (pMipSet->m_format)
    {
    //uncompressed format case
    case CMP_FORMAT_ARGB_8888 :
    case CMP_FORMAT_RGB_888 :
    case CMP_FORMAT_RG_8 :
    case CMP_FORMAT_R_8 :
        isCompressed = false;
        break;
    case  CMP_FORMAT_ARGB_2101010 :      
        break;
    case  CMP_FORMAT_ARGB_16 :   
        break;
    case  CMP_FORMAT_RG_16 :
        break;
    case  CMP_FORMAT_R_16 :      
        break;
    case  CMP_FORMAT_ARGB_16F :                 
    case  CMP_FORMAT_RG_16F :                   
    case  CMP_FORMAT_R_16F : 
        break;
    case  CMP_FORMAT_ARGB_32F :                 
    case  CMP_FORMAT_RGB_32F :                  
    case  CMP_FORMAT_RG_32F :                   
    case  CMP_FORMAT_R_32F :
        break;
    //compressed format case
    case  CMP_FORMAT_ATI1N :                   
    case  CMP_FORMAT_ATI2N :                   
    case  CMP_FORMAT_ATI2N_XY :                
    case  CMP_FORMAT_ATI2N_DXT5 :              
    case  CMP_FORMAT_ATC_RGB :                 
    case  CMP_FORMAT_ATC_RGBA_Explicit :       
    case  CMP_FORMAT_ATC_RGBA_Interpolated :   
    case  CMP_FORMAT_BC1 :                     
    case  CMP_FORMAT_BC2 :                     
    case  CMP_FORMAT_BC3 :                     
    case  CMP_FORMAT_BC4 :                     
    case  CMP_FORMAT_BC5 :                     
    case  CMP_FORMAT_BC6H :  
    case  CMP_FORMAT_BC6H_SF:
    case  CMP_FORMAT_BC7 :                     
    case  CMP_FORMAT_DXT1 :                    
    case  CMP_FORMAT_DXT3 :                    
    case  CMP_FORMAT_DXT5 :                    
    case  CMP_FORMAT_DXT5_xGBR :               
    case  CMP_FORMAT_DXT5_RxBG :               
    case  CMP_FORMAT_DXT5_RBxG :               
    case  CMP_FORMAT_DXT5_xRBG :               
    case  CMP_FORMAT_DXT5_RGxB :               
    case  CMP_FORMAT_DXT5_xGxR :               
    case  CMP_FORMAT_ETC_RGB :                 
    case  CMP_FORMAT_ETC2_RGB:
    case  CMP_FORMAT_ASTC :
    case  CMP_FORMAT_GT:
        isCompressed            = true;
        break;
    //default case
    default:
        isCompressed = false;
        break;
    }

    switch (pMipSet->m_TextureDataType)
    {
    case TDT_R:  //single component-- can be Luminance and Alpha case, here only cover R
    {
        if (!isCompressed) 
        {
            textureCreateInfo.glInternalformat = GL_RED;
        }
        else
        {
            textureCreateInfo.glInternalformat = GL_RED;
        }
    }
    break;
    case TDT_RG:  //two component
    {
        if (!isCompressed)
        {
            textureCreateInfo.glInternalformat = GL_RG;
        }
        else
        {
            textureCreateInfo.glInternalformat = GL_COMPRESSED_RG;
        }
    }
    break;
    case TDT_XRGB:  //normally 3 component
    {
        if (!isCompressed)
        {
            textureCreateInfo.glInternalformat = GL_RGB;
        }
        else
        {
            if (pMipSet->m_format == CMP_FORMAT_BC1 || pMipSet->m_format == CMP_FORMAT_DXT1)
            {
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            }
            else
            {
                textureCreateInfo.glInternalformat = GL_RGB;
            }
        }
    }
    break;
    case TDT_ARGB:  //4 component
    {
        if (!isCompressed)
        {
            textureCreateInfo.glInternalformat = GL_RGBA8;
        }
        else
        {
            switch (pMipSet->m_format)
            {
            case CMP_FORMAT_BC1:
            case CMP_FORMAT_DXT1:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                break;
            case CMP_FORMAT_BC2:
            case CMP_FORMAT_DXT3:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
            case CMP_FORMAT_BC3:
            case CMP_FORMAT_DXT5:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
            case CMP_FORMAT_BC7:
                textureCreateInfo.glInternalformat = RGB_BP_UNorm;
                break;
            case  CMP_FORMAT_ATI1N:
                textureCreateInfo.glInternalformat = R_ATI1N_UNorm;
                break;
            case  CMP_FORMAT_ATI2N:
                textureCreateInfo.glInternalformat = R_ATI1N_SNorm;
                break;
            case  CMP_FORMAT_ATI2N_XY:
                textureCreateInfo.glInternalformat = RG_ATI2N_UNorm;
                break;
            case  CMP_FORMAT_ATI2N_DXT5:
                textureCreateInfo.glInternalformat = RG_ATI2N_SNorm;
                break;
            case  CMP_FORMAT_ATC_RGB:
                textureCreateInfo.glInternalformat = ATC_RGB_AMD;
                break;
            case  CMP_FORMAT_ATC_RGBA_Explicit:
                textureCreateInfo.glInternalformat = ATC_RGBA_EXPLICIT_ALPHA_AMD;
                break;
            case  CMP_FORMAT_ATC_RGBA_Interpolated:
                textureCreateInfo.glInternalformat = ATC_RGBA_INTERPOLATED_ALPHA_AMD;
                break;
            case  CMP_FORMAT_BC4:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RED_RGTC1;
                break;
            case  CMP_FORMAT_BC5:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RG_RGTC2;
                break;
            case  CMP_FORMAT_BC6H:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
                break;
            case  CMP_FORMAT_BC6H_SF:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
                break;
            case  CMP_FORMAT_ASTC:
                if ((pMipSet->m_nBlockWidth == 4) && (pMipSet->m_nBlockHeight == 4))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 5) && (pMipSet->m_nBlockHeight == 4))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 5) && (pMipSet->m_nBlockHeight == 5))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 6) && (pMipSet->m_nBlockHeight == 5))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 6) && (pMipSet->m_nBlockHeight == 6))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 8) && (pMipSet->m_nBlockHeight == 5))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 8) && (pMipSet->m_nBlockHeight == 6))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 8) && (pMipSet->m_nBlockHeight == 8))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 10) && (pMipSet->m_nBlockHeight == 5))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 10) && (pMipSet->m_nBlockHeight == 6))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 10) && (pMipSet->m_nBlockHeight == 8))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 10) && (pMipSet->m_nBlockHeight == 10))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 12) && (pMipSet->m_nBlockHeight == 10))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
                }
                else if ((pMipSet->m_nBlockWidth == 12) && (pMipSet->m_nBlockHeight == 12))
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
                }
                else
                {
                    textureCreateInfo.glInternalformat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
                }
                break;
            case CMP_FORMAT_ETC_RGB:
                textureCreateInfo.glInternalformat = ETC1_RGB8_OES;
                break;
            case CMP_FORMAT_ETC2_RGB:
                textureCreateInfo.glInternalformat = GL_COMPRESSED_RGB8_ETC2;
                break;
            case CMP_FORMAT_DXT5_xGBR:
                textureCreateInfo.glInternalformat = COMPRESSED_FORMAT_DXT5_xGBR;
                break;
            case CMP_FORMAT_DXT5_RxBG:
                textureCreateInfo.glInternalformat = COMPRESSED_FORMAT_DXT5_RxBG;
                break;
            case CMP_FORMAT_DXT5_RBxG:
                textureCreateInfo.glInternalformat = COMPRESSED_FORMAT_DXT5_RBxG;
                break;
            case CMP_FORMAT_DXT5_xRBG:
                textureCreateInfo.glInternalformat = COMPRESSED_FORMAT_DXT5_xRBG;
                break;
            case CMP_FORMAT_DXT5_RGxB:
                textureCreateInfo.glInternalformat = COMPRESSED_FORMAT_DXT5_RGxB;
                break;
            case CMP_FORMAT_DXT5_xGxR:
                textureCreateInfo.glInternalformat = COMPRESSED_FORMAT_DXT5_xGxR;
                break;
            }
        }
    }
    break;
    default:
        break;
    }

    //


    ktxTexture1* texture = nullptr;
    KTX_error_code createStatus = ktxTexture1_Create(&textureCreateInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
    if (createStatus != KTX_SUCCESS)
    {
        if (KTX_CMips)
        {
            KTX_CMips->PrintError(("Error(%d): KTX Plugin ID(%d) saving file = %s "), EL_Error, IDS_ERROR_FILE_OPEN, pszFilename);
        }
        return -1;
    }

    int nSlices = (pMipSet->m_TextureType == TT_2D) ? 1 : MaxFacesOrSlices(pMipSet, 0);
    for (int nSlice = 0; nSlice < nSlices; nSlice++)
    {
        for (int nMipLevel = 0; nMipLevel < pMipSet->m_nMipLevels; nMipLevel++)
        {
            KTX_error_code setMemory = ktxTexture_SetImageFromMemory(ktxTexture(texture), nMipLevel, 0, nSlice, KTX_CMips->GetMipLevel(pMipSet, nMipLevel, nSlice)->m_pbData, KTX_CMips->GetMipLevel(pMipSet, nMipLevel, nSlice)->m_dwLinearSize);
            if (setMemory != KTX_SUCCESS)
            {
                return -1;
            }
        }
    }

    KTX_error_code save = ktxTexture_WriteToNamedFile(ktxTexture(texture), pszFilename);
    if (save != KTX_SUCCESS)
    {
        return -1;
    }

    return 0;
}


