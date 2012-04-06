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
// CSocket.cpp: Implements the base set of functionality required
//              for all socket connections.
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

#include "Socket.h"
#include "SocketFactory.h"

#ifdef WIN32
#endif // WIN32

#ifdef UNIX
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Constant Macros

#define SO_LINGER_TIMEOUT  60


//////////////////////////////////////////////////////////////////////
// Exception messages defines

#define EXCEP_INVALIDSOCKET  "Attempt to construct a socket object with an invalid socket handle."
#define EXCEP_PEERADDR       "Unable to retrieve the peer socket's interface address details."
#define EXCEP_SOCKETADDR     "Unable to retrieve the client socket's local interface details."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CSocket::CSocket( SOCKET socket, 
                  int nSocketType )
{
    if (socket == INVALID_SOCKET)
    {
        THROW(CSocketException, EXCEP_MSSG(EXCEP_INVALIDSOCKET));
    }

#ifdef WIN32
    int nSize;
#endif // WIN32
#ifdef UNIX
    unsigned int nSize;
#endif // UNIX

    nSize = sizeof(struct sockaddr);
    if (getpeername(socket, (struct sockaddr *) &m_saPeer, &nSize) == SOCKET_ERROR)
    {
        THROW(CSocketException, EXCEP_MSSG(EXCEP_PEERADDR));
    }
    if (getsockname(socket, (struct sockaddr *) &m_saLocal, &nSize) == SOCKET_ERROR)
    {
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKETADDR));
    }

    m_socket         = socket;
    m_nSocketType    = nSocketType;
    m_nSecurityLevel = SECURE_NONE;
    m_nSocketTimeout = -1;

    setSocketLinger(TRUE, SO_LINGER_TIMEOUT);
}

CSocket::~CSocket()
{
    if (m_socket != INVALID_SOCKET) close();
}


//////////////////////////////////////////////////////////////////////
// Implementation

void CSocket::getSockOption( int nLevel, 
                             int nOptionName, 
                             void *pOptionValue,
                             int *pnOptionLen )
{
#ifdef WIN32
    if (getsockopt(m_socket, nLevel, nOptionName, (char FAR *) pOptionValue, pnOptionLen) == SOCKET_ERROR)
#endif // WIN32
#ifdef UNIX
    if (getsockopt(m_socket, nLevel, nOptionName, pOptionValue, (socklen_t *) pnOptionLen) == SOCKET_ERROR)
#endif // UNIX
    {
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKOPTIONS));
    }
}

void CSocket::setSockOption( int nLevel, 
                             int nOptionName, 
                             const void *pOptionValue,
                             int nOptionLen )
{
#ifdef WIN32
    if (setsockopt(m_socket, nLevel, nOptionName, (char FAR *) pOptionValue, nOptionLen) == SOCKET_ERROR)
#endif // WIN32
#ifdef UNIX
    if (setsockopt(m_socket, nLevel, nOptionName, pOptionValue, nOptionLen) == SOCKET_ERROR)
#endif // UNIX
    {
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKOPTIONS));
    }
}
