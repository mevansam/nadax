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
// CInputStream.h: interface for CInputStream class.
//
// Author  : Mevan Samaratunga
// Date    : 05/2011
//
//////////////////////////////////////////////////////////////////////

#if !defined(_INPUTSTREAM_H__INCLUDED_)
#define _INPUTSTREAM_H__INCLUDED_

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
#include "Input.h"


//////////////////////////////////////////////////////////////////////
// This class defines a buffered output stream for writing data to 
// an output device. To improve preformance the buffer is flushed by
// means of a background. A concrete implementor of the IOutput 
// interface must be provided for performing the physical writes. This
// object must be cleaned up outside of this class.
class CInputStream :
    protected CThread
{
public:

	/**** Constructor / Destructor ****/

    CInputStream( IInput* pInput,
                  long nBlockSize = 1024,
                  int nReadAhead = 1,
                  int nReadTimeout = -1 );

    virtual ~CInputStream();

public:

	/**** Public method interfaces ****/
    
    // Repositions this stream to the beginning 
    // of the stream. If mark has not been 
    // called then a call to reset will reposition 
    // the stream pointer to the beginning of the 
    // stream.
    void reset();

    // Marks the current position in this input 
    // stream. A subsequent call to the reset 
    // method repositions this stream at the 
    // last marked position so that subsequent 
    // reads re-read the same bytes. 
    void mark(long nReadLimit);

    // Skips over and discards n bytes of data 
    // from this input stream
    long skip(long nLen);
    
    // Returns the number of bytes that can be 
    // read (or skipped over) from this input 
    // stream without blocking by the next 
    // caller of a method for this input stream
    long available();

    // Reads the next byte of data 
    // from the input stream.
    char read();

    // Reads up to nLen bytes of data from the 
    // input stream into an array of bytes.
    long read(char* pBuffer, long nLen);

    // Closes the input stream and clears all 
    // buffers.
    void close();

protected:

	/**** Protected implementation ****/

    // The thread main loop that reads 
    // data into the buffer as read 
    // ahead blocks become available
    virtual void* run();

private:

	/**** Implementation Specific Member variables ****/

    IInput* m_pInput;

    long m_nBlockSize;    // The size of a block of data in the buffer
    int m_nBeginBlock;    // The first block in the buffer
    int m_nEndBlock;      // The Last block in the buffer
    long m_nBeginOffset;  // The position in the first queue buffer block from which data is valid
    long m_nEndOffset;    // The position in the last queue buffer block where new data can be bufferred
    
    char* m_pBuffer;             // The data buffer
    CriticalSection m_csBuffer;  // Critical section used to protect the buffer

    bool m_bDataAvailable;          // Indicates that there is data available for reading
    CSemaphore m_semDataAvailable;  // Semaphore indicating that data is available

    int m_nReadAhead;           // The number of blocks to read ahead
    CSemaphore m_semReadAhead;  // Semaphore which signals thread to read ahead

    int m_nReadTimeout;  // The amount of time a read will block until data is available

    Number<bool> m_bRun;  // Indicates background thread should continue running
};

#endif // !defined(_INPUTSTREAM_H__INCLUDED_)
