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
// ThreadPoolMgr.cpp: implementation of the CThreadPoolMgr class.
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

#include "ThreadPoolMgr.h"
#include "ThreadHelper.h"
#include "ThreadException.h"


//////////////////////////////////////////////////////////////////////
// Exception messages

#define EXCEP_THREADMGRINITFAILED  "Unable to initialize thread pool manager."
#define EXCEP_THREADMGRSTOPPED     "The thread pool manager has alread been stopped."
#define EXCEP_THREADSTARTFAILED    "Unable to start thread as the thread pool has been stopped."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CThreadPoolMgr::CThreadPoolMgr(int nNumThreads) :
    m_semThreadCount(nNumThreads, nNumThreads)
{
    if (nNumThreads == 0)
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADMGRINITFAILED));
    }

    m_nNumThreads = nNumThreads;
    m_pThreads = new CThreadHelper[nNumThreads];

    CThreadHelper* pThread = m_pThreads;
    int i = 0;

    while (i < m_nNumThreads)
    {
        pThread->m_pThreadPoolMgr = this;
        pThread->m_nNextThread = i + 1;
        pThread->start();
        pThread->yield();

        pThread++;
        i++;
    }

    (--pThread)->m_nNextThread = -1;
    m_nFreeList = 0;
}

CThreadPoolMgr::~CThreadPoolMgr()
{
}

void CThreadPoolMgr::shutdown()
{
    // Set run state to stopped (Pool manager 
    // can be finalized only once)

    if (m_nRunState.interlockedInc() > 1)
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADMGRSTOPPED));
    }

    // Stop all threads in pool manager

    CThreadHelper* pThread = m_pThreads;
    int i = 0;

    while (i < m_nNumThreads)
    {
        pThread->terminate();
        pThread++;
        i++;
    }

    m_csThreadList.enter();

    delete[] m_pThreads;
    m_nFreeList = -1;
    
    m_csThreadList.exit();
}


//////////////////////////////////////////////////////////////////////
// Implementation

void CThreadPoolMgr::startThread(IPooledThread* pThreadImpl)
{
    startThread(pThreadImpl, THREAD_PRIORITY_NORMAL);
}

void CThreadPoolMgr::startThread(IPooledThread* pThreadImpl, int nPriority)
{
    m_semThreadCount.p();

    m_csThreadList.enter();

    if (m_nRunState > 0 || m_nFreeList == -1)
    {
        m_csThreadList.exit();
        THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADSTARTFAILED));
    }
    
    int nThreadIndex = m_nFreeList;
    CThreadHelper& ThreadHelper = m_pThreads[nThreadIndex];

    m_nFreeList = ThreadHelper.m_nNextThread;
    ThreadHelper.m_nNextThread = nThreadIndex;

    m_csThreadList.exit();

    ThreadHelper.startThread(pThreadImpl, nPriority);
}

void CThreadPoolMgr::releaseThread(CThreadHelper* pThreadHelper)
{
    m_csThreadList.enter();

    int nThreadIndex = pThreadHelper->m_nNextThread;
    pThreadHelper->m_nNextThread = m_nFreeList;
    m_nFreeList = nThreadIndex;
    
    m_csThreadList.exit();

    m_semThreadCount.v();
}
