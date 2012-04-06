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
// ThreadPoolMgr.h: interface for the CThreadPoolMgr class.
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

#if !defined(_THREADPOOLMGR_H__25B04C53_FA72_43BB_B5C5_D05B41C478FF__INCLUDED_)
#define _THREADPOOLMGR_H__25B04C53_FA72_43BB_B5C5_D05B41C478FF__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#endif // UNIX

#include "macros.h"
#include "number.h"
#include "criticalsection.h"
#include "Semaphore.h"

class CThreadHelper;
class IPooledThread;


//////////////////////////////////////////////////////////////////////
// This class implements the thread pool manager.

class CThreadPoolMgr  
{
public:
    CThreadPoolMgr(int nNumThreads);
    virtual ~CThreadPoolMgr();

    void shutdown();

    void startThread(IPooledThread* pPooledThreadImpl);
    void startThread(IPooledThread* pPooledThreadImpl, int priority);

private:    // Implementation

    void releaseThread(CThreadHelper* pThreadHelper);

private:    // Members

    // Thread Pool Attributes

    int m_nFreeList;
    int m_nNumThreads;
    CThreadHelper* m_pThreads;
    
    Number<char> m_nRunState;  // Default 0 if running and 1 if stopped

    CriticalSection m_csThreadList;  // Protects the thread list
    CSemaphore m_semThreadCount;     // Counts and blocks for available pooled threads

    // Enables a running thread helper to call 
    // releaseThread once it is done processing.
    friend class CThreadHelper;
};

#endif // !defined(_THREADPOOLMGR_H__25B04C53_FA72_43BB_B5C5_D05B41C478FF__INCLUDED_)
