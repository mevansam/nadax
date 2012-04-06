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

#ifndef MOCKHTTPSERVICE_H_
#define MOCKHTTPSERVICE_H_

#include <iostream>
#include <sstream>

#include "HttpService.h"

class MockResponse;

class MockHttpService : public mb::Service {

public:
	MockHttpService();
	virtual ~MockHttpService();

	void intialize();
	void destroy();

	virtual void pause(std::ostream* output = NULL) { };
	virtual void resume(std::istream* input = NULL) { };

	virtual void onMessage(mb::MessagePtr message);

	mb::Message* createMessage();

protected:

	virtual void getMockTemplate(std::ostream& mockTemplate) = 0;

	virtual void getTmplVars(const mb::MessagePtr message, std::list<mb::Message::NameValue>& tmplVars) {
	}
	virtual mb::Message::ContentType getContentType() {
		return mb::Message::CNT_UNKNOWN;
	}
	virtual int getDelay() {
		return 500;
	}

	mb::Message::MessageType msgType;
	MockResponse* response;
};

#endif /* MOCKHTTPSERVICE_H_ */
