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

#include <iostream>
#include <cppunit/extensions/HelperMacros.h>

#include "Path.h"

using namespace binding;


class PathTest : public CppUnit::TestFixture {

CPPUNIT_TEST_SUITE(PathTest);
CPPUNIT_TEST(testInitialization);
CPPUNIT_TEST(testEquality);
CPPUNIT_TEST(testPushAndPop);
CPPUNIT_TEST_SUITE_END();

public:
	PathTest();
	virtual ~PathTest();

	void setUp();
	void tearDown();

	void testInitialization();
	void testEquality();
	void testPushAndPop();

private:

	binding::Path path1;
	binding::Path path2;
	binding::Path path3;
	binding::Path path4;
};


//CPPUNIT_TEST_SUITE_REGISTRATION(PathTest);

PathTest::PathTest() :
	path1(),
	path2("/"),
	path3("aa/bbbb/ccc/ddddd"),
	path4("/xxxxx/yy/zzzzzzzz") {

}

PathTest::~PathTest() {
}

void PathTest::setUp() {
}

void PathTest::tearDown() {
}

void PathTest::testInitialization() {

	CPPUNIT_ASSERT(path1.isValid());
	CPPUNIT_ASSERT(path2.isValid());
	CPPUNIT_ASSERT(path3.isValid());
	CPPUNIT_ASSERT(path4.isValid());

	path1.debug("path1");
	CPPUNIT_ASSERT(path1.isEmpty());
	CPPUNIT_ASSERT(0 == path1.length());
	CPPUNIT_ASSERT(strlen(path1.leaf()) == 0);

	path2.debug("path2");
	CPPUNIT_ASSERT(!path2.isEmpty());
	CPPUNIT_ASSERT(0 == path2.length());
	CPPUNIT_ASSERT(strlen(path1.leaf()) == 0);

	path3.debug("path3");
	CPPUNIT_ASSERT(!path3.isEmpty());
	CPPUNIT_ASSERT(4 == path3.length());
	CPPUNIT_ASSERT(strcmp("aa", path3.getPathElement(0)) == 0);
	CPPUNIT_ASSERT(strcmp("bbbb", path3.getPathElement(1)) == 0);
	CPPUNIT_ASSERT(strcmp("ccc", path3.getPathElement(2)) == 0);
	CPPUNIT_ASSERT(strcmp("ddddd", path3.getPathElement(3)) == 0);
	CPPUNIT_ASSERT(strcmp("ddddd", path3.leaf()) == 0);
	CPPUNIT_ASSERT(strcmp("aa/bbbb/ccc/ddddd", (const char *) path3) == 0);

	path4.debug("path4");
	CPPUNIT_ASSERT(!path4.isEmpty());
	CPPUNIT_ASSERT(3 == path4.length());
	CPPUNIT_ASSERT(strcmp("", path4.getPathElement(0)) == 0);
	CPPUNIT_ASSERT(strcmp("xxxxx", path4.getPathElement(1)) == 0);
	CPPUNIT_ASSERT(strcmp("yy", path4.getPathElement(2)) == 0);
	CPPUNIT_ASSERT(strcmp("zzzzzzzz", path4.getPathElement(3)) == 0);
	CPPUNIT_ASSERT(strcmp("zzzzzzzz", path4.leaf()) == 0);
	CPPUNIT_ASSERT(strcmp("/xxxxx/yy/zzzzzzzz", (const char *) path4) == 0);

	Path* path = new Path(path4);
	path->debug("copy of path4");
	CPPUNIT_ASSERT(!path->isEmpty());
	CPPUNIT_ASSERT(path4.length() == path->length());
	CPPUNIT_ASSERT(strcmp(path4.getPathElement(0), path->getPathElement(0)) == 0);
	CPPUNIT_ASSERT(strcmp(path4.getPathElement(1), path->getPathElement(1)) == 0);
	CPPUNIT_ASSERT(strcmp(path4.getPathElement(2), path->getPathElement(2)) == 0);
	CPPUNIT_ASSERT(strcmp(path4.getPathElement(3), path->getPathElement(3)) == 0);
	CPPUNIT_ASSERT(strcmp(path4.leaf(), path->leaf()) == 0);
	delete path;
}

void PathTest::testEquality() {

	Path* path;

	path = new Path("aa/bbbb/ccc/ddddd");
	path->debug("wildcard match with path3");
	CPPUNIT_ASSERT(path3 == *path);
	delete path;

	path = new Path("/aa/bbbb/ccc/ddddd");
	path->debug("wildcard match with path3");
	CPPUNIT_ASSERT(path3 != *path);
	delete path;

	path = new Path("*");
	path->debug("wildcard match with path3");
	CPPUNIT_ASSERT(path3 == *path);
	delete path;

	path = new Path("*/bbbb/ccc/ddddd");
	path->debug("wildcard match with path3");
	CPPUNIT_ASSERT(path3 == *path);
	delete path;

	path = new Path("*/ccc/ddddd");
	path->debug("wildcard match with path3");
	CPPUNIT_ASSERT(path3 == *path);
	delete path;

	path = new Path("aa/bbbb/?/ddddd");
	path->debug("wildcard match with path3");
	CPPUNIT_ASSERT(path3 == *path);
	delete path;

	CPPUNIT_ASSERT(path4 == "/xxxxx/yy/zzzzzzzz");
	CPPUNIT_ASSERT(path4 != "xxxxx/yy/zzzzzzzz");
	CPPUNIT_ASSERT(path4 == "*/yy/zzzzzzzz");
	CPPUNIT_ASSERT(path4 == "*/zzzzzzzz");
	CPPUNIT_ASSERT(path4 == "/xxxxx/?/zzzzzzzz");
	CPPUNIT_ASSERT(path4 != "/xx1xx/?/zzzzzzzz");
	CPPUNIT_ASSERT(path4 != "/xxxxx/?/zzz1zzzz");
	CPPUNIT_ASSERT(path4 != "aa/bbbb/ccc/ddddd");
}

void PathTest::testPushAndPop() {

	Path path5("/xxxxx");
	Path path6("/yy/zzzzzzzz");
	path5.append(path6);
	path5.debug("/yy/zzzzzzzz appended to /xxxxx");
	CPPUNIT_ASSERT(path4 == path5);

	Path path7("aa/bbb/cccc");
	path5.append(path7);
	path5.debug("aa/bbb/cccc appended to /xxxxx/yy/zzzzzzzz");
	CPPUNIT_ASSERT(path5 == "/xxxxx/yy/zzzzzzzz/aa/bbb/cccc");

	CPPUNIT_ASSERT(strcmp(path5.pop(), "cccc") == 0);
	CPPUNIT_ASSERT(strcmp(path5.pop(), "bbb") == 0);
	CPPUNIT_ASSERT(strcmp(path5.pop(), "aa") == 0);
	path5.debug("/xxxxx/yy/zzzzzzzz/aa/bbb/cccc after popping 3x");
	CPPUNIT_ASSERT(path4 == path5);

	path5.tag();
	CPPUNIT_ASSERT(strcmp(path5.push("gggg"), "/xxxxx/yy/zzzzzzzz/gggg") == 0 && path5.isTagged());
	CPPUNIT_ASSERT(strcmp(path5.push("hhhhhh"), "/xxxxx/yy/zzzzzzzz/gggg/hhhhhh") == 0 && path5.isTagged());
	CPPUNIT_ASSERT(strcmp(path5.push("iiiiiiii"), "/xxxxx/yy/zzzzzzzz/gggg/hhhhhh/iiiiiiii") == 0 && path5.isTagged());
	CPPUNIT_ASSERT(strcmp(path5.push("jjj"), "/xxxxx/yy/zzzzzzzz/gggg/hhhhhh/iiiiiiii/jjj") == 0 && path5.isTagged());

	path5.debug("after pushing /gggg/hhhhhh/iiiiiiii/jjj");

	std::cout << std::endl;
	std::cout << "Pop - " << path5.pop() << std::endl;
	std::cout << "Pop - " << path5.pop() << std::endl;
	std::cout << "Pop - " << path5.pop() << std::endl;
	std::cout << "Pop - " << path5.pop() << std::endl;
	std::cout << "Path5 - " << (const char *) path5 << std::endl;
	CPPUNIT_ASSERT(strcmp((const char *) path5, "/xxxxx/yy/zzzzzzzz") == 0 && path5.isTagged());

	std::cout << "Pop - " << path5.pop() << std::endl;
	std::cout << "Path5 - " << (const char *) path5 << std::endl;
	CPPUNIT_ASSERT(strcmp((const char *) path5, "/xxxxx/yy") == 0 && !path5.isTagged());

	CPPUNIT_ASSERT(path5.isValid());
}
