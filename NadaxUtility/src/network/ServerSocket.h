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
// CServerSocket.h: interface for CServerSocket class. 
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

#if !defined(_SERVERSOCKET_H__INCLUDED_)
#define _SERVERSOCKET_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "unix.h"
#endif // UNIX

#include "number.h"

#include "Thread.h"
#include "SocketException.h"
#include "Socket.h"

class IServerFactory;


//////////////////////////////////////////////////////////////////////
// Macro to create an asynchronous socket server listener. The 
// following two macros must be called within the same local method
// as the server's listen thread is created on the stack and will be 
// orphaned if the method exits without stopping it.

#define START_ASYNC_SERVER(ss, sf, tps) \
    CListenThread listenThread; \
    listenThread.m_pSS = ss; \
    listenThread.m_pSF = sf; \
    listenThread.m_nTPS = tps; \
    listenThread.start();

#define STOP_ASYNC_SERVER() \
    listenThread.m_pSS->stopServer(); \
    listenThread.join();


//////////////////////////////////////////////////////////////////////
// This class defines server socket services required to
// create socket servers that listen on a particular TCP port.

class CServerSocket
{
protected:

	/**** Constructor ****/

    CServerSocket( SOCKET socket,
                   int nSocketType,
                   int nSecurityFlags );

public:

	/**** Destructor ****/

    virtual ~CServerSocket();

public:

	/**** Public method interfaces ****/
 
    // Gets the local address to 
    // which the socket is bound. 
    const struct sockaddr_in* getLocalAddress(); 

    // Listens for a connection to be made to 
    // this socket and accepts it and returns 
    // the appropriate socket implementation.
    CSocket* acceptConnection();
 
    // This method starts a socket server which 
    // services client connections asynchronously 
    // by means of a pool of threads. It will
    // block indefinitely until stopServer() is
    // called.
    void startServer( IServerFactory* pServerFactory,
                      int nThreadPoolSize );

    // Stops the socket server 
    // if it is running.
    void stopServer();

    // Get methods to get socket options

    BOOL getDebug();            // Option 'SO_DEBUG'
    BOOL getAccept();           // Option 'SO_ACCEPTCONN'
    int  getSocketTimeout();    // Socket time out for accept wait

    // Set methods to set socket options

    void setDebug(BOOL bDebug);             // Option 'SO_DEBUG'
    void setSocketTimeout(int nTimeout);    // Socket time out for accept wait

    // Closes the socket 

    void close();

protected:

	/**** Protected inheritable / overridable methods ****/

    // Retrieves the socket options for 
    // the given level / name in a buffer.

    void getSockOption( int nLevel, 
                        int nOptionName, 
                        void *pOptionValue,
                        int *pnOptionLen );

    // Sets the socket options for 
    // the given level / name using 
    // the value in the given buffer.

    void setSockOption( int nLevel, 
                        int nOptionName, 
                        const void *pOptionValue,
                        int nOptionLen );
private:

	/**** Protected inheritable member variables ****/

    // Underlying socket handle
    SOCKET m_socket;

    // Underlying socket protocal type.
    //
    // SOCK_STREAM - connection-oriented tcp socket
    // SOCK_DGRAM  - connectionless udp socket
    int m_nSocketType;

    // Security flags for connections
    int m_nSecurityFlags;

    // Address to which socket is bound
    struct sockaddr_in m_saLocal;

    // Timeout for sending and receiving data
    int m_nSocketTimeout;

	// Flag that signals the socket server to stop
    Number<bool> m_bStopServer;

	// Friend classes of CSocketImpl
	friend class CSocketFactory;

};


//////////////////////////////////////////////////////////////////////
// Declares a server thread class for
// listening on a socket conenction.

class CListenThread : 
    public CThread
{
public:
    virtual void* run()
    {
        m_pSS->startServer(m_pSF, m_nTPS);
        return NULL;
    }
    CServerSocket* m_pSS;
    IServerFactory* m_pSF;
    int m_nTPS;
};


//////////////////////////////////////////////////////////////////////
// Inline function declarations

inline const struct sockaddr_in* CServerSocket::getLocalAddress()
{
    return &m_saLocal;
}

inline BOOL CServerSocket::getDebug()
{
    BOOL bDebug;
    GETSOCKOPT(SOL_SOCKET, SO_DEBUG, bDebug);
    return bDebug;
}

inline BOOL CServerSocket::getAccept()
{
    BOOL bAccept;
    GETSOCKOPT(SOL_SOCKET, SO_ACCEPTCONN, bAccept);
    return bAccept;
}

inline int CServerSocket::getSocketTimeout()
{
    return m_nSocketTimeout;    
}

inline void CServerSocket::setDebug(BOOL bDebug)
{
    SETSOCKOPT(SOL_SOCKET, SO_DEBUG, bDebug);
}

inline void CServerSocket::setSocketTimeout(int nTimeout)
{
    m_nSocketTimeout = nTimeout;
}


#endif // !defined(_SERVERSOCKET_H__INCLUDED_)
