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
//
// Macros.h: Declares common macros
//

#if !defined(_MACROS_H__INCLUDED_)
#define _MACROS_H__INCLUDED_


// Maximum number values (two's-complement)

#define MIN_SIGNED_INT     (1 << (sizeof(int) * 8 - 1))
#define MAX_SIGNED_INT     (-1 ^ (1 << (sizeof(int) * 8 - 1)))
#define MIN_SIGNED_LONG    (1L << (sizeof(long) * 8 - 1))
#define MAX_SIGNED_LONG    (-1L ^ (1L << (sizeof(long) * 8 - 1)))
#define MAX_UNSIGNED_INT   ((unsigned int)  -1)
#define MAX_UNSIGNED_LONG  ((unsigned long) -1)

// File system specific

#ifdef WIN32

#define PATH_SEP  '\\'
#define MKDIR(name)  _mkdir(name)

#endif // WIN32

#ifdef UNIX

#define PATH_SEP  '/'
#define MKDIR(name)  mkdir(name, S_IFDIR | S_IRWXU)

#endif // UNIX

// Open SSL

#ifdef OPENSSL

#define  SSL_ERROR  ERR_error_string(ERR_get_error(), NULL)

#endif // OPENSSL


#endif // !defined(_MACROS_H__INCLUDED_)
