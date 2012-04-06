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
// Unix.h: Declares win32 types for Unix (non-unicode)
//

#if !defined(_UNIX_H__INCLUDED_)
#define _UNIX_H__INCLUDED_

#include <stdlib.h>
#include <string.h>


// Values

#ifndef NULL
#define NULL   0x0
#endif

#ifndef FALSE
#define FALSE  0x0
#endif

#ifndef TRUE
#define TRUE   0x1
#endif

// Generic

#define VOID void

// Numeric

#ifndef OBJC_BOOL_DEFINED
#define BOOL  unsigned int
#endif

#define WORD  unsigned int
#define DWORD unsigned long

#define SHORT short
#define INT   int
#define LONG  long

// Character

#define CHAR    char
#define TCHAR   char
#define LPSTR   char*
#define LPCSTR  const char*
#define LPTSTR  char*
#define LPCTSTR const char*

#define _T(str) str

// Math

#ifndef ANDROID
inline char   min(char a, char b)     { return (a < b ? a : b); }
inline char   max(char a, char b)     { return (a > b ? a : b); }
inline short  min(short a, short b)   { return (a < b ? a : b); }
inline short  max(short a, short b)   { return (a > b ? a : b); }
inline int    min(int a, int b)       { return (a < b ? a : b); }
inline int    max(int a, int b)       { return (a > b ? a : b); }
inline long   min(long a, long b)     { return (a < b ? a : b); }
inline long   max(long a, long b)     { return (a > b ? a : b); }
inline float  min(float a, float b)   { return (a < b ? a : b); }
inline float  max(float a, float b)   { return (a > b ? a : b); }
inline double min(double a, double b) { return (a < b ? a : b); }
inline double max(double a, double b) { return (a > b ? a : b); }
#endif // ANDROID


// I/O 

#define MAX_PATH  256

#define _S_IFDIR      S_IFDIR
#define _S_IREAD      S_IREAD 
#define _S_IWRITE     S_IWRITE

#define _O_CREAT      O_CREAT
#define _O_APPEND     O_APPEND
#define _O_TRUNC      O_TRUNC
#define _O_RDONLY     O_RDONLY
#define _O_WRONLY     O_WRONLY
#define _O_RDWR       O_RDWR
#define _O_EXCL       O_EXCL
#define _O_TEXT       0
#define _O_BINARY     0
#define _O_RAW        0
#define _O_NOINHERIT  O_NOINHERIT

#define _snprintf   snprintf
#define _vsprintf   vsprintf
#define _vsnprintf  vsnprintf

#define _open   open
#define _close  close
#define _read   read
#define _write  write
#define _stat   stat

#define _finddata_t finddata_t
#define _findfirst  findfirst
#define _findnext   findnext
#define _findclose  findclose

// Socket

#define SOCKET  int

#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1

#ifndef ANDROID
#ifndef TCP_NODELAY
#define	TCP_NODELAY  0x0001
#endif // TCP_NODELAY
#endif // ANDROID

#ifndef SO_ACCEPTCONN
#define	SO_ACCEPTCONN  0x0002
#endif // TCP_NODELAY

#define closesocket  ::close

#endif // !defined(_UNIX_H__INCLUDED_)
