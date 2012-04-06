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
// Semaphore.h: interface for the CSemaphore class.
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

#if !defined(_SEMAPHORE_H__C8BEF6E2_DDF9_43B7_AEF0_4941A4925C78__INCLUDED_)
#define _SEMAPHORE_H__C8BEF6E2_DDF9_43B7_AEF0_4941A4925C78__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#include "pthread.h"
#endif // UNIX

#include "macros.h"
#include "criticalsection.h"


//////////////////////////////////////////////////////////////////////
// This class implements a semaphore using the kernel synchronization
// objects available within the target platform.

class CSemaphore  
{
public:

    CSemaphore(long nInitCount = 0, long nMaxCount = MAX_SIGNED_LONG);
    virtual ~CSemaphore();

    long getCount();  // Returns the semaphore's current counter value

    void p();                        // Wait
    bool p(int nMilliSecs);          // Timed Wait : returns true if timeout
    void v(long nReleaseCount = 1);  // Signal

    // It is important that when this method 
    // is called there is no possibility that 
    // a p() or v() is called as that can leave 
    // the semaphore count in an invalid state.
    void reset();

private:

    long m_nCount;
    long m_nMaxCount;
    BOOL m_bValid;

#ifdef WIN32
    CriticalSection m_csCounter;
    HANDLE m_hSemaphore;
#endif // WIN32

#ifdef UNIX
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
#endif // UNIX

};

#endif // !defined(_SEMAPHORE_H__C8BEF6E2_DDF9_43B7_AEF0_4941A4925C78__INCLUDED_)
