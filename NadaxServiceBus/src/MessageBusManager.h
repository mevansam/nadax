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

#ifndef MESSAGEBUSMANAGER_H_
#define MESSAGEBUSMANAGER_H_

#include "boost/thread.hpp"
#include "boost/regex.hpp"

#include "Service.h"
#include "Manager.h"


namespace mb {


/* Called when a service/listener for a particular subject is registered
 */
typedef void (*SubjectRegisteredCallback)(const char* subject, bool isService);

/* Called when a service/listener for a particular subject is unregistered
 */
typedef void (*SubjectUnregisteredCallback)(const char* subject, bool isService);

/* Called whenever a message is posted to the SMB. If this 
 * callback returns false then the message post will be canceled.
 */
typedef bool (*ActivityCallback)(MessagePtr message);


/* The message bus is a message/event distribution infrastructure. It
 * keeps track of a dictionary of subjects and associated listeners and
 * providers and distributes messages to one or more listeners as they
 * are published to the bus.
 *
 * A subject identifies the type and content of a message and is also
 * used as the key that determines the intended recipients of a message.
 * Any one can post a message to the message bus via the Message Bus
 * postMessage API as long as they are able to create a message via
 * the Message Bus' createMessage API and can populate it with valid
 * data.
 *
 * A "provider" provides message instances for a particular subject. It
 * can also be considered a factory for message creation. There can be
 * only one provider for each subject in the dictionary. A provide will
 * not necessarily be the source of information for the particular
 * messages content, however it will construct a message type with all
 * the necessary logic required to transmit and interpret the message
 * data.
 *
 * A "listener" receives notification when a message with the subject
 * it is listening for is added to the message bus. A listener should
 * implement all the necessary logic to be able to handle the message
 * type and content based on the subject it is listening on. Multiple
 * listeners can listen in on the same subject causing a message with
 * that subject to be multicast to all those listeners.
 *
 * A "service" is both a "provider" and "listener". It provides a
 * particular message type to the bus that can be used by clients to
 * post targeted messages to that service. This means a service can
 * be associated with only one subject. Since a service is also a
 * listener it may internally choose to listen to messages with other
 * subjects. In such a implementation the service implementation is
 * responsible for sorting and identifying messages based on its
 * subject when it receives notifications from the bus. Another
 * interesting use case would be that service can also establish
 * point to point communication with other services. Messages
 * targeted to a service is delivered synchronously, although a
 * service may act upon it asynchronously.
 */

class MessageQueue;

class MessageBusManager : protected Manager {

public:
	virtual ~MessageBusManager();

	static void initialize();
	static void destroy();

	static MessageBusManager* instance();

	MessagePtr createMessage( 
        const char* subject, 
        Message::MessageType type = Message::MSG_REQ,
        bool defaultCreateNVMessage = true);

	MessagePtr sendMessage(MessagePtr message);
	int postMessage(MessagePtr message, Listener* callback = NULL);

	void registerProvider(const char* subject, Provider* provider);
	void unregisterProvider(Provider* provider);

	void registerListener(const char* subject, Listener* listener);
	void unregisterListener(Listener* listener);

	void registerService(Service* service);
	void unregisterService(Service* service);
    
    static void addSubjectRegisteredCallback(SubjectRegisteredCallback callback);
    static void addSubjectUnregisteredCallback(SubjectUnregisteredCallback callback);
    static void addActivityCallback(ActivityCallback callback, Message::MessageType msgType);
    
	void debug(const char* msg);

protected:

	MessageBusManager();

	void foreground();
	bool background();

private:

	static void handleP2PReply(void* context, mb::MessagePtr message);
	static void handleMulticastReply(std::list<Listener*>* listeners, MessagePtr message);

	static bool unmarshalMessage(void* context, mb::MessagePtr message, void* buffer, size_t size);
	static bool readMessageStream(void* context, mb::MessagePtr message, void* buffer, size_t size);

	boost::shared_mutex m_listenersLock;
	boost::shared_mutex m_providersLock;
	boost::shared_mutex m_servicesLock;

	__gnu_cxx::hash_map<std::string, Provider*, mb::hashstr, mb::eqstr> m_providers;
	__gnu_cxx::hash_map<std::string, Service*, mb::hashstr, mb::eqstr> m_services;

	struct MessageListener {

		MessageListener(const char* subject, bool isRegex, Listener* listener)
			: subject(subject), listener(listener), isRegex(isRegex) { }

		bool operator== (const MessageListener& ml)
			{ return (subject == ml.subject && listener == ml.listener); }

		std::string subject;
		Listener* listener;

		bool isRegex;
		boost::regex regex;
	};

	std::list<MessageListener> m_listeners;
    std::list<MessageListener> m_passiveListeners;
	__gnu_cxx::hash_map<std::string, std::list<Listener*>, mb::hashstr, mb::eqstr> m_activeListeners;

	boost::shared_ptr<MessageQueue> m_messageQueue;
	boost::shared_ptr<boost::thread> m_queueWorker;

	static MessageBusManager* _manager;
    
    static std::list<SubjectRegisteredCallback> _subjectRegisteredCallbacks;
    static std::list<SubjectUnregisteredCallback> _subjectUnregisteredCallbacks;
};


}  // namespace : mb

#endif /* MESSAGEBUSMANAGER_H_ */
