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
// CacheObjectHelper.h: interface for the CCacheObjectHelper class.
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

#if !defined(_CACHEOBJECTHELPER_H__INCLUDED_)
#define _CACHEOBJECTHELPER_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#endif // UNIX

#include "number.h"

class CCacheMgr;
class ICacheObject;


//////////////////////////////////////////////////////////////////////
// This class implements a cache object wrapper which wraps all the
// cache objects enumerated by the cache manager.

class CCacheObjectHelper  
{
public:

    CCacheObjectHelper( char* pszCacheName, 
                        char* pszCacheDirPath, 
                        ICacheObject* pCacheObject );

    virtual ~CCacheObjectHelper();

    enum STATE { OBJECT_EMPTY, OBJECT_IN_MEMORY, OBJECT_IN_DISK };

    const char* getCachedObjectName();
    ICacheObject* getCachedObject(const char* pszName);

    void unloadCachedObject();
    void serializeCachedObject();
    void deleteCachedObject();

    void updateMFU();
    bool operator<(CCacheObjectHelper &CacheObject);
    bool operator>(CCacheObjectHelper &CacheObject);

    void serialize(int nOutFile);
    void deserialize(int nInFile);

    operator const char*() const throw();

private:

    int m_nIndex;
    int m_nPrev;
    int m_nNext;

    STATE m_LoadState;

    char m_szCacheObjName[512];
    char m_szCacheObjFile[MAX_PATH];

    ICacheObject* m_pCacheObject;

    DWORD m_nUsageCount;
    DWORD m_nTimeStamp;

    // Used to create unique name 
    // for serializing to disk cache
    static Number<long> g_nCounter;

    // State string populated each time this 
    // object is cast to a char * type
    char m_szCacheObjState[512];

    // Enable cache manager access 
    // private members of this class
    friend class CCacheMgr;
};

#endif // !defined(AFX_CACHEOBJECTHELPER_H__BD03494B_36CD_4DD5_B103_EEBF379350C5__INCLUDED_)
