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
#include "DynaModel.h"

#define BUFFER_SIZE  1024

#define XML_PARSE_TEST  "./data/xml_parse_test.xml"
#define GENERIC_BINDING_TEST  "./data/generic_binding_test.xml"

using namespace binding;
using namespace parser;


class TestDataBinder : public DataBinder {

public:
	TestDataBinder() {
		DataBinder::addBeginRule("root/nested1/nested2", beginNested2);
		DataBinder::addBeginRule("*/dataC1", beginDataC1);
		DataBinder::addEndRule("*/dataC1", endDataC1);
		DataBinder::addEndRule("root/nested1/nested2", endNested2);
		DataBinder::addEndRule("root/?/blob1", endBlob1);
		DataBinder::addBeginRule("*/dataD1/@d1", beginDataD1);
		DataBinder::addEndRule("*/dataD1/@d1", endDataD1);
		DataBinder::addBeginRule("root/nested3", beginNested3);
		DataBinder::addEndRule("*/nested3/aaaaaa", endAAAAAA);
		DataBinder::addEndRule("*/nested3/bbbbbb", endBBBBBB);
		DataBinder::addEndRule("*/nested3/cccccc", endCCCCCC);
	}

	static void beginNested2(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "nested2") == 0, "Trigger element name is not same as rule leaf.");
		std::cout << "Fire begin nested2: element=" << element << std::endl;

		((TestDataBinder *) binder)->counter = 1;
	}

	static void beginDataC1(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "dataC1") == 0, "Trigger element name is not same as rule leaf.");
		std::cout << "Fire begin dataC1: element=" << element << std::endl;

		int counter1 = ((TestDataBinder *) binder)->counter;
		int counter2 = 1;

		std::map<std::string, std::string>::iterator i;
		for (i = attribs.begin(); i != attribs.end(); i++) {

			const char* value = i->second.c_str();
			BOOST_REQUIRE_MESSAGE(counter1 == (*value - '0') && counter2++ == (*(value + 1) - '0'), "Attribute mismatch." );

			std::cout << "  Attribute: name=" << i->first;
			std::cout << ", value = " << i->second << std::endl;
		}
	}

	static void endDataC1(void* binder, const char *element, const char *body) {

		int counter = ((TestDataBinder *) binder)->counter++;

		BOOST_REQUIRE_MESSAGE(strcmp(element, "dataC1") == 0, "Trigger element name is not same as rule leaf.");
		BOOST_REQUIRE_MESSAGE(counter == (*(body + 3) - '0'), "Body mismatch.");
		std::cout << "Fire end dataC1: element=" << element << ", body : \"" << body << '"' << std::endl;
	}

	static void beginDataD1(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

		std::cout << "Fire begin dataD1/@d1: attribute=" << element << std::endl;
	}

	static void endDataD1(void* binder, const char *element, const char *body) {

		std::cout << "Fire end dataD1/@d1: attribute=" << element << ", body : \"" << body << '"' << std::endl;
	}

	static void endNested2(void* binder, const char *element, const char *body) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "nested2") == 0, "Trigger element name is not same as rule leaf.");

		std::cout << "Fire end nested2: element=" << element << std::endl;
	}

	static void endBlob1(void* binder, const char *element, const char *body) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "blob1") == 0, "Trigger element name is not same as rule leaf.");

		std::cout << "Fire end blob1: " << element << ", body : \"" << body << '"' << std::endl;
	}

	static void beginNested3(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "nested3") == 0, "Trigger element name is not same as rule leaf.");

		std::cout << "Fire begin nested2: element=" << element << std::endl;
	}

	static void endAAAAAA(void* binder, const char *element, const char *body) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "aaaaaa") == 0, "Trigger element name is not same as rule leaf.");

		std::cout << "Fire end aaaaaa: " << element << ", body : \"" << body << '"' << std::endl;
	}

	static void endBBBBBB(void* binder, const char *element, const char *body) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "bbbbbb") == 0, "Trigger element name is not same as rule leaf.");

		std::cout << "Fire end bbbbbb: " << element << ", body : \"" << body << '"' << std::endl;
	}

	static void endCCCCCC(void* binder, const char *element, const char *body) {

		BOOST_REQUIRE_MESSAGE(strcmp(element, "cccccc") == 0, "Trigger element name is not same as rule leaf.");

		std::cout << "Fire end cccccc: " << element << ", body : \"" << body << '"' << std::endl;
	}

private:

	int counter;
};


BOOST_AUTO_TEST_CASE( xml_data_binder ) {

	TestDataBinder dataBinder;
	dataBinder.debug("BasicTest01Binder...");

	XmlBinder xmlBinder(&dataBinder);

	void* buffer;
	buffer = xmlBinder.initialize(BUFFER_SIZE);

	FILE* file = fopen(XML_PARSE_TEST, "r");
	BOOST_REQUIRE_MESSAGE(file != NULL, "Error opening test XML file.");

	size_t len;

	std::cout << std::endl;

	while (!feof(file) && !ferror(file)) {
		len = fread(buffer, 1, BUFFER_SIZE, file);
		BOOST_REQUIRE_MESSAGE(ferror(file) == 0, "Test XML file read error.");
		xmlBinder.parse(len, len == 0);
	}

	fclose(file);
}

BOOST_AUTO_TEST_CASE( xml_generic_data_binding ) {

	// Test Dyna Model Creation and Accessing

	DynaModelNode node;

	node = DynaModel::create();
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
	dynaBinder.addBinding("*/overview/terms/line", DynaModel::LIST, "t");
	dynaBinder.addBinding("*/overview/legal", DynaModel::MAP, "l" );
	dynaBinder.addBinding("*/overview/legal/header", "h" );
	dynaBinder.addBinding("*/overview/legal/body", "b");
	dynaBinder.addBinding("*/overview/legal/footer", "f" );

	dynaBinder.addBinding("*/summary/sumitem", DynaModel::LIST, "summary");
	dynaBinder.addBinding("*/summary/sumitem/@id", "k", true);
	dynaBinder.addBinding("*/summary/sumitem/name", "n");
	dynaBinder.addBinding("*/summary/sumitem/desc", "d");
	dynaBinder.addBinding("*/summary/sumitem/value", "v");

	dynaBinder.addBinding("*/detail1/detailitem", DynaModel::MAP, "di1", "summary");
	dynaBinder.addBinding("*/detail1/detailitem/@id", "k", true);
	dynaBinder.addBinding("*/detail1/detailitem/value1", "v1");
	dynaBinder.addBinding("*/detail1/detailitem/value2", "v2");
	dynaBinder.addBinding("*/detail1/detailitem/y/z", DynaModel::LIST, "y");

	dynaBinder.addBinding("*/detail2/y", DynaModel::MAP, "di2", "summary");
	dynaBinder.addBinding("*/detail2/y/id", "k", true);
	dynaBinder.addBinding("*/detail2/y/x", "v1");

	XmlBinder xmlBinder(&dynaBinder);

	void* buffer;
	buffer = xmlBinder.initialize(BUFFER_SIZE);

	FILE* file = fopen(GENERIC_BINDING_TEST, "r");
	BOOST_REQUIRE_MESSAGE(file != NULL, "Error opening test XML file.");

	size_t len = 0;
	std::cout << std::endl;

	while (!feof(file) && !ferror(file)) {
		len = fread(buffer, 1, BUFFER_SIZE, file);
		BOOST_REQUIRE_MESSAGE(ferror(file) == 0, "Test XML file read error.");
		xmlBinder.parse(len, len == 0);
	}
	if (len)
		xmlBinder.parse(0, true);

	fclose(file);

	node = dynaBinder.getRootPtr();
	std::cout << "Marshalled Dynamic Data Model: " << std::endl << node << std::endl;
}
