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
// number.h: This a thread safe implementation of a numeric
//           template class
//

#if !defined(_NUMBER_H__INCLUDED_)
#define _NUMBER_H__INCLUDED_

#ifdef WIN32
#include "windows.h"
#endif // WIN32

#ifdef UNIX
#include "unix.h"
#include "pthread.h" 
#endif // UNIX

#include "macros.h"
#include "criticalsection.h"
#include "exception.h"


//////////////////////////////////////////////////////////////////////
// Macro for comparison operation

#define NUMBER_COMPARE(comparison) \
	bool bResult; \
	m_cs.enter(); \
	bResult = (comparison); \
	m_cs.exit(); \
	return bResult;


//////////////////////////////////////////////////////////////////////
// This template class implements a synchronized number.
// The type <T> must be one of the standard numeric.

template <class T>
class Number
{
public:
	Number();
	Number(T nValue);
	Number(Number& nValue);
	virtual ~Number();

    // Atomic inc/dec operations

    T interlockedInc();
    T interlockedDec();

    bool interlockedIncComp(T nValue) { NUMBER_COMPARE(++m_nNumber == nValue) }
	bool interlockedDecComp(T nValue) { NUMBER_COMPARE(--m_nNumber == nValue) }

    // Assignment

    Number& operator=(Number& nValue);
	Number& operator=(T nValue);

    // Unary operators

    Number operator++(int);
	Number operator--(int);
	Number& operator++();
	Number& operator--();
	Number& operator+=(T nDelta);
	Number& operator-=(T nDelta);

    // Comparison operators

    bool operator! () { NUMBER_COMPARE(m_nNumber == 0)   }
	operator bool  () { NUMBER_COMPARE((bool) m_nNumber) }

    bool operator&& (const Number nValue) 
        { NUMBER_COMPARE((bool) m_nNumber && (bool) nValue.m_nNumber) }
    bool operator|| (const Number nValue) 
        { NUMBER_COMPARE((bool) m_nNumber || (bool) nValue.m_nNumber) }
    bool operator== (const Number nValue) 
        { NUMBER_COMPARE(m_nNumber == nValue.m_nNumber) }
	bool operator!= (const Number nValue) 
        { NUMBER_COMPARE(m_nNumber != nValue.m_nNumber) }
	bool operator<  (const Number nValue) 
        { NUMBER_COMPARE(m_nNumber < nValue.m_nNumber) }
	bool operator>  (const Number nValue) 
        { NUMBER_COMPARE(m_nNumber > nValue.m_nNumber) }
	bool operator<= (const Number nValue) 
        { NUMBER_COMPARE(m_nNumber <= nValue.m_nNumber) }
	bool operator>= (const Number nValue) 
        { NUMBER_COMPARE(m_nNumber >= nValue.m_nNumber) }

    bool operator&& (const char nValue) 
        { NUMBER_COMPARE((bool) m_nNumber && nValue) }
    bool operator|| (const char nValue) 
        { NUMBER_COMPARE((bool) m_nNumber || nValue) }
	bool operator== (const char nValue) 
        { NUMBER_COMPARE(m_nNumber == nValue) }
	bool operator!= (const char nValue) 
        { NUMBER_COMPARE(m_nNumber != nValue) }
	bool operator<  (const char nValue) 
        { NUMBER_COMPARE(m_nNumber < nValue) }
	bool operator>  (const char nValue) 
        { NUMBER_COMPARE(m_nNumber > nValue) }
	bool operator<= (const char nValue) 
        { NUMBER_COMPARE(m_nNumber <= nValue) }
	bool operator>= (const char nValue) 
        { NUMBER_COMPARE(m_nNumber >= nValue) }

    bool operator&& (const short nValue) 
        { NUMBER_COMPARE((bool) m_nNumber && nValue) }
    bool operator|| (const short nValue) 
        { NUMBER_COMPARE((bool) m_nNumber || nValue) }
	bool operator== (const short nValue) 
        { NUMBER_COMPARE(m_nNumber == nValue) }
	bool operator!= (const short nValue) 
        { NUMBER_COMPARE(m_nNumber != nValue) }
	bool operator<  (const short nValue) 
        { NUMBER_COMPARE(m_nNumber < nValue) }
	bool operator>  (const short nValue) 
        { NUMBER_COMPARE(m_nNumber > nValue) }
	bool operator<= (const short nValue) 
        { NUMBER_COMPARE(m_nNumber <= nValue) }
	bool operator>= (const short nValue) 
        { NUMBER_COMPARE(m_nNumber >= nValue) }
	
    bool operator&& (const int nValue) 
        { NUMBER_COMPARE((bool) m_nNumber && nValue) }
    bool operator|| (const int nValue) 
        { NUMBER_COMPARE((bool) m_nNumber || nValue) }
    bool operator== (const int nValue) 
        { NUMBER_COMPARE(m_nNumber == nValue) }
	bool operator!= (const int nValue) 
        { NUMBER_COMPARE(m_nNumber != nValue) }
	bool operator<  (const int nValue) 
        { NUMBER_COMPARE(m_nNumber < nValue) }
	bool operator>  (const int nValue) 
        { NUMBER_COMPARE(m_nNumber > nValue) }
	bool operator<= (const int nValue) 
        { NUMBER_COMPARE(m_nNumber <= nValue) }
	bool operator>= (const int nValue) 
        { NUMBER_COMPARE(m_nNumber >= nValue) }

    bool operator&& (const long nValue) 
        { NUMBER_COMPARE((bool) m_nNumber && nValue) }
    bool operator|| (const long nValue) 
        { NUMBER_COMPARE((bool) m_nNumber || nValue) }
    bool operator== (const long nValue) 
        { NUMBER_COMPARE(m_nNumber == nValue) }
	bool operator!= (const long nValue) 
        { NUMBER_COMPARE(m_nNumber != nValue) }
	bool operator<  (const long nValue) 
        { NUMBER_COMPARE(m_nNumber < nValue) }
	bool operator>  (const long nValue) 
        { NUMBER_COMPARE(m_nNumber > nValue) }
	bool operator<= (const long nValue) 
        { NUMBER_COMPARE(m_nNumber <= nValue) }
	bool operator>= (const long nValue) 
        { NUMBER_COMPARE(m_nNumber >= nValue) }

    bool operator&& (const double nValue) 
        { NUMBER_COMPARE((bool) m_nNumber && nValue) }
    bool operator|| (const double nValue) 
        { NUMBER_COMPARE((bool) m_nNumber || nValue) }
    bool operator== (const double nValue) 
        { NUMBER_COMPARE(m_nNumber == nValue) }
	bool operator!= (const double nValue) 
        { NUMBER_COMPARE(m_nNumber != nValue) }
	bool operator<  (const double nValue) 
        { NUMBER_COMPARE(m_nNumber < nValue) }
	bool operator>  (const double nValue) 
        { NUMBER_COMPARE(m_nNumber > nValue) }
	bool operator<= (const double nValue) 
        { NUMBER_COMPARE(m_nNumber <= nValue) }
	bool operator>= (const double nValue) 
        { NUMBER_COMPARE(m_nNumber >= nValue) }

    bool operator&& (const float nValue) 
        { NUMBER_COMPARE((bool) m_nNumber && nValue) }
    bool operator|| (const float nValue) 
        { NUMBER_COMPARE((bool) m_nNumber || nValue) }
    bool operator== (const float nValue) 
        { NUMBER_COMPARE(m_nNumber == nValue) }
	bool operator!= (const float nValue) 
        { NUMBER_COMPARE(m_nNumber != nValue) }
	bool operator<  (const float nValue) 
        { NUMBER_COMPARE(m_nNumber < nValue) }
	bool operator>  (const float nValue) 
        { NUMBER_COMPARE(m_nNumber > nValue) }
	bool operator<= (const float nValue) 
        { NUMBER_COMPARE(m_nNumber <= nValue) }
	bool operator>= (const float nValue) 
        { NUMBER_COMPARE(m_nNumber >= nValue) }

    // Type cast operators

	operator char();
	operator unsigned char();
	operator short();
	operator unsigned short();
	operator int();
	operator unsigned int();
	operator long();
	operator unsigned long();
	operator double();
	operator float();

private:

	T m_nNumber;
	CriticalSection m_cs;
};


//////////////////////////////////////////////////////////////////////
// Inline declarations

template <class T>
inline Number<T>::Number() 
{
	m_nNumber = 0;
}

template <class T>
inline Number<T>::Number(T nValue) 
{
	m_nNumber = nValue;
}

template <class T>
inline Number<T>::Number(Number& nValue) 
{
	m_nNumber = nValue.m_nNumber;
}

template <class T>
inline Number<T>::~Number() 
{
}

template <class T>
inline T Number<T>::interlockedInc()
{
	T nNumber;

	m_cs.enter();
	nNumber = ++m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline T Number<T>::interlockedDec()
{
	T nNumber;

	m_cs.enter();
	nNumber = --m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>& Number<T>::operator=(T nValue)
{
	m_cs.enter();
	m_nNumber = nValue;
	m_cs.exit();

	return *this;
}

template <class T>
inline Number<T> Number<T>::operator++(int)
{
    Number temp = *this;
    ++*this;
	return temp;
}

template <class T>
inline Number<T> Number<T>::operator--(int)
{
    Number temp = *this;
    --*this;
	return temp;
}

template <class T>
inline Number<T>& Number<T>::operator++()
{
	m_cs.enter();
	m_nNumber++;
	m_cs.exit();

	return *this;
}

template <class T>
inline Number<T>& Number<T>::operator--()
{
	m_cs.enter();
	m_nNumber--;
	m_cs.exit();

	return *this;
}

template <class T>
inline Number<T>& Number<T>::operator+=(T nDelta)
{
	m_cs.enter();
	m_nNumber += nDelta;
	m_cs.exit();

	return *this;
}

template <class T>
inline Number<T>& Number<T>::operator-=(T nDelta)
{
	m_cs.enter();
	m_nNumber -= nDelta;
	m_cs.exit();

	return *this;
}

template <class T>
inline Number<T>::operator char()
{
	char nNumber;

	m_cs.enter();
	nNumber = (char) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator unsigned char()
{
	unsigned char nNumber;

	m_cs.enter();
	nNumber = (unsigned char) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator short()
{
	short nNumber;

	m_cs.enter();
	nNumber = (short) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator unsigned short()
{
	unsigned short nNumber;

	m_cs.enter();
	nNumber = (unsigned short) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator int()
{
	int nNumber;

	m_cs.enter();
	nNumber = (int) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator unsigned int()
{
	unsigned int nNumber;

	m_cs.enter();
	nNumber = (unsigned int) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator long()
{
	long nNumber;

	m_cs.enter();
	nNumber = (long) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator unsigned long()
{
	unsigned long nNumber;

	m_cs.enter();
	nNumber = (unsigned long) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator float()
{
	float nNumber;

	m_cs.enter();
	nNumber = (float) m_nNumber;
	m_cs.exit();

	return nNumber;
}

template <class T>
inline Number<T>::operator double()
{
	double nNumber;

	m_cs.enter();
	nNumber = (double) m_nNumber;
	m_cs.exit();

	return nNumber;
}


#endif // !defined(_NUMBER_H__INCLUDED_)
