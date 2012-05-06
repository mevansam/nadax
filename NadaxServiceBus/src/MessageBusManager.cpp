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

#include "MessageBusManager.h"

#include <iostream>
#include <queue>
#include <deque>
#include <set>

#include "boost/date_time/posix_time/posix_time_types.hpp"

#include "number.h"
#include "DataBinder.h"
#include "Unmarshaller.h"
#include "XmlStreamParser.h"

#define PROVIDER_FOR_SUBJECT_EXISTS  "A provider for the subject '%s' already exists."
#define SERVICE_FOR_SUBJECT_EXISTS   "A service for the subject '%s' already exists."
#define LISTENER_ALREADY_ADDED       "Listener has already been added to the subject '%s'."
#define SUBJECT_REGEX_ERROR          "Subject regex search pattern for listener is invalid: %s"
#define CAN_ONLY_SEND_P2P_MESSAGES   "Only P2P messages can be sent via sendMessage()."
#define INVALID_CALLBACK_LISTENER    "Invalid call back Listener applied to a P2P Message."
#define RESPONSE_BINDER_IS_LOCKED    "A prior response for the same subject is still being bound."

#define SEARCH_CHARS  "[]*+."

#define MAX_POLL_COUNT  65536


namespace mb {


// Message container for P2P request responses
struct Response {
    
    Response() {
        unmarshaller = NULL;
        isFirst = true;
        isNotified = false;
    }
    
    void wait() {
        boost::unique_lock<boost::mutex> lock(doneM);
        if (!isNotified)
            doneC.wait(lock);
    }
    
    void notify() {
        boost::lock_guard<boost::mutex> lock(doneM);
        isNotified = true;
        doneC.notify_all();
    }
    
    MessagePtr message;
    
    binding::DataBinderPtr dataBinder;
    binding::Unmarshaller* unmarshaller;
    
    std::list<Listener*> listeners;
    
    bool isFirst;
    bool isNotified;
    
    boost::mutex doneM;
    boost::condition_variable doneC;
};


// **** Message Bus Message Queue ****

// Provides access to the underlying
// priority queue's container
template <class T, class S, class C>
S& container(std::priority_queue<T, S, C>& q) {
    struct HackedQueue : private std::priority_queue<T, S, C> {
        static S& container(std::priority_queue<T, S, C>& q) {
            return q.*&HackedQueue::c;
        }
    };
    return HackedQueue::container(q);
}

class MessageQueue {
    
public:
    
    MessageQueue() {
        
        m_stop = false;
    }
    
    static void addActivityCallback(ActivityCallback callback, Message::MessageType msgType) {
        
        _activityCallbacks[msgType].push_back(callback);
    }
    
    void stop() {
        
        m_stop = true;
        notifyMessageAvailable();
    }
    
    void pause() {
    }
    
    void resume() {
    }
    
    void post(std::list<Listener*>* listeners, MessagePtr message) {
        
        bool canPost = true;
        
        Message::MessageType msgType = message->getType();
        
        std::list<ActivityCallback>::iterator callback = _activityCallbacks[msgType].begin();
        std::list<ActivityCallback>::iterator end = _activityCallbacks[msgType].end();
        
        while (canPost && callback != end)
            canPost = (**callback)(message), callback++;
        
        if (canPost) {
            
            QueuedMessage queuedMessage(*listeners, message);

            if (m_processing1.try_lock()) {
                
                try {
                    
                    m_queue.push(queuedMessage);
                    notifyMessageAvailable();
                    
                    TRACE( "New message with subject '%s' pushed to message queue. Queue size is %d.", 
                        message->getSubject().c_str(), m_queue.size() );
                    
                } catch (...) {
                    
                    ERROR("Unknown exception caught while posting a message to the queue.");
                }
                
                m_processing1.unlock();
                
            } else {
                
                // If a lock cannot be obtained then the queue is in the
                // process of sending messages, add message to a secondary
                // buffer to be picked up once the current send loop
                // is done.
                
                boost::shared_lock<boost::shared_mutex> lock(m_processing2);
                m_waitq.push_back(queuedMessage);
                
                TRACE( "Listeners for message '%s' will be wait listed as the message "
                    "queue is currently processing messages. Queue size is %d.",
                    message->getSubject().c_str(), m_waitq.size() );
            }
        }
    }
    
    void process() {
        
        void* threadContext = Manager::onBeginManagerThread();
        
        bool error = false;
        long long timeout = 0;
        
        while (!m_stop) {
            
            if (!error) { 
                
                boost::shared_lock<boost::shared_mutex> lock(m_processing2);
                
                // Handle termination on acquiring lock
                if (m_stop)
                    break;
                
                if (m_waitq.empty())
                    waitForMessage(timeout);
                
                // Handle termination on signal messages are in queue
                if (m_stop)
                    break;
                
                std::list<QueuedMessage>::iterator waitqend = m_waitq.end();
                std::list<QueuedMessage>::iterator waitq;
                
                for (waitq = m_waitq.begin(); waitq != waitqend; waitq++) {
                    
                    boost::shared_lock<boost::shared_mutex> lock(m_processing1);
                    m_queue.push(*waitq);
                }
                
                m_waitq.clear();
                
            } else
                error = false;
            
            try {
                
                { boost::shared_lock<boost::shared_mutex> lock(m_processing1);
                    
                    // Handle termination on acquiring lock
                    if (m_stop)
                        break;
                    
                    timeout = 0;
                    
                    while (!m_queue.empty()) {
                        
                        TRACE("Processing messages in queue. Current size is %d.", m_queue.size());
                        
                        QueuedMessage queuedMessage = m_queue.top();
                        
                        const std::string& subject = queuedMessage.message->getSubject();
                        TRACE("    * Message at head of queue has subject '%s'.", subject.c_str());
                        
                        if (!(timeout = queuedMessage.message->getDelay())) {
                            
                            m_queue.pop();
                            
                            if (queuedMessage.isSuspended) {
                                
                                queuedMessage.message->schedulePost();
                                m_queue.push(queuedMessage);
                                continue;
                            }
                            
                            // Multi-cast message to all registered listeners for the message's subject
                            
                            TRACE( "Multi-casting queued message with id %s of type %d with post count %d and with subject %s to %d listeners.",
                                queuedMessage.message->getId().c_str(),
                                queuedMessage.message->getType(), 
                                queuedMessage.message->getPostCount(), 
                                subject.c_str(), 
                                queuedMessage.listeners.size() );
                            
                            if (queuedMessage.message->getType() == Message::MSG_P2P_SUB) {
                                
                                P2PMessage* message = (P2PMessage *) queuedMessage.message.get();
                                P2PMessage::ControlAction subscriptionAction = message->getControlAction();
                                
                                // If the message is a normal P2P sub message then reschedule it in the queue
                                // else associate the p2p message control action of the message with the subject.
                                
                                if (subscriptionAction == P2PMessage::NONE) {
                                    
                                    std::list<Listener*>::iterator end = queuedMessage.listeners.end();
                                    for (std::list<Listener*>::iterator listener = queuedMessage.listeners.begin(); listener != end; listener++)
                                        (*listener)->onMessage(queuedMessage.message);
                                    
                                    if ( message->getDelayInterval() && 
                                        message->getPostCount() < MAX_POLL_COUNT ) {
                                        
                                        TRACE("Subscription message '%s' is being requeued to be processed again after the poll interval.", subject.c_str());
                                        
                                        message->incPostCount();
                                        message->schedulePost();
                                        m_queue.push(queuedMessage);
                                    }
                                    
                                } else if (!m_queue.empty()) {
                                    
                                    const std::string& respSubject = message->getRespSubject();
                                    const std::string& targetMsgId = message->getTargetMsgId();
                                    
                                    std::vector<QueuedMessage>& messages = container(m_queue);
                                    std::vector<QueuedMessage>::iterator qm = messages.end();
                                    std::vector<QueuedMessage>::iterator firstqm = messages.begin();

                                    TRACE("Processing subscription control action '%d' for subject '%s' and target message id '%s', and response subject: '%s'.", 
                                        subscriptionAction, subject.c_str(), targetMsgId.c_str(), respSubject.c_str());
                                    
                                    do {
                                        
                                        qm--;
                                        
                                        const std::string& id = qm->message->getId();
                                        const std::string& qmRespSubject = qm->message->getRespSubject();
                                        const std::string& qmSubject = qm->message->getSubject();
                                        
                                        TRACE(" ++ subject:'%s', target message id:'%s', response subject:'%s', type:'%d'", 
                                            qmSubject.c_str(), id.c_str(), qmRespSubject.c_str(), qm->message->getType() );
                                        
                                        if ( qm->message->getType() == Message::MSG_P2P_SUB &&
                                            qmSubject == subject &&
                                            (!respSubject.length() || qmRespSubject == respSubject) &&
                                            (!targetMsgId.length() || targetMsgId == id) ) {
                                            
                                            P2PMessage* subMessage = (P2PMessage *) qm->message.get();
                                            subMessage->setAttachment(queuedMessage.message);
                                            subMessage->setControlAction(subscriptionAction);
                                            
                                            std::list<Listener*>::iterator end = qm->listeners.end();
                                            for (std::list<Listener*>::iterator listener = qm->listeners.begin(); listener != end; listener++)
                                                (*listener)->onMessage(qm->message);
                                            
                                            subMessage->removeAttachment();
                                            subMessage->setControlAction(P2PMessage::NONE);
                                            
                                            switch (subscriptionAction) {
                                                    
                                                case P2PMessage::CANCEL:
                                                    TRACE("    * Subscription message '%s' with id '%s' has been cancelled.", subject.c_str(), id.c_str());
                                                    messages.erase(qm);
                                                    break;
                                                    
                                                case P2PMessage::SUSPEND:
                                                    TRACE("    * Subscription message '%s' with id '%s' has been suspended.", subject.c_str(), id.c_str());
                                                    qm->isSuspended = true;
                                                    break;
                                                    
                                                case P2PMessage::RESUME:
                                                    TRACE("    * Subscription message '%s' with id '%s' has been resumed.", subject.c_str(), id.c_str());
                                                    qm->isSuspended = false;
                                                    break;
                                                    
                                                default:
                                                    break;
                                            }
                                        }
                                        
                                    } while (qm != firstqm);
                                }
                            
                            } else {
                                
                                std::list<Listener*>::iterator end = queuedMessage.listeners.end();
                                for (std::list<Listener*>::iterator listener = queuedMessage.listeners.begin(); listener != end; listener++)
                                    (*listener)->onMessage(queuedMessage.message);
                            }
                            
                        } else
                            break;
                    }
                    
                    TRACE("Message queue waiting for %d ms.", timeout);
                }
                
            } catch (...) {
                
                ERROR("Exception caught in message queue processing thread.");
                
                // Set error to true so the message procesing loop reruns
                error = true;
            }
        }
        
        Manager::onEndManagerThread(threadContext);
    }
    
private:
    
    bool waitForMessage(long millis) {
        boost::unique_lock<boost::mutex> lock(m_messageAvailable_M);
        if (millis) {
            const boost::system_time timeout = boost::get_system_time() + boost::posix_time::milliseconds(millis);
            return m_messageAvailable_C.timed_wait(lock, timeout);
        } else {
            m_messageAvailable_C.wait(lock);
            return false;
        }
    }
    void notifyMessageAvailable() {
        boost::lock_guard<boost::mutex> lock(m_messageAvailable_M);
        m_messageAvailable_C.notify_all();
    }
    
    struct QueuedMessage {
        
        QueuedMessage() { }
        
        QueuedMessage(std::list<Listener*> l, MessagePtr m) 
            : listeners(l), message(m), isSuspended(false) { }
        
        QueuedMessage(const QueuedMessage& qm)
            : listeners(qm.listeners), message(qm.message), isSuspended(qm.isSuspended) { }
        
        QueuedMessage& operator= (const QueuedMessage& qm) {
            
            if (this != &qm) {
                
                listeners = qm.listeners;
                message = qm.message;
                isSuspended = qm.isSuspended;
            }
            return *this;
        }
        
        bool operator< (const QueuedMessage& qm) const {
            
            return qm.message->m_posttime < message->m_posttime;
        }
        
        std::list<Listener*> listeners;
        MessagePtr message;
        
        bool isSuspended;
    };
    
    std::priority_queue<QueuedMessage> m_queue;
    
    boost::shared_mutex m_processing1;
    boost::mutex m_messageAvailable_M;
    boost::condition_variable m_messageAvailable_C;
    
    std::list<QueuedMessage> m_waitq;
    boost::shared_mutex m_processing2;
    
    Number<bool> m_stop;
    
    static std::list<ActivityCallback> _activityCallbacks[Message::NUM_TYPES];
};

std::list<ActivityCallback> MessageQueue::_activityCallbacks[Message::NUM_TYPES];


// **** Static Implementation ****

SINGLETON_MANAGER_IMPLEMENTATION(MessageBusManager, MessageBusException)

std::list<SubjectRegisteredCallback> MessageBusManager::_subjectRegisteredCallbacks;
std::list<SubjectUnregisteredCallback> MessageBusManager::_subjectUnregisteredCallbacks;

void MessageBusManager::addSubjectRegisteredCallback(SubjectRegisteredCallback callback) {
    
    _subjectRegisteredCallbacks.push_back(callback);
}

void MessageBusManager::addSubjectUnregisteredCallback(SubjectUnregisteredCallback callback) {
    
    _subjectUnregisteredCallbacks.push_back(callback);
}

void MessageBusManager::addActivityCallback(ActivityCallback callback, Message::MessageType msgType) {
    
    MessageQueue::addActivityCallback(callback, msgType);
}


// **** Implementation ****

MessageBusManager::MessageBusManager() {
    
    m_messageQueue = boost::shared_ptr<MessageQueue>(new MessageQueue());
    m_queueWorker = boost::shared_ptr<boost::thread>(new boost::thread(&MessageQueue::process, m_messageQueue.get()));
}

MessageBusManager::~MessageBusManager() {
    
    m_messageQueue->stop();
    m_queueWorker->join();
}

void MessageBusManager::foreground() {
    
    { boost::shared_lock<boost::shared_mutex> lock(m_servicesLock);
        
        boost::unordered_map<std::string, Service*>::iterator i;
        
        for (i = m_services.begin(); i != m_services.end(); i++)
            i->second->resume(NULL);
    }

    m_messageQueue->resume();
}

bool MessageBusManager::background() {
    
    { boost::shared_lock<boost::shared_mutex> lock(m_servicesLock);
        
        boost::unordered_map<std::string, Service*>::iterator i;
        
        for (i = m_services.begin(); i != m_services.end(); i++)
            i->second->pause(NULL);
    }

    m_messageQueue->pause();
    return true;
}

MessagePtr MessageBusManager::createMessage(const char* subject, Message::MessageType type, bool defaultCreateNVMessage) {
    
    Message* message = NULL;
    
    if (type == Message::MSG_REQ) {
        
        boost::shared_lock<boost::shared_mutex> lock(m_providersLock);
        
        if (m_providers.find(subject) != m_providers.end())
            message = m_providers[subject]->createMessage();
    }
    
    if (!message) {
        
        if (defaultCreateNVMessage)
            message = new NVMessage();
        else
            message = new DataMessage();
    }
    
    if (message->m_subject.length() == 0)
        message->m_subject = subject;
    
    if (message->m_msgType == Message::MSG_UNKNOWN)
        message->m_msgType = type;
    
    MessagePtr result(message);
    return result;
}

MessagePtr MessageBusManager::sendMessage(MessagePtr message) {
    
    Response response;
    
    if (message->getType() == Message::MSG_P2P || message->getType() == Message::MSG_P2P_SUB) {
        
        const char* subject = message->getSubject().c_str();
        
        TRACE("Begin sending sync P2P message type '%d' for subject '%s'.", message->getType(), subject);
        
        Listener* serviceListener;
        
        { boost::shared_lock<boost::shared_mutex> lock(m_servicesLock);
            
            boost::unordered_map<std::string, Service*>::iterator element = m_services.find(subject);
            serviceListener = (element != m_services.end() ? element->second : NULL);
        }
        
        if (serviceListener) {
            
            response.dataBinder = message->getDataBinder();
            
            ((P2PMessage *) message.get())->setCallback(&response, handleP2PReply);
            serviceListener->onMessage(message);
            
            response.wait();
        }
        
        TRACE("End sending sync P2P message type '%d' for subject '%s'.", message->getType(), subject);
    }
    else
        THROW(MessageBusException, EXCEP_MSSG(CAN_ONLY_SEND_P2P_MESSAGES));
    
    return response.message;
}

int MessageBusManager::postMessage(MessagePtr message, Listener* callback) {
    
    int numReceivers = 0;
    const char* subject = message->getSubject().c_str();
    
    Response* response = new Response();
    response->dataBinder = message->getDataBinder();
    
    if (message->getType() == Message::MSG_P2P || message->getType() == Message::MSG_P2P_SUB) {
        
        if (callback)
            THROW(MessageBusException, EXCEP_MSSG(INVALID_CALLBACK_LISTENER));
        
        { boost::shared_lock<boost::shared_mutex> lock(m_servicesLock);
            
            boost::unordered_map<std::string, Service*>::iterator element = m_services.find(subject);
            if (element == m_services.end())
                return 0;
            
            response->listeners.push_back(element->second);
        }
        
        TRACE("Begin posting P2P message type '%d' for subject '%s'.", message->getType(), subject);
        
    } else {
        
        if (callback)
            response->listeners.push_back(callback);
        
        { boost::shared_lock<boost::shared_mutex> lock(m_listenersLock);
            
            if (m_activeListeners.find(subject) != m_activeListeners.end()) {
                
                std::list<Listener*>* listeners = &m_activeListeners[subject];
                response->listeners.insert(response->listeners.end(), listeners->begin(), listeners->end());
            }
            
            std::list<MessageListener>::iterator ml = m_passiveListeners.begin();
            std::list<MessageListener>::iterator mlend = m_passiveListeners.end();
            
            while (ml != mlend) {
                
                if (boost::regex_match(subject, ml->regex))
                    response->listeners.push_back(ml->listener);
                
                ml++;
            }
        }
        
        TRACE("Begin posting message type '%d' for subject '%s'.", message->getType(), subject);
    }
    
    if ((numReceivers = response->listeners.size()) > 0) {
        
        if ( message->getDataBinder() &&
            (message->getType() ==  Message::MSG_RESP_STRING || message->getType() == Message::MSG_RESP_STREAM) ) {
            
            TRACE("Will be binding message data for subject '%s'.", subject);
            
            if (message->getType() == Message::MSG_RESP_STRING) {
                
                TRACE("Message data for subject '%s' is string.", subject);
                
                std::string msg = (const char *) message->getData();
                int size = msg.length();
                
                if (size)
                    unmarshalMessage(response, message, (void *) msg.c_str(), size);
                
                unmarshalMessage(response, message, NULL, 0);
                
            } else {
                
                TRACE("Message data for subject '%s' will be streamed to a call back handler function.", subject);
                ((StreamMessage *) message.get())->setCallback(response, unmarshalMessage);
            }
            
        } else {
            handleMulticastReply(&(response->listeners), message);
            delete response;
        }
        
        TRACE( "Done posting message type '%d' for subject '%s' to %d listeners.",
              message->getType(), subject, numReceivers );
    }
    return numReceivers;
}

void MessageBusManager::handleP2PReply(void* context, mb::MessagePtr message) {
    
    Response* response = (Response *) context;
    
    if (message->getType() == Message::MSG_RESP_STREAM) {
        
        TRACE( "Setting call back function to read message stream for P2P response message with subject '%s'.",
              message->getSubject().c_str() );
        
        response->message = MessagePtr(new StringMessage(message.get()));
        ((StreamMessage *) message.get())->setCallback(context, readMessageStream);
        
    } else {
        
        response->message = message;
        response->notify();
    }
}

bool MessageBusManager::readMessageStream(void* context, mb::MessagePtr message, void* buffer, size_t size) {
    
    Response* response = (Response *) context;
    
    TRACE( "Reading streamed %d bytes of message data for P2P response message with subject '%s'.",
          size, response->message->getSubject().c_str() );
    
    if (!size)
        response->notify();
    else
        ((StringMessage *) response->message.get())->append((char *) buffer, size);
    
    return true;
}

void MessageBusManager::handleMulticastReply(std::list<Listener*>* listeners, MessagePtr message) {
    
    if (message->getType() == Message::MSG_RESP_STREAM || message->getType() == Message::MSG_RESP_UPDATE) {
        
        TRACE("Synchronously multi-casting response message with subject '%s'.", message->getSubject().c_str());
        
        std::list<Listener*>::iterator end = listeners->end();
        
        for (std::list<Listener*>::iterator listener = listeners->begin(); listener != end; listener++)
            (*listener)->onMessage(message);
        
    } else {
        
        TRACE("Asynchronously multi-casting response message with subject '%s'.", message->getSubject().c_str());
        
        MessageBusManager::_manager->m_messageQueue->post(listeners, message);
    }
}

bool MessageBusManager::unmarshalMessage(void* context, mb::MessagePtr message, void* buffer, size_t size) {
    
    Response* response = (Response *) context;
    
    try {
        
        if (!size) {
            
            if (response->unmarshaller) {
                
                response->unmarshaller->parse("", 0);
                
                response->message = MessagePtr(new DataMessage(message.get()));
                response->message->setData(response->unmarshaller->getResult());
                response->message->m_cntType = Message::CNT_MODEL;
                
                response->dataBinder->reset();
                delete response->unmarshaller;
                
                TRACE( "Returning unmarshalled message data for P2P response message with subject '%s'.",
                      response->message->getSubject().c_str() );
                
            } else
                response->message = MessagePtr(new Message(message.get()));
            
            response->message->m_msgType = Message::MSG_RESP;
            
            handleMulticastReply(&(response->listeners), response->message);
            delete response;
            
            return true;
            
        } else {
            
            Message::ContentType cntType = message->getContentType();
            
            if (response->isFirst) {
                
                binding::DataBinderPtr dataBinder = response->dataBinder;
                
                if (!dataBinder->lock())
                    THROW(MessageBusException, EXCEP_MSSG(RESPONSE_BINDER_IS_LOCKED)); 
                
                switch (cntType) {

					//case Message::ContentType::JSON:
					//
					//	response->unmarshaller = new parser::JsonBinder(dataBinder);
					//	response->unmarshaller->initialize();
					//  TRACE("Unmarshalling json data stream for response message with subject '%s'.", response->message->getSubject());
					//	break;
                        
                    case Message::CNT_XML:
                    default:
                        
                        // Assume XML as default content type
                        response->unmarshaller = new parser::XmlBinder(dataBinder);
                        response->unmarshaller->initialize();
                        
                        TRACE( "Unmarshalling xml data stream for response message with subject '%s'.",
                              message->getSubject().c_str() );

                        break;
                }
                
                response->isFirst = false;
            }
            
            if (response->unmarshaller) {
                
                TRACE( "Unmarshalling streamed %d bytes of message data for P2P response message with subject '%s'.",
                      size, message->getSubject().c_str() );
                
                response->unmarshaller->parse((char *) buffer, size);
            }
        }
        
        return true;
        
    } catch (CException* e) {
        
        ERROR("Exception caught while binding message response data: %s", e->getMessage());
        TRACE("Error parsing buffer: %s", std::string((const char *) buffer, size).c_str());
        
        response->message = MessagePtr(new Message(message.get()));
        response->message->m_msgType = Message::MSG_RESP;
        response->message->setError(Message::ERR_SERVICE, 500, e->getMessage());
        
        handleMulticastReply(&(response->listeners), response->message);
        
        if (response->unmarshaller)
            delete response->unmarshaller;
        
        delete response;
    }
    
    return false;
}

void MessageBusManager::registerProvider(const char* subject, Provider* provider) {
    
    boost::unique_lock<boost::shared_mutex> lock(m_providersLock);
    
    if (m_providers.find(subject) != m_providers.end())
        THROW(MessageBusException, EXCEP_MSSG(PROVIDER_FOR_SUBJECT_EXISTS, subject));
    
    m_providers[subject] = provider;
}

void MessageBusManager::unregisterProvider(Provider* provider) {
    
    boost::unique_lock<boost::shared_mutex> lock(m_providersLock);
    
    boost::unordered_map<std::string, Provider*>::iterator i;
    for (i = m_providers.begin(); i != m_providers.end(); i++) {
        
        if (i->second == provider) {
            m_providers.erase(i);
            break;
        }
    }
}

void MessageBusManager::registerService(Service* service) {
    
    const char* subject = service->getSubject();
    
    { boost::unique_lock<boost::shared_mutex> lock(m_servicesLock);
        
        if (m_services.find(subject) != m_services.end())
            THROW(MessageBusException, EXCEP_MSSG(SERVICE_FOR_SUBJECT_EXISTS, subject));
        
        registerProvider(subject, service);
        m_services[subject] = service;
    }
    
    service->intialize();
    
    std::list<SubjectRegisteredCallback>::iterator callback = _subjectRegisteredCallbacks.begin();
    while (callback != _subjectRegisteredCallbacks.end())
        (**callback)(subject, true), callback++;
}

void MessageBusManager::unregisterService(Service* service) {
    
    const char* subject = service->getSubject();
    
    std::list<SubjectUnregisteredCallback>::iterator callback = _subjectUnregisteredCallbacks.begin();
    while (callback != _subjectUnregisteredCallbacks.end())
        (**callback)(subject, true), callback++;
    
    { boost::unique_lock<boost::shared_mutex> lock(m_servicesLock);
        
        boost::unordered_map<std::string, Service*>::iterator i = m_services.find(subject);
        
        if (i != m_services.end()) {
            unregisterProvider(service);
            m_services.erase(i);
        }
    }
    
    service->destroy();
}

void MessageBusManager::registerListener(const char* subject, Listener* listener) {
    
    boost::unique_lock<boost::shared_mutex> lock(m_listenersLock);
    
    bool isRegex = false;
    const char* last = subject + strlen(subject);
    for (const char* ch = subject; ch < last; ch++) {
        if ( strchr(SEARCH_CHARS, *ch) && (ch == subject || *(ch - 1) != '\\') )
            isRegex = true;
    }
    
    MessageListener ml(subject, isRegex, listener);
    
    std::list<MessageListener>::iterator i;
    for (i = m_listeners.begin(); i != m_listeners.end(); i++) {
        
        if (ml == *i)
            THROW(MessageBusException, EXCEP_MSSG(LISTENER_ALREADY_ADDED, subject));
    }
    
    m_listeners.push_back(ml);
    
    if (isRegex) {
        
        try {
            ml.regex.assign(subject);
        } catch (boost::regex_error& e) {
            THROW(MessageBusException, EXCEP_MSSG(SUBJECT_REGEX_ERROR, e.what()));
        }
        m_passiveListeners.push_back(ml);
        
    } else {
        
        std::list<Listener*>* listeners = &m_activeListeners[subject];
        listeners->remove(listener);
        listeners->push_back(listener);
        
        std::list<SubjectRegisteredCallback>::iterator callback = _subjectRegisteredCallbacks.begin();
        while (callback != _subjectRegisteredCallbacks.end())
            (**callback)(subject, false), callback++;
    }
}

void MessageBusManager::unregisterListener(Listener* listener) {
    
    boost::unique_lock<boost::shared_mutex> lock(m_listenersLock);
    
    std::set<std::string> subjects;
    
    std::list<MessageListener>::iterator j, i = m_listeners.begin();
    std::list<MessageListener>::iterator end = m_listeners.end();
    while (i != end) {
        
        j = i++;
        
        if (j->listener == listener) {
            
            if (j->isRegex) {
                
                m_passiveListeners.remove(*j);
                
            } else if (m_activeListeners.find(j->subject) != m_activeListeners.end()) {
                
                std::list<Listener*>* listeners = &m_activeListeners[j->subject];
                listeners->remove(listener);
                
                subjects.insert(j->subject);
            }
            
            m_listeners.remove(*j);
        }
    }
    
    std::list<SubjectUnregisteredCallback>::iterator callback = _subjectUnregisteredCallbacks.begin();
    while (callback != _subjectUnregisteredCallbacks.end()) {
        
        std::set<std::string>::iterator subject = subjects.begin();
        while (subject != subjects.end())
            (**callback)(subject->c_str(), false), subject++;
        
        callback++;
    }
}

void MessageBusManager::debug(const char* msg) {
    
    std::cout << std::endl;
    std::cout << "Debug output for MessageBusManager instance '" << msg;
    std::cout << "' : " << std::endl;
    
    char temp[20];
    
    std::list<MessageListener>::iterator i;
    
    std::cout << "  All Registered Listeners: " << std::endl;
    for (i = m_listeners.begin(); i != m_listeners.end(); i++) {
        
        sprintf(temp, "[%p]", i->listener);
        
        std::cout <<
        "    * " << temp <<
        " [subject = " << i->subject <<
        ", isRegex = " << (i->isRegex ? 'Y' : 'N') << "]" << std::endl;
    }
    
    std::list<MessageListener>::iterator j;
    std::cout << "  Passive Listeners: " << std::endl;
    for (i = m_passiveListeners.begin(); i != m_passiveListeners.end(); i++) {
        
        sprintf(temp, "[%p]", i->listener);
        std::cout << "    * " << temp << std::endl;
    }
    
    boost::unordered_map<std::string, std::list<Listener*> >::iterator k;
    
    std::cout << "  Active Listeners: " << std::endl;
    for (k = m_activeListeners.begin(); k != m_activeListeners.end(); k++) {
        
        std::cout << "    * " << k->first << std::endl;
        std::list<Listener*>::iterator end = k->second.end();
        
        for (std::list<Listener*>::iterator l = k->second.begin(); l != end; l++) {
            sprintf(temp, "[%p]", *l);
            std::cout << "        - " << temp << std::endl;
        }
    }
    
    boost::unordered_map<std::string, Provider*>::iterator m;
    
    std::cout << "  Providers: " << std::endl;
    for (m = m_providers.begin(); m != m_providers.end(); m++) {
        sprintf(temp, "[%p]", m->second);
        std::cout << "    * " << m->first << temp << std::endl;
    }
    
    boost::unordered_map<std::string, Service*>::iterator n;
    
    std::cout << "  Services: " << std::endl;
    for (n = m_services.begin(); n != m_services.end(); n++) {
        sprintf(temp, "[%p]", n->second);
        std::cout << "    * " << n->first << temp << std::endl;
    }
    
    std::cout << std::endl;
}


}  // namespace : mb
