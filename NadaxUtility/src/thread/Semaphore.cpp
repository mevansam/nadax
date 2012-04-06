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
// Semaphore.cpp: implementation of the CSemaphore class.
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

#include "Semaphore.h"
#include "ThreadException.h"

#ifdef UNIX
#include <errno.h>
#include <sched.h>
#include <sys/time.h>
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Exception messages

#define EXCEP_INITFAILED     "Failed to initialize semaphore."
#define EXCEP_INVALID        "Attempt to use an invalid semaphore."
#define EXCEP_WAITFAILED     "Wait on a semaphore failed."
#define EXCEP_SIGNAL_FAILED  "Failed to signal the semaphore."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CSemaphore::CSemaphore( long nInitCount, 
                        long nMaxCount )
#ifdef WIN32
    : m_csCounter()
#endif // WIN32
{
    m_nCount = nInitCount;
    m_nMaxCount = nMaxCount;
    m_bValid = false;

#ifdef WIN32
    if ( !(m_hSemaphore = ::CreateSemaphore(NULL, nInitCount, nMaxCount, NULL)) )
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INITFAILED));
    }
#endif // WIN32

#ifdef UNIX
    if ( pthread_mutex_init(&m_mutex, NULL) || 
         pthread_cond_init(&m_cond, NULL) ) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INITFAILED));
    }
#endif // UNIX

    m_bValid = true;
}

CSemaphore::~CSemaphore()
{
#ifdef WIN32
    ::CloseHandle(m_hSemaphore);
#endif // WIN32

#ifdef UNIX
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
#endif // UNIX
}


//////////////////////////////////////////////////////////////////////
// Implementation

void CSemaphore::reset()
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

#ifdef WIN32
    // Simply count the semaphore 
    // down to an unsignalled state

    while (this->getCount())
    {
        this->p();
    }
#endif // WIN32

#ifdef UNIX
        pthread_mutex_lock(&m_mutex);
        m_nCount = 0;
        pthread_mutex_unlock(&m_mutex);
#endif // UNIX
}

long CSemaphore::getCount()
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

    long nCount;

#ifdef WIN32
    m_csCounter.enter();
    nCount = m_nCount;
    m_csCounter.exit();
#endif // WIN32

#ifdef UNIX
    pthread_mutex_lock(&m_mutex);
    nCount = m_nCount;
    pthread_mutex_unlock(&m_mutex);
#endif // UNIX

    return nCount;
}

void CSemaphore::p()
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

#ifdef WIN32
    if (::WaitForSingleObject(m_hSemaphore, INFINITE) == WAIT_FAILED)
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_WAITFAILED));
    }

    m_csCounter.enter();
    m_nCount--;
    m_csCounter.exit();
#endif // WIN32

#ifdef UNIX
    pthread_mutex_lock(&m_mutex);
    while (m_nCount == 0) 
        pthread_cond_wait(&m_cond, &m_mutex);
    m_nCount--;
    pthread_mutex_unlock(&m_mutex);
#endif // UNIX
}

bool CSemaphore::p(int nMilliSecs)
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

#ifdef WIN32
    DWORD nResult = ::WaitForSingleObject(m_hSemaphore, nMilliSecs);

    if (nResult == WAIT_FAILED) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_WAITFAILED));
    }
    else if (nResult == WAIT_TIMEOUT) 
    {
        return true;
    }

    m_csCounter.enter();
    m_nCount--;
    m_csCounter.exit();
    return false;
#endif // WIN32

#ifdef UNIX
    timeval tp;
    timespec ts;
    gettimeofday(&tp, NULL);
    ts.tv_sec = tp.tv_sec + (nMilliSecs / 1000);
    ts.tv_nsec = (tp.tv_usec * 1000) + ((nMilliSecs % 1000) * 1000000);
    pthread_mutex_lock(&m_mutex);
    while (m_nCount == 0)
    {
        if (pthread_cond_timedwait(&m_cond, &m_mutex, &ts) == ETIMEDOUT)
        {
            pthread_mutex_unlock(&m_mutex);
            return true;
        }
    }
    m_nCount--;
    pthread_mutex_unlock(&m_mutex);
    return false;
#endif // UNIX
}

void CSemaphore::v(long nReleaseCount)
{
    if (!m_bValid) 
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_INVALID));
    }

#ifdef WIN32
    m_csCounter.enter();
    if (::ReleaseSemaphore(m_hSemaphore, nReleaseCount, NULL))
    {
        m_nCount += nReleaseCount;
        ::Sleep(0);
    }
    else if (::GetLastError() != ERROR_TOO_MANY_POSTS)
    {
        THROW(CThreadException, EXCEP_MSSG(EXCEP_SIGNAL_FAILED));
    }
    m_csCounter.exit();
#endif // WIN32

#ifdef UNIX
    pthread_mutex_lock(&m_mutex);
    if (m_nCount < m_nMaxCount) 
    {
        m_nCount += nReleaseCount;
        pthread_cond_signal(&m_cond);
        sched_yield();
    }
    pthread_mutex_unlock(&m_mutex);
#endif // UNIX
}
