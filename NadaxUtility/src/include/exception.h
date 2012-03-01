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
// Exception.h: interface for the CException class.
//

#if !defined(_EXCEPTION_H__INCLUDED_)
#define _EXCEPTION_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#endif // UNIX

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>


//////////////////////////////////////////////////////////////////////
// Exceptions Macros

#define THROW(excep, mssg) \
{ \
    excep* pe = new excep(__FILE__, __LINE__); \
    pe->mssg; \
    throw pe; \
}

#define EXCEP_MSSG  setMessage


//////////////////////////////////////////////////////////////////////
// This class implements a generic exception thrown by 
// synchronization objects.

class CException  
{
public:

	CException(const char* pszSource, int nLineNumber)
    {
        m_pszMssg = NULL;
        m_pszSource = strdup(pszSource);
        m_nLineNumber = nLineNumber;
    }

    virtual ~CException() 
    { 
        if (m_pszMssg) free(m_pszMssg); 
        if (m_pszSource) free(m_pszSource); 
    }

	const char* getMessage()
    { 
        return m_pszMssg; 
    }

	void setMessage(const char* pszFmtMssg, ...)
	{
		char szMssg[1024];

		va_list arglist;
		va_start(arglist, pszFmtMssg);
		_vsnprintf(szMssg, sizeof(szMssg), pszFmtMssg, arglist);
		va_end(arglist);

		m_pszMssg = strdup(szMssg);

#ifdef _DEBUG

        time_t ltime;
        time(&ltime);

        char szToday[50];

#ifdef WIN32
        struct tm *now;
        now = localtime(&ltime);

        strftime(szToday, sizeof(szToday), "%m/%d/%y %H:%M:%S", now);
        printf("\nEXCEPTION [%s]\n", szToday);
#endif // WIN32

#ifdef UNIX
        struct tm now;
        localtime_r(&ltime, &now);

#ifdef LINUX
        printf("\nEXCEPTION %s", asctime_r(&now, szToday));
#else
        strftime(szToday, sizeof(szToday), "%m/%d/%y %H:%M:%S", &now);
        printf("\nEXCEPTION [%s]\n", szToday);
#endif // LINUX

#endif // UNIX

        printf("  * Message: \"%s\"\n", m_pszMssg);
        printf("  * At Source [%s] line number [%d].\n", m_pszSource, m_nLineNumber);
        perror("  * C/C++ sub-system ");

#ifdef WIN32
        printf("  * Win32 sub-system: GetLastError() = %d\n", ::GetLastError());
#endif // WIN32

        printf("\n");

#endif // _DEBUG

	}

private:

	char* m_pszMssg;
    char* m_pszSource;
    int   m_nLineNumber;
};


#endif // !defined(_EXCEPTION_H__INCLUDED_)
