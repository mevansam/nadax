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

#ifndef XMLSTREAMPARSER_H_
#define XMLSTREAMPARSER_H_

#include <string>

#include "expat.h"
#include "Unmarshaller.h"
#include "DataBinder.h"

#define EnableElementHandlers               1<<0   // Enable/Disable the element handlers
#define EnableCharacterDataHandler          1<<1   // Enable/Disable the character data handler
#define EnableCdataSectionHandlers          1<<2   // Enable/Disable the CDATA section handlers
#define EnableProcessingInstructionHandler  1<<3   // Enable/Disable the processing instruction handler
#define EnableCommentHandler                1<<4   // Enable/Disable the comment handler
#define EnableNamespaceDeclHandlers         1<<5   // Enable/Disable namespace handlers
#define EnableXmlDeclHandler                1<<6   // Enable/Disable the XML declaration handler
#define EnableDoctypeDeclHandlers           1<<7   // Enable/Disable the DOCTYPE declaration handlers
#define EnableUnknownEncodingHandler        1<<13  // Enable/Disable unknown encoding handler
#define EnableDefaultHandlerExpand          1<<14  // Enable/Disable the default handler with expanded internal entity refs.
#define EnableDefaultHandler                1<<15  // Enable/Disable default handler


namespace parser {

template <class HandlerT>
class XmlStreamParser {

public:
	XmlStreamParser();
	virtual ~XmlStreamParser();

protected:

	// **** Expat parser creation and parsing ****

	bool createParser(const XML_Char* encoding = NULL, const XML_Char* sep = NULL);
    bool resetParser();

	bool parseLocalBuffer(int size, bool isFinal);
	bool parseExternalBuffer(const char* data, int size, bool isFinal);

	void enableHandlers(unsigned int handlers);

	char* getBuffer(int size) {
		return (m_buffer = (char *) malloc(size));
	}

	// **** Expat error handling ****

	// Get last error
	enum XML_Error getErrorCode() {
		return XML_GetErrorCode(m_parser);
	}

	// Get last error string
	const char* getError() {
		return XML_ErrorString(getErrorCode());
	}

	// Get the current byte index
	long getCurrentByteIndex() {
		return XML_GetCurrentByteIndex(m_parser);
	}

	// Get the current line number
	int getCurrentLineNumber() {
		return XML_GetCurrentLineNumber(m_parser);
	}

	// Get the current column number
	int getCurrentColumnNumber() {
		return XML_GetCurrentColumnNumber(m_parser);
	}

	// Get the current byte count
	int getCurrentByteCount() {
		return XML_GetCurrentByteCount(m_parser);
	}

	// Get the input context
	const char* getInputContext (int* offset, int* size) {
		return XML_GetInputContext(m_parser, offset, size);
	}

	// **** BEGIN: Parsing Expat and corresponding Binder Callbacks ****

	static void XMLCALL StartElementHandler(void* ctx, const XML_Char* name, const XML_Char** attribs) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->startElement(name, attribs);
	}
	static void XMLCALL EndElementHandler(void* ctx, const XML_Char* name) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->endElement(name);
	}
	virtual void startElement(const XML_Char* name, const XML_Char** attribs) { }
	virtual void endElement(const XML_Char* name) { }

	static void XMLCALL CharacterDataHandler(void* ctx, const XML_Char* text, int len) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->characters(text, len);
	}
	virtual void characters(const XML_Char* text, int len) { }

	static void XMLCALL StartCdataSectionHandler(void* ctx) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->startCDataSection();
	}
	static void XMLCALL EndCdataSectionHandler(void* ctx) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->endCDataSection();
	}
	virtual void startCDataSection() { }
	virtual void endCDataSection() { }

	static void XMLCALL ProcessingInstructionHandler(void* ctx, const XML_Char* target, const XML_Char* data) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->processingInstruction(target, data);
	}
	static void XMLCALL CommentHandler(void* ctx, const XML_Char *comment) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->comment(comment);
	}
	virtual void processingInstruction(const XML_Char* target, const XML_Char* data) { }
	virtual void comment(const XML_Char* comment) { }

	static void XMLCALL StartNamespaceDeclHandler(void* ctx, const XML_Char* prefix, const XML_Char* uri) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->startNamespaceDecl(prefix, uri);
	}
	static void XMLCALL EndNamespaceDeclHandler(void* ctx, const XML_Char* prefix) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->endNamespaceDecl(prefix);
	}
	virtual void startNamespaceDecl(const XML_Char* prefix, const XML_Char* uri) { }
	virtual void endNamespaceDecl (const XML_Char* prefix) { }

	static void XMLCALL XmlDeclHandler (void* ctx, const XML_Char* version, const XML_Char* encoding, int isStandAlone) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->xmlDecl(version, encoding, isStandAlone != 0);
	}
	virtual void xmlDecl(const XML_Char* version, const XML_Char* encoding, bool isStandAlone) { }

	static void XMLCALL StartDoctypeDeclHandler ( void* ctx, const XML_Char* name,
		const XML_Char* systemId, const XML_Char* publicId, int hasInternalSubset ) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->startDoctypeDecl(name, systemId, publicId, hasInternalSubset != 0);
	}
	static void XMLCALL EndDoctypeDeclHandler (void* ctx) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->endDoctypeDecl();
	}
	virtual void startDoctypeDecl( const XML_Char* name,
		const XML_Char* systemId, const XML_Char* publicId, bool hasInternalSubset ) { }
	virtual void endDoctypeDecl() { }

	static int XMLCALL UnknownEncodingHandler(void* ctx, const XML_Char* name, XML_Encoding* encoding) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		return binder->unknownEncoding(name, encoding) ? 1 : 0;
	}
	virtual bool unknownEncoding(const XML_Char* name, XML_Encoding* encoding) { return false; }

	static void XMLCALL DefaultHandler (void* ctx, const XML_Char* data, int length) {
		HandlerT *binder = static_cast<HandlerT *>((XmlStreamParser<HandlerT> *) ctx);
		binder->handleDefault(data, length);
	}
	virtual void handleDefault(const XML_Char* data, int length) { }

	// **** The parser instance ****

	XML_Parser getParser() {
		return m_parser;
	}

private:

	XML_Parser m_parser;
    XML_Char* m_charEncoding;

    unsigned int m_activeHandlers;
    
	char* m_buffer;
};


template <class HandlerT>
XmlStreamParser<HandlerT>::XmlStreamParser() {
    m_parser = NULL;
    m_charEncoding = NULL;
	m_buffer = NULL;
}

template <class HandlerT>
XmlStreamParser<HandlerT>::~XmlStreamParser() {

	if (m_parser)
		XML_ParserFree(m_parser);
    if (m_charEncoding)
        free(m_charEncoding);
	if (m_buffer)
		free(m_buffer);
}

template <class HandlerT>
bool XmlStreamParser<HandlerT>::createParser(const XML_Char* encoding, const XML_Char *sep) {

    if (encoding == NULL || encoding[0] == 0)
        m_charEncoding = NULL;
    else
        m_charEncoding = strdup(encoding);
    
    if (sep != NULL && sep[0] == 0)
        sep = NULL;

	m_parser = XML_ParserCreate_MM(m_charEncoding, NULL, sep);
	if (!m_parser)
		return false;

	XML_SetUserData(m_parser, (void *) this);

	return true;
}

template <class HandlerT>
bool XmlStreamParser<HandlerT>::resetParser() {
    
    if (XML_ParserReset(m_parser, m_charEncoding)) {
        
        XML_SetUserData(m_parser, (void *) this);
        enableHandlers(m_activeHandlers);
        
        return true;
    }
    
    return false;
}

template <class HandlerT>
bool XmlStreamParser<HandlerT>::parseLocalBuffer(int size, bool isFinal) {

	return XML_Parse(m_parser, m_buffer, size, isFinal);
}

template <class HandlerT>
bool XmlStreamParser<HandlerT>::parseExternalBuffer(const char* data, int size, bool isFinal) {

	return XML_Parse(m_parser, data, size, isFinal);
}

template <class HandlerT>
void XmlStreamParser<HandlerT>::enableHandlers(unsigned int handlers) {
    
    m_activeHandlers = handlers;

	if (handlers & EnableElementHandlers) {
		XML_SetElementHandler(m_parser, StartElementHandler, EndElementHandler);
	} else {
		XML_SetElementHandler(m_parser, NULL, NULL);
	}
	if (handlers & EnableCharacterDataHandler) {
		XML_SetCharacterDataHandler(m_parser, CharacterDataHandler);
	} else {
		XML_SetCharacterDataHandler(m_parser, NULL);
	}
	if (handlers & EnableCdataSectionHandlers) {
		XML_SetCdataSectionHandler(m_parser, StartCdataSectionHandler, EndCdataSectionHandler);
	} else {
		XML_SetCdataSectionHandler(m_parser, NULL, NULL);
	}
	if (handlers & EnableProcessingInstructionHandler) {
		XML_SetProcessingInstructionHandler(m_parser, ProcessingInstructionHandler);
	} else {
		XML_SetProcessingInstructionHandler(m_parser, NULL);
	}
	if (handlers & EnableCommentHandler) {
		XML_SetCommentHandler(m_parser, CommentHandler);
	} else {
		XML_SetCommentHandler(m_parser, NULL);
	}
	if (handlers & EnableNamespaceDeclHandlers) {
		XML_SetNamespaceDeclHandler(m_parser, StartNamespaceDeclHandler, EndNamespaceDeclHandler);
	} else {
		XML_SetNamespaceDeclHandler(m_parser, NULL, NULL);
	}
	if (handlers & EnableXmlDeclHandler) {
		XML_SetXmlDeclHandler(m_parser, XmlDeclHandler);
	} else {
		XML_SetXmlDeclHandler(m_parser, NULL);
	}
	if (handlers & EnableDoctypeDeclHandlers) {
		XML_SetDoctypeDeclHandler(m_parser, StartDoctypeDeclHandler, EndDoctypeDeclHandler);
	} else {
		XML_SetDoctypeDeclHandler(m_parser, NULL, NULL);
	}
	if (handlers & EnableUnknownEncodingHandler) {
		XML_SetUnknownEncodingHandler(m_parser, UnknownEncodingHandler, (void *) this);
	} else {
		XML_SetUnknownEncodingHandler(m_parser, NULL, NULL);
	}
	if (handlers & EnableDefaultHandlerExpand) {
		XML_SetDefaultHandlerExpand(m_parser, DefaultHandler);
	} else {
		XML_SetDefaultHandlerExpand(m_parser, NULL);
	}
	if (handlers & EnableDefaultHandler) {
		XML_SetDefaultHandler(m_parser, DefaultHandler);
	} else {
		XML_SetDefaultHandler(m_parser, NULL);
	}
}


class XmlBinder : public binding::Unmarshaller, public XmlStreamParser<XmlBinder> {
    
public:
    XmlBinder(binding::DataBinder* binder);
    XmlBinder(binding::DataBinderPtr binder);
    virtual ~XmlBinder();
    
    void* initialize(int size = -1);
    void reset();
    
    void parse(int size, bool isFinal);
    void parse(const char* buffer, int size, bool isFinal);

    void* getResult() {
    	return m_binder->detachRoot();
    }

    virtual void startElement(const XML_Char* name, const XML_Char** attribs) {
        m_binder->startElement(name, attribs);
    }
    virtual void endElement(const XML_Char* name) {
        m_binder->endElement(name);
    }
    virtual void characters(const XML_Char* text, int len) {
        m_binder->characters(text, len);
    }
    
    virtual void startCDataSection() {
        m_binder->startCDataSection();
    }
    virtual void endCDataSection() {
        m_binder->endCDataSection();
    }
    
private:
    
    const char* getParsingErrorMessage();
    
    binding::DataBinderPtr m_binderPtr;
    binding::DataBinder* m_binder;
    
    std::string m_errorMessage;

    bool m_binding;
};

}

#endif /* XMLSTREAMPARSER_H_ */
