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
// CServerSocket.cpp: Implements a server socket that listens on 
//                    a given bound socket handle and accepts
//                    connections. If the connection is secure
//                    then a SSL socket connection is created
//                    otherwise a regular socket connection is
//                    created. This will also handle UDP datagram
//                    server sockets by waiting for datagrams
//                    and creating socket objects that handle
//                    packets from specific addresses.
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

#include "ServerSocket.h"
#include "SocketFactory.h"
#include "SocketImpl.h"
#include "SSLSocketImpl.h"

#include "IServerFactory.h"
#include "IPooledThread.h"
#include "ThreadPoolMgr.h"

#ifdef WIN32
#endif // WIN32

#ifdef UNIX
#include <sys/time.h>
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Constant macros

#define SERVER_SOCKET_TIMEOUT  1000     // Socket timeout interval in order that 
                                        // server stop signals can be intercepted.

//////////////////////////////////////////////////////////////////////
// Exception messages defines

#define EXCEP_INVALIDSOCKET        "Attempt to construct a server socket object with an invalid socket handle."
#define EXCEP_SOCKETADDR           "Unable to retrieve the server socket's local interface details."
#define EXCEP_INVALIDSECURITYFLAG  "Unable to accept a socket connection as the server socket was created with invalid security flags."
#define EXCEP_LISTEN               "An error occurred whilst listening and accepting a socket connection from a client."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CServerSocket::CServerSocket( SOCKET socket, 
                              int nSocketType,
                              int nSecurityFlags )
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
    if (getsockname(socket, (struct sockaddr *) &m_saLocal, &nSize) == SOCKET_ERROR)
    {
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKETADDR));
    }

    m_socket         = socket;
    m_nSocketType    = nSocketType;
    m_nSecurityFlags = nSecurityFlags;
    m_nSocketTimeout = -1;
    m_bStopServer    = false;
}

CServerSocket::~CServerSocket()
{
    if (m_socket != INVALID_SOCKET) close();
}


//////////////////////////////////////////////////////////////////////
// Public Method Interface Implementation

CSocket* CServerSocket::acceptConnection()
{
    if (m_nSocketType == SOCK_STREAM) 
    { 
        if (listen(m_socket, SOMAXCONN) != SOCKET_ERROR)
        {
            timeval tv;
            tv.tv_sec  = m_nSocketTimeout / 1000;
            tv.tv_usec = (m_nSocketTimeout % 1000) * 1000;

            fd_set fdAccept;
            FD_ZERO(&fdAccept);
            FD_SET(m_socket, &fdAccept);

            if ( select( FD_SETSIZE, 
                         &fdAccept, 
                         NULL, 
                         NULL, 
                         (m_nSocketTimeout == -1 ? NULL : &tv) ) != SOCKET_ERROR )
            {
                CSocket* pSocket = NULL;

                if (FD_ISSET(m_socket, &fdAccept))
                {
                    SOCKET socket = accept(m_socket, NULL, NULL); 

                    try
                    {
                        switch (m_nSecurityFlags)
                        {
                            case SECURE_NONE:
                                pSocket = new CSocketImpl( socket, 
                                                           m_nSocketType );
                                break;
                            case SECURE_SSL:
                                pSocket = new CSSLSocketImpl( socket, 
                                                              m_nSocketType, 
                                                              CSSLSocketImpl::sslAccept,
                                                              false );
                                break;
                            case (SECURE_SSL | SECURE_AUTH):
                                pSocket = new CSSLSocketImpl( socket, 
                                                              m_nSocketType, 
                                                              CSSLSocketImpl::sslAccept,
                                                              true );
                                break;
                            default:
                                if ((m_nSecurityFlags & MULTI_SERVER) != 0x0)
                                {
                                    return CSocketFactory::serverHandshake( socket, 
                                                                            m_nSocketType, 
                                                                            m_nSecurityFlags );
                                }
                                else
                                {
                                    THROW(CSocketException, EXCEP_MSSG(EXCEP_INVALIDSECURITYFLAG));
                                }
                        }
                    }
                    catch (CSocketException* pe)
                    {
                        closesocket(socket);
                        if (pSocket) delete pSocket;
                        throw pe;
                    }
                }
                return pSocket;
            }
        }
    } 

    THROW(CSocketException, EXCEP_MSSG(EXCEP_LISTEN));
}
 
void CServerSocket::startServer( IServerFactory* pServerFactory,
                                 int nThreadPoolSize )
{
    CThreadPoolMgr ServerThreadPoolMgr(nThreadPoolSize);

    setSocketTimeout(SERVER_SOCKET_TIMEOUT);
    while (!m_bStopServer)
    {
        CSocket* pSocket;

        try
        {
            if ((pSocket = acceptConnection()))
            {
                IPooledThread* pServerThread = pServerFactory->create(pSocket);
                ServerThreadPoolMgr.startThread(pServerThread);
            }
        }
        catch (...)
        {
            // Do nothing as exception is logged
        }
    }

    ServerThreadPoolMgr.shutdown();
}

void CServerSocket::stopServer()
{
    m_bStopServer = true;
}

void CServerSocket::close()
{
    shutdown(m_socket, 0x00 /* SD_RECEIVE */ );
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
}


//////////////////////////////////////////////////////////////////////
// Implementation

void CServerSocket::getSockOption( int nLevel, 
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

void CServerSocket::setSockOption( int nLevel, 
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
