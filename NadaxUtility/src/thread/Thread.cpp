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
// Thread.cpp: implementation of the CThread class.
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

#include "Thread.h"
#include "ThreadException.h"

#ifdef UNIX
#include "unistd.h"
#include "sched.h"
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Exception messages

#define EXCEP_THREADSTART     "Failed to create thread."
#define EXCEP_THREADJOIN      "Error occurred whilst joining with thread."
#define EXCEP_THREADPRIORITY  "Error occurred whilst updating the thread priority."


//////////////////////////////////////////////////////////////////////
// Helper C functions

#ifdef UNIX
inline void unix_sleep(int nMilliSecs) {
    sleep(nMilliSecs / 1000);
    usleep((nMilliSecs % 1000) * 1000);
}
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CThread::CThread(BOOL bDetached)
{
#ifdef WIN32
    m_hThread = NULL;
#endif // WIN32

#ifdef UNIX
    pthread_attr_init(&m_ptAttribs);
    pthread_attr_setdetachstate( &m_ptAttribs, 
                                 ( bDetached ? PTHREAD_CREATE_DETACHED : 
                                               PTHREAD_CREATE_JOINABLE ) );
#endif // UNIX

    m_bRunning = false;
}

CThread::~CThread()
{
#ifdef WIN32
    if (m_hThread) ::CloseHandle(m_hThread);
#endif // WIN32
}

void CThread::start()
{
#ifdef WIN32
    DWORD nThreadID;
    if ( ( m_hThread = ::CreateThread( NULL, 0, 
                                       (LPTHREAD_START_ROUTINE) CThread::threadProc, 
                                       (LPVOID) this, 0, &nThreadID ) ) == NULL )
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADSTART));
    }
    m_bRunning = true;
#endif // WIN32

#ifdef UNIX
    if ( pthread_create( &m_ptThread, &m_ptAttribs,
                         (void* (*)(void *)) CThread::threadProc,
                         (void *) this ) )
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADSTART));
    }
    m_bRunning = true;
#endif // UNIX
}

void CThread::join()
{
	if (m_bRunning)
	{
#ifdef WIN32
		if (::WaitForSingleObject(m_hThread, INFINITE) == WAIT_FAILED)
		{
			THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADJOIN));
		}
#endif // WIN32

#ifdef UNIX
		void* pResult;
		if (pthread_join(m_ptThread, &pResult))
		{
			THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADJOIN));
		}
#endif // UNIX
	}
}

void CThread::stop(int nExit)
{
	if (m_bRunning)
	{
#ifdef WIN32
		::TerminateThread(m_hThread, nExit);
#endif // WIN32

#ifdef UNIX
		pthread_kill(m_ptThread, -9);
#endif // UNIX
	}
}

void CThread::setPriority(int nPriority)
{
#ifdef WIN32
    if (!::SetThreadPriority(m_hThread, nPriority))
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADPRIORITY));
    }
#endif // WIN32

#ifdef UNIX

    int nPolicy;
    sched_param param;

    if (pthread_getschedparam(m_ptThread, &nPolicy, &param))
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADPRIORITY));
    }

    int nMinPriority = sched_get_priority_min(nPolicy);
    int nMaxPriority = sched_get_priority_max(nPolicy);

    if (nMinPriority != nMaxPriority || nMinPriority != 0)
    {
        param.sched_priority = ( nPriority == THREAD_PRIORITY_TIME_CRITICAL ? nMaxPriority :
                                 nPriority == THREAD_PRIORITY_NORMAL ? (nMaxPriority + nMinPriority) / 2 : 
                                 nPriority == THREAD_PRIORITY_IDLE ? nMinPriority : param.sched_priority + nPriority );

        if ( nPriority >= THREAD_PRIORITY_IDLE && 
             nPriority <= THREAD_PRIORITY_TIME_CRITICAL )
        {
            if (pthread_setschedparam(m_ptThread, nPolicy, &param))
            {
                THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADPRIORITY));
            }
        }
        else
        {
            THROW(CThreadException, EXCEP_MSSG(EXCEP_THREADPRIORITY));
        }
    }

#endif // UNIX
}

void CThread::yield()
{
#ifdef WIN32
    ::Sleep(0);
#endif // WIN32

#ifdef UNIX
    sched_yield();
#endif // UNIX
}

void CThread::sleep(int nMilliSecs)
{
#ifdef WIN32
    ::Sleep(nMilliSecs);
#endif // WIN32

#ifdef UNIX
    unix_sleep(nMilliSecs);
#endif // UNIX
}

DWORD CThread::getTLSKey()
{
#ifdef WIN32
#endif // WIN32

#ifdef UNIX
#endif // UNIX

    return 0;
}

void CThread::setTLSKey(DWORD nKey, DWORD nValue)
{
#ifdef WIN32
#endif // WIN32

#ifdef UNIX
#endif // UNIX
}

void CThread::delTLSKey(DWORD nKey)
{
#ifdef WIN32
#endif // WIN32

#ifdef UNIX
#endif // UNIX
}


//////////////////////////////////////////////////////////////////////
// Thread procedure

#ifdef WIN32
DWORD WINAPI CThread::threadProc(void* pObj)
{
    CThread* pThread = (CThread *) pObj;
    DWORD nResult = (DWORD) pThread->run();
    pThread->m_bRunning = false;
    return nResult;
}
#endif // WIN32

#ifdef UNIX
void* CThread::threadProc(void* pObj)
{
    CThread* pThread = (CThread *) pObj;
    void* pResult = pThread->run();
    pThread->m_bRunning = false;
    return pResult;
}
#endif // UNIX
