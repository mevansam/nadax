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
#include <cppunit/extensions/HelperMacros.h>

#include "XmlStreamParser.h"
#include "DynaModel.h"

#define BUFFER_SIZE  1024
#define WHITESPACE   "\t\r\n "

#define XML_PARSE_TEST01  "./data/XmlParseTest01.xml"
#define XML_PARSE_TEST02  "./data/XmlParseTest02.xml"
#define XML_PARSE_TEST03  "./data/dynabindingtest.xml"

using namespace binding;
using namespace parser;


class XmlBindingTest : public CppUnit::TestFixture {

CPPUNIT_TEST_SUITE(XmlBindingTest);
//CPPUNIT_TEST(testXmlStreamParser);
//CPPUNIT_TEST(testXmlBinding);
CPPUNIT_TEST(testGenericBinding);
CPPUNIT_TEST_SUITE_END();

public:
	XmlBindingTest();
	virtual ~XmlBindingTest();

	void setUp();
	void tearDown();

	void testXmlStreamParser();
	void testXmlBinding();
	void testGenericBinding();
};


CPPUNIT_TEST_SUITE_REGISTRATION(XmlBindingTest);

XmlBindingTest::XmlBindingTest() {
}

XmlBindingTest::~XmlBindingTest() {
}

void XmlBindingTest::setUp() {
}

void XmlBindingTest::tearDown() {
}


class XmlEchoParser : public XmlStreamParser<XmlBinder> {

public:

	XmlEchoParser() {
		indent = 0;
	}

	void* initialize(int size) {

		CPPUNIT_ASSERT_MESSAGE("Parser creation failed", createParser(NULL, "|"));

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

			CPPUNIT_ASSERT_MESSAGE(oss.str().c_str(), success);
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

void XmlBindingTest::testXmlStreamParser() {

	XmlEchoParser xmlEchoParser;

	void* buffer;
	buffer = xmlEchoParser.initialize(BUFFER_SIZE);

	FILE* file = fopen(XML_PARSE_TEST02, "r");
	CPPUNIT_ASSERT_MESSAGE("Error opening test XML file.", file != NULL);

	size_t len;

	while (!feof(file) && !ferror(file)) {
		len = fread(buffer, 1, BUFFER_SIZE, file);

		std::cout << std::endl << std::endl << "**** Begin File Data: " << len << std::endl;
		std::cout << std::string((char *) buffer, len) << std::endl;
		std::cout << "**** End File Data" << std::endl;

		CPPUNIT_ASSERT_MESSAGE("Test XML file read error.", ferror(file) == 0);

		std::cout << std::endl << std::endl << "**** Begin Parsing Events" << std::endl;
		xmlEchoParser.parse(len);
		std::cout << "**** End Parsing Events" << std::endl;
	}

	fclose(file);
}


class BasicTest01Binder : public DataBinder {

public:
	BasicTest01Binder() {
		DataBinder::addBeginRule("root/nested1/nested2", beginNested2);
		DataBinder::addBeginRule("*/dataC1", beginDataC1);
		DataBinder::addEndRule("*/dataC1", endDataC1);
		DataBinder::addEndRule("root/nested1/nested2", endNested2);
		DataBinder::addEndRule("root/?/blob1", endBlob1);
		DataBinder::addBeginRule("root/nested3", beginNested3);
		DataBinder::addEndRule("*/nested3/aaaaaa", endAAAAAA);
		DataBinder::addEndRule("*/nested3/bbbbbb", endBBBBBB);
		DataBinder::addEndRule("*/nested3/cccccc", endCCCCCC);
	}

	static void beginNested2(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "nested2") == 0);
		std::cout << "Fire begin nested2: element=" << element << std::endl;

		((BasicTest01Binder *) binder)->counter = 1;
	}

	static void beginDataC1(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "dataC1") == 0);
		std::cout << "Fire end dataC1: element=" << element << std::endl;

		int counter1 = ((BasicTest01Binder *) binder)->counter;
		int counter2 = 1;

		std::map<std::string, std::string>::iterator i;
		for (i = attribs.begin(); i != attribs.end(); i++) {

			const char* value = i->second.c_str();
			CPPUNIT_ASSERT_MESSAGE( "Attribute mismatch.",
				counter1 == (*value - '0') && counter2++ == (*(value + 1) - '0') );

			std::cout << "  Attribute: name=" << i->first;
			std::cout << ", value = " << i->second << std::endl;
		}
	}

	static void endDataC1(void* binder, const char *element, const char *body) {

		int counter = ((BasicTest01Binder *) binder)->counter++;

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "dataC1") == 0);
		CPPUNIT_ASSERT_MESSAGE("Body mismatch.", counter == (*(body + 3) - '0'));
		std::cout << "Fire end dataC1: element=" << element << ", body : \"" << body << '"' << std::endl;
	}

	static void endNested2(void* binder, const char *element, const char *body) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "nested2") == 0);

		std::cout << "Fire end nested2: element=" << element << std::endl;
	}

	static void endBlob1(void* binder, const char *element, const char *body) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "blob1") == 0);

		std::cout << "Fire end blob1: " << element << ", body : \"" << body << '"' << std::endl;
	}

	static void beginNested3(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "nested3") == 0);

		std::cout << "Fire begin nested2: element=" << element << std::endl;
	}

	static void endAAAAAA(void* binder, const char *element, const char *body) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "aaaaaa") == 0);

		std::cout << "Fire end aaaaaa: " << element << ", body : \"" << body << '"' << std::endl;
	}

	static void endBBBBBB(void* binder, const char *element, const char *body) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "bbbbbb") == 0);

		std::cout << "Fire end bbbbbb: " << element << ", body : \"" << body << '"' << std::endl;
	}

	static void endCCCCCC(void* binder, const char *element, const char *body) {

		CPPUNIT_ASSERT_MESSAGE("Trigger element name is not same as rule leaf.", strcmp(element, "cccccc") == 0);

		std::cout << "Fire end cccccc: " << element << ", body : \"" << body << '"' << std::endl;
	}

private:

	int counter;
};

void XmlBindingTest::testXmlBinding() {

	BasicTest01Binder dataBinder;
	dataBinder.debug("BasicTest01Binder...");

	XmlBinder xmlBinder(&dataBinder);

	void* buffer;
	buffer = xmlBinder.initialize(BUFFER_SIZE);

	FILE* file = fopen(XML_PARSE_TEST01, "r");
	CPPUNIT_ASSERT_MESSAGE("Error opening test XML file.", file != NULL);

	size_t len;

	std::cout << std::endl;

	while (!feof(file) && !ferror(file)) {
		len = fread(buffer, 1, BUFFER_SIZE, file);
		CPPUNIT_ASSERT_MESSAGE("Test XML file read error.", ferror(file) == 0);
		xmlBinder.parse(len, len == 0);
	}

	fclose(file);
}

void XmlBindingTest::testGenericBinding() {

	// Test Dyna Model Creation and Accessing

	DynaModelNode node = DynaModel::create();

	DynaModelNode test1 = node->add("test1");
	DynaModelNode test2 = node->add("test2", DynaModel::LIST);

	DynaModelNode test11A = test1->add("test11", DynaModel::LIST);
	DynaModelNode test11Ai = test11A->add();
	test11Ai->setValue("aa1", "111");
	test11Ai->setValue("bb1", "122");
	test11Ai->setValue("cc1", "133");
	test11Ai = test11A->add();
	test11Ai->setValue("aa2", "211");
	test11Ai->setValue("bb2", "222");
	test11Ai->setValue("cc2", "233");

	test2->addValue("test2_0");
	test2->addValue("test2_1");
	test2->addValue("test2_2");
	test2->addValue("test2_3");

	std::cout << "Test Data: " << std::endl << node << std::endl;

	// Test Dyna Model Binding

	DynaModelBinder dynaBinder;

	dynaBinder.addBinding("*/overview/intro", DynaModel::VALUE, "i");
	dynaBinder.addBinding("*/overview/terms", DynaModel::LIST, "t" );
	dynaBinder.addBinding("*/overview/terms/line");
	dynaBinder.addBinding("*/overview/legal", DynaModel::MAP, "l" );
	dynaBinder.addBinding("*/overview/legal/header", "h" );
	dynaBinder.addBinding("*/overview/legal/body", "b");
	dynaBinder.addBinding("*/overview/legal/footer", "f" );

	dynaBinder.addBinding("*/summary/sumitem", DynaModel::LIST, "summary");
	dynaBinder.addBinding("*/summary/sumitem/@id", "k", true);
	dynaBinder.addBinding("*/summary/sumitem/name", "n");
	dynaBinder.addBinding("*/summary/sumitem/desc", "d");
	dynaBinder.addBinding("*/summary/sumitem/value", "v");

	dynaBinder.addBinding("*/detail1/detailitem", DynaModel::MAP, "di", "summary");
	dynaBinder.addBinding("*/detail1/detailitem/@id", "k", true);
	dynaBinder.addBinding("*/detail1/detailitem/value1", "v1");
	dynaBinder.addBinding("*/detail1/detailitem/value2", "v2");
	dynaBinder.addBinding("*/detail1/detailitem/y", DynaModel::LIST, "y");
	dynaBinder.addBinding("*/detail1/detailitem/y/z");

	XmlBinder xmlBinder(&dynaBinder);

	void* buffer;
	buffer = xmlBinder.initialize(BUFFER_SIZE);

	FILE* file = fopen(XML_PARSE_TEST03, "r");
	CPPUNIT_ASSERT_MESSAGE("Error opening test XML file.", file != NULL);

	size_t len = 0;
	std::cout << std::endl;

	while (!feof(file) && !ferror(file)) {
		len = fread(buffer, 1, BUFFER_SIZE, file);
		CPPUNIT_ASSERT_MESSAGE("Test XML file read error.", ferror(file) == 0);
		xmlBinder.parse(len, len == 0);
	}
	if (len)
		xmlBinder.parse(0, true);

	fclose(file);

	node = dynaBinder.getRootPtr();
	std::cout << "Marshalled Dynamic Data Model: " << std::endl << node << std::endl;
}
