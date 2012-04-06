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
// CSocketFactory.h: interface for CSocketFactory class.
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

#if !defined(_SOCKETFACTORY_H__INCLUDED_)
#define _SOCKETFACTORY_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include <sys/socket.h>
#include "unix.h"
#endif // UNIX

#include "macros.h"
#include "number.h"

class CSocket;
class CServerSocket;
class CSocketOutputStream;
typedef struct ssl_ctx_st SSL_CTX;


//////////////////////////////////////////////////////////////////////
// Socket security properites.
//
// SECURE_NONE       - Socket connections are not secured.
// SECURE_SSL        - Socket connections are secured using SSL.
//                     with peer certificate authentication.
// SECURE_AUTH       - Socket connections are authenticated. This
//                   - is valid only for ssl socket connections.
// MULTI_CLIENT      - This flag causes the client connection 
//                     to negotiate whether the connection uses
//                     ssl.
// MULTI_SERVER      - This flag causes the server to negotiate
//                     with the client the type of connection
//                     that needs to be established. i.e. regular
//                     or over ssl.

#define SECURE_NONE   0x0
#define SECURE_SSL    0x1
#define SECURE_AUTH   0x2
#define MULTI_CLIENT  0x4
#define MULTI_SERVER  0x8


//////////////////////////////////////////////////////////////////////
// This class implements methods which create client or server
// socket connection and returns socket objects.

class CSocketFactory
{
public:

	/**** Public method interfaces ****/

    // Calls initialization and cleanup 
    // functions within the socket 
    // sub-system.

    static void startup( const char* szSSLCARootsFile = NULL,
                         const char* szSSLCertFile = NULL,
                         const char* szSSLPrivKeyFile = NULL,
                         const char* szSSLPrivKeyPasswd = NULL );

    static void cleanup();

    // Authentication state
    static bool canSSLVerify()  { return m_bSSLCAVerify; }
    static bool canSSLCertify() { return m_bSSLCertify;  }

    // Creates a tcp socket connection to the given
    // remote address-port combination and returns
    // a socket object which implements the given
    // connection type.
    static CSocket* connectTCP( const char* pszServerAddress, 
                                int nServerPort,
                                int nSecurityFlags = SECURE_NONE );

    // Creates a udp socket connection to the given
    // remote address-port combination and returns
    // a socket object which implements the given
    // connection type.
    static CSocket* connectUDP( const char* pszServerAddress, 
                                int nServerPort,
                                int nSecurityFlags = SECURE_NONE );

    // Creates a connection-oriented server 
    // socket which will bind to the given
    // local network interface and accept 
    // tcp connections from it.
    static CServerSocket* createServerTCP( const char* pszLocalInterface, 
                                           int nLocalPort,
                                           int nSecurityFlags = SECURE_NONE );

    // Creates a connectionless server 
    // socket which will bind to the given 
    // local network interfaces and accept 
    // udp connections from it.
    static CServerSocket* createServerUDP( const char* pszLocalInterface,
                                           int nLocalPort,
                                           int nSecurityFlags = SECURE_NONE );

    // Creates a connection-oriented server 
    // socket which will bind to all local 
    // network interfaces  and accept tcp 
    // connections from any one of them.
    static CServerSocket* createServerTCP( int nLocalPort,
                                           int nSecurityFlags = SECURE_NONE );

    // Creates a connectionless server 
    // socket which will bind to all local 
    // network interfaces  and accept udb 
    // connections from any one of them.
    static CServerSocket* createServerUDP( int nLocalPort,
                                           int nSecurityFlags = SECURE_NONE );

    // Create a client socket which is 
    // either secure or non-secure depending 
    // on how the server is configured.
    static CSocket* clientHandshake( SOCKET socket, 
                                     int nSockType,
                                     int nSecurityFlags );

    // Create a server connection socket 
    // which negotiates either a secure 
    // or non-secure connection with the 
    // client depending on whether the 
    // socket server is running in secure 
    // more.
    static CSocket* serverHandshake( SOCKET socket, 
                                     int nSockType,
                                     int nSecurityFlags );

#ifdef OPENSSL
    // Returns the sharable open ssl context
    static SSL_CTX* getSSLContext() { return m_pSSLContext; }
#endif // OPENSSL

private:

	/**** Implementation methods ****/

    static CSocket* createClientSocket( const char* pszServerAddress, 
                                        int nServerPort,
                                        int nSocketType,
                                        int nSecurityFlags );

    static CServerSocket* createServerSocket( const char* pszLocalInterface, 
                                              int nLocalPort,
                                              int nSocketType,
                                              int nSecurityFlags );
    // Handshake support methods

    static bool authenticate( const char* szServerCert, 
                              const char* szPeerMssg );
    
    static bool writeCertificate(CSocket* pSocket);
    static bool writeSignature(CSocket* pSocket);

#ifdef OPENSSL
    static int openSSLPasswdCallback(char *pBuffer, int nSize, int, void *);
#endif // OPENSSL

private:

	/**** Implementation Specific Member variables ****/

    static Number<char> g_nInitRefCount;
    
    static bool  m_bSSLCAVerify;
    static bool  m_bSSLCertify;
    static char* m_szSSLPrivKeyPasswd;

    // Shared SSL Context

#ifdef OPENSSL
    static SSL_CTX* m_pSSLContext;
#endif // OPENSSL
};


//////////////////////////////////////////////////////////////////////
// Inline function declarations

inline CSocket* CSocketFactory::connectTCP( const char* pszServerAddress, 
                                            int nServerPort,
                                            int nSecurityFlags )
{
    return createClientSocket( pszServerAddress, 
                               nServerPort, 
                               SOCK_STREAM,
                               nSecurityFlags );
}

inline CSocket* CSocketFactory::connectUDP( const char* pszServerAddress, 
                                            int nServerPort,
                                            int nSecurityFlags )
{
    return createClientSocket( pszServerAddress, 
                               nServerPort, 
                               SOCK_DGRAM,
                               nSecurityFlags );
}

inline CServerSocket* CSocketFactory::createServerTCP( const char* pszLocalInterface, 
                                                       int nLocalPort,
                                                       int nSecurityFlags )
{
    return createServerSocket( pszLocalInterface, 
                               nLocalPort, 
                               SOCK_STREAM,
                               nSecurityFlags );
}

inline CServerSocket* CSocketFactory::createServerUDP( const char* pszLocalInterface,
                                                       int nLocalPort,
                                                       int nSecurityFlags )
{
    return createServerSocket( pszLocalInterface, 
                               nLocalPort, 
                               SOCK_DGRAM,
                               nSecurityFlags );
}

inline CServerSocket* CSocketFactory::createServerTCP( int nLocalPort,
                                                       int nSecurityFlags )
{
    return createServerTCP(NULL, nLocalPort, nSecurityFlags);
}

inline CServerSocket* CSocketFactory::createServerUDP( int nLocalPort,
                                                       int nSecurityFlags )
{
    return createServerUDP(NULL, nLocalPort, nSecurityFlags);
}

#endif // !defined(_SOCKETFACTORY_H__INCLUDED_)
