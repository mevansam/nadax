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
// CSocket.h: interface for CSocket class.
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

#if !defined(_SOCKET_H__INCLUDED_)
#define _SOCKET_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "unix.h"
#endif // UNIX

#include "SocketException.h"
#include "Input.h"
#include "Output.h"

#include <list>

class CSocket;
typedef std::list<CSocket *> CSocketList;


//////////////////////////////////////////////////////////////////////
// Constants

// Data is sent over the socket in 32K packets
#define PACKET_SIZE  32768


//////////////////////////////////////////////////////////////////////
// This abstract class defines basic socket services required for 
// server and client sockets. Implementation specific details can
// only be accessed by casting this interfaces to its corresponding
// implementation class.

class CSocket :
    public IInput,
    public IOutput
{
public:

	/**** Constructor / Destructor ****/

    CSocket( SOCKET socket,
             int nSocketType );

    virtual ~CSocket();

public:

	/**** Public method interfaces ****/

    // Returns whether this object contains
    // a valid socket connection.
    bool isValid();

    // Returns the address to which 
    // the socket is connected. 
    const struct sockaddr_in* getPeerAddress();
 
    // Gets the local address to 
    // which the socket is bound. 
    const struct sockaddr_in* getLocalAddress();
 
    // Get methods to get socket options

    BOOL getDebug();                // Option 'SO_DEBUG'
    BOOL getBroadcast();            // Option 'SO_BROADCAST'
    BOOL getKeepAlive();            // Option 'SO_KEEPALIVE'
    BOOL getTcpNoDelay();           // Option 'TCP_NODELAY'
    int  getSocketLinger();         // Option 'SO_LINGER'
    int  getSocketSendTimeout();    // Option 'SO_SNDTIMEO'
    int  getSocketRecvTimeout();    // Option 'SO_RCVTIMEO'
    int  getSendBufferSize();       // Option 'SO_SNDBUF' 
    int  getReceiveBufferSize();    // Option 'SO_RCVBUF' 
    int  getSocketTimeout();        // Socket time out for receiving and sending data

    // Set methods to set socket options

    void setDebug(BOOL bDebug);                         // Option 'SO_DEBUG'
    void setBroadcast(BOOL bBroadcast);                 // Option 'SO_BROADCAST'
    void setKeepAlive(BOOL bOn);                        // Option 'SO_KEEPALIVE'
    void setTcpNoDelay(BOOL bDelay);                    // Option 'TCP_NODELAY'
    void setSocketLinger(BOOL bOn, int bLinger = 0);    // Option 'SO_LINGER'
    void setSocketSendTimeout(int nTimeout);            // Option 'SO_SNDTIMEO'
    void setSocketRecvTimeout(int nTimeout);            // Option 'SO_RCVTIMEO'
    void setSendBufferSize(int nSize);                  // Option 'SO_SNDBUF'
    void setReceiveBufferSize(int nSize);               // Option 'SO_RCVBUF'
    void setSocketTimeout(int nTimeout);                // Socket time out for receiving and sending data

    // Detaches the socket handle 
    // from this object

    SOCKET detach();

    // Closes the socket 

    void close();

public:

    // I/O Stream implementation

    // Reads the given amount of data from the socket 
    // into the buffer. The actual amount of data read 
    // is returned.
    virtual long read(void* pBuffer, long nLen)
    {
        return this->recvData(pBuffer, nLen);
    }

    // Writes data to the socket. The number  of bytes 
    // written are returned. If a write  error occurs 
    // an exception will be thrown.
    virtual long write(void* pBuffer, long nLen)
    {
        return this->sendData(pBuffer, nLen);
    }

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
 
    // Reads data from this socket and 
    // saves it in the given buffer.
    virtual int recvData(void* pBuffer, long nSize) = 0;
    
    // Sends the data in the given 
    // buffer over this socket.
    virtual int sendData(void* pBuffer, long nSize) = 0;

protected:

	/**** Inheritable Implementation Specific Member variables ****/

    // The handle to the socket
    SOCKET m_socket;

    // Underlying socket protocal type.
    //
    // SOCK_STREAM - connection-oriented tcp socket
    // SOCK_DGRAM  - connectionless udp socket
    int m_nSocketType;

    // Security level for this connection.
    int m_nSecurityLevel;

    // Address to which socket is connected
    struct sockaddr_in m_saPeer;
    // Local address of socket
    struct sockaddr_in m_saLocal;

    // Timeout for sending and receiving data
    int m_nSocketTimeout;

	// Friend classes of CSocket
	friend class CSocketFactory;

};


//////////////////////////////////////////////////////////////////////
// Macros for inline declarations

#define EXCEP_SOCKOPTIONS "Error occurred whilst writing or retrieving socket options."

#ifdef WIN32

#define GETSOCKOPT(level, option, value) \
    int nSize = sizeof(value); \
    if (getsockopt(m_socket, level, option, (char FAR *) &value, &nSize) == SOCKET_ERROR) \
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKOPTIONS));

#define SETSOCKOPT(level, option, value) \
    if (setsockopt(m_socket, level, option, (char FAR *) &value, sizeof(value)) == SOCKET_ERROR) \
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKOPTIONS));

#endif // WIN32

#ifdef UNIX

#define GETSOCKOPT(level, option, value) \
    socklen_t nSize = sizeof(value); \
    if (getsockopt(m_socket, level, option, (void *) &value, &nSize) == SOCKET_ERROR) \
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKOPTIONS));

#define SETSOCKOPT(level, option, value) \
    if (setsockopt(m_socket, level, option, (void *) &value, sizeof(value)) == SOCKET_ERROR) \
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SOCKOPTIONS));

#endif // UNIX


//////////////////////////////////////////////////////////////////////
// Inline function declarations

inline bool CSocket::isValid()
{
    return (m_socket != INVALID_SOCKET);
}

inline const struct sockaddr_in* CSocket::getPeerAddress()
{
    return &m_saPeer;
}

inline const struct sockaddr_in* CSocket::getLocalAddress()
{
    return &m_saLocal;
}

inline BOOL CSocket::getDebug()
{
    BOOL bDebug;
    GETSOCKOPT(SOL_SOCKET, SO_DEBUG, bDebug);
    return bDebug;
}

inline BOOL CSocket::getBroadcast()
{
    BOOL bBroadcast;
    GETSOCKOPT(SOL_SOCKET, SO_BROADCAST, bBroadcast);
    return bBroadcast;
}

inline BOOL CSocket::getKeepAlive()
{
    BOOL bKeepAlive;
    GETSOCKOPT(SOL_SOCKET, SO_KEEPALIVE, bKeepAlive);
    return bKeepAlive;    
}

inline BOOL CSocket::getTcpNoDelay()
{
    BOOL bTCPNoDelay;
    GETSOCKOPT(IPPROTO_TCP, TCP_NODELAY, bTCPNoDelay);
    return bTCPNoDelay;
}

inline int CSocket::getSocketLinger()
{
    struct linger l;
    GETSOCKOPT(SOL_SOCKET, SO_LINGER, l);
    return l.l_onoff;
}

inline int CSocket::getSocketSendTimeout()
{
    int nSendTimeout;
    GETSOCKOPT(SOL_SOCKET, SO_BROADCAST, nSendTimeout);
    return nSendTimeout;    
}

inline int CSocket::getSocketRecvTimeout()
{
    int nRecvTimeout;
    GETSOCKOPT(SOL_SOCKET, SO_BROADCAST, nRecvTimeout);
    return nRecvTimeout;
}

inline int CSocket::getSendBufferSize()
{
    int nSendBufferSize;
    GETSOCKOPT(SOL_SOCKET, SO_BROADCAST, nSendBufferSize);
    return nSendBufferSize;
}

inline int CSocket::getReceiveBufferSize()
{
    int nRecvBufferSize;
    GETSOCKOPT(SOL_SOCKET, SO_BROADCAST, nRecvBufferSize);
    return nRecvBufferSize;
}

inline int CSocket::getSocketTimeout()
{
    return m_nSocketTimeout;    
}

inline void CSocket::setDebug(BOOL bDebug)
{
    SETSOCKOPT(SOL_SOCKET, SO_DEBUG, bDebug);
}

inline void CSocket::setBroadcast(BOOL bBroadcast)
{
    SETSOCKOPT(SOL_SOCKET, SO_BROADCAST, bBroadcast);
}

inline void CSocket::setKeepAlive(BOOL bOn)
{
    SETSOCKOPT(SOL_SOCKET, SO_KEEPALIVE, bOn);
}

inline void CSocket::setTcpNoDelay(BOOL bDelay)
{
    SETSOCKOPT(IPPROTO_TCP, TCP_NODELAY, bDelay);
}

inline void CSocket::setSocketLinger(BOOL bOn, int nLinger)
{
    struct linger l;
    l.l_onoff  = bOn;
    l.l_linger = nLinger;
    SETSOCKOPT(SOL_SOCKET, SO_LINGER, l);
}

inline void CSocket::setSocketSendTimeout(int nTimeout)
{
    SETSOCKOPT(SOL_SOCKET, SO_SNDTIMEO, nTimeout);
}

inline void CSocket::setSocketRecvTimeout(int nTimeout)
{
    SETSOCKOPT(SOL_SOCKET, SO_RCVTIMEO, nTimeout);
}

inline void CSocket::setSendBufferSize(int nSize)
{
    SETSOCKOPT(SOL_SOCKET, SO_SNDBUF, nSize);
}

inline void CSocket::setReceiveBufferSize(int nSize)
{
    SETSOCKOPT(SOL_SOCKET, SO_RCVBUF, nSize);
}

inline void CSocket::setSocketTimeout(int nTimeout)
{
    m_nSocketTimeout = nTimeout;
}

inline SOCKET CSocket::detach()
{
    SOCKET s = m_socket;
    m_socket = INVALID_SOCKET;
    return s;
}

inline void CSocket::close()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

#endif // !defined(_SOCKET_H__INCLUDED_)
