// The MIT License
//
// Copyright (c) 2011 Mevan Samaratunga
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// debug.h : Functions that aid with debugging.
//

#if !defined(_DEBUG_H__INCLUDED_)
#define _DEBUG_H__INCLUDED_


//////////////////////////////////////////////////////////////////////
// When compiled for debugging this function can be used to serially
// print a buffer to stdout with an associated message.

#ifdef _DEBUG

#include "criticalsection.h"

#include <memory.h>
#include <iostream>

using namespace std;

#define DEBUG_BUFFER_SIZE  256

static CriticalSection _csDebug;
static char _szDebugOutput[DEBUG_BUFFER_SIZE + 1];

inline void _debugPrint(const char* pszMessage, const char* pBuffer, long nLen)
{
    if (nLen > 0)
    {
        _csDebug.enter();

        long nOutLen;

        while (nLen)
        {
            nOutLen = min(nLen, DEBUG_BUFFER_SIZE);
            memcpy(_szDebugOutput, pBuffer, nOutLen);
            *(_szDebugOutput + nOutLen) = 0x0;
            nLen -= nOutLen;

            cout << pszMessage << '[' << _szDebugOutput << ']' << endl;
        }

        _csDebug.exit();
    }
}

#endif


#endif // !defined(_DEBUG_H__INCLUDED_)
