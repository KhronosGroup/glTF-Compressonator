//===============================================================================
// Copyright (c) 2014-2016  Advanced Micro Devices, Inc. All rights reserved.
//===============================================================================
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
//
//  File Name:   Codec_BASIS.cpp
//  Description: implementation of the CCodec_BASIS class
//
//////////////////////////////////////////////////////////////////////////////

#pragma warning(disable:4100)    // Ignore warnings of unreferenced formal parameters

#ifdef _WIN32
#include "Common.h"
#include "BASIS/Codec_BASIS.h"
#include <process.h>

#ifdef GT_COMPDEBUGGER
#include "CompClient.h"
#endif

CCodec_BASIS::CCodec_BASIS() :
    CCodec(CT_BASIS)
{
    m_Quality = 0.0;
}

CCodec_BASIS::~CCodec_BASIS()
{
}

bool CCodec_BASIS::SetParameter(const CMP_CHAR* pszParamName, CODECFLOAT fValue)
{
    m_Quality = (double)fValue;

    if (m_Quality < 0.0)
    {
        m_Quality = 0.0;
    }
    else if (m_Quality > 1.0)
    {
        m_Quality = 1.0;
    }

    return true;
}

bool CCodec_BASIS::GetParameter(const CMP_CHAR* pszParamName, CODECFLOAT& fValue)
{
    fValue = (CODECFLOAT)m_Quality;

    return true;
}

bool CCodec_BASIS::SetParameter(const CMP_CHAR* pszParamName, CMP_DWORD dwValue)
{
    m_srcBufferType = (CodecBufferType)dwValue;

    return true;
}

// Required interfaces
CCodecBuffer* CCodec_BASIS::CreateBuffer(CMP_BYTE nBlockWidth, CMP_BYTE nBlockHeight, CMP_BYTE nBlockDepth, CMP_DWORD dwWidth, CMP_DWORD dwHeight, CMP_DWORD dwPitch, CMP_BYTE* pData) const
{
    return CreateCodecBuffer(m_srcBufferType, nBlockWidth, nBlockHeight, nBlockDepth, dwWidth, dwHeight, dwPitch, pData);
}

CodecError CCodec_BASIS::Compress(CCodecBuffer& bufferIn, CCodecBuffer& bufferOut, Codec_Feedback_Proc pFeedbackProc, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)
{
    bufferOut.Copy(bufferIn);

    return CE_OK;
}

CodecError CCodec_BASIS::Decompress(CCodecBuffer& bufferIn, CCodecBuffer& bufferOut, Codec_Feedback_Proc pFeedbackProc, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)
{
    // TODO:
    return CE_Aborted;
}

#endif
