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
// ReaderWriter.cpp: implementation of the CReaderWriter class.
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

#include "ReaderWriter.h"
#include "Semaphore.h"
#include "ThreadException.h"

#ifdef UNIX
#include "sched.h"
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Exception messages

#define EXCEP_INVALID  "Attempt to use an invalid reader-writer object."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CReaderWriter::CReaderWriter()
{
    m_nReadCount = 0; 
    m_nWriteCount = 0;
    m_bValid = FALSE;

    m_psemWriteAllow = new CSemaphore(1, 1);
    m_psemReadAllow = new CSemaphore(1, 1);

    m_bValid = TRUE;
}

CReaderWriter::~CReaderWriter()
{
    if (m_psemWriteAllow) delete m_psemWriteAllow;
    if (m_psemReadAllow) delete m_psemReadAllow;
}

void CReaderWriter::startReading()
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

    m_csRead1.enter();
    m_psemReadAllow->p();
    m_csRead2.enter();
    
    if (++m_nReadCount == 1) 
        m_psemWriteAllow->p();

    m_csRead2.exit();
    m_psemReadAllow->v();
    m_csRead1.exit();
}

void CReaderWriter::endReading()
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

    m_csRead2.enter();
    
    if (--m_nReadCount == 0) 
        m_psemWriteAllow->v();

    m_csRead2.exit();
}

void CReaderWriter::startWriting()
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

    m_csWrite.enter();

    if (++m_nWriteCount == 1) 
        m_psemReadAllow->p();
    
    m_csWrite.exit();

    m_psemWriteAllow->p();
}

void CReaderWriter::endWriting()
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

    m_psemWriteAllow->v();

    m_csWrite.enter();

    if (--m_nWriteCount == 0) 
        m_psemReadAllow->v();
    
    m_csWrite.exit();
}
