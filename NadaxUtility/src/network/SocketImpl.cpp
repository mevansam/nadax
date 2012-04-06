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
// CSocketImpl.cpp: Implements a regular non-secure socket.
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

#include "SocketImpl.h"

#ifdef WIN32
#endif // UNIX

#ifdef UNIX
#include <errno.h>
#include <sys/time.h>
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Exception messages defines

#define EXCEP_RECV  "An error occurred whilst reading data from the socket connected to the network address '%s'."
#define EXCEP_SEND  "An error occurred whilst attempting to send data thru the socket connected to the network address '%s'."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CSocketImpl::CSocketImpl( SOCKET socket, 
                          int nSocketType )
    : CSocket(socket, nSocketType)
{
}

CSocketImpl::~CSocketImpl()
{
}


//////////////////////////////////////////////////////////////////////
// Protected Implementation

int CSocketImpl::recvData(void* pBuffer, long nSize)
{
    if (m_nSocketType == SOCK_STREAM)
    {
        timeval tv;
        tv.tv_sec  = m_nSocketTimeout / 1000;
        tv.tv_usec = (m_nSocketTimeout % 1000) * 1000;

        fd_set fdRecv;
        FD_ZERO(&fdRecv);
        FD_SET(m_socket, &fdRecv);

        if ( select( FD_SETSIZE, 
                     &fdRecv, 
                     NULL, 
                     NULL, 
                     (m_nSocketTimeout == -1 ? NULL : &tv) ) != SOCKET_ERROR )
        {
            if (FD_ISSET(m_socket, &fdRecv))
            {
                int nRecvLen;
                if ((nRecvLen = recv(m_socket, (char *) pBuffer, nSize, 0)) != SOCKET_ERROR) return nRecvLen;
            }
            else 
                // This indicates a recv timeout
                return -1;
        }
    } 

    THROW(CSocketException, EXCEP_MSSG(EXCEP_RECV, inet_ntoa(m_saPeer.sin_addr)));
}

int CSocketImpl::sendData(void* pBuffer, long nSize)
{
    if (m_nSocketType == SOCK_STREAM)
    {
        timeval tv;
        tv.tv_sec  = m_nSocketTimeout / 1000;
        tv.tv_usec = (m_nSocketTimeout % 1000) * 1000;

        fd_set fdSend;
        FD_ZERO(&fdSend);
        FD_SET(m_socket, &fdSend);

        if ( select( FD_SETSIZE, 
                     NULL, 
                     &fdSend, 
                     NULL, 
                     (m_nSocketTimeout == -1 ? NULL : &tv) ) != SOCKET_ERROR )
        {
            if (FD_ISSET(m_socket, &fdSend))
            {
                int nSentLen;
                if ((nSentLen = send(m_socket, (char *) pBuffer, nSize, 0)) != SOCKET_ERROR) return nSentLen;
            }
            else 
                // This indicates a send timeout
                return -1;
        }
    }

    THROW(CSocketException, EXCEP_MSSG(EXCEP_RECV, inet_ntoa(m_saPeer.sin_addr)));
}
