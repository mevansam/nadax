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
// IServer.h: interface for server socket connection handler
//            factory implementation.
//
// Author  : Mevan Samaratunga
// Date    : 08/01/2000
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

#if !defined(_ISERVERFACTORY_H__INCLUDED_)
#define _ISERVERFACTORY_H__INCLUDED_

#include "IPooledThread.h"

class CSocket;


//////////////////////////////////////////////////////////////////////
// This interface class should be implemented to instantiate and
// initialize IPooledThread implementations to service server socket
// connections. The IPooledThread implementation is responsible for
// cleaning up the connections once done as well as itself. Typically
// the IPooledThread implementation should call the following 
// sequence of methods when it is destroyed:
// 
// <Server Socket>->close();    - Closes the server socket member
// delete <Server Socket>;      - deletes the instance of the socket
// delete this;                 - deletes the instance of the 
//                                IPooledThread server socket 
//                                handler.

class IServerFactory
{
public:

    // This method is invoked when a connection 
    // is accepted and needs to be serviced.
    virtual IPooledThread* create(CSocket* pSocket) = 0;
};

#endif // !defined(_ISERVERFACTORY_H__INCLUDED_)
