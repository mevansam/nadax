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

#include "MockHttpService.h"

#include <stdlib.h>
#include <sstream>
#include "ctemplate/template.h"

#include "MessageBusManager.h"
#include "Thread.h"


class MockResponse : public CThread {
public:
	MockResponse() {
	}

	~MockResponse() {
	}

	void initialize(mb::MessagePtr message, int delay) {

		m_message = message;

		m_output = "";
		m_delay = delay;
	}

	std::string* getOutput() {
		return &m_output;
	}

	void reset() {

		m_message.reset();
		m_output = "";
		m_delay = 0;
	}

	void* run() {

		CThread::sleep(m_delay);

		int chunks = ((double) rand() / (double) RAND_MAX) * 5.0;
		int size = m_output.size();

		char* buffer = (char *) m_output.c_str();
		int chunkSize = size;

		std::cout << "Begin: Streaming message [" <<
			m_message << ":" << m_message.use_count() << "] ..." << std::endl;

		mb::StreamMessage* streamMessage = (mb::StreamMessage *) m_message.get();

		for (int i = 0; i < chunks; i++) {

			chunkSize = ((double) rand() / (double) RAND_MAX) * ((double) size / 2.0);
			if (!chunkSize)
				break;

			streamMessage->sendData(m_message, buffer, chunkSize);

			size -= chunkSize;
			buffer += chunkSize;
		}

		streamMessage->sendData(m_message, buffer, size);
		streamMessage->sendData(m_message, NULL, 0);

		std::cout << "End: Streaming message 3 [" <<
			m_message << ":" << m_message.use_count() << "] ..." << std::endl;

		m_message.reset();
		return NULL;
	}

private:
	mb::MessagePtr m_message;
	std::string m_output;

	int m_delay;
};

MockHttpService::MockHttpService() {
	msgType = mb::Message::MSG_P2P;
	response = new MockResponse();
}

MockHttpService::~MockHttpService() {
	delete response;
}

void MockHttpService::intialize() {

	std::cout << "Initializing Mock HTTP Service: " << this->getSubject() << std::endl;

	std::ostringstream mockTemplate;
	this->getMockTemplate(mockTemplate);

	ctemplate::StringToTemplateCache(this->getSubject(), mockTemplate.str(), ctemplate::DO_NOT_STRIP);
}

void MockHttpService::destroy() {

	std::cout << "Destroying Mock HTTP Service: " << this->getSubject();
}

mb::Message* MockHttpService::createMessage() {
	mb::Message* message = new mb::http::HttpMessage(mb::http::HttpMessage::POST);
	mb::Service::initMessage(message, msgType);
	return message;
}

void MockHttpService::onMessage(mb::MessagePtr message) {

	response->stop();

	ctemplate::TemplateDictionary dict("tmplVars");

	std::list<mb::Message::NameValue> tmplVars;
	this->getTmplVars(message, tmplVars);

	for (std::list<mb::Message::NameValue>::iterator i = tmplVars.begin(); i != tmplVars.end(); i++) {
		dict.SetValue(i->name, i->value);
	}

	mb::MessagePtr responseMessage(new mb::StreamMessage());
	mb::Service::initMessage(message.get(), responseMessage.get(), mb::Message::MSG_RESP_STREAM, this->getContentType());

	response->initialize(responseMessage, this->getDelay());

	ctemplate::ExpandTemplate(this->getSubject(), ctemplate::DO_NOT_STRIP, &dict, response->getOutput());

	Listener* callback = (message->getType() == mb::Message::MSG_P2P ? dynamic_cast<Listener *>(message.get()) : NULL);
	bool startResponseThread = (mb::MessageBusManager::instance()->postMessage(responseMessage, callback) > 0);

	if (startResponseThread)
		response->start();
	else
		response->reset();
}
