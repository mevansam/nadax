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
// log.h : Logging Macros
//

#ifndef _LOG_H__INCLUDED_
#define _LOG_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#include "pthread.h"
#endif // UNIX

#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef LOG_LEVEL_TRACE

#define IS_TRACE  true
#define IS_WARN   true
#define IS_INFO   true
#define IS_ERROR  true

#define TRACE(...)  log("TRACE", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define WARN(...)   log("WARN", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define INFO(...)   log("INFO", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define ERROR(...)  log("ERROR", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define FATAL(...)  log("FATAL", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define AUDIT(...)  log("AUDIT", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#elif LOG_LEVEL_WARN

#define IS_TRACE  false
#define IS_WARN   true
#define IS_INFO   true
#define IS_ERROR  true

#define TRACE(...)
#define WARN(...)   log("WARN", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define INFO(...)   log("INFO", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define ERROR(...)  log("ERROR", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define FATAL(...)  log("FATAL", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define AUDIT(...)  log("AUDIT", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#elif LOG_LEVEL_INFO

#define IS_TRACE  false
#define IS_WARN   false
#define IS_INFO   true
#define IS_ERROR  true

#define TRACE(...)
#define WARN(...)
#define INFO(...)   log("INFO", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define ERROR(...)  log("ERROR", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define FATAL(...)  log("FATAL", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define AUDIT(...)  log("AUDIT", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#elif LOG_LEVEL_ERROR

#define IS_TRACE  false
#define IS_WARN   false
#define IS_INFO   false
#define IS_ERROR  true

#define TRACE(...)
#define WARN(...)
#define INFO(...)
#define ERROR(...)  log("ERROR", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define FATAL(...)  log("FATAL", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define AUDIT(...)  log("AUDIT", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#else

#define IS_TRACE  false
#define IS_WARN   false
#define IS_INFO   false
#define IS_ERROR  false

#define TRACE(...)
#define WARN(...)
#define INFO(...)
#define ERROR(...)
#define FATAL(...)  log("FATAL", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define AUDIT(...)  log("AUDIT", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#endif // LOG_LEVELS

inline void log(const char* pszLogLevel, const char* pszSource, const char* pszFunction, int nLineNumber, const char* pszFmtMssg, ...)
{
    char szLogContextMssg[100];
    szLogContextMssg[0] = 0;

    const char* pszSourceFileName;

#ifdef WIN32

#endif // WIN32

#ifdef UNIX
    struct timeval time;
	gettimeofday(&time, NULL);

    struct tm now;
    localtime_r(&time.tv_sec, &now);

    char szTimestamp[25];
    strftime(szTimestamp, sizeof(szTimestamp), "%m/%d/%y-%H:%M:%S", &now);

    unsigned long tid =  (unsigned long) pthread_self();

    sprintf(szLogContextMssg, "%s.%04d; 0x%012lx", szTimestamp, (time.tv_usec / 1000) % 1000, tid);
    pszSourceFileName = strrchr(pszSource, '/');

#endif // UNIX

    pszSourceFileName = (!pszSourceFileName ? pszSource : pszSourceFileName + 1);

	char szMssg[65536];

    va_list arglist;
	va_start(arglist, pszFmtMssg);
	_vsnprintf(szMssg, sizeof(szMssg), pszFmtMssg, arglist);
	va_end(arglist);

	printf("%-6s>> %s; %s(%s:%d): %s\n", pszLogLevel, szLogContextMssg, pszSourceFileName, pszFunction, nLineNumber, szMssg);
}

#endif /* _LOG_H__INCLUDED_ */
