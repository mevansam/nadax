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
// Thread.h: interface for the CThread class.
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

#if !defined(_THREAD_H__INCLUDED_)
#define _THREAD_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#include "pthread.h"
#endif // UNIX

#include "number.h"


//////////////////////////////////////////////////////////////////////
// Thread priority levels for unix

#ifdef UNIX

#define THREAD_PRIORITY_TIME_CRITICAL    15
#define THREAD_PRIORITY_HIGHEST          2
#define THREAD_PRIORITY_ABOVE_NORMAL     1
#define THREAD_PRIORITY_NORMAL           8
#define THREAD_PRIORITY_BELOW_NORMAL     -1
#define THREAD_PRIORITY_LOWEST           -2
#define THREAD_PRIORITY_IDLE             1

#endif // UNIX


//////////////////////////////////////////////////////////////////////
// This class wraps the underlying OSs implementation of threads. For
// example "pthreads" for Posix based systems (Unix) and "Win32 
// Threads" for MS Windows based systems.

class CThread  
{
public:
    
    CThread(BOOL bDetached = FALSE /* Applies only for Posix implementations */ );
    virtual ~CThread();

    void start();
    void stop(int nExit = 1);
    void join();

    void setPriority(int nPriority);

    bool isRunning() { return m_bRunning; }

    static void yield();
    static void sleep(int nMilliSecs);

    static DWORD getTLSKey();
    static void setTLSKey(DWORD nKey, DWORD nValue);
    static void delTLSKey(DWORD nKey);

protected:

    virtual void* run() = 0;

private:

#ifdef WIN32
    HANDLE m_hThread;
    static DWORD WINAPI threadProc(void* pObj); 
#endif // WIN32

#ifdef UNIX
    pthread_t      m_ptThread;
    pthread_attr_t m_ptAttribs;
    static void* threadProc(void* pObj); 
#endif // UNIX

    // Set to 0 if the thread is 
    // inactive and 1 if it is 
    // active.
    Number<bool> m_bRunning;
};

#endif // !defined(_THREAD_H__INCLUDED_)
