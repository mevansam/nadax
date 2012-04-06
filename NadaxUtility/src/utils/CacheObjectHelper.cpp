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
// CacheObjectHelper.cpp: implementation of the CCacheObjectHelper class.
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

#include "CacheObjectHelper.h"

#ifdef WIN32
#include <io.h>
#include <time.h>
#endif // UNIX

#ifdef UNIX
#include <unistd.h>
#include <sys/time.h>
#endif // UNIX

#include "ICacheObject.h"
#include "CacheException.h"
#include "macros.h"

#include <sys/types.h>
#include <sys/stat.h>


//////////////////////////////////////////////////////////////////////
// Exception messages

#define EXCEP_CTOR_INVALIDARGS  "One or more arguments passed for constructing a cache object was null."
#define EXCEP_NOOBJFILENAME     "Unable to create a unique cache object file name."
#define EXCEP_NONAME            "Cache object does not have a name."
#define EXCEP_NOFILE            "Cache object is not associated with a unique file for hydration."


//////////////////////////////////////////////////////////////////////
// A constant period in time for creating 
// smaller time stamps when a unix stdlib 
// gettimeofday() is called as the latter 
// returns the seconds elapsed since 
// 01/01/1970. This number is calculated
// as approx 05/22/2001 00:00 hours.

#ifdef UNIX
#define MAGIC_TS  (990500500 * 100)
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Static counter for generating unique file names

Number<long> CCacheObjectHelper::g_nCounter;


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CCacheObjectHelper::CCacheObjectHelper( char* pszCacheName, 
                                        char* pszCacheDirPath, 
                                        ICacheObject* pCacheObject )
{
    m_nIndex = -1;
    m_nPrev = -1;
    m_nNext = -1;

    m_nUsageCount = 0;
    m_nTimeStamp = 0;

    m_LoadState = CCacheObjectHelper::OBJECT_EMPTY;
    
    m_pCacheObject = pCacheObject;
    m_szCacheObjName[0] = 0x0;
    m_szCacheObjFile[0] = 0x0;

    if (pszCacheName && pszCacheDirPath[0])
    {
        char szPathFormat[MAX_PATH];

        strncpy(szPathFormat, pszCacheDirPath, MAX_PATH);
        strncat(szPathFormat, pszCacheName, MAX_PATH);
        strncat(szPathFormat, "%06d", MAX_PATH);

        if (strlen(szPathFormat) > (MAX_PATH - 20 /* Detect overflow */))
        {
            THROW(CCacheException, EXCEP_MSSG(EXCEP_NOOBJFILENAME));
        }

        struct _stat FileStatus;

        do
        {
            _snprintf( m_szCacheObjFile, 
                       sizeof(m_szCacheObjFile), 
                       szPathFormat, 
                       g_nCounter.interlockedInc() );
        }
        while (_stat(m_szCacheObjFile, &FileStatus) == 0);
    }
    else
    {
        m_szCacheObjFile[0] = 0;
    }
}

CCacheObjectHelper::~CCacheObjectHelper()
{
    if (m_pCacheObject) delete m_pCacheObject;
}


//////////////////////////////////////////////////////////////////////
// Implementation

const char* CCacheObjectHelper::getCachedObjectName()
{
    return m_szCacheObjName;
}

ICacheObject* CCacheObjectHelper::getCachedObject(const char* pszName)
{
    switch (m_LoadState)
    {
        case OBJECT_EMPTY:
        {
            strncpy(m_szCacheObjName, pszName, sizeof(m_szCacheObjName));
            m_pCacheObject->load(pszName);
            m_LoadState = OBJECT_IN_MEMORY;
            break;
        }
        case OBJECT_IN_DISK:
        {
            if (m_szCacheObjName[0] == 0x0)
            {
                THROW(CCacheException, EXCEP_MSSG(EXCEP_NONAME));
            }
            else if (m_szCacheObjFile[0] == 0x0)
            {
                THROW(CCacheException, EXCEP_MSSG(EXCEP_NOFILE));
            }

            m_pCacheObject->deserialize(m_szCacheObjFile);
            m_LoadState = OBJECT_IN_MEMORY;
            break;
        }
        case OBJECT_IN_MEMORY:
        {
        	break;
        }
    }

    return m_pCacheObject;
}

void CCacheObjectHelper::unloadCachedObject()
{
    m_szCacheObjName[0] = 0x0;

    m_nUsageCount = 0;
    m_nTimeStamp = 0;

    m_LoadState = CCacheObjectHelper::OBJECT_EMPTY;

    m_pCacheObject->unload();
}

void CCacheObjectHelper::serializeCachedObject()
{
    if (m_szCacheObjFile[0] == 0x0)
    {
        THROW(CCacheException, EXCEP_MSSG(EXCEP_NOFILE));
    }

    m_pCacheObject->serialize(m_szCacheObjFile);
    m_pCacheObject->unload();

    m_LoadState = CCacheObjectHelper::OBJECT_IN_DISK;
}

void CCacheObjectHelper::deleteCachedObject()
{
    if (m_szCacheObjFile[0] == 0x0)
    {
        THROW(CCacheException, EXCEP_MSSG(EXCEP_NOFILE));
    }

    m_pCacheObject->unload();
    remove(m_szCacheObjFile);
}

bool CCacheObjectHelper::operator<(CCacheObjectHelper &CacheObject)
{
    return (m_nUsageCount * m_nTimeStamp) <
           (CacheObject.m_nUsageCount * CacheObject.m_nTimeStamp);
}

bool CCacheObjectHelper::operator>(CCacheObjectHelper &CacheObject)
{
    return (m_nUsageCount * m_nTimeStamp) >
           (CacheObject.m_nUsageCount * CacheObject.m_nTimeStamp);
}

void CCacheObjectHelper::updateMFU()
{
    m_nUsageCount++;

#ifdef WIN32
    m_nTimeStamp = ::GetTickCount() / 10;
#endif // WIN32

#ifdef UNIX
    timeval tp;
    gettimeofday(&tp, NULL);
    m_nTimeStamp  = (tp.tv_sec * 100) + (tp.tv_usec / 1000) - MAGIC_TS;
#endif // UNIX
}

void CCacheObjectHelper::serialize(int nOutFile)
{
    int nLen;

    nLen = strlen(m_szCacheObjName) + 1;
    _write(nOutFile, &nLen, sizeof(int));
    _write(nOutFile, m_szCacheObjName, nLen);

    nLen = strlen(m_szCacheObjFile) + 1;
    _write(nOutFile, &nLen, sizeof(int));
    _write(nOutFile, m_szCacheObjFile, nLen);

    _write(nOutFile, &m_LoadState, sizeof(CCacheObjectHelper::STATE));
    _write(nOutFile, &m_nUsageCount, sizeof(long));
    _write(nOutFile, &m_nTimeStamp, sizeof(long));
}

void CCacheObjectHelper::deserialize(int nInFile)
{
    int nLen;

    _read(nInFile, &nLen, sizeof(int));
    _read(nInFile, m_szCacheObjName, nLen);

    _read(nInFile, &nLen, sizeof(int));
    _read(nInFile, m_szCacheObjFile, nLen);

    _read(nInFile, &m_LoadState, sizeof(CCacheObjectHelper::STATE));
    _read(nInFile, &m_nUsageCount, sizeof(long));
    _read(nInFile, &m_nTimeStamp, sizeof(long));
}

CCacheObjectHelper::operator const char*() const throw()
{
    const char* lpszLoadState = 
        ( m_LoadState == CCacheObjectHelper::OBJECT_IN_MEMORY ? "IN MEMORY" :
                         m_LoadState == CCacheObjectHelper::OBJECT_IN_DISK ? "IN DISK" : "EMPTY" );

    _snprintf( (char *) m_szCacheObjState, 
               sizeof(m_szCacheObjState), 
               "[%s, %s, %s, (%lu * %lu = %lu)]",
               m_szCacheObjName, 
               m_szCacheObjFile, 
               lpszLoadState,
               m_nUsageCount, 
               m_nTimeStamp, 
               (m_nUsageCount * m_nTimeStamp) );

    return (char *) m_szCacheObjState;
}
