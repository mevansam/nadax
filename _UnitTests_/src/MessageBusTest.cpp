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
#include <stdlib.h>
#include <iostream>

#include <cppunit/extensions/HelperMacros.h>

#include "Semaphore.h"
#include "Thread.h"
#include "MessageBusManager.h"
#include "MockHttpService.h"

#define BUFFER_SIZE  1024

#define ALERTS_SIGNON_RESP_FILE  "./data/alertssignon.xml"


class MessageBusManagerTest : public CppUnit::TestFixture {

CPPUNIT_TEST_SUITE(MessageBusManagerTest);
CPPUNIT_TEST(testServiceRequestResponse);
CPPUNIT_TEST(testAsyncTimedMessages);
CPPUNIT_TEST(testServiceBindings);
CPPUNIT_TEST_SUITE_END();

public:
	MessageBusManagerTest();
	virtual ~MessageBusManagerTest();

	void setUp();
	void tearDown();

	void testServiceRequestResponse();
	void testAsyncTimedMessages();
	void testServiceBindings();

	std::ostringstream testService1Reply;
	CSemaphore testService1ReplyDone;
};

CPPUNIT_TEST_SUITE_REGISTRATION(MessageBusManagerTest);

long getCurrentTimeInMillis() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec * 1000 + time.tv_usec / 1000);
}

// **** Test service that takes an input and doubles it and maintains a count of the number times service is called ****

class IncCtrService : public MockHttpService {

public:
	IncCtrService( const char* subject = "TestService1", int delay = 1000,
		mb::Message::MessageType msgType = mb::Message::MSG_P2P) {

		m_subject = subject;
		m_delay = delay;
		m_counter = 0;
		MockHttpService::msgType = msgType;
	}
	~IncCtrService() {
	}
	const char* getSubject() {
		return m_subject.c_str();
	}
	void getMockTemplate(std::ostream& mockTemplate) {
		mockTemplate << "{{CNTR}},{{IN1}}";
	}
	void getTmplVars(const mb::MessagePtr message, std::list<mb::Message::NameValue>& tmplVars) {

		char value[15];
		sprintf(value, "%d", m_counter++);

		tmplVars.push_back(mb::Message::NameValue("CNTR", value));

		mb::http::HttpMessage* httpMessage = (mb::http::HttpMessage *) message.get();
		std::list<mb::http::HttpMessage::NameValue> params = httpMessage->getParams();

		for (std::list<mb::http::HttpMessage::NameValue>::iterator param = params.begin(); param != params.end(); param++) {

			sprintf(value, "%d", atoi(param->value.c_str()) * 2);
			tmplVars.push_back(mb::Message::NameValue(param->name.c_str(), value));
		}
	}
	virtual void onMessage(mb::MessagePtr message) {

		if ( message->getType() == mb::Message::MSG_P2P_SUB &&
			((mb::P2PMessage *) message.get())->getControlAction() != mb::P2PMessage::NONE )
			return;

		MockHttpService::onMessage(message);
	}
	int getDelay() {
		return m_delay;
	}

private:
	std::string m_subject;
	int m_counter;
	int m_delay;
};


// **** Test service that reads and XML file and returns it as the response ****

class FileRespService : public MockHttpService {

public:
	FileRespService(const char* filePath) {
		m_filePath = filePath;
	}
	~FileRespService() {
	}
	const char* getSubject() {
		return "TestService2";
	}
	void getMockTemplate(std::ostream& mockTemplate) {

		size_t len;

		FILE* file = fopen(m_filePath.c_str(), "r");
		CPPUNIT_ASSERT_MESSAGE("Error opening test data message file.", file != NULL);

		char buffer[BUFFER_SIZE];

		while (!feof(file) && !ferror(file)) {
			len = fread(buffer, 1, BUFFER_SIZE, file);
			CPPUNIT_ASSERT_MESSAGE("Test data file read error.", ferror(file) == 0);
			mockTemplate.write(buffer, len);
		}

		fclose(file);
	}
	virtual mb::Message::ContentType getContentType() {
		return mb::Message::CNT_XML;
	}
	int getDelay() {
		return 500;
	}

private:
	std::string m_filePath;
};


// **** Test Listeners 1 - listens for a servce response and captures its streamed data in a local instance ****

class TestListener1 : public mb::Listener {

public:
	TestListener1() {
		start = getCurrentTimeInMillis();
	}
	void onMessage(mb::MessagePtr message) {
		((mb::StreamMessage *) message.get())->setCallback(this, readStream);
	}
	void waitUntilDone() {
		done.p();
	}
	static bool readStream(void* context, mb::MessagePtr message, void* buffer, size_t size);

	std::ostringstream reply;

	long start;
	long timestamp;

	CSemaphore done;
};

bool TestListener1::readStream(void* context, mb::MessagePtr message, void* buffer, size_t size) {

	if (buffer) {
		((TestListener1 *) context)->reply.write((char *) buffer, size);
	} else {

		TestListener1* listener = (TestListener1 *) context;
		listener->timestamp = getCurrentTimeInMillis();

		std::cout << "TestListener1 [" << listener <<
			"] is done reading response of message with subject " << message->getSubject() <<
			" at " << (long) (listener->timestamp - listener->start) << " ms from creation." << std::endl;

		listener->reply << ';';
		listener->done.v();
	}

	return true;
}


// **** Test Listener 2 - listens for a bound instance of SubscriptionStatus instance ****

class TestListener2 : public mb::Listener {

public:
	void onMessage(mb::MessagePtr message) {
//		status = mb::Datum<alerts::SubscriptionStatus>(message).getDataPtr();
		done.v();
	}
	void waitUntilDone() {
		done.p();
	}

//	boost::shared_ptr<alerts::SubscriptionStatus> status;
	CSemaphore done;
};

// **** Stream Update Callbacks - writes stream to a member in Test Case instance ****

bool handleTestService1ResponseStream(void* context, mb::MessagePtr message, void* buffer, size_t size) {

	if (buffer)
		((MessageBusManagerTest *) context)->testService1Reply.write((char *) buffer, size);
	else
		((MessageBusManagerTest *) context)->testService1ReplyDone.v();

	return true;
}

void handleTestService1Reply(void* context, mb::MessagePtr message) {

	((mb::StreamMessage *) message.get())->setCallback(context, handleTestService1ResponseStream);
}


MessageBusManagerTest::MessageBusManagerTest() {
}

MessageBusManagerTest::~MessageBusManagerTest() {
}

void MessageBusManagerTest::setUp() {
	mb::MessageBusManager::initialize();
}

void MessageBusManagerTest::tearDown() {
	mb::MessageBusManager::destroy();
}

void MessageBusManagerTest::testServiceRequestResponse() {

	mb::MessageBusManager* manager = mb::MessageBusManager::instance();

	mb::MessagePtr message;
	mb::http::HttpMessage* request;

	mb::MessagePtr response;

	TestListener1 testListener1A, testListener1B;
	TestListener2 testListener2A, testListener2B;

	IncCtrService testService1;
	FileRespService testService2(ALERTS_SIGNON_RESP_FILE);

	manager->registerService(&testService1);
	manager->registerService(&testService2);

	manager->registerListener("TestService2", &testListener2A);
	manager->registerListener("TestService2", &testListener2B);

	manager->debug("Message Bus after services and listeners for TestService2 have been initialized:");

	// Synchronous send/recv on TestService1

	message = manager->createMessage("TestService1");

	CPPUNIT_ASSERT_MESSAGE("Http message has incorrect type.", message->getType() == mb::Message::MSG_P2P);
	CPPUNIT_ASSERT_MESSAGE("Http message has incorrect subject.", message->getSubject() == "TestService1");

	message->setData("IN1", "20");

 	response = manager->sendMessage(message);

	std::cout << "TestService1 sync send/recv response : " << (char *) response->getData() << std::endl;
	CPPUNIT_ASSERT_MESSAGE("TestService1 response does not match.", strcmp((char *) response->getData(), "0,40") == 0);

	// Async send send/recv with multicast on TestService1

	manager->registerListener("TestService1", &testListener1A);
	manager->registerListener("TestService1", &testListener1B);

	manager->debug("Message Bus after listeners for TestService1 have been added:");

	message = manager->createMessage("TestService1");
	request = (mb::http::HttpMessage *) message.get();;

	request->setParam("IN1", "10");
	request->setCallback(this, handleTestService1Reply);

	if (manager->postMessage(message)) {

		testService1ReplyDone.p();
		std::cout << "TestService1 async send recv with multicast response : " << testService1Reply.str() << std::endl;

		CPPUNIT_ASSERT_MESSAGE("TestService1 response does not match.", testService1Reply.str() == "1,20");

		testListener1A.waitUntilDone();
		testListener1B.waitUntilDone();

		std::cout << "TestListener 1A received response : " << testListener1A.reply.str() << std::endl;
		std::cout << "TestListener 1B received response : " << testListener1B.reply.str() << std::endl;

		CPPUNIT_ASSERT_MESSAGE(
			"TestListener1's response does not match service response.",
			testListener1A.reply.str() == "1,20;" );
		CPPUNIT_ASSERT_MESSAGE(
			"TestListener2's response does not match service response.",
			testListener1B.reply.str() == "1,20;" );

	} else
		CPPUNIT_FAIL("TestService1 was not registered in the MessageBusManager.");

	// Synchronous send/recv with binding on TestService2 and publishing to multiple listeners

//	{
//		std::cout << std::endl << "Running Synchronous send/recv with binding on TestService2." << std::endl;
//
//		alerts::AlertsSignOnBinder binder;
//		alerts::SubscriptionStatus* status = new alerts::SubscriptionStatus();
//		binder.setRoot(status);
//
//		message = manager->createMessage("TestService2");
//		CPPUNIT_ASSERT_MESSAGE("Http message has incorrect type.", message->getType() == mb::Message::MSG_P2P);
//		CPPUNIT_ASSERT_MESSAGE("Http message has incorrect subject.", message->getSubject() == "TestService2");
//
//		message->setDataBinder(&binder);
//		response = manager->sendMessage(message);
//		std::cout << "Bound status data: " << *status << std::endl;
//
//		boost::shared_ptr<alerts::SubscriptionStatus>* actualRoot = (boost::shared_ptr<alerts::SubscriptionStatus> *) binder.getActualRoot();
//
//		testListener2A.waitUntilDone();
//		testListener2B.waitUntilDone();
//
//		CPPUNIT_ASSERT_MESSAGE(
//			"SubscriptionStatus ref count should be 3 as there is a local reference and 2 listeners are holding on to it.",
//			testListener2A.status.use_count() == 3 && testListener2B.status.use_count() == 3 && actualRoot->use_count() == 3 );
//
//		mb::Datum<alerts::SubscriptionStatus> statusResult(response);
//
//		CPPUNIT_ASSERT_MESSAGE("Incorrect status data bound.", status->getCode() == "ATBTALERTS");
//		CPPUNIT_ASSERT_MESSAGE("Bound data is not same as initial root for binding.", statusResult.getData() == status);
//		CPPUNIT_ASSERT_MESSAGE("Data in TestListener2A is not same as initial root for binding.", testListener2A.status.get() == status);
//		CPPUNIT_ASSERT_MESSAGE("Data in TestListener2B not same as initial root for binding.", testListener2B.status.get() == status);
//	}
//
//	CPPUNIT_ASSERT_MESSAGE(
//		"SubscriptionStatus ref count should be 2 as only the 2 listeners are holding on to it.",
//		testListener2A.status.use_count() == 2 && testListener2B.status.use_count() == 2 );
//
//	// Test multiple async post
//
//	{
//		alerts::AlertsSignOnBinder binder;
//		alerts::SubscriptionStatus* status = new alerts::SubscriptionStatus();
//		binder.setRoot(status);
//
//		mb::MessagePtr message1 = manager->createMessage("TestService1");
//		message1->setData("IN1", "30");
//
//		mb::MessagePtr message2 = manager->createMessage("TestService2");
//		message2->setDataBinder(&binder);
//
//		manager->postMessage(message1);
//		manager->postMessage(message2);
//
//		testListener1A.waitUntilDone();
//		testListener1B.waitUntilDone();
//
//		std::cout << "TestListener 1A received response : " << testListener1A.reply.str() << std::endl;
//		std::cout << "TestListener 1B received response : " << testListener1B.reply.str() << std::endl;
//
//		CPPUNIT_ASSERT_MESSAGE("TestListener 1A response does not match.", testListener1A.reply.str() == "1,20;2,60;");
//		CPPUNIT_ASSERT_MESSAGE("TestListener 2A response does not match.", testListener1B.reply.str() == "1,20;2,60;");
//
//		testListener2A.waitUntilDone();
//		testListener2B.waitUntilDone();
//
//		CPPUNIT_ASSERT_MESSAGE("Data in TestListener2A is not same as initial root for binding.", testListener2A.status.get() == status);
//		CPPUNIT_ASSERT_MESSAGE("Data in TestListener2B not same as initial root for binding.", testListener2B.status.get() == status);
//	}

	manager->unregisterListener(&testListener1A);
	manager->unregisterListener(&testListener1B);
	manager->unregisterListener(&testListener2A);
	manager->unregisterListener(&testListener2B);

	manager->unregisterService(&testService1);
	manager->unregisterService(&testService2);

	manager->debug("Message Bus after services and listeners have been removed:");
}

void MessageBusManagerTest::testAsyncTimedMessages() {

	mb::MessageBusManager* manager = mb::MessageBusManager::instance();

	mb::MessagePtr message;
	mb::MessagePtr response;

	TestListener1 testListener1A, testListener1B;
	manager->registerListener("TestService1A", &testListener1A);
	manager->registerListener("TestService1B", &testListener1B);

	IncCtrService testService1A("TestService1A", 0, mb::Message::MSG_P2P_SUB);
	manager->registerService(&testService1A);

	IncCtrService testService1B("TestService1B", 0, mb::Message::MSG_P2P_SUB);
	manager->registerService(&testService1B);

	manager->debug("Message Bus for testing timed messages:");

	message = manager->createMessage("TestService1A");
	message->setData("IN1", "10");
	message->setDelay(15000);

	manager->postMessage(message);

	message = manager->createMessage("TestService1B");
	message->setData("IN1", "20");
	message->setDelay(5000);

	manager->postMessage(message);

	long timestamp = getCurrentTimeInMillis();;

	std::cout << "Waiting for listeners to complete..." << std::endl;

	CThread::sleep(35000);

	std::cout << "TestListener 1A received response in " << (testListener1A.timestamp - timestamp) <<
		" ms: " << testListener1A.reply.str() << std::endl;

	std::cout << "TestListener 1B received response in " << (testListener1B.timestamp - timestamp) <<
		" ms: " << testListener1B.reply.str() << std::endl;

	message = manager->createMessage("TestService1B");
	((mb::P2PMessage *) message.get())->setControlAction(mb::P2PMessage::CANCEL);
	manager->postMessage(message);

	CThread::sleep(35000);

	std::cout << "TestListener 1A received response in " << (testListener1A.timestamp - timestamp) <<
		" ms: " << testListener1A.reply.str() << std::endl;

	std::cout << "TestListener 1B received response in " << (testListener1B.timestamp - timestamp) <<
		" ms: " << testListener1B.reply.str() << std::endl;
}

void MessageBusManagerTest::testServiceBindings() {

	mb::MessageBusManager* manager = mb::MessageBusManager::instance();

	TestListener1 testListener1A, testListener1B;
	TestListener2 testListener2A, testListener2B;

	IncCtrService testService1;
	FileRespService testService2(ALERTS_SIGNON_RESP_FILE);

	manager->registerService(&testService1);
	manager->registerService(&testService2);

	manager->debug("Message Bus after services have been added:");

	manager->registerListener("Test*1", &testListener1A);
	manager->registerListener("TestService1", &testListener1B);
	manager->registerListener("Test*2", &testListener2A);
	manager->registerListener("TestService2", &testListener2B);

	manager->registerListener("TestService2", &testListener1A);
	manager->registerListener("TestService2", &testListener1B);

	manager->debug("Message Bus after listeners have been added:");

	try {

		manager->registerListener("TestService2", &testListener1A);
		CPPUNIT_FAIL("Re-added testListener1A to subject TestService2.");
	}
	catch (...) {
	}

	manager->unregisterListener(&testListener1A);
	manager->unregisterListener(&testListener2A);
	manager->debug("Message Bus after listeners testListener1A & testListener2A have been removed:");

	manager->unregisterListener(&testListener1B);
	manager->debug("Message Bus after listener testListener1B have been removed:");

	manager->unregisterService(&testService2);

	manager->debug("Message Bus after service testService2 have been removed:");
}
