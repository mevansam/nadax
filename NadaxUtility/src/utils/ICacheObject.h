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
// ICacheObject.h: interface for the ICacheObject class.
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

#if !defined(_ICACHEOBJECT_H__INCLUDED_)
#define _ICACHEOBJECT_H__INCLUDED_


//////////////////////////////////////////////////////////////////////
// This interface class should be implemented by all objects, which
// implement an instance of a object cached by the cache manager. 
// These objects should contain methods to hydrate and rehydrate from
// the disk drive if requested.

class ICacheObject
{
public:

    // This method should create a duplicate of the data held by the 
    // cache object or implement a reference counting mechanism such 
    // that data does not go out of scope when a cache client is using
    // a cache object. For example a client may be using suchd data 
    // when another thread executes a get operation on  the cache which 
    // causes the data being used by that client to be unloaded. To 
    // handle such a case this method should either:
    //
    // 1) Make a duplicate of the data and return a pointer to it
    // 2) Implement reference counting on the data and return a 
    //    pointer to it.
    virtual void* getData() = 0;

    // This method should populate this cache object
    // with object data identified by the given name.
    virtual void load(const char* pszName) = 0;

    // Unloads the object data currently held by this
    // cache object. Typically this method should clean
    // up memory used by the data of the object it
    // currently represents.
    virtual void unload() = 0;

    // Writes the object's data members to the given data output
    // stream. This stream typically writes to a disk cache.
    virtual void serialize(const char* pszFileName) = 0;

    // Reads the object's data members from the given data input
    // stream. This stream typically writes to a disk cache.
    virtual void deserialize(const char* pszFileName) = 0;
};

#endif // !defined(_ICACHEOBJECT_H__INCLUDED_)
