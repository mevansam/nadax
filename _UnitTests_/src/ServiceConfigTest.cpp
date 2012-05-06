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
#include <fstream>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/regex.hpp>
#include "boost/regex.hpp"

#include "ServiceConfigManager.h"
#include "CurlHttpService.h"

#define XML_PARSE_TEST  "./data/service_config_test.xml"

struct TokenLookup {
        std::string operator()(const boost::match_results<const char*>& match)
        {
        	std::cout << "Match -- " << match[0] << std::endl;
            return std::string("[foo]");
        }
    };

BOOST_AUTO_TEST_CASE( service_config_test ) {

	std::cout << "Begin service config test..." << std::endl;
	mb::ServiceConfigManager::initialize();

	mb::ServiceConfigManager* manager = mb::ServiceConfigManager::instance();

	manager->addToken("LOGIN", "http:://login-test/");
	manager->loadConfigFile(XML_PARSE_TEST);
}
