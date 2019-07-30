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
#ifdef _WIN32
#ifdef CMP_Texture_IO_SUPPORTED
    assert(pszFilename);

    FILE* pFile = NULL;
    pFile = fopen(&pFile, pszFilename, ("rb")) ;
    if (pFile == NULL)
    {
        return -1;
    }

    //using libktx
    KTX_header fheader;
    KTX_supplemental_info texinfo;
    if (fread(&fheader, sizeof(KTX_header), 1, pFile) != 1)
    {
        fclose(pFile);
        return -1;
    }

    if (_ktxCheckHeader(&fheader, &texinfo) != KTX_SUCCESS)
    {
        fclose(pFile);
        return -1;
    }

    memset(srcTexture, 0, sizeof(*srcTexture));
    srcTexture->dwSize = sizeof(*srcTexture);
    srcTexture->dwWidth = fheader.pixelWidth;
    srcTexture->dwHeight = fheader.pixelHeight;
    srcTexture->dwPitch = 0;

    if (texinfo.compressed)
    {
        srcTexture->nBlockHeight = 4;
        srcTexture->nBlockWidth = 4;
        srcTexture->nBlockDepth = 1;

        //todo: add in texture data type; 1D, 2D etc, please refer to load texture mipset
        switch (fheader.glInternalFormat)
        {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            srcTexture->format = CMP_FORMAT_BC1;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            srcTexture->format = CMP_FORMAT_BC1;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            srcTexture->format = CMP_FORMAT_BC2;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            srcTexture->format = CMP_FORMAT_BC3;
            break;
        case RGB_BP_UNorm:
            srcTexture->format = CMP_FORMAT_BC7;
            break;
        case R_ATI1N_UNorm:
            srcTexture->format = CMP_FORMAT_ATI1N;
            break;
        case R_ATI1N_SNorm:
            srcTexture->format = CMP_FORMAT_ATI2N;
            break;
        case RG_ATI2N_UNorm:
            srcTexture->format = CMP_FORMAT_ATI2N_XY;
            break;
        case RG_ATI2N_SNorm:
            srcTexture->format = CMP_FORMAT_ATI2N_DXT5;
            break;
        case RGB_BP_UNSIGNED_FLOAT:
            srcTexture->format = CMP_FORMAT_BC6H;
            break;
        case RGB_BP_SIGNED_FLOAT:
            srcTexture->format = CMP_FORMAT_BC6H;
            break;
        case ATC_RGB_AMD:
            srcTexture->format = CMP_FORMAT_ATC_RGB;
            break;
        case ATC_RGBA_EXPLICIT_ALPHA_AMD:
            srcTexture->format = CMP_FORMAT_ATC_RGBA_Explicit;
            break;
        case ATC_RGBA_INTERPOLATED_ALPHA_AMD:
            srcTexture->format = CMP_FORMAT_ATC_RGBA_Interpolated;
            break;
        case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 4;
            srcTexture->nBlockHeight = 4;
            break;
        case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 5;
            srcTexture->nBlockHeight = 4;
            break;
        case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 5;
            srcTexture->nBlockHeight = 5;
            break;
        case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 6;
            srcTexture->nBlockHeight = 5;
            break;
        case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 6;
            srcTexture->nBlockHeight = 6;
            break;
        case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 8;
            srcTexture->nBlockHeight = 5;
            break;
        case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 8;
            srcTexture->nBlockHeight = 6;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 10;
            srcTexture->nBlockHeight = 5;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 10;
            srcTexture->nBlockHeight = 6;
            break;
        case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 8;
            srcTexture->nBlockHeight = 8;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 10;
            srcTexture->nBlockHeight = 8;
            break;
        case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 10;
            srcTexture->nBlockHeight = 10;
            break;
        case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 12;
            srcTexture->nBlockHeight = 10;
            break;
        case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
            srcTexture->format = CMP_FORMAT_ASTC;
            srcTexture->nBlockWidth = 12;
            srcTexture->nBlockHeight = 12;
            break;
        case ETC1_RGB8_OES:
            srcTexture->format = CMP_FORMAT_ETC_RGB;
            break;
        case GL_COMPRESSED_RGB8_ETC2:
            srcTexture->format = CMP_FORMAT_ETC2_RGB;
            break;
        case COMPRESSED_FORMAT_DXT5_RxBG:
            srcTexture->format = CMP_FORMAT_DXT5_RxBG;
            break;
        case COMPRESSED_FORMAT_DXT5_RBxG:
            srcTexture->format = CMP_FORMAT_DXT5_RBxG;
            break;
        case COMPRESSED_FORMAT_DXT5_xRBG:
            srcTexture->format = CMP_FORMAT_DXT5_xRBG;
            break;
        case COMPRESSED_FORMAT_DXT5_RGxB:
            srcTexture->format = CMP_FORMAT_DXT5_RGxB;
            break;
        case COMPRESSED_FORMAT_DXT5_xGxR:
            srcTexture->format = CMP_FORMAT_DXT5_xGxR;
            break;
        default:
            fclose(pFile);
            return -1;
        }
    }
    else
    {
        switch (fheader.glType)
        {
        case GL_UNSIGNED_BYTE:
            srcTexture->format = CMP_FORMAT_ARGB_8888;
            break;
        default:
            fclose(pFile);
            return -1;
        }
    }

    srcTexture->dwDataSize = CMP_CalculateBufferSize(srcTexture);
    srcTexture->pData = (CMP_BYTE*)malloc(srcTexture->dwDataSize);
    

    fclose(pFile);
    pFile = NULL;
    pFile = fopen(pszFilename, "rb");
    if (pFile == NULL)
    {
        return -1;
    }

    //skip key value data
    int imageSizeOffset = sizeof(KTX_header) + fheader.bytesOfKeyValueData;
    if (fseek(pFile, imageSizeOffset, SEEK_SET))
    {
        fclose(pFile);
        return -1;
    }

    //load image size
    unsigned int imageByteCount = 0;
    if (fread((void*)&imageByteCount, 1, 4, pFile) != 4)
    {
        fclose(pFile);
        return -1;
    }

    //read image data
    const unsigned int bytesRead = fread(srcTexture->pData, 1, imageByteCount, pFile);
    if (bytesRead != imageByteCount)
    {
        fclose(pFile);
        return -1;
    }
    fclose(pFile);
    return 0;
#endif
#endif
    return -1;
}

int Plugin_KTX::TC_PluginFileSaveTexture(const char* pszFilename, CMP_Texture *srcTexture)
{
#ifdef _WIN32
#ifdef CMP_Texture_IO_SUPPORTED
    assert(pszFilename);

    FILE* pFile = NULL;
    pFile = fopen(pszFilename, "wb");
    if (pFile == NULL)
    {
        return -1;
    }

    //using libktx
    KTX_supplemental_info textureinfo;
    KTX_image_info* inputMip = new KTX_image_info();

    unsigned int pDataLen = 0;
    CMP_BYTE* pData = NULL;
    bool isCompressed = false;

    //todo: textureinfo.numberOfFaces = 6; reserved for cubemap
    textureinfo.numberOfFaces = 1;

    //todo: array textures
    textureinfo.numberOfArrayElements = 0;

    inputMip->data = srcTexture->pData;
    int w = srcTexture->dwWidth;
    int h = srcTexture->dwHeight;

    switch (srcTexture->format)
    {
        //uncompressed format case
    case CMP_FORMAT_ARGB_8888:
    case CMP_FORMAT_RGB_888:
    case CMP_FORMAT_RG_8:
    case CMP_FORMAT_R_8:
        isCompressed = false;
        textureinfo.glType = GL_UNSIGNED_BYTE;
        textureinfo.glTypeSize = 1;
        break;
    case  CMP_FORMAT_ARGB_2101010:
        textureinfo.glType = GL_UNSIGNED_INT_2_10_10_10_REV;
        textureinfo.glTypeSize = 1;
        break;
    case  CMP_FORMAT_ARGB_16:
        textureinfo.glType = GL_UNSIGNED_SHORT;
        textureinfo.glTypeSize = 2;
        break;
    case  CMP_FORMAT_RG_16:
        textureinfo.glType = GL_UNSIGNED_SHORT;
        textureinfo.glTypeSize = 2;
        break;
    case  CMP_FORMAT_R_16:
        textureinfo.glType = GL_UNSIGNED_SHORT;
        textureinfo.glTypeSize = 2;
        break;
    case  CMP_FORMAT_ARGB_16F:
    case  CMP_FORMAT_RG_16F:
    case  CMP_FORMAT_R_16F:
        textureinfo.glType = GL_HALF_FLOAT;
        textureinfo.glTypeSize = 1;
        break;
    case  CMP_FORMAT_ARGB_32F:
    case  CMP_FORMAT_RGB_32F:
    case  CMP_FORMAT_RG_32F:
    case  CMP_FORMAT_R_32F:
        textureinfo.glType = GL_FLOAT;
        textureinfo.glTypeSize = 1;
        break;
        //compressed format case
    case  CMP_FORMAT_ATI1N:
    case  CMP_FORMAT_ATI2N:
    case  CMP_FORMAT_ATI2N_XY:
    case  CMP_FORMAT_ATI2N_DXT5:
    case  CMP_FORMAT_ATC_RGB:
    case  CMP_FORMAT_ATC_RGBA_Explicit:
    case  CMP_FORMAT_ATC_RGBA_Interpolated:
    case  CMP_FORMAT_BC1:
    case  CMP_FORMAT_BC2:
    case  CMP_FORMAT_BC3:
    case  CMP_FORMAT_BC4:
    case  CMP_FORMAT_BC5:
    case  CMP_FORMAT_BC6H:
    case  CMP_FORMAT_BC7:
    case  CMP_FORMAT_DXT1:
    case  CMP_FORMAT_DXT3:
    case  CMP_FORMAT_DXT5:
    case  CMP_FORMAT_DXT5_xGBR:
    case  CMP_FORMAT_DXT5_RxBG:
    case  CMP_FORMAT_DXT5_RBxG:
    case  CMP_FORMAT_DXT5_xRBG:
    case  CMP_FORMAT_DXT5_RGxB:
    case  CMP_FORMAT_DXT5_xGxR:
    case  CMP_FORMAT_ETC_RGB:
    case  CMP_FORMAT_ETC2_RGB:
    case  CMP_FORMAT_ASTC:
    case  CMP_FORMAT_GT:
        isCompressed = true;
        textureinfo.glType = 0;
        textureinfo.glTypeSize = 1;
        textureinfo.glFormat = 0;
        break;
        //default case
    default:
        isCompressed = false;
        textureinfo.glType = GL_UNSIGNED_BYTE;
        textureinfo.glTypeSize = 1;
        break;
    }

    //todo:check for texture type: RGBA, XRGB etc, refer to save mipset
    inputMip->size = srcTexture->dwDataSize;
    if (!isCompressed)
    {
        textureinfo.glFormat = textureinfo.glBaseInternalformat = GL_RGBA;
        textureinfo.glInternalformat = GL_RGBA8;
    }
    else
    {
        textureinfo.glBaseInternalformat = GL_RGBA;
        switch (srcTexture->format)
        {
        case CMP_FORMAT_BC1:
        case CMP_FORMAT_DXT1:
            textureinfo.glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case CMP_FORMAT_BC2:
        case CMP_FORMAT_DXT3:
            textureinfo.glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case CMP_FORMAT_BC3:
        case CMP_FORMAT_DXT5:
            textureinfo.glInternalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        case CMP_FORMAT_BC7:
            textureinfo.glInternalformat = RGB_BP_UNorm;
            break;
        case  CMP_FORMAT_ATI1N:
            textureinfo.glInternalformat = R_ATI1N_UNorm;
            break;
        case  CMP_FORMAT_ATI2N:
            textureinfo.glInternalformat = R_ATI1N_SNorm;
            break;
        case  CMP_FORMAT_ATI2N_XY:
            textureinfo.glInternalformat = RG_ATI2N_UNorm;
            break;
        case  CMP_FORMAT_ATI2N_DXT5:
            textureinfo.glInternalformat = RG_ATI2N_SNorm;
            break;
        case  CMP_FORMAT_ATC_RGB:
            textureinfo.glInternalformat = ATC_RGB_AMD;
            break;
        case  CMP_FORMAT_ATC_RGBA_Explicit:
            textureinfo.glInternalformat = ATC_RGBA_EXPLICIT_ALPHA_AMD;
            break;
        case  CMP_FORMAT_ATC_RGBA_Interpolated:
            textureinfo.glInternalformat = ATC_RGBA_INTERPOLATED_ALPHA_AMD;
            break;
        case  CMP_FORMAT_BC4:
            textureinfo.glInternalformat = COMPRESSED_RED_RGTC1;
            break;
        case  CMP_FORMAT_BC5:
            textureinfo.glInternalformat = COMPRESSED_RG_RGTC2;
            break;
        case  CMP_FORMAT_BC6H:
            textureinfo.glInternalformat = RGB_BP_UNSIGNED_FLOAT;
            break;
        case CMP_FORMAT_ETC_RGB:
            textureinfo.glInternalformat = ETC1_RGB8_OES;
            break;
        case CMP_FORMAT_ETC2_RGB:
            textureinfo.glInternalformat = GL_COMPRESSED_RGB8_ETC2;
            break;
        case CMP_FORMAT_DXT5_xGBR:
            textureinfo.glInternalformat = COMPRESSED_FORMAT_DXT5_xGBR;
            break;
        case CMP_FORMAT_DXT5_RxBG:
            textureinfo.glInternalformat = COMPRESSED_FORMAT_DXT5_RxBG;
            break;
        case CMP_FORMAT_DXT5_RBxG:
            textureinfo.glInternalformat = COMPRESSED_FORMAT_DXT5_RBxG;
            break;
        case CMP_FORMAT_DXT5_xRBG:
            textureinfo.glInternalformat = COMPRESSED_FORMAT_DXT5_xRBG;
            break;
        case CMP_FORMAT_DXT5_RGxB:
            textureinfo.glInternalformat = COMPRESSED_FORMAT_DXT5_RGxB;
            break;
        case CMP_FORMAT_DXT5_xGxR:
            textureinfo.glInternalformat = COMPRESSED_FORMAT_DXT5_xGxR;
            break;
        }
    }
  
    textureinfo.pixelWidth = srcTexture->dwWidth;
    textureinfo.pixelHeight = srcTexture->dwHeight;
    textureinfo.pixelDepth = 0; //for 1D, 2D and cube texture , depth =0;

                                //1D 
    if (srcTexture->dwHeight == 1 && pData != NULL) {
        delete(pData);
        pData = NULL;
        pDataLen = 0;
    }

    textureinfo.numberOfMipLevels = 1;

    KTX_error_code save = ktxWriteKTXF(pFile, &textureinfo, pDataLen, pData, 1, inputMip);
    if (save == KTX_SUCCESS) {
        fclose(pFile);
    }
    else {
        fclose(pFile);
        return -1;
    }
    fclose(pFile);
    return 0;
#endif
#endif
    return -1;
}

uint32_t ktx_u32_byterev(uint32_t v)
{
    return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24);
}

// perform endianness switch on raw data
static void switch_endianness2(void *dataptr, int bytes)
{
    int i;
    uint8_t *data = (uint8_t *) dataptr;

    for (i = 0; i < bytes / 2; i++)
    {
        uint8_t d0 = data[0];
        uint8_t d1 = data[1];
        data[0] = d1;
        data[1] = d0;
        data += 2;
    }
}

static void switch_endianness4(void *dataptr, int bytes)
{
    int i;
    uint8_t *data = (uint8_t *) dataptr;

    for (i = 0; i < bytes / 4; i++)
    {
        uint8_t d0 = data[0];
        uint8_t d1 = data[1];
        uint8_t d2 = data[2];
        uint8_t d3 = data[3];
        data[0] = d3;
        data[1] = d2;
        data[2] = d1;
        data[3] = d0;
        data += 4;
    }
}

static void copy_scanline(void *dst, const void *src, int pixels, int method)
{

#define id(x) (x)
#define u16_sf16(x) float_to_sf16( x * (1.0f/65535.0f), SF_NEARESTEVEN )
#define f32_sf16(x) sf32_to_sf16( x, SF_NEARESTEVEN )

#define COPY_R( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[i]); \
        d[4*i+1] = 0; \
        d[4*i+2] = 0; \
        d[4*i+3] = oneval; \
        } \
    } while(0); \
    break;

#define COPY_RG( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[2*i]); \
        d[4*i+1] = convfunc(s[2*i+1]); \
        d[4*i+2] = 0; \
        d[4*i+3] = oneval; \
        } \
    } while(0); \
    break;

#define COPY_RGB( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[3*i]); \
        d[4*i+1] = convfunc(s[3*i+1]); \
        d[4*i+2] = convfunc(s[3*i+2]); \
        d[4*i+3] = oneval; \
        } \
    } while(0); \
    break;

#define COPY_BGR( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[3*i+2]); \
        d[4*i+1] = convfunc(s[3*i+1]); \
        d[4*i+2] = convfunc(s[3*i]); \
        d[4*i+3] = oneval; \
        } \
    } while(0); \
    break;

#define COPY_RGBX( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[4*i]); \
        d[4*i+1] = convfunc(s[4*i+1]); \
        d[4*i+2] = convfunc(s[4*i+2]); \
        d[4*i+3] = oneval; \
        } \
    } while(0); \
    break;

#define COPY_BGRX( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[4*i+2]); \
        d[4*i+1] = convfunc(s[4*i+1]); \
        d[4*i+2] = convfunc(s[4*i]); \
        d[4*i+3] = oneval; \
        } \
    } while(0); \
    break;

#define COPY_RGBA( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[4*i]); \
        d[4*i+1] = convfunc(s[4*i+1]); \
        d[4*i+2] = convfunc(s[4*i+2]); \
        d[4*i+3] = convfunc(s[4*i+3]); \
        } \
    } while(0); \
    break;

#define COPY_BGRA( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[4*i+2]); \
        d[4*i+1] = convfunc(s[4*i+1]); \
        d[4*i+2] = convfunc(s[4*i]); \
        d[4*i+3] = convfunc(s[4*i+3]); \
        } \
    } while(0); \
    break;

#define COPY_L( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[i]); \
        d[4*i+1] = convfunc(s[i]); \
        d[4*i+2] = convfunc(s[i]); \
        d[4*i+3] = oneval; \
        } \
    } while(0); \
    break;

#define COPY_LA( dsttype, srctype, convfunc, oneval ) \
    do { \
    srctype *s = (srctype *)src; \
    dsttype *d = (dsttype *)dst; \
    for(i=0;i<pixels;i++)\
        {\
        d[4*i] = convfunc(s[2*i]); \
        d[4*i+1] = convfunc(s[2*i]); \
        d[4*i+2] = convfunc(s[2*i]); \
        d[4*i+3] = convfunc(s[2*i+1]); \
        } \
    } while(0); \
    break;

    int i;
    switch (method)
    {
    case R8_TO_RGBA8:
        COPY_R(uint8_t, uint8_t, id, 0xFF);
    case RG8_TO_RGBA8:
        COPY_RG(uint8_t, uint8_t, id, 0xFF);
    case RGB8_TO_RGBA8:
        COPY_RGB(uint8_t, uint8_t, id, 0xFF);
    case RGBA8_TO_RGBA8:
        COPY_RGBA(uint8_t, uint8_t, id, 0xFF);
    case BGR8_TO_RGBA8:
        COPY_BGR(uint8_t, uint8_t, id, 0xFF);
    case BGRA8_TO_RGBA8:
        COPY_BGRA(uint8_t, uint8_t, id, 0xFF);
    case RGBX8_TO_RGBA8:
        COPY_RGBX(uint8_t, uint8_t, id, 0xFF);
    case BGRX8_TO_RGBA8:
        COPY_BGRX(uint8_t, uint8_t, id, 0xFF);
    case L8_TO_RGBA8:
        COPY_L(uint8_t, uint8_t, id, 0xFF);
    case LA8_TO_RGBA8:
        COPY_LA(uint8_t, uint8_t, id, 0xFF);

    case R16F_TO_RGBA16F:
        COPY_R(uint16_t, uint16_t, id, 0x3C00);
    case RG16F_TO_RGBA16F:
        COPY_RG(uint16_t, uint16_t, id, 0x3C00);
    case RGB16F_TO_RGBA16F:
        COPY_RGB(uint16_t, uint16_t, id, 0x3C00);
    case RGBA16F_TO_RGBA16F:
        COPY_RGBA(uint16_t, uint16_t, id, 0x3C00);
    case BGR16F_TO_RGBA16F:
        COPY_BGR(uint16_t, uint16_t, id, 0x3C00);
    case BGRA16F_TO_RGBA16F:
        COPY_BGRA(uint16_t, uint16_t, id, 0x3C00);
    case L16F_TO_RGBA16F:
        COPY_L(uint16_t, uint16_t, id, 0x3C00);
    case LA16F_TO_RGBA16F:
        COPY_LA(uint16_t, uint16_t, id, 0x3C00);

    case R16_TO_RGBA16F:
        COPY_R(uint16_t, uint16_t, u16_sf16, 0x3C00);
    case RG16_TO_RGBA16F:
        COPY_RG(uint16_t, uint16_t, u16_sf16, 0x3C00);
    case RGB16_TO_RGBA16F:
        COPY_RGB(uint16_t, uint16_t, u16_sf16, 0x3C00);
    case RGBA16_TO_RGBA16F:
        COPY_RGBA(uint16_t, uint16_t, u16_sf16, 0x3C00);
    case BGR16_TO_RGBA16F:
        COPY_BGR(uint16_t, uint16_t, u16_sf16, 0x3C00);
    case BGRA16_TO_RGBA16F:
        COPY_BGRA(uint16_t, uint16_t, u16_sf16, 0x3C00);
    case L16_TO_RGBA16F:
        COPY_L(uint16_t, uint16_t, u16_sf16, 0x3C00);
    case LA16_TO_RGBA16F:
        COPY_LA(uint16_t, uint16_t, u16_sf16, 0x3C00);

    case R32F_TO_RGBA16F:
        COPY_R(uint16_t, uint32_t, f32_sf16, 0x3C00);
    case RG32F_TO_RGBA16F:
        COPY_RG(uint16_t, uint32_t, f32_sf16, 0x3C00);
    case RGB32F_TO_RGBA16F:
        COPY_RGB(uint16_t, uint32_t, f32_sf16, 0x3C00);
    case RGBA32F_TO_RGBA16F:
        COPY_RGBA(uint16_t, uint32_t, f32_sf16, 0x3C00);
    case BGR32F_TO_RGBA16F:
        COPY_BGR(uint16_t, uint32_t, f32_sf16, 0x3C00);
    case BGRA32F_TO_RGBA16F:
        COPY_BGRA(uint16_t, uint32_t, f32_sf16, 0x3C00);
    case L32F_TO_RGBA16F:
        COPY_L(uint16_t, uint32_t, f32_sf16, 0x3C00);
    case LA32F_TO_RGBA16F:
        COPY_LA(uint16_t, uint32_t, f32_sf16, 0x3C00);
    };
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


