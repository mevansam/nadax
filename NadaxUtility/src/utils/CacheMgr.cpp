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
// CacheMgr.cpp: implementation of the CCacheMgr class.
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

#include "CacheMgr.h"

#ifdef WIN32
#include <io.h>
#include <direct.h>
#endif // WIN32

#ifdef UNIX
#include <unistd.h>
#endif // UNIX

#include "CacheException.h"
#include "CacheObjectHelper.h"
#include "ICacheObjectFactory.h"
#include "ICacheObject.h"

#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CCacheMgr::CCacheMgr( char* szCacheName,
                      int nMaxMemSize,
                      ICacheObjectFactory* pCacheObjectFactory )
{
    m_rwCacheObectMap.startWriting();
    initCacheMgr(szCacheName, NULL, nMaxMemSize, nMaxMemSize, pCacheObjectFactory);
    m_rwCacheObectMap.endWriting();
}

CCacheMgr::CCacheMgr( char* szCacheName,
                      char* szCacheDirPath,
                      int nMaxMemSize,
                      int nMaxDirSize,
                      ICacheObjectFactory* pCacheObjectFactory )
{
    m_rwCacheObectMap.startWriting();
    initCacheMgr(szCacheName, szCacheDirPath, nMaxMemSize, nMaxDirSize, pCacheObjectFactory);
    m_rwCacheObectMap.endWriting();
}

CCacheMgr::~CCacheMgr() 
{ 
    closeCacheMgr();
}


//////////////////////////////////////////////////////////////////////
// Cache Management Implementation

void CCacheMgr::initCacheMgr( char* szCacheName,
                              char* szCacheDirPath,
                              int nMaxMemSize,
                              int nMaxDirSize,
                              ICacheObjectFactory* pCacheObjectFactory )
{
    if (szCacheName == NULL)
    {
        THROW(CCacheException, EXCEP_MSSG("A cache name was not specified."));
    }
    else if (nMaxMemSize <= 0)
    {
        THROW(CCacheException, EXCEP_MSSG("Memory cache size must always be greater than zero."));
    }
    else if (nMaxDirSize < nMaxMemSize)
    {
        THROW(CCacheException, EXCEP_MSSG("Disk cache size should always be greater than or equal to the memory cache size."));
    }
    else if (pCacheObjectFactory == NULL)
    {
        THROW(CCacheException, EXCEP_MSSG("A cache object factory was not specified."));
    }

    strncpy(m_szCacheName, szCacheName, sizeof(m_szCacheName));

    m_nMaxMemSize = nMaxMemSize;
    m_nMaxDirSize = nMaxDirSize;
    m_nCurrMemSize = 0;

    // Create cache directory if it does not exist

    if (szCacheDirPath)
    {
        strncpy(m_szCacheDirPath, szCacheDirPath, sizeof(m_szCacheDirPath));

        struct _stat FileStatus;

        if ( _stat(m_szCacheDirPath, &FileStatus) != 0 ||
             !(FileStatus.st_mode & _S_IFDIR) )
        {
            if (MKDIR(m_szCacheDirPath) != 0)
            {
                THROW(CCacheException, EXCEP_MSSG("Unable to create a working directory for the cache."));
            }
        }

        int nLen = strlen(m_szCacheDirPath);
        if (m_szCacheDirPath[nLen - 1] != PATH_SEP)
        {
            m_szCacheDirPath[nLen] = PATH_SEP;
            m_szCacheDirPath[nLen + 1] = 0;
        }
    }
    else
    {
        m_szCacheDirPath[0] = 0;
    }

    // Create cache objects wrappers

    m_ppCacheObjects = new CCacheObjectHelper*[m_nMaxDirSize];

    int i;
    for (i = 0; i < m_nMaxDirSize; i++)
    {
        ICacheObject* pCacheObject;

        if (!(pCacheObject = pCacheObjectFactory->create()))
        {
            THROW(CCacheException, EXCEP_MSSG("The cache object factory returned a null pointer."));
        }

        m_ppCacheObjects[i] = new CCacheObjectHelper( m_szCacheName, 
                                                      m_szCacheDirPath, 
                                                      pCacheObject );

        m_ppCacheObjects[i]->m_nIndex = i;
        m_ppCacheObjects[i]->m_nPrev = (i == 0 ? -1 : i - 1);
    }

    m_nFreeObject = m_nMaxDirSize - 1;

    m_nMemObjsHead = -1;
    m_nMemObjsTail = -1;

    m_nDirObjsHead = -1;
    m_nDirObjsTail = -1;

    // Check if a cache with the given cache name exists
    // in the given dirPath. If so then load it. The cache
    // size remains fixed as the values passed into the 
    // this method.

    char szCachMmgrIndex[MAX_PATH];

    strncpy(szCachMmgrIndex, m_szCacheDirPath, MAX_PATH);
    strncat(szCachMmgrIndex, m_szCacheName, MAX_PATH);

    int fh;

    if ((fh = _open(szCachMmgrIndex, _O_RDONLY | _O_BINARY)) != -1)
    {
        int nNumCacheObjects;
        _read(fh, &nNumCacheObjects, sizeof(int));

        int j;
        for (j = 0; j < nNumCacheObjects; j++)
        {
            if (m_nFreeObject == -1)
            {
                CCacheObjectHelper CacheObjectHelper(NULL, NULL, NULL);
                CacheObjectHelper.deserialize(fh);
                CacheObjectHelper.deleteCachedObject();
            }
            else
            {
                CCacheObjectHelper* pCacheObjectHelper = m_ppCacheObjects[m_nFreeObject];
                m_nFreeObject = pCacheObjectHelper->m_nPrev;

                pCacheObjectHelper->deserialize(fh);
                m_CacheObjectMap[pCacheObjectHelper->getCachedObjectName()] = pCacheObjectHelper;
                pushTail(pCacheObjectHelper->m_nIndex, DIR_CACHE);
            }
        }

        _close(fh);
    }
}

void CCacheMgr::closeCacheMgr(bool bSave)
{
    m_rwCacheObectMap.startWriting();

    int i;

    if (m_ppCacheObjects)
    {
        if (m_szCacheDirPath[0])
        {
            if (bSave)
            {
                // Push all objects in memory to disk
                while (m_nCurrMemSize > 0) moveObjectMemToDir();

                // Serialize cache manager

                char szCachMmgrIndex[MAX_PATH];

                strncpy(szCachMmgrIndex, m_szCacheDirPath, MAX_PATH);
                strncat(szCachMmgrIndex, m_szCacheName, MAX_PATH);

                int fh;
                int nNumCacheObjects;

                if ( (fh = _open( szCachMmgrIndex, 
                                  O_CREAT | O_WRONLY | O_TRUNC | _O_BINARY, 
                                  _S_IREAD | _S_IWRITE )) != -1 )
                {
                    for ( i = m_nDirObjsHead, nNumCacheObjects = 0;
                          i != -1; 
                          i = m_ppCacheObjects[i]->m_nNext, nNumCacheObjects++ );

                    _write(fh, &nNumCacheObjects, sizeof(int));

                    for (i = m_nDirObjsHead; i != -1; i = m_ppCacheObjects[i]->m_nNext)
                        m_ppCacheObjects[i]->serialize(fh);

                    _close(fh);
                }
            }
            else
            {
                // Delete all cache files
                for (i = 0; i < m_nMaxDirSize; i++)
                    m_ppCacheObjects[i]->deleteCachedObject();
            }
        }

        // Release objects from memory

        for (i = 0; i < m_nMaxDirSize; i++)
        {
            delete m_ppCacheObjects[i];
        }
    
        delete[] m_ppCacheObjects;
        m_ppCacheObjects = NULL;
    }

    m_szCacheName[0] = 0x0;
    m_szCacheDirPath[0] = 0x0;

    m_nMaxMemSize = -1;
    m_nMaxDirSize = -1;
    m_nCurrMemSize = -1;

    m_nFreeObject = -1;

    m_nMemObjsHead = -1;
    m_nMemObjsTail = -1;

    m_nDirObjsHead = -1;
    m_nDirObjsTail = -1;

    m_CacheObjectMap.clear();

    m_rwCacheObectMap.endWriting();
}

void CCacheMgr::clear()
{
    m_rwCacheObectMap.startWriting();

    int i;
    for (i = 0; i < m_nMaxDirSize; i++)
    {
        m_ppCacheObjects[i]->deleteCachedObject();

        m_ppCacheObjects[i]->m_nIndex = i;
        m_ppCacheObjects[i]->m_nNext = -1;
        m_ppCacheObjects[i]->m_nPrev = (i == 0 ? -1 : i - 1);
    }
    
    m_nFreeObject = m_nMaxDirSize - 1;
    m_CacheObjectMap.clear();

    m_rwCacheObectMap.endWriting();
}

void CCacheMgr::clear(char* szName)
{
    m_rwCacheObectMap.startWriting();

    CCacheObjectMap::iterator Iterator = m_CacheObjectMap.find(szName);

    if (Iterator != m_CacheObjectMap.end())
    {
        CCacheObjectHelper* pCacheObjectHelper = Iterator->second;
        m_CacheObjectMap.erase(szName);

        // Remove cache container object from lists 
        // and lookup map and insert into free list

        if (pCacheObjectHelper->m_LoadState == CCacheObjectHelper::OBJECT_IN_MEMORY)
        {
            remove(pCacheObjectHelper->m_nIndex, MEM_CACHE);
        }
        else if (pCacheObjectHelper->m_LoadState == CCacheObjectHelper::OBJECT_IN_DISK)
        {
            remove(pCacheObjectHelper->m_nIndex, DIR_CACHE);
        }

        pCacheObjectHelper->unloadCachedObject();
        pCacheObjectHelper->m_nNext = -1;
        pCacheObjectHelper->m_nPrev = m_nFreeObject;
        m_nFreeObject = pCacheObjectHelper->m_nIndex;
    }

    m_rwCacheObectMap.endWriting();
}


//////////////////////////////////////////////////////////////////////
// Cache Access Implementation

void* CCacheMgr::get(char* szName)
{
    CCacheObjectHelper* pCacheObjectHelper = NULL;
    void* pData = NULL;

    // Look up object in map
    bool bReading = true;
    m_rwCacheObectMap.startReading();

    CCacheObjectMap::iterator Iterator = m_CacheObjectMap.find(szName);
    bool bObjectInCache = true;

    if ( Iterator == m_CacheObjectMap.end() ||
         Iterator->second->m_LoadState == CCacheObjectHelper::OBJECT_IN_DISK )
    {
        bReading = false;
        m_rwCacheObectMap.endReading();
        m_rwCacheObectMap.startWriting();

        // Confirm whether whilst waiting some other 
        // thread loaded the required object.
        bObjectInCache = ((Iterator = m_CacheObjectMap.find(szName)) != m_CacheObjectMap.end());
    }

    if (bObjectInCache)
    {
        pCacheObjectHelper = Iterator->second;

        if (pCacheObjectHelper->m_LoadState == CCacheObjectHelper::OBJECT_IN_DISK)
        {
            remove(pCacheObjectHelper->m_nIndex, DIR_CACHE);
            if (m_nCurrMemSize == m_nMaxMemSize) moveObjectMemToDir();
            pushTail(pCacheObjectHelper->m_nIndex, MEM_CACHE);
        }
    }
    else
    {
        if (m_nFreeObject == -1)
        {
            // Pop the tail object in the dir list if
            // objects are serialized to disk else get
            // the tail object in the memory list. The
            // object currently loaded is evicted from
            // the cache.

            if (m_nMaxDirSize > m_nMaxMemSize)
            {
                pCacheObjectHelper = m_ppCacheObjects[popTail(DIR_CACHE)];
                if (m_nCurrMemSize == m_nMaxMemSize) moveObjectMemToDir();
            }
            else
            {
                pCacheObjectHelper = m_ppCacheObjects[popTail(MEM_CACHE)];
            }

            m_CacheObjectMap.erase(pCacheObjectHelper->getCachedObjectName());
            pCacheObjectHelper->unloadCachedObject();
        }
        else
        {
            // Get a free object from the free list
            pCacheObjectHelper = m_ppCacheObjects[m_nFreeObject];
            m_nFreeObject = m_ppCacheObjects[m_nFreeObject]->m_nPrev;

            if (m_nCurrMemSize == m_nMaxMemSize) moveObjectMemToDir();
        }

        m_CacheObjectMap[szName] = pCacheObjectHelper;
        pushHead(pCacheObjectHelper->m_nIndex, MEM_CACHE);
    }

    try
    {
        if (bReading) m_csMemObjList.enter();
        pCacheObjectHelper->updateMFU();
        updatePosition(pCacheObjectHelper->m_nIndex, MEM_CACHE);
        if (bReading) m_csMemObjList.exit();

        pData = pCacheObjectHelper->getCachedObject(szName)->getData();
    }
    catch (...)
    {
        if (bReading)
        {
            // We have a problem with an already loaded cache 
            // object. This cannot be handled and requires the 
            // cache to be explicitely cleared.
            m_rwCacheObectMap.endReading();
            throw;
        }
        else
        {
            // Unload object and release to free list
            m_CacheObjectMap.erase(szName);
            remove(pCacheObjectHelper->m_nIndex, MEM_CACHE);

            pCacheObjectHelper->unloadCachedObject();
            pCacheObjectHelper->m_nNext = -1;
            pCacheObjectHelper->m_nPrev = m_nFreeObject;
            m_nFreeObject = pCacheObjectHelper->m_nIndex;
        }
    }

    if (bReading)
    {
        m_rwCacheObectMap.endReading();
    }
    else
    {
        m_rwCacheObectMap.endWriting();
    }

    return pData;
}

void CCacheMgr::moveObjectMemToDir()
{
    // NB: Reader writer usage of caller guarantees single 
    // threaded access to resources used within this method.
    
    int i = popTail(MEM_CACHE);
    m_ppCacheObjects[i]->serializeCachedObject();

    pushHead(i, DIR_CACHE);
    updatePosition(i, DIR_CACHE);
}

int CCacheMgr::popHead(CCacheMgr::CACHETYPE CacheType)
{
    // NB: Reader writer usage of caller guarantees single 
    // threaded access to resources used within this method.
    
    int i = -1;

    switch (CacheType)
    {
        case MEM_CACHE:
        {
            if (m_nMemObjsHead != -1)
            {
                i = m_nMemObjsHead;
                m_nMemObjsHead = m_ppCacheObjects[i]->m_nNext;

                if (m_nMemObjsHead == -1)
                    m_nMemObjsTail = -1;
                else
                    m_ppCacheObjects[m_nMemObjsHead]->m_nPrev = -1;

                m_ppCacheObjects[i]->m_nNext = -1;
                m_nCurrMemSize--;
            }
            break;
        }
        case DIR_CACHE:
        {
            if (m_nDirObjsHead != -1)
            {
                i = m_nDirObjsHead;
                m_nDirObjsHead = m_ppCacheObjects[i]->m_nNext;

                if (m_nDirObjsHead == -1)
                    m_nDirObjsTail = -1;
                else
                    m_ppCacheObjects[m_nDirObjsHead]->m_nPrev = -1;

                m_ppCacheObjects[i]->m_nNext = -1;
            }
            break;
        }
    }

    return i;
}

int CCacheMgr::popTail(CCacheMgr::CACHETYPE CacheType)
{
    // NB: Reader writer usage of caller guarantees single 
    // threaded access to resources used within this method.
    
    int i = -1;

    switch (CacheType)
    {
        case MEM_CACHE:
        {
            if (m_nMemObjsTail != -1)
            {
                i = m_nMemObjsTail;
                m_nMemObjsTail = m_ppCacheObjects[i]->m_nPrev;

                if (m_nMemObjsTail == -1)
                    m_nMemObjsHead = -1;
                else
                    m_ppCacheObjects[m_nMemObjsTail]->m_nNext = -1;

                m_ppCacheObjects[i]->m_nPrev = -1;
                m_nCurrMemSize--;
            }
            break;
        }
        case DIR_CACHE:
        {
            if (m_nDirObjsTail != -1)
            {
                i = m_nDirObjsTail;
                m_nDirObjsTail = m_ppCacheObjects[i]->m_nPrev;

                if (m_nDirObjsTail == -1)
                    m_nDirObjsHead = -1;
                else
                    m_ppCacheObjects[m_nDirObjsTail]->m_nNext = -1;

                m_ppCacheObjects[i]->m_nPrev = -1;
            }
            break;
        }
    }

    return i;
}

void CCacheMgr::pushHead(int i, CCacheMgr::CACHETYPE CacheType)
{
    // NB: Reader writer usage of caller guarantees single 
    // threaded access to resources used within this method.
    
    switch (CacheType)
    {
        case MEM_CACHE:
        {
            m_ppCacheObjects[i]->m_nPrev = -1;
            m_ppCacheObjects[i]->m_nNext = m_nMemObjsHead;
            m_nMemObjsHead = i;

            if (m_nMemObjsTail == -1)
                m_nMemObjsTail = m_nMemObjsHead;
            else
                m_ppCacheObjects[m_ppCacheObjects[i]->m_nNext]->m_nPrev = i;

            m_nCurrMemSize++;
            break;
        }
        case DIR_CACHE:
        {
            m_ppCacheObjects[i]->m_nPrev = -1;
            m_ppCacheObjects[i]->m_nNext = m_nDirObjsHead;
            m_nDirObjsHead = i;

            if (m_nDirObjsTail == -1)
                m_nDirObjsTail = m_nDirObjsHead;
            else
                m_ppCacheObjects[m_ppCacheObjects[i]->m_nNext]->m_nPrev = i;

            break;
        }
    }
}

void CCacheMgr::pushTail(int i, CCacheMgr::CACHETYPE CacheType)
{
    // NB: Reader writer usage of caller guarantees single 
    // threaded access to resources used within this method.
    
    switch (CacheType)
    {
        case MEM_CACHE:
        {
            m_ppCacheObjects[i]->m_nPrev = m_nMemObjsTail;
            m_ppCacheObjects[i]->m_nNext = -1;
            m_nMemObjsTail = i;

            if (m_nMemObjsHead == -1)
                m_nMemObjsHead = m_nMemObjsTail;
            else
                m_ppCacheObjects[m_ppCacheObjects[i]->m_nPrev]->m_nNext = i;

            m_nCurrMemSize++;
            break;
        }
        case DIR_CACHE:
        {
            m_ppCacheObjects[i]->m_nPrev = m_nDirObjsTail;
            m_ppCacheObjects[i]->m_nNext = -1;
            m_nDirObjsTail = i;

            if (m_nDirObjsHead == -1)
                m_nDirObjsHead = m_nDirObjsTail;
            else
                m_ppCacheObjects[m_ppCacheObjects[i]->m_nPrev]->m_nNext = i;

            break;
        }
    }
}

void CCacheMgr::remove(int i, CCacheMgr::CACHETYPE CacheType)
{
    // NB: Reader writer usage of caller guarantees single 
    // threaded access to resources used within this method.
    
    int m_nPrev = m_ppCacheObjects[i]->m_nPrev;
    int m_nNext = m_ppCacheObjects[i]->m_nNext;

    if (m_ppCacheObjects[i]->m_nPrev == -1)
    {
        // Object at head
        switch (CacheType)
        {
            case MEM_CACHE:
                m_nMemObjsHead = m_nNext;
                if (m_nMemObjsHead == -1) m_nMemObjsTail = -1;
                break;

            case DIR_CACHE:
                m_nDirObjsHead = m_nNext;
                if (m_nDirObjsHead == -1) m_nDirObjsTail = -1;
                break;
        }
    }
    else if (m_ppCacheObjects[i]->m_nNext == -1)
    {
        // Object at tail
        switch (CacheType)
        {
            case MEM_CACHE:
                m_nMemObjsTail = m_nPrev;
                if (m_nMemObjsTail== -1) m_nMemObjsHead = -1;
                break;

            case DIR_CACHE:
                m_nDirObjsTail = m_nPrev;
                if (m_nDirObjsTail == -1) m_nDirObjsHead = -1;
                break;
        }
    }

    if (m_nPrev != -1)
        m_ppCacheObjects[m_nPrev]->m_nNext = m_nNext;
    if (m_nNext != -1)
        m_ppCacheObjects[m_nNext]->m_nPrev = m_nPrev;

    m_ppCacheObjects[i]->m_nNext = -1;
    m_ppCacheObjects[i]->m_nPrev = -1;

    if (CacheType == MEM_CACHE) m_nCurrMemSize--;
}

void CCacheMgr::updatePosition(int i, CCacheMgr::CACHETYPE CacheType)
{
    // NB: Reader writer usage of caller guarantees single 
    // threaded access to resources used within this when
    // doing updates to the disk cache. 

    int m_nPrev, m_nNext;

    while (true)
    {
        m_nPrev = m_ppCacheObjects[i]->m_nPrev;
        m_nNext = m_ppCacheObjects[i]->m_nNext;

        if ( m_nPrev != -1 &&
             *m_ppCacheObjects[i] > *m_ppCacheObjects[m_nPrev] )
        {
            /* Object moves towards head of list */

            if (m_nNext == -1)
            {
                /* Object moves away from tail */
                switch (CacheType)
                {
                    case MEM_CACHE:
                        m_nMemObjsTail = m_nPrev;
                        break;
                    case DIR_CACHE:
                        m_nDirObjsTail = m_nPrev;
                        break;
                    default:
                        return;
                }
            }
            else
            {
                m_ppCacheObjects[m_nNext]->m_nPrev = m_nPrev;
            }

            m_ppCacheObjects[m_nPrev]->m_nNext = m_nNext;

            m_ppCacheObjects[i]->m_nPrev = m_ppCacheObjects[m_nPrev]->m_nPrev;
            m_ppCacheObjects[i]->m_nNext = m_nPrev;
            m_ppCacheObjects[m_nPrev]->m_nPrev = i;

            if (m_ppCacheObjects[i]->m_nPrev != -1)
            {
                m_ppCacheObjects[m_ppCacheObjects[i]->m_nPrev]->m_nNext = i;
            }
            else
            {
                /* Object moved to head */
                switch (CacheType)
                {
                    case MEM_CACHE:
                        m_nMemObjsHead = i;
                        break;
                    case DIR_CACHE:
                        m_nDirObjsHead = i;
                        break;
                    default:
                        return;
                }
                break;
            }
        }
        else if ( m_nNext != -1 &&
                  *m_ppCacheObjects[i] < *m_ppCacheObjects[m_nNext] )
        {
            /* Object moves towards tail of list */

            if (m_ppCacheObjects[i]->m_nPrev == -1)
            {
                /* Object moves away from head */
                switch (CacheType)
                {
                    case MEM_CACHE:
                        m_nMemObjsHead = m_nNext;
                        break;
                    case DIR_CACHE:
                        m_nDirObjsHead = m_nNext;
                        break;
                    default:
                        return;
                }
            }
            else
            {
                m_ppCacheObjects[m_nPrev]->m_nNext = m_nNext;
            }

            m_ppCacheObjects[m_nNext]->m_nPrev = m_nPrev;

            m_ppCacheObjects[i]->m_nNext = m_ppCacheObjects[m_nNext]->m_nNext;
            m_ppCacheObjects[i]->m_nPrev = m_nNext;
            m_ppCacheObjects[m_nNext]->m_nNext = i;

            if (m_ppCacheObjects[i]->m_nNext != -1)
            {
                m_ppCacheObjects[m_ppCacheObjects[i]->m_nNext]->m_nPrev = i;
            }
            else
            {
                /* Object moved to tail */
                switch (CacheType)
                {
                    case MEM_CACHE:
                        m_nMemObjsTail = i;
                        break;
                    case DIR_CACHE:
                        m_nDirObjsTail = i;
                        break;
                    default:
                        return;
                }
                break;
            }
        }
        else
        {
            break;
        }
    }
}

CCacheMgr::operator const char*() const throw()
{
    int nBufferLen = sizeof(m_szCacheMgrState);
    char* pszCacheMgrState = (char *) m_szCacheMgrState;

    _snprintf( pszCacheMgrState, 
               min(nBufferLen, sizeof(m_szCacheMgrState)), 
               "\nCache '%s' state :\n* Memory cache :\n",
               m_szCacheName );

    int nLen = strlen(pszCacheMgrState);
    pszCacheMgrState += nLen;
    nBufferLen += nLen;

    int i = m_nMemObjsHead;
    while (i != -1)
    {
        _snprintf( pszCacheMgrState, 
                   min(nBufferLen, sizeof(m_szCacheMgrState)), 
                   "  %d) %s\n",
                   i, (const char *) *m_ppCacheObjects[i] );

        nLen = strlen(pszCacheMgrState);
        pszCacheMgrState += nLen;
        nBufferLen += nLen;

        i = m_ppCacheObjects[i]->m_nNext;
    }

    _snprintf( pszCacheMgrState,
               min(nBufferLen, sizeof(m_szCacheMgrState)), 
               "* Disk cache :\n" );

    nLen = strlen(pszCacheMgrState);
    pszCacheMgrState += nLen;
    nBufferLen += nLen;

    i = m_nDirObjsHead;
    while (i != -1)
    {
        _snprintf( pszCacheMgrState, 
                   min(nBufferLen, sizeof(m_szCacheMgrState)), 
                   "  %d) %s\n",
                   i, (const char *) *m_ppCacheObjects[i] );

        nLen = strlen(pszCacheMgrState);
        pszCacheMgrState += nLen;
        nBufferLen += nLen;

        i = m_ppCacheObjects[i]->m_nNext;
    }

    _snprintf( pszCacheMgrState,
               min(nBufferLen, sizeof(m_szCacheMgrState)), 
               "* State [%d, %d, %d, %d] :\n",
               m_nMemObjsHead,
               m_nMemObjsTail,
               m_nDirObjsHead,
               m_nDirObjsTail );

    nLen = strlen(pszCacheMgrState);
    pszCacheMgrState += nLen;
    nBufferLen += nLen;
    
    return m_szCacheMgrState;
}
