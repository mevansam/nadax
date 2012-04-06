//--------------------------------------------------------------------
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
//--------------------------------------------------------------------
//
// ReaderWriter.h: interface for the CReaderWriter class.
//
// Author  : Mevan Samaratunga
// Date    : 08/01/2000
// Version : 0.1a
//
// Update Information:
// -------------------
//
// 1) Author :
//    Date   :
//    Update :
//
//////////////////////////////////////////////////////////////////////

#if !defined(_READERWRITER_H__2296653C_ABE6_4C99_A379_93E007D4C8E1__INCLUDED_)
#define _READERWRITER_H__2296653C_ABE6_4C99_A379_93E007D4C8E1__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#include "pthread.h" 
#endif // UNIX

#include "macros.h"
#include "criticalsection.h"

class CSemaphore;


//////////////////////////////////////////////////////////////////////
// This class implements the reader-writer synchronization problem
// using the semaphores.

class CReaderWriter  
{
public:
    CReaderWriter();
    virtual ~CReaderWriter();

    void startReading();
    void endReading();
    void startWriting();
    void endWriting();

private:

    long m_nReadCount;
    long m_nWriteCount;
    BOOL m_bValid;

    CSemaphore* m_psemWriteAllow;
    CSemaphore* m_psemReadAllow;

    CriticalSection m_csRead1;
    CriticalSection m_csRead2;
    CriticalSection m_csWrite;
};

#endif // !defined(_READERWRITER_H__2296653C_ABE6_4C99_A379_93E007D4C8E1__INCLUDED_)
