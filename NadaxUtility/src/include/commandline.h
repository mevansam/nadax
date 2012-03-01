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
// CommandLine.h: interface for the CCommandLine class.
//

#if !defined(_COMMANDLINE_H__INCLUDED_)
#define _COMMANDLINE_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#endif // UNIX


//////////////////////////////////////////////////////////////////////
// This class implements a command line parser

class CCommandLine  
{
public:
    
    CCommandLine( int nArgc, 
                  char* aszArgv[], 
                  char chCommandPrefix = '-' )
    {
        m_nArgc           = nArgc;
        m_aszArgv         = aszArgv;
        m_chCommandPrefix = chCommandPrefix;
    }

    bool exists(const char* szArg)
    {
        int i;
        for (i = 0; i < m_nArgc; i++)
        {
            if (strcmp(m_aszArgv[i], szArg) == 0)
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    const char* lookup(const char* szCommand)
    {
        int nCommandLen = strlen(szCommand);
        
        int i, len;
        for (i = 0; i < m_nArgc; i++)
        {
            if ( (len = strlen(m_aszArgv[i])) >= nCommandLen &&
                 strncmp(m_aszArgv[i], szCommand, nCommandLen) == 0 )
            {
                if (len > nCommandLen)
                {
                    return m_aszArgv[i] + nCommandLen;
                }
                else
                    return ( i < (m_nArgc - 1) && *m_aszArgv[i + 1] != m_chCommandPrefix ? 
                             m_aszArgv[i + 1] : NULL );
            }
        }

        return NULL;
    }

private:

    int    m_nArgc;
    char** m_aszArgv;
    char   m_chCommandPrefix;
};


#endif // !defined(_COMMANDLINE_H__INCLUDED_)
