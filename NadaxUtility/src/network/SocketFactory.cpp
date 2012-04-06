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
// CSocketFactory.cpp: This class implements methods for creating
//                     client sockets and server sockets. All
//                     types of sockets should be created via this
//                     class an not directly.
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

#include "SocketFactory.h"
#include "SocketException.h"
#include "ServerSocket.h"
#include "SocketImpl.h"
#include "SSLSocketImpl.h"

#ifdef WIN32
#include <strstrea.h>
#endif // WIN32

#ifdef UNIX
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#endif // UNIX

#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"
#include "openssl/evp.h"
#include "openssl/bio.h"
#include "openssl/pem.h"
#include "openssl/x509.h" 
#include "openssl/x509_vfy.h" 

#include <ctype.h>


//////////////////////////////////////////////////////////////////////
// Macros

// Win32 version numbers applicable 
// for windows compilation only
#define WS_MAJOR_VER  2
#define WS_MINOR_VER  2

// Server / Client server managed secure 
// connection handshake messages

#define SIG_MLEN  64
#define SIG_BLEN  512

const char _helo_svr[]        = "HELO:SECURE=0:CERT=";
const char _helo_svr_secure[] = "HELO:SECURE=1:CERT=";
const char _helo_clnt[]       = "HELO:CERT=";
const char _resp_ok[]         = "OK:MD=";

const char _helo[]   = "HELO:";
const char _secure[] = "SECURE=";
const char _cert[]   = "CERT=";
const char _ok[]     = "OK:";
const char _md[]     = "MD=";

#define INIT_READ \
    char achBuffer[64]; \
    int nRecvSize; \
    int nRecvLen;

#define READ_MSSG(size, mssg) \
    if (sock.recvData((void *) achBuffer, size) < size) \
    { \
        THROW(CSocketException, EXCEP_MSSG(EXCEP_HANDSHAKE_NODATA)); \
    } \
    else if (memcmp(mssg, achBuffer, size) != 0) \
    { \
        THROW(CSocketException, EXCEP_MSSG(EXCEP_HANDSHAKE_DATA, mssg)); \
    }

#define READ_BUFFER(ostrBuffer) \
    nRecvLen = 0; \
    if (sock.recvData((void *) achBuffer, 2) == 2) \
    { \
        nRecvSize = (achBuffer[0] << 8) | achBuffer[1]; \
        while (nRecvSize > 0 && nRecvLen != -1) \
        { \
            nRecvLen = sock.recvData((void *) achBuffer, min(sizeof(achBuffer), nRecvSize)); \
            if (nRecvLen > 0) \
            { \
                ostrBuffer.write(achBuffer, nRecvLen); \
                nRecvSize -= nRecvLen; \
            } \
        } \
        ostrBuffer << (char) 0; \
    }

#define WRITE_MSSG(mssg) \
    sock.sendData((void *) mssg, sizeof(mssg) - 1);


//////////////////////////////////////////////////////////////////////
// Exception messages defines

#define EXCEP_INITFAILED        "Unable to initialize socket subsystem."
#define EXCEP_SERVERNOTFOUND    "Unable to resolve remote server address '%s'."
#define EXCEP_CLIENTSOCKET      "Error occurred whilst attempting to create a client socket to server at '%s' on port '%d'."
#define EXCEP_CLIENTSECURITY    "Client socket security flag other than SECURE_NONE or SECURE_SSL was specified."
#define EXCEP_SERVERSOCKET      "Error occurred whilst attempting to create a server socket at local interface '%s' on port '%d'."
#define EXCEP_HANDSHAKE         "Handshake to establish a server managed secure socket connection failed."
#define EXCEP_HANDSHAKE_DATA    "Whilst attempting handshake for a server managed secure connection '%s' was not received."
#define EXCEP_HANDSHAKE_NODATA  "Insufficient data received when attempting handshake for a server managed secure connection."


//////////////////////////////////////////////////////////////////////
// Static member initialization

Number<char> CSocketFactory::g_nInitRefCount(0);
bool CSocketFactory::m_bSSLCAVerify = true;
bool CSocketFactory::m_bSSLCertify = true; 
char* CSocketFactory::m_szSSLPrivKeyPasswd = NULL;

#ifdef OPENSSL
SSL_CTX* CSocketFactory::m_pSSLContext = NULL;

#elif RSASSL

#endif // SSL


//////////////////////////////////////////////////////////////////////
// Public methods and interfaces

void CSocketFactory::startup( const char* szSSLCARootsFile,
                              const char* szSSLCertFile,
                              const char* szSSLPrivKeyFile,
                              const char* szSSLPrivKeyPasswd )
{
#ifdef _DEBUG
    printf("Initializing socket functions:\n");
    printf("  * SSL CA root file = %s\n", szSSLCARootsFile);
    printf("  * Local host certificate file = %s\n", szSSLCertFile);
    printf("  * Local host private key file = %s\n", szSSLPrivKeyFile);
#endif // _DEBUG

    if (g_nInitRefCount.interlockedIncComp(1))
    {
#ifdef WIN32
        WSADATA wsaData; 
        if (::WSAStartup(MAKEWORD(WS_MAJOR_VER, WS_MINOR_VER), &wsaData) == SOCKET_ERROR) 
        {
            CSocketFactory::cleanup();
            THROW(CSocketException, EXCEP_MSSG(EXCEP_INITFAILED));
        } 
#endif // WIN32

#ifdef UNIX
        // Disable SIGPIPE signals which occur if an
        // attempt is made to write to a closed socket,
        // as this will kill the process.
        sigset_t s;
        sigaddset(&s, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &s, NULL);
#endif // UNIX

#ifdef OPENSSL
        SSL_load_error_strings();
        SSL_library_init();

        m_pSSLContext = SSL_CTX_new(SSLv3_method());
        SSL_CTX_set_default_passwd_cb(m_pSSLContext, CSocketFactory::openSSLPasswdCallback);

        if (!m_pSSLContext)
        {
            THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
        }

        try
        {
            if (SSL_CTX_load_verify_locations(m_pSSLContext, szSSLCARootsFile, NULL) <= 0)
            {
                m_bSSLCAVerify = false;
                THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
            }
        }
        catch (CSocketException* pe)
        {
            // Do nothing as exception is logged 
            // and appropriate SSL authentication 
            // state is recorded.
            delete pe;
        }

        try
        {
            if (szSSLPrivKeyPasswd)
                m_szSSLPrivKeyPasswd = strdup(szSSLPrivKeyPasswd);

            if ( SSL_CTX_use_certificate_file(m_pSSLContext, szSSLCertFile, SSL_FILETYPE_PEM) <= 0 ||
                 SSL_CTX_use_PrivateKey_file(m_pSSLContext, szSSLPrivKeyFile, SSL_FILETYPE_PEM) <= 0 ||
                 !SSL_CTX_check_private_key(m_pSSLContext) ) 
            {
                m_bSSLCertify = false;
                THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
            }
        }
        catch (CSocketException* pe)
        {
            // Do nothing as exception is logged 
            // and appropriate SSL authentication 
            // state is recorded.
            delete pe;
        }
        
#elif RSASSL

#else
#pragma message("SSL will not be compiled into this binary.")
#endif // SSL
    }
}

void CSocketFactory::cleanup()
{
    if (g_nInitRefCount.interlockedDecComp(0))
    {
#ifdef WIN32
        ::WSACleanup();
#endif // WIN32

#ifdef OPENSSL
        if (m_szSSLPrivKeyPasswd) free(m_szSSLPrivKeyPasswd);
        SSL_CTX_free(m_pSSLContext);
        RAND_cleanup();
        EVP_cleanup();
#elif RSASSL

#endif // SSL
    }
}


//////////////////////////////////////////////////////////////////////
// Implementation

CSocket* CSocketFactory::createClientSocket( const char* pszServerAddress, 
                                             int nServerPort,
                                             int nSocketType,
                                             int nSecurityFlags )
{
    struct hostent* pHostRemote;
    struct sockaddr_in saServer;
    SOCKET clientSocket;
    
    if (isalpha(*pszServerAddress)) 
    {
        pHostRemote = gethostbyname(pszServerAddress); 
    } 
    else  
    {
        unsigned long nInetAddr = inet_addr(pszServerAddress); 
        pHostRemote = gethostbyaddr((char *) &nInetAddr, sizeof(long), AF_INET);
    }

    if (pHostRemote == NULL)
    {
        THROW(CSocketException, EXCEP_MSSG(EXCEP_SERVERNOTFOUND, pszServerAddress));
    }
    
    memset(&saServer, 0, sizeof(struct sockaddr_in));
    memcpy(&(saServer.sin_addr), pHostRemote->h_addr, pHostRemote->h_length);

    saServer.sin_family = AF_INET; 
    saServer.sin_port   = htons(nServerPort); 

    CSocket* pSocket = NULL;

    try
    {
        if ( (clientSocket = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET &&
             connect(clientSocket, (struct sockaddr *) &saServer, sizeof(struct sockaddr)) != SOCKET_ERROR )
        {
            switch (nSecurityFlags)
            {
                case SECURE_NONE:
                    pSocket = new CSocketImpl( clientSocket, 
                                               nSocketType );
                    break;
                case SECURE_SSL:
                    pSocket = new CSSLSocketImpl( clientSocket, 
                                                  nSocketType, 
                                                  CSSLSocketImpl::sslConnect,
                                                  false );
                    break;
                case (SECURE_SSL | SECURE_AUTH):
                    pSocket = new CSSLSocketImpl( clientSocket, 
                                                  nSocketType, 
                                                  CSSLSocketImpl::sslConnect,
                                                  true );
                    break;
                default:
                    if ((nSecurityFlags & MULTI_CLIENT) != 0x0)
                    {
                        pSocket = clientHandshake(clientSocket, nSocketType, nSecurityFlags);
                    }
                    else
                    {
                        THROW(CSocketException, EXCEP_MSSG(EXCEP_CLIENTSECURITY));
                    }
            }
        }
        else
        {
            THROW(CSocketException, EXCEP_MSSG(EXCEP_CLIENTSOCKET, pszServerAddress, nServerPort));
        }
    }
    catch (CSocketException* pe)
    {
        if (clientSocket != INVALID_SOCKET) closesocket(clientSocket);
        if (pSocket) delete pSocket;
        throw pe;
    }

    return pSocket;
}

CServerSocket* CSocketFactory::createServerSocket( const char* pszLocalInterface, 
                                                   int nLocalPort,
                                                   int nSocketType,
                                                   int nSecurityFlags )
{
    struct sockaddr_in saLocal;
    SOCKET serverSocket;

    saLocal.sin_addr.s_addr = ( !pszLocalInterface ? INADDR_ANY : 
                                                     inet_addr(pszLocalInterface) );
    saLocal.sin_family = AF_INET; 
    saLocal.sin_port   = htons(nLocalPort); 

    CServerSocket* pServerSocket = NULL;

    try
    {
        if ( (serverSocket = socket(AF_INET, nSocketType, 0)) != INVALID_SOCKET &&
             bind(serverSocket, (struct sockaddr *) &saLocal, sizeof(struct sockaddr)) != SOCKET_ERROR )
        {
            pServerSocket = new CServerSocket(serverSocket, nSocketType, nSecurityFlags);
        }
        else
        {
            THROW( CSocketException, EXCEP_MSSG( EXCEP_SERVERSOCKET, 
                                                 (!pszLocalInterface ? "ALL" : pszLocalInterface), 
                                                 nLocalPort ) );
        }
    }
    catch (CSocketException* pe)
    {
        if (serverSocket != INVALID_SOCKET) closesocket(serverSocket);
        if (pServerSocket) delete pServerSocket;
        throw pe;
    }

    return pServerSocket;
}

CSocket* CSocketFactory::clientHandshake( SOCKET socket, 
                                          int nSockType,
                                          int nSecurityFlags )
{
    CSocketImpl sock(socket, SOCK_STREAM);
    bool bSecure;

    // The handshake for a server managed secure client with the server.
    // Client waits for server hello with connection method.
    //
    // Without Authentication and not secure:
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=' + Server Certificate + '\0'
    // Client  -> Server  'HELO:CERT=' + Client Certificate + '\0'
    // Server  -> Client  'OK:MD=\0'
    // Client  -> Server  'OK:MD=\0'
    // Client <-> Server  Communication proceeds
    // 
    // With Server Authentication and not secure:
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=' + Server Certificate + '\0'
    // Client  -> Server  'HELO:CERT=\0'
    // Server  -> Client  'OK:MD=[random sig of 63 chars][{digest[random sig of 63 chars]}encrypted with server's PK]\0'
    // Client  -> Server  'OK:MD=\0'
    // Client <-> Server  Communication proceeds
    // 
    // With Server & Client Authentication and not secure:
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=' + Server Certificate + '\0'
    // Client  -> Server  'HELO:CERT=' + Client Certificate + '\0'
    // Server  -> Client  'OK:MD=[random sig of 63 chars][{digest[random sig of 63 chars]}encrypted with server's PK]\0'
    // Client  -> Server  'OK:MD=[random sig of 63 chars][{digest[random sig of 63 chars]}encrypted with client's PK]\0'
    // Client <-> Server  Communication proceeds
    //
    // Secure where SSL will handle authentication:
    //
    // Server               -> Client              'HELO:SECURE=1:CERT=\0'
    // Client               -> Server              'HELO:CERT=\0'
    // Server               -> Client              'OK:MD=\0'
    // Client               -> Server              'OK:MD=\0'
    // Client[SSL_Connect] <-> Server[SSL_Accept]  Communication proceeds
    //
    // Non-Secure 
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=\0'
    // Client  -> Server  'HELO:CERT=\0'
    // Server  -> Client  'OK:MD=\0'
    // Client  -> Server  'OK:MD=\0'
    // Client <-> Server  Communication proceeds

    std::ostringstream ostrServerCert;
    std::ostringstream ostrServerSig;

    const char* szServerCert;
    const char* szServerSig;

    INIT_READ;

    // Read server handshake message from server

    // 1) Read 'HELO:' 
    READ_MSSG(5, _helo);

    // 2) Read 'SECURE=?:' 
    READ_MSSG(9, _secure);
    bSecure = (achBuffer[7] == '1');

    // 3) Read 'CERT=' from server
    READ_MSSG(5, _cert);
    READ_BUFFER(ostrServerCert);
    szServerCert = ostrServerCert.str().c_str();

    // Send client handshake message to server

    // 1) Write 'HELO:CERT='
    WRITE_MSSG(_helo_clnt);

    // 2) Write certificate if available
    if (m_bSSLCertify) writeCertificate(&sock);

    // Read acknowledgment message from server

    // 1) Read 'OK:' 
    READ_MSSG(3, _ok);

    // 2) Read 'MD=' 
    READ_MSSG(3, _md);
    READ_BUFFER(ostrServerSig);
    szServerSig = ostrServerSig.str().c_str();

    // Send connection acknoledgement response

    // 1) Write 'OK:MD='
    WRITE_MSSG(_resp_ok);

    // 2) Write signature if possible
    if (m_bSSLCertify) writeSignature(&sock);

    // Verify peers's certificate and 
    // authenticate peer's signature.

    bool bAuthReqd = ((nSecurityFlags & SECURE_AUTH) != 0x0);
    int nConnectOK = 0 /* 0: error, 1: normal, 2: secure */;

    if (bSecure)
    {
        // Make sure the socket is not closed
        // when the 'sock' object is destroyed.
        sock.detach();
        nConnectOK = 2;
    }
    else if ( !bAuthReqd ||
              ( *szServerCert && *szServerSig && authenticate(szServerCert, szServerSig) ) )
    {
        // Make sure the socket is not closed
        // when the 'sock' object is destroyed.
        sock.detach();
        nConnectOK = 1;
    }

    // Create the appropriate socket 
    // object and return
    switch (nConnectOK)
    {
        case 1:
            return new CSocketImpl(socket, nSockType);
        case 2:
            return new CSSLSocketImpl( socket, 
                                       nSockType, 
                                       CSSLSocketImpl::sslConnect,
                                       bAuthReqd );
        default:
            THROW(CSocketException, EXCEP_MSSG(EXCEP_HANDSHAKE));
    }
}

CSocket* CSocketFactory::serverHandshake( SOCKET socket, 
                                          int nSockType,
                                          int nSecurityFlags )
{
    CSocketImpl sock(socket, SOCK_STREAM);
    bool bSecure = ((nSecurityFlags & SECURE_SSL) != 0x0);

    // The handshake for a server managed secure client with the server.
    // Client waits for server hello with connection method.
    //
    // Without Authentication and not secure:
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=' + Server Certificate + '\0'
    // Client  -> Server  'HELO:CERT=' + Client Certificate + '\0'
    // Server  -> Client  'OK:MD=\0'
    // Client  -> Server  'OK:MD=\0'
    // Client <-> Server  Communication proceeds
    // 
    // With Server Authentication and not secure:
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=' + Server Certificate + '\0'
    // Client  -> Server  'HELO:CERT=\0'
    // Server  -> Client  'OK:MD=[random sig of 63 chars][{digest[random sig of 63 chars]}encrypted with server's PK]\0'
    // Client  -> Server  'OK:MD=\0'
    // Client <-> Server  Communication proceeds
    // 
    // With Server & Client Authentication and not secure:
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=' + Server Certificate + '\0'
    // Client  -> Server  'HELO:CERT=' + Client Certificate + '\0'
    // Server  -> Client  'OK:MD=[random sig of 63 chars][{digest[random sig of 63 chars]}encrypted with server's PK]\0'
    // Client  -> Server  'OK:MD=[random sig of 63 chars][{digest[random sig of 63 chars]}encrypted with client's PK]\0'
    // Client <-> Server  Communication proceeds
    //
    // Secure where SSL will handle authentication:
    //
    // Server               -> Client              'HELO:SECURE=1:CERT=\0'
    // Client               -> Server              'HELO:CERT=\0'
    // Server               -> Client              'OK:MD=\0'
    // Client               -> Server              'OK:MD=\0'
    // Client[SSL_Connect] <-> Server[SSL_Accept]  Communication proceeds
    //
    // Non-Secure 
    //
    // Server  -> Client  'HELO:SECURE=0:CERT=\0'
    // Client  -> Server  'HELO:CERT=\0'
    // Server  -> Client  'OK:MD=\0'
    // Client  -> Server  'OK:MD=\0'
    // Client <-> Server  Communication proceeds

    std::ostringstream ostrClientCert;
    std::ostringstream ostrClientSig;

    const char* szClientCert;
    const char* szClientSig;

    INIT_READ;

    // Send server handshake message to client

    // 1) Write 'HELO:SECURE=?:CERT='
    if (bSecure)
    {
        WRITE_MSSG(_helo_svr_secure);
    }
    else
    {
        WRITE_MSSG(_helo_svr);
    }

    // 2) Write certificate if available
    if (m_bSSLCertify) writeCertificate(&sock);

    // Read server handshake message from client

    // 1) Read 'HELO:' 
    READ_MSSG(5, _helo);

    // 3) Read 'CERT=' from client
    READ_MSSG(5, _cert);
    READ_BUFFER(ostrClientCert);
    szClientCert = ostrClientCert.str().c_str();

    // Send connection acknoledgement response

    // 1) Write 'OK:MD='
    WRITE_MSSG(_resp_ok);

    // 2) Write signature if possible
    if (m_bSSLCertify) writeSignature(&sock);

    // Read acknowledgment message from client

    // 1) Read 'OK:' 
    READ_MSSG(3, _ok);

    // 2) Read 'MD=' 
    READ_MSSG(3, _md);
    READ_BUFFER(ostrClientSig);
    szClientSig = ostrClientSig.str().c_str();

    // Verify peers's certificate and 
    // authenticate peer's signature.

    bool bAuthReqd = ((nSecurityFlags & SECURE_AUTH) != 0x0);
    int nConnectOK = 0 /* 0: error, 1: normal, 2: secure */;

    if (bSecure)
    {
        // Make sure the socket is not closed
        // when the 'sock' object is destroyed.
        sock.detach();
        nConnectOK = 2;
    }
    else if ( !bAuthReqd ||
              ( *szClientCert && *szClientSig && authenticate(szClientCert, szClientSig) ) )
    {
        // Make sure the socket is not closed
        // when the 'sock' object is destroyed.
        sock.detach();
        nConnectOK = 1;
    }

    // Create the appropriate socket 
    // object and return
    switch (nConnectOK)
    {
        case 1:
            return new CSocketImpl(socket, nSockType);
        case 2:
            return new CSSLSocketImpl( socket, 
                                       nSockType, 
                                       CSSLSocketImpl::sslAccept,
                                       bAuthReqd );
        default:
            THROW(CSocketException, EXCEP_MSSG(EXCEP_HANDSHAKE));
    }
}

bool CSocketFactory::authenticate( const char* szPeerCert, 
                                   const char* szPeerMssg )
{
    bool bOK = true;

    try
    {
#ifdef OPENSSL
        X509_STORE* x509Store = SSL_CTX_get_cert_store(m_pSSLContext);

        // Extract peer's certificate
        X509* px509Cert = NULL;
        BIO* pbio = BIO_new_mem_buf((void *) szPeerCert, -1);

        bOK = (PEM_read_bio_X509(pbio, &px509Cert, NULL, NULL) != 0x0);
        if (bOK)
        {
            // Verify the certificate
            X509_STORE_CTX x509Context;
            X509_STORE_CTX_init(&x509Context, x509Store, px509Cert, NULL);

            bOK = (X509_verify_cert(&x509Context) != 0x0);
            if (bOK)
            {               
                // Validate peer's signature.
                EVP_PKEY* pevpPKey = X509_get_pubkey(px509Cert);

                EVP_MD_CTX evpMDContext;
                EVP_VerifyInit(&evpMDContext, EVP_sha1());
                EVP_VerifyUpdate(&evpMDContext, szPeerMssg, SIG_MLEN);

                unsigned char* szPeerSig = ((unsigned char *) szPeerMssg) + SIG_MLEN;
                int nPeerSigLen = strlen((char *) szPeerSig);

                unsigned char achSigBuff[SIG_BLEN];
                int nSigBuffLen = EVP_DecodeBlock(achSigBuff, szPeerSig, nPeerSigLen) - 1;
                bOK = (EVP_VerifyFinal(&evpMDContext, achSigBuff, nSigBuffLen, pevpPKey) != 0x0);

                EVP_PKEY_free(pevpPKey);
            }

            X509_STORE_CTX_cleanup(&x509Context);
        }

#ifdef _DEBUG
        char* szCertificate;
        printf("Authenticated peer certificate :\n");

        szCertificate = X509_NAME_oneline(px509Cert->cert_info->subject, 0, 0);
        printf("   * cert's subject is : %s\n", (szCertificate ? szCertificate : "UNKNOWN"));
        free(szCertificate);

        szCertificate = X509_NAME_oneline(px509Cert->cert_info->issuer, 0, 0);
        printf("   * cert's issuer is : %s\n\n", (szCertificate ? szCertificate : "UNKNOWN"));
        free(szCertificate);
#endif // _DEBUG

        BIO_free(pbio);
        X509_free(px509Cert);

        if (!bOK)
        {
            THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
        }

#elif RSASSL

#endif // SSL
    }
    catch (CSocketException* pe)
    {
        // Do nothing as exception is logged.
        delete pe;
    }
    return bOK;
}

bool CSocketFactory::writeCertificate(CSocket* pSocket)
{
    bool bOK = true;

    try
    {
#ifdef OPENSSL
        SSL* pSSL = SSL_new(m_pSSLContext);
        X509* px509Cert = SSL_get_certificate(pSSL);

        BUF_MEM* bufMem = BUF_MEM_new();
        BIO* pbio = BIO_new(BIO_s_mem());
        BIO_set_mem_buf(pbio, bufMem, BIO_CLOSE);

        bOK = (PEM_write_bio_X509(pbio, px509Cert) != 0x0);
        if (bOK) 
        {
            // Write certificate buffer size as
            // high + low order byte pair sequence
            unsigned char achHILO[2];
            achHILO[0] = (bufMem->length & 0xFF00) >> 8;
            achHILO[1] = bufMem->length & 0x00FF;
            pSocket->sendData(achHILO, 2);

            // Write certificate buffer to socket
            pSocket->sendData(bufMem->data, bufMem->length);
        }

        BIO_free(pbio);
        SSL_free(pSSL);

        if (!bOK)
        {
            THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
        }

#elif RSASSL

#endif // SSL
    }
    catch (CSocketException* pe)
    {
        // Do nothing as exception is logged.
        delete pe;
    }
    return bOK;
}

bool CSocketFactory::writeSignature(CSocket* pSocket)
{
    bool bOK = true;

    try
    {

#ifdef OPENSSL
        clock_t nSeed = clock();
        RAND_seed(&nSeed, sizeof(nSeed));

        // Generate a random sequence of ASCII 
        // chars (ASCII value 32 - 127)
        unsigned char achRandMssg[SIG_MLEN];

        bOK = (RAND_bytes(achRandMssg, SIG_MLEN) != 0x0);
        if (bOK)
        {
            int i;
            for (i = 0; i < SIG_MLEN; i++)
                achRandMssg[i] = (unsigned char) (((double) (achRandMssg[i] + 1) / 256.0) * 95.0 + 32.0);

            // Sign the random sequence
            unsigned char achSigBuff_b64[SIG_BLEN];
            unsigned char achSigBuff[SIG_BLEN];
            unsigned int nSigBuffLen = SIG_BLEN;

            SSL* pSSL = SSL_new(m_pSSLContext);
            EVP_PKEY* evpPKey = SSL_get_privatekey(pSSL);

            EVP_MD_CTX evpMDContext;
            EVP_SignInit(&evpMDContext, EVP_sha1());
            EVP_SignUpdate(&evpMDContext, achRandMssg, SIG_MLEN);

            bOK = (EVP_SignFinal(&evpMDContext, achSigBuff, &nSigBuffLen, evpPKey) != 0x0);
            if (bOK)
            {
                nSigBuffLen = EVP_EncodeBlock(achSigBuff_b64, achSigBuff, nSigBuffLen);
                int nSigLen = SIG_MLEN + nSigBuffLen;

                // Write signature buffer size as
                // high + low order byte pair sequence
                unsigned char achHILO[2];
                achHILO[0] = (nSigLen & 0xFF00) >> 8;
                achHILO[1] = nSigLen & 0x00FF;
                pSocket->sendData(achHILO, 2);
     
                // Write signature
                pSocket->sendData(achRandMssg, SIG_MLEN);
                pSocket->sendData(achSigBuff_b64, nSigBuffLen);
            }

            SSL_free(pSSL);
        }

        if (!bOK)
        {
            THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
        }

#elif RSASSL

#endif // SSL
    }
    catch (CSocketException* pe)
    {
        // Do nothing as exception is logged.
        delete pe;
    }
    return bOK;
}

int CSocketFactory::openSSLPasswdCallback(char *szBuffer, int nSize, int, void *)
{
    if (m_szSSLPrivKeyPasswd)
        strncpy(szBuffer, m_szSSLPrivKeyPasswd, nSize);
    else
        *szBuffer = 0x0;

    return strlen(szBuffer);
}
