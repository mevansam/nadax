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

#include "XmlStreamParser.h"

#include <iostream>
#include <sstream>

#include "exception.h"

// Exception Messages
#define PARSER_CREATION_ERROR  "Unable to allocate enough memory to create a new parser"
#define PARSER_RESET_ERROR  "Unable to reset parser for reuse"


namespace parser {


class XmlParsingException : public CException {

public:
	XmlParsingException(const char* source, int lineNumber) : CException(source, lineNumber) { }
    virtual ~XmlParsingException() { }
};

XmlBinder::XmlBinder(binding::DataBinder* binder) {
    
    m_binder = binder;
    m_binding = false;
}

XmlBinder::XmlBinder(binding::DataBinderPtr binder) {

    m_binderPtr = binder;
	m_binder = m_binderPtr.get();
	m_binding = false;
}

XmlBinder::~XmlBinder() {
}

void* XmlBinder::initialize(int size) {

	void* buffer = NULL;

	if (!createParser()) {
		THROW(XmlParsingException, EXCEP_MSSG(PARSER_CREATION_ERROR));
    }
    
	if (size > 0)
		buffer = getBuffer(size);

	enableHandlers(
		EnableElementHandlers|
		EnableCharacterDataHandler|
		EnableCdataSectionHandlers );

	return buffer;
}
    
void XmlBinder::reset() {
    
	m_binder->reset();
	m_binding = false;

    if (!resetParser()) {
        THROW(XmlParsingException, EXCEP_MSSG(PARSER_RESET_ERROR));
    }
}

void XmlBinder::parse(int size, bool isFinal) {

	if (!m_binding) {
		m_binder->beginBinding();
		m_binding = true;
	}

	if (!parseLocalBuffer(size, isFinal)) {
		THROW(XmlParsingException, EXCEP_MSSG(getParsingErrorMessage()));
	}

	if (isFinal) {
		m_binder->endBinding();
		m_binding = false;
	}
}

void XmlBinder::parse(const char* data, int size, bool isFinal) {

	if (!m_binding) {
		m_binder->beginBinding();
		m_binding = true;
	}

	if (!parseExternalBuffer(data, size, isFinal)) {
		THROW(XmlParsingException, EXCEP_MSSG(getParsingErrorMessage()));
	}

	if (isFinal) {
		m_binder->endBinding();
		m_binding = false;
	}
}

const char* XmlBinder::getParsingErrorMessage() {

	std::ostringstream oss;

	oss << "Parsing error at line " << getCurrentLineNumber()
		<< " and column " <<  getCurrentColumnNumber()
		<< " : " << getError();

	m_errorMessage = oss.str();
	return m_errorMessage.c_str();
}


}  // namespace : parser
