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
// mutex.h : This implements a critical section synchronization object.
//

#if !defined(_CRITICALSECTION_H__INCLUDED_)
#define _CRITICALSECTION_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#include "pthread.h"
#endif // UNIX

#include "macros.h"
#include "exception.h"


//////////////////////////////////////////////////////////////////////
// Exception message

#define EXCEP_MUTEX_INIT  "Unable to initialize mutex."


//////////////////////////////////////////////////////////////////////
// Simply wraps the mutex macros as an inline class implementation

#ifdef WIN32

class CriticalSection
{
public:

    CriticalSection()
        { ::InitializeCriticalSection(&m_cs); }

    ~CriticalSection()
        { ::DeleteCriticalSection(&m_cs); }

    void enter()
        { ::EnterCriticalSection(&m_cs); }

    void exit()
        { ::LeaveCriticalSection(&m_cs); }

private:

    CRITICAL_SECTION m_cs;
};

#endif // WIN32

#ifdef UNIX

class CriticalSection
{
public:

    CriticalSection()
        { if (pthread_mutex_init(&m_mtx, NULL)) THROW(CException, EXCEP_MSSG(EXCEP_MUTEX_INIT)); }

    ~CriticalSection()
        { pthread_mutex_destroy(&m_mtx); }

    void enter()
        { pthread_mutex_lock(&m_mtx); }

    void exit()
        { pthread_mutex_unlock(&m_mtx); }

private:

	pthread_mutex_t m_mtx;
};

#endif // UNIX

#endif // !defined(_CRITICALSECTION_H__INCLUDED_)
