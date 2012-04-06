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
// COutputStream.cpp : Implementation for COutputStream class.
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
////////////////////////////////////////////////////////////////////////

#include "OutputStream.h"

#ifdef WIN32
#endif // WIN32

#ifdef UNIX
#endif // UNIX

#include "debug.h"


//////////////////////////////////////////////////////////////////////
// Exception messages defines

#define EXCEP_NULLOUTPUT  "Interface to output device cannot be null."
#define EXCEP_SYNCERROR   "Unable to initialize the output stream's synchronization objects."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

COutputStream::COutputStream( IOutput* pOutput,
                              long nBlockSize, 
                              int nBlockThreshold )
    : m_semFlushAll(0, 1)
{
    if (!pOutput)
    {
        THROW(CException, EXCEP_MSSG(EXCEP_NULLOUTPUT));
    }

    m_pOutput = pOutput;

    m_nBlockSize      = nBlockSize;
    m_nBlockThreshold = nBlockThreshold;

    m_nBeginOffset = 0;
    m_nEndOffset   = 0;

    m_buffer.push_back((char *) malloc(m_nBlockSize));

    m_bRun   = true;
    m_bFlush = false;

    this->start();
    this->setPriority(THREAD_PRIORITY_NORMAL);
}

COutputStream::~COutputStream()
{
    this->close();
}


//////////////////////////////////////////////////////////////////////
// Public Method Interface Implementation

void COutputStream::write(const char ch)
{
    this->write(&ch, 1);
}

void COutputStream::write(const char* pBuffer, long nLen)
{
    char* pOutputBuffer;
    long nOutputLen;

    while (nLen > 0)
    {
        pOutputBuffer = m_buffer.back();
        nOutputLen = m_nBlockSize - m_nEndOffset;
        nOutputLen = min(nOutputLen, nLen);
        memcpy(pOutputBuffer + m_nEndOffset, pBuffer, nOutputLen);

#ifdef _DEBUG
#ifdef _CHAR_STREAM
        _debugPrint("\tBuffering ", pBuffer, nOutputLen);
#endif
#endif
        m_csBuffer.enter();

        m_nEndOffset += nOutputLen;
        if (m_nEndOffset == m_nBlockSize)
        {
            // Signal thread to flush blocks in buffer
            if ((m_buffer.size() % m_nBlockThreshold) == 0) m_semFlush.v();            
            
            m_nEndOffset = 0;
            if (!m_free.empty())
            {
                m_buffer.push_back(m_free.top());
                m_free.pop();
            }
            else
            {
                m_buffer.push_back((char *) malloc(m_nBlockSize));
            }
        }

        m_csBuffer.exit();

        nLen -= nOutputLen;
        pBuffer += nOutputLen;
    }
}

void COutputStream::flush()
{
    // Ensure that all pending 
    // buffers are flushed
    m_bFlush = true;

    m_semFlush.v();
    m_semFlushAll.p();
}

void COutputStream::close()
{
    if (m_pOutput)
    {
        // Signal thread to stop, ensure all buffer blocks 
        // have been written and stop the background flush 
        // thread and wait until thread terminates.
        m_bRun = false;
        this->flush();
        this->join();

        m_pOutput = NULL;

        // Clean up buffer

        CBuffer::iterator iterBuffer = m_buffer.begin();
        CBuffer::iterator iterBufferEnd = m_buffer.end();

        while (iterBuffer != iterBufferEnd)
        {
            free(*iterBuffer);
            ++iterBuffer;
        }

        m_buffer.clear();

        // Clean up free blocks

        while (!m_free.empty())
        {
            free(m_free.top());
            m_free.pop();
        }
    }
}


//////////////////////////////////////////////////////////////////////
// Implementation

void COutputStream::writeBuffer()
{
    if (m_pOutput != NULL)
    {
        bool bFlush = m_bFlush;

        // Determine blocks in buffer to flush

        m_csBuffer.enter();

        int nNumBlocks = m_buffer.size();
        int nBeginOffset = m_nBeginOffset;
        int nEndOffset;

        if (bFlush || nNumBlocks <= m_nBlockThreshold)
        {
            nEndOffset = m_nEndOffset;
            m_nBeginOffset = m_nEndOffset;
        }
        else
        {
            nNumBlocks = m_nBlockThreshold;
            nEndOffset = m_nBlockSize;
            m_nBeginOffset = 0;
        }

        m_csBuffer.exit();

        // Write blocks to the underlying device

        char* pOutputBuffer;
        long nSendLen;

        while (nNumBlocks)
        {
            if (nNumBlocks > 1)
            {
                nSendLen = m_nBlockSize - nBeginOffset;
            }
            else if (nNumBlocks == 1 && nEndOffset > 0)
            {
                nSendLen = nEndOffset - nBeginOffset;
            }
            else
            {
                // Last block is empty
                break;
            }

            pOutputBuffer = m_buffer.front();

#ifdef _DEBUG
#ifdef _CHAR_STREAM
            _debugPrint("\t\tFlushing ", pOutputBuffer + nBeginOffset, nSendLen);
#endif
#endif
            nSendLen = m_pOutput->write(pOutputBuffer + nBeginOffset, nSendLen);

            // If the whole block was flushed
            // then remove it from the list
            nBeginOffset += nSendLen;
            if (nBeginOffset == m_nBlockSize)
            {
                m_csBuffer.enter();
                m_buffer.pop_front();
                m_free.push(pOutputBuffer);
                m_csBuffer.exit();

                nBeginOffset = 0;
            }

            --nNumBlocks;
        }

        // If the all remaining blocks in buffer were
        // written then reset all flags and semaphores
        if (bFlush)
        {
            m_bFlush = false;

            m_semFlush.reset();
            m_semFlushAll.v();
        }
    }
}

void* COutputStream::run()
{
    try
    {
        while (m_bRun || m_bFlush)
        {
            m_semFlush.p();
            this->writeBuffer();
        }
    }
    catch (CException* pe)
    {
        m_bRun = false;
        m_pOutput = NULL;

        delete pe;
    }

    return NULL;
}
