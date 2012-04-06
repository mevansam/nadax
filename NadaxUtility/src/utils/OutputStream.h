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
// COutputStream.h: interface for COutputStream class.
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

#if !defined(_OUTPUTSTREAM_H__INCLUDED_)
#define _OUTPUTSTREAM_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#endif // UNIX

#include "macros.h"
#include "criticalsection.h"
#include "number.h"
#include "Semaphore.h"
#include "Thread.h"
#include "Output.h"

#include <stack>
#include <list>

typedef std::stack<char *> CFree;
typedef std::list<char *> CBuffer;


//////////////////////////////////////////////////////////////////////
// This class defines a buffered output stream for writing data to 
// an output device. To improve preformance the buffer is flushed by
// means of a background. A concrete implementor of the IOutput 
// interface must be provided for performing the physical writes. This
// object must be cleaned up outside of this class.

class COutputStream :
    protected CThread
{
public:

	/**** Constructor / Destructor ****/

    COutputStream( IOutput* pOutput,
                   long nBlockSize = 1024, 
                   int nBlockThreshold = 1 );

    virtual ~COutputStream();

public:

	/**** Public method interfaces ****/

    // Writes a byte to the output stream.
    void write(const char ch);

    // Writes len bytes from the specified 
    // byte array starting at offset off to 
    // this output stream.
    void write(const char* pBuffer, long nLen);

    // Flushes all buffers to the output
    // device and clears them. This method
    // SHOULD be synchronized with write()
    // methods. Typically the buffer is
    // flushed automatically and there should
    // not be any need to call this method
    // directly.
    void flush();

    // Closes this output stream and 
    // releases any system resources 
    // associated with this stream.
    void close();

protected:

	/**** Protected implementation ****/

    // The thread main loop that 
    // flushes the buffer when the 
    // threshold limit is reached.
    virtual void* run();

private:

	/**** Private implementation ****/

    void writeBuffer();

private:

	/**** Implementation Specific Member variables ****/

    IOutput* m_pOutput;

    CFree m_free;           // Freed data blocks that can be reused
    CBuffer m_buffer;       // The data buffer

    long m_nBlockSize;      // The size of a block of data in the buffer
    int m_nBlockThreshold;  // The number of completed blocks that signal a auto-flush

    long m_nBeginOffset;  // The position in the first queue buffer block from which data is valid
    long m_nEndOffset;    // The position in the last queue buffer block where new data can be bufferred

    CriticalSection m_csBuffer;  // Mutex used to protect the buffer
    CSemaphore m_semFlush;       // Signals a buffer flush
    CSemaphore m_semFlushAll;    // Signals a all data in buffer has been flushed

    Number<bool> m_bRun;    // Indicates background thread should continue running
    Number<bool> m_bFlush;  // Indicates background thread should flush the buffer
};

#endif // !defined(_OUTPUTSTREAM_H__INCLUDED_)
