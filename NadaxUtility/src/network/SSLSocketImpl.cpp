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
// CSSLSocketImpl.cpp: This class implements a socket that implements
//                     secure protocols on top of data recieved or sent
//                     through the socket connection.
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

#include "SSLSocketImpl.h"
#include "SocketFactory.h"

#ifdef WIN32
#endif // UNIX

#ifdef UNIX
#include <sys/time.h>
#endif // UNIX

#include "macros.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"

#include <ctype.h>


//////////////////////////////////////////////////////////////////////
// Exception messages defines

#define EXCEP_CONN  "Invalid SSL connection type specified."
#define EXCEP_RECV  "An error occurred whilst reading data from the socket connected to the network address '%s'."
#define EXCEP_SEND  "An error occurred whilst attempting to send data thru the socket connected to the network address '%s'."


//////////////////////////////////////////////////////////////////////
// Construction / Destruction

CSSLSocketImpl::CSSLSocketImpl( SOCKET socket, 
                                int nSocketType,
                                eCONNTYPE eConnectType,
                                bool bAuthenticate )
    : CSocket(socket, nSocketType)
{
#ifdef OPENSSL

    clock_t nSeed = clock();
    RAND_seed(&nSeed, sizeof(nSeed));

    m_pSSL = SSL_new(CSocketFactory::getSSLContext());

    bool bOK = (m_pSSL != NULL);
    if (bOK)
    {
        SSL_set_verify( m_pSSL, 
                        ( bAuthenticate && CSocketFactory::canSSLVerify() ? 
                          SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT : SSL_VERIFY_NONE ),
                        NULL );

        SSL_set_fd(m_pSSL, socket);

        switch (eConnectType)
        {
            case sslAccept:
                bOK = (SSL_accept(m_pSSL) != -1);
                break;
            case sslConnect:
                bOK = (SSL_connect(m_pSSL) != -1);
                break;
            default:
                THROW(CSocketException, EXCEP_MSSG(EXCEP_CONN));
        }
    }
    if (!bOK)
    {
        THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
    }

#ifdef _DEBUG
    if (bAuthenticate)
    {
        char* szCertificate;
        X509* pPeerCert = SSL_get_peer_certificate(m_pSSL);

        if (pPeerCert)
        {
            printf("Authenticated peer certificate in SSL connection :\n");

            szCertificate = X509_NAME_oneline(pPeerCert->cert_info->subject, 0, 0);
            printf("   * cert's subject is : %s\n", (szCertificate ? szCertificate : "UNKNOWN"));
            free(szCertificate);

            szCertificate = X509_NAME_oneline(pPeerCert->cert_info->issuer, 0, 0);
            printf("   * cert's issuer is : %s\n\n", (szCertificate ? szCertificate : "UNKNOWN"));
            free(szCertificate);

            X509_free(pPeerCert);
        }
        else
        {
            THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
        }
    }
#endif // _DEBUG

#elif RSASSL

#endif // SSL

    m_nSecurityLevel = (bAuthenticate ? SECURE_SSL | SECURE_AUTH : SECURE_SSL);
}

CSSLSocketImpl::~CSSLSocketImpl()
{
#ifdef OPENSSL
    SSL_free(m_pSSL);

#elif RSASSL

#endif // SSL
}


//////////////////////////////////////////////////////////////////////
// Protected Implementation

int CSSLSocketImpl::recvData(void* pBuffer, long nSize)
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
#ifdef OPENSSL
                if ((nRecvLen = SSL_read(m_pSSL, (char *) pBuffer, nSize)) != -1) 
                {
                    return nRecvLen;
                }

#elif RSASSL

#endif // SSL
                else
                    THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));
            }
            else 
                // This indicates a recv timeout
                return -1;
        }
    } 

    THROW(CSocketException, EXCEP_MSSG(EXCEP_RECV, inet_ntoa(m_saPeer.sin_addr)));
}
    
int CSSLSocketImpl::sendData(void* pBuffer, long nSize)
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
#ifdef OPENSSL
                if ((nSentLen = SSL_write(m_pSSL, (char *) pBuffer, nSize)) != -1) 
                    return nSentLen;
                else
                    THROW(CSocketException, EXCEP_MSSG(SSL_ERROR));

#elif RSASSL

#endif // SSL
            }
            else 
                // This indicates a send timeout
                return -1;
        }
    }

    THROW(CSocketException, EXCEP_MSSG(EXCEP_RECV, inet_ntoa(m_saPeer.sin_addr)));
}
