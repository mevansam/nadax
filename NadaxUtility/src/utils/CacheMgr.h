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
// CacheMgr.h: interface for the CCacheMgr class.
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

#if !defined(_CACHEMGR_H__INCLUDED_)
#define _CACHEMGR_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#include "pthread.h"
#endif // UNIX

#include "macros.h"
#include "criticalsection.h"
#include "ReaderWriter.h"

#include <string>
#include <map>

class ICacheObjectFactory;
class ICacheObject;
class CCacheObjectHelper;

typedef std::map<std::string, CCacheObjectHelper *> CCacheObjectMap;


//////////////////////////////////////////////////////////////////////
// This class implements a cache manager which manages a cache of
// client objects that can be cached in memory and at the same time in
// disk as well.

class CCacheMgr  
{
public:

    CCacheMgr( char* szCacheName,
               int nMaxMemSize,
               ICacheObjectFactory* pCacheObjectFactory );

    CCacheMgr( char* szCacheName,
               char* szCacheDirPath,
               int nMaxMemSize,
               int nMaxDirSize,
               ICacheObjectFactory* pCacheObjectFactory );

    virtual ~CCacheMgr();

    void closeCacheMgr(bool bSave = false);

    void clear();
    void clear(char* szName);

    void* get(char* szName);

    operator const char*() const throw();

private:

    enum CACHETYPE { MEM_CACHE = 1, DIR_CACHE = 2 };

    void initCacheMgr( char* szCacheName,
                       char* szCacheDirPath,
                       int nMaxMemSize,
                       int nMaxDirSize,
                       ICacheObjectFactory* pCacheObjectFactory );

    void moveObjectMemToDir();
    int popHead(CCacheMgr::CACHETYPE CacheType);
    int popTail(CCacheMgr::CACHETYPE CacheType);
    void pushHead(int i, CCacheMgr::CACHETYPE CacheType);
    void pushTail(int i, CCacheMgr::CACHETYPE CacheType);
    void remove(int i, CCacheMgr::CACHETYPE CacheType);
    void updatePosition(int i, CCacheMgr::CACHETYPE CacheType);

private:

    char m_szCacheName[512];
    char m_szCacheDirPath[MAX_PATH];

    int m_nMaxMemSize;
    int m_nMaxDirSize;
    int m_nCurrMemSize;

    CCacheObjectMap m_CacheObjectMap;
    CCacheObjectHelper** m_ppCacheObjects;

    int m_nFreeObject;

    int m_nMemObjsHead;
    int m_nMemObjsTail;

    int m_nDirObjsHead;
    int m_nDirObjsTail;

    CriticalSection m_csMemObjList;
    CReaderWriter m_rwCacheObectMap;

    // State string populated each time this 
    // object is cast to a char * type
    char m_szCacheMgrState[8192];
};

#endif // !defined(_CACHEMGR_H__INCLUDED_)
