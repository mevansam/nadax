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
// ThreadHelper.h: interface for the CThreadHelper class.
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

#if !defined(_THREADHELPER_H__AE7010D6_98E8_4A0D_970B_7A92038279A2__INCLUDED_)
#define _THREADHELPER_H__AE7010D6_98E8_4A0D_970B_7A92038279A2__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#endif // UNIX

#include "macros.h"
#include "number.h"
#include "Semaphore.h"
#include "Thread.h"

class CThreadPoolMgr;
class IPooledThread;


//////////////////////////////////////////////////////////////////////
// This class is used by the thread manager to wrap pooled thread 
// objects. Typically there will be a helper class for each thread
// pooled.

class CThreadHelper : 
    protected CThread
{
public:
    CThreadHelper();
    virtual ~CThreadHelper();

    void startThread(IPooledThread* pThreadImpl, int priority);
    void terminate();

protected:

    virtual void* run();

private:

    CThreadPoolMgr* m_pThreadPoolMgr;    // Parent manager object
    int m_nNextThread;                    // Index to the next thread in the thread pool lists

    IPooledThread* m_pThreadImpl;
    CSemaphore m_semStartThread;
    CSemaphore m_semEndThread;
    
    Number<bool> m_bRun;

    // Gives the pool manager container access 
    // to private members of this class
    friend class CThreadPoolMgr;
};

#endif // !defined(_THREADHELPER_H__AE7010D6_98E8_4A0D_970B_7A92038279A2__INCLUDED_)
