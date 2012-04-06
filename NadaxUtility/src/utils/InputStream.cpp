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
// CInputStream.cpp : Implementtion for CInputStream class.
//
// Author  : Mevan Samaratunga
// Date    : 05/25/2001
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

#include "InputStream.h"

#ifdef WIN32
#endif // UNIX

#ifdef UNIX
#endif // WIN32

#include "debug.h"


//////////////////////////////////////////////////////////////////////
// Exception messages defines

#define EXCEP_NULLINTPUT  "Interface to input device cannot be null."
#define EXCEP_SYNCERROR   "Unable to initialize the input stream's synchronization objects."
#define EXCEP_NOTMARKED   "Stream was not marked in order to reset the stream pointer."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CInputStream::CInputStream( IInput* pInput,
                            long nBlockSize,
                            int nReadAhead,
                            int nReadTimeout )
    : m_semDataAvailable(0, 1), m_semReadAhead(nReadAhead, nReadAhead)
{
    if (!pInput)
    {
        THROW(CException, EXCEP_MSSG(EXCEP_NULLINTPUT));
    }

    m_pInput = pInput;

    m_nBlockSize   = nBlockSize;
    m_nBeginBlock  = 0;
    m_nEndBlock    = 0;
    m_nBeginOffset = 0;
    m_nEndOffset   = 0;

    m_pBuffer = (char *) malloc(nBlockSize * nReadAhead);

    m_bDataAvailable = true;
    m_nReadAhead     = nReadAhead;
    m_nReadTimeout   = nReadTimeout;

    m_bRun = true;

    this->start();
    this->setPriority(THREAD_PRIORITY_NORMAL);
}

CInputStream::~CInputStream()
{
    close();

    free(m_pBuffer);
}


//////////////////////////////////////////////////////////////////////
// Public Method Interface Implementation

void CInputStream::reset()
{
}

void CInputStream::mark(long nReadLimit)
{
}

long CInputStream::skip(long nLen)
{
    return 0;
}

long CInputStream::available()
{
    return 0;
}

char CInputStream::read()
{
    char ch = -1;
    read(&ch, 1);
    return ch;
}

long CInputStream::read(char* pBuffer, long nLen)
{
    if (m_bDataAvailable)
    {
        char* pInputBuffer;
        long nInputLen;

        long nBufferLen = 0;

        pInputBuffer = m_pBuffer + ((m_nBeginBlock % m_nReadAhead) * m_nBlockSize) + m_nBeginOffset;

        while (nLen)
        {
            m_csBuffer.enter();

            // Check if there is data to be read 
            // in the current buffer block
            if (m_nBeginBlock == m_nEndBlock)
            {
                if (m_nBeginOffset == m_nEndOffset)
                {
                    m_csBuffer.exit();

                    if (m_bRun)
                    {
                        if (m_nReadTimeout > 0)
                        {
                            if (m_semDataAvailable.p(m_nReadTimeout))
                            {
                                break;
                            }
                        }
                        else
                        {
                            // Wait indefinitely until data becomes available
                            m_semDataAvailable.p();
                        }
                        continue;
                    }
                    else
                    {
                        m_bDataAvailable = false;
                        break;
                    }
                }

                nInputLen = m_nEndOffset - m_nBeginOffset;
            }
            else
            {
                nInputLen = m_nBlockSize - m_nBeginOffset;
            }

            m_csBuffer.exit();

            nInputLen = min(nInputLen, nLen);
            memcpy(pBuffer, pInputBuffer, nInputLen);

#ifdef _DEBUG
#ifdef _CHAR_STREAM
                _debugPrint("\tReading ", pBuffer, nInputLen);
#endif
#endif

            nLen -= nInputLen;
            pBuffer += nInputLen;
            nBufferLen += nInputLen;
            pInputBuffer += nInputLen;

            m_nBeginOffset += nInputLen;
            if (m_nBeginOffset == m_nBlockSize)
            {
                ++m_nBeginBlock;
                m_nBeginOffset = 0;

                pInputBuffer = (m_nBeginBlock % m_nReadAhead ? pInputBuffer : m_pBuffer);

                // Signal that a buffer has been consumed
                m_semReadAhead.v();
            }
        }

        return nBufferLen;
    }

    return -1;
}

void CInputStream::close()
{
    // Flag the thread loop to stop and signal the 
    // read ahead in case the thread is waiting
    m_bRun = false;
    m_semReadAhead.v();

    // Wait for thread to terminate
    this->join();
}


//////////////////////////////////////////////////////////////////////
// Implementation

void* CInputStream::run()
{
    char* pInputBuffer;
    long nInputLen;

    try
    {
        while (m_bRun)
        {
            // Wait until space is available to read more blocks
            m_semReadAhead.p();

            pInputBuffer = m_pBuffer + ((m_nEndBlock % m_nReadAhead) * m_nBlockSize);

            while (m_bRun)
            {
                nInputLen = m_pInput->read(pInputBuffer, m_nBlockSize - m_nEndOffset);

#ifdef _DEBUG
#ifdef _CHAR_STREAM
                _debugPrint("\tBuffering ", pInputBuffer, nInputLen);
#endif
#endif

                if (nInputLen > 0)
                {
                    m_csBuffer.enter();
                    m_nEndOffset += nInputLen;

                    if (m_nEndOffset == m_nBlockSize)
                    {
                        ++m_nEndBlock;
                        m_nEndOffset = 0;

                        m_csBuffer.exit();
                        m_semDataAvailable.v();

                        break;
                    }

                    m_csBuffer.exit();
                    m_semDataAvailable.v();

                    pInputBuffer += nInputLen;
                }
                else if (nInputLen == -1)
                {
                    m_bRun = false;
                }

                // Allow OS to do a context switch to 
                // another thread of equal priority
                CThread::yield();
            }
        }

        m_semDataAvailable.v();
    }
    catch (CException* pe)
    {
        m_pInput = NULL;
        delete pe;
    }

    return NULL;
}
