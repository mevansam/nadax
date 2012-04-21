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

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>

#include "XmlStreamParser.h"

#define BUFFER_SIZE  1024
#define WHITESPACE   "\t\r\n "

#define XML_PARSE_TEST  "./data/xml_parse_test.xml"

using namespace binding;
using namespace parser;


class TestXmlStreamParser : public XmlStreamParser<TestXmlStreamParser> {

public:

	TestXmlStreamParser() {
		indent = 0;
	}

	void* initialize(int size) {

		BOOST_REQUIRE_MESSAGE(createParser(NULL, "|"), "Parser creation failed");

		enableHandlers(
			EnableNamespaceDeclHandlers|
			EnableElementHandlers|
			EnableCharacterDataHandler );

		return getBuffer(size);
	}

	void parse(int len) {
		bool success = parseLocalBuffer(len, len == 0);
		if (!success) {

			std::ostringstream oss;

			oss << "Parsing error at line " << getCurrentLineNumber()
				<< " and column " <<  getCurrentColumnNumber()
				<< " : " << getError();

			BOOST_REQUIRE_MESSAGE(success, oss.str().c_str());
		}
	}

	void startNamespaceDecl(const XML_Char* prefix, const XML_Char* uri) {
		std::cout << std::setfill(' ') << std::setw(indent) << "";
		if (prefix)
			std::cout << "begin namespace : " << uri << " for prefix " << prefix << std::endl;
		else
			std::cout << "begin namespace : " << uri << std::endl;
	}
	void endNamespaceDecl (const XML_Char* prefix) {
		std::cout << std::setfill(' ') << std::setw(indent) << "";
		if (prefix)
			std::cout << "end namespace : prefix " << prefix << std::endl;
		else
			std::cout << "end namespace" << std::endl;
	}

	void startElement(const XML_Char* name, const XML_Char** attribs) {
		std::cout << std::setfill(' ') << std::setw(indent) << "";
		std::cout << "begin : " << name << std::endl;
		indent += 4;
	}

	void endElement(const XML_Char* name) {
		indent -= 4;
		std::cout << std::setfill(' ') << std::setw(indent) << "";
		std::cout << "end : " << name << std::endl;
	}

	void characters(const XML_Char* text, int len) {

		std::string str(text, len);

		size_t left = str.find_first_not_of(WHITESPACE);
		size_t right = str.find_last_not_of(WHITESPACE);

		if (left != std::string::npos) {
			std::cout << std::setfill(' ') << std::setw(indent) << "";
			std::cout << "body : " << str.substr(left, right - left + 1) << std::endl;
		}
	}

private:

	int indent;
};

BOOST_AUTO_TEST_CASE( test_xml_stream_parser ) {

	TestXmlStreamParser xmlEchoParser;

	void* buffer;
	buffer = xmlEchoParser.initialize(BUFFER_SIZE);

	FILE* file = fopen(XML_PARSE_TEST, "r");
	BOOST_REQUIRE_MESSAGE(file != NULL, "Error opening test XML file.");

	size_t len;

	while (!feof(file) && !ferror(file)) {
		len = fread(buffer, 1, BUFFER_SIZE, file);

		std::cout << std::endl << std::endl << "**** Begin File Data: " << len << std::endl;
		std::cout << std::string((char *) buffer, len) << std::endl;
		std::cout << "**** End File Data" << std::endl;

		BOOST_REQUIRE_MESSAGE(ferror(file) == 0, "Test XML file read error.");

		std::cout << std::endl << std::endl << "**** Begin Parsing Events" << std::endl;
		xmlEchoParser.parse(len);
		std::cout << "**** End Parsing Events" << std::endl;
	}

	fclose(file);
}
