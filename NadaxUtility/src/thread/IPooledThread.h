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
// IPooledThread.h: interface for the IPooledThread class.
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

#if !defined(_IPOOLEDTHREAD_H__INCLUDED_)
#define _IPOOLEDTHREAD_H__INCLUDED_

class CThread;


//////////////////////////////////////////////////////////////////////
// This interface class should be implemented by all objects, which
// implement an instance of a pooled thread managed by the thread
// pool manager.

class IPooledThread
{
public:

    // This method is called in order that the implementor
    // may record the thread that executes this implementation.
    virtual void setThread(CThread* pThread) = 0;

    // This method should implement
    // the main thread body.
    virtual void run() = 0;

    // This method should implement any termination
    // logic if necessary that causes the run()
    // method to terminate cleanly.
    virtual void stop() = 0;

    // This method should implement
    // any clean up that needs to be 
    // done once the main thread
    // body finishes.
    virtual void cleanup() = 0;
};

#endif // !defined(_IPOOLEDTHREAD_H__INCLUDED_)
