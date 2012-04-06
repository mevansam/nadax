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
// CSocketImpl.h: interface for CSocketImpl which extends the CSocket 
//                class implementation.
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

#if !defined(_SOCKETIMPL_H__INCLUDED_)
#define _SOCKETIMPL_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#endif // UNIX

#include "Socket.h"


//////////////////////////////////////////////////////////////////////
// Implements a socket connection object.

class CSocketImpl :
    public CSocket
{
protected:

	/**** Constructor / Destructor ****/

    CSocketImpl( SOCKET socket, 
                 int nSocketType );

    virtual ~CSocketImpl();

	/**** Protected implementation ****/

    // Reads data from this socket and 
    // saves it in the given buffer.
    virtual int recvData(void* pBuffer, long nSize);
    
    // Sends the data in the given 
    // buffer over this socket.
    virtual int sendData(void* pBuffer, long nSize);

private:

	/**** Implementation Specific Member variables ****/

	// Friend classes of CSocketImpl
	friend class CSocketFactory;
	friend class CServerSocket;

};

#endif // !defined(_SOCKETIMPL_H__INCLUDED_)
