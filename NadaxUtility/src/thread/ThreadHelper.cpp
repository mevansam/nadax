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
// ThreadHelper.cpp: implementation of the CThreadHelper class.
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

#include "ThreadHelper.h"
#include "ThreadPoolMgr.h"
#include "ThreadException.h"

#include "IPooledThread.h"


//////////////////////////////////////////////////////////////////////
// Exception messages

#define EXCEP_INVALIDTHREADIMPL  "Invalid or null thread implementation given."
#define EXCEP_NOTRUNNING         "Pooled thread object is not running."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CThreadHelper::CThreadHelper() : 
    m_semStartThread(0, 1),
    m_semEndThread(1, 1)
{
    m_pThreadImpl = NULL;
    m_bRun = true;
}

CThreadHelper::~CThreadHelper()
{
}

void CThreadHelper::startThread(IPooledThread* pThreadImpl, int nPriority)
{
    if (m_bRun)
    {
        if (!pThreadImpl)
        {
            THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALIDTHREADIMPL));
        }

        m_semEndThread.p();

        CThread::setPriority(nPriority);
        m_pThreadImpl = pThreadImpl;
        m_pThreadImpl->setThread(this);

        m_semStartThread.v();
    }
    else
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_NOTRUNNING));
    }
}

void CThreadHelper::terminate()
{
    if (m_bRun)
    {
        m_bRun = false; 

        if (m_pThreadImpl) m_pThreadImpl->stop();
        CThread::yield();

        m_semEndThread.p();        // Wait until pThreadImpl is done
        m_semStartThread.v();    // Fall through into thread terminate test

        CThread::join();
    }
}

void* CThreadHelper::run()
{
    while (m_bRun)
    {
        CThread::setPriority(THREAD_PRIORITY_NORMAL);
        m_semStartThread.p();

        if (m_pThreadImpl)
        {
            // Run thread implementation
            m_pThreadImpl->run();
            m_pThreadImpl->cleanup();
            m_pThreadImpl = NULL;

            m_pThreadPoolMgr->releaseThread(this);
        }
    
        m_semEndThread.v();
    }
    return NULL;
}
