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

#ifndef SERVICE_H_
#define SERVICE_H_

#include <sys/time.h>
#include <string>

#include <sstream>
#include <list>
#include <map>

#include "boost/shared_ptr.hpp"
#include "boost/unordered_map.hpp"
#include "boost/unordered_set.hpp"

#include "log.h"
#include "uuid.h"

#include "DynaModel.h"

#ifndef CSTR_TRUE
#define CSTR_TRUE   "true"
#endif
#ifndef CSTR_FALSE
#define CSTR_FALSE  "false"
#endif

#define SUBSCRIPTION_ID     "SUBSCRIPTION_ID"
#define DATA_IS_DYNA_MODEL  "IS_DYNA_MODEL"
#define STREAMING_UPDATE    "IS_STREAMING"
#define REQUEST_ID          "REQUEST_ID"

#define POST_RESPONSE(response, message) \
    MessageBusManager::instance()->postMessage( response, \
        ((message->getType() == Message::MSG_P2P || message->getType() == Message::MSG_P2P_SUB) \
            && ((P2PMessage *) message.get())->hasCallback() \
                ? dynamic_cast<Listener *>(message.get()) : NULL) )

#define SEND_P2P_REPLY(response, message) \
    if ((message->getType() == Message::MSG_P2P || message->getType() == Message::MSG_P2P_SUB) \
        && ((P2PMessage *) message.get())->hasCallback()) { \
            dynamic_cast<Listener *>(message.get())->onMessage(response); }

#define SEND_DATA(message, buffer, size) \
    ((StreamMessage *) message.get())->sendData(message, buffer, size)


namespace mb {

class Message;
typedef boost::shared_ptr<Message> MessagePtr;

/* A Listener listens for message associated with a
 * particular subject that is posted to the Message Bus.
 */
class Listener {
public:
	virtual ~Listener() { }
	virtual void onMessage(MessagePtr message) = 0;
};

/* A Provider provides message class instance for
 * particular subject types.
 */
class Provider {
public:
	virtual ~Provider() { }
	virtual Message* createMessage() = 0;
};

/* Data callback which clients provide to
 * receive streamed data from a message
 */
typedef bool (*DataCallback)(void* context, MessagePtr message, void* buffer, size_t size);

/* Response callback which clients can use to receive
 * a one time response for a specific message posted
 * by that client.
 */
typedef void (*MessageCallback)(void* context, MessagePtr message);
    
/* Callback for cleaning up message instances.
 */
typedef void (*MessageCleanupCallback)(Message* message);

/* STL map of string name value pairs.
 */
typedef boost::unordered_map<std::string, std::string> NameValueMap;

/* Base message structure. The Message Bus will create
 * a particular instance of a Message given a subject
 * name of the intended recipient of that message.
 */
class Message {

public:

	enum MessageType {

		MSG_UNKNOWN,
		MSG_P2P,          // P2P service request
		MSG_P2P_SUB,      // P2P subscription request
		MSG_REQ,          // Service request
		MSG_RESP,         // Service response
		MSG_RESP_STRING,  // Service response string
		MSG_RESP_STREAM,  // Service response stream
        MSG_RESP_UPDATE,  // Service update response
		MSG_ERROR,        // Error message
        
        NUM_TYPES
	};

	enum ContentType {

		CNT_UNKNOWN,
		CNT_XML,      // XML
		CNT_JSON,     // JSON
		CNT_MODEL,    // Marshalled data
        CNT_NVMAP     // NameValue map
	};

	enum Error {
		ERR_NONE,
		ERR_MESSAGE_BUS,         // Error in message bus
		ERR_MESSAGE_TIMEOUT,     // Async message post timeout
		ERR_SERVICE,             // Error in service
		ERR_CONNECTION_ERROR,    // Service connect error
		ERR_CONNECTION_BREAK,    // Service connection break
		ERR_CONNECTION_TIMEOUT,  // Service connection initiation timeout
		ERR_EXECUTION_TIMEOUT    // Service execution timeout
	};

	struct NameValue {

		NameValue(const std::string& name, const std::string& value)
			: name(name), value(value) { };

		NameValue(const char* name, const char* value)
			: name(name), value(value) { };

		bool operator== (NameValue& nameValue) const
			{ return name == nameValue.name; }

		std::string name;
		std::string value;
	};

public:
	Message() {

		CUUID uuid;
		m_id = uuid.get();

		m_delay = 0;
		m_posttime = 0;
        
        m_postCount = 0;

		m_msgType = MSG_UNKNOWN;
		m_cntType = CNT_UNKNOWN;

		m_error = ERR_NONE;
		m_errorCode = 0;
        
        m_hasBinder = 0;
        
        m_cleanupCallback = NULL;

		TRACE("Constructing Message: %p[%s]", this, m_id.c_str());
	}
	Message(Message* message) {

		CUUID uuid;
		m_id = uuid.get();

		m_delay = message->m_delay;
		m_posttime = message->m_posttime;
        
        m_postCount = 0;
        
		m_msgType = message->m_msgType;
		m_cntType = message->m_cntType;

		m_error = message->m_error;
		m_errorCode = message->m_errorCode;
		m_errorDescription = message->m_errorDescription;

		m_dataBinder = message->m_dataBinder;
        m_hasBinder = 0;

		m_subject = message->m_subject;

		m_msgMetaData.insert(message->m_msgMetaData.begin(), message->m_msgMetaData.end());
        
        m_cleanupCallback = message->m_cleanupCallback;
        message->m_cleanupCallback = NULL;
        
		TRACE("Copying Message: %p[%s] to %p[%s]", message, message->m_id.c_str(), this, m_id.c_str());
	}
	virtual ~Message() {
        
        if (m_cleanupCallback)
            m_cleanupCallback(this);
        
		TRACE("Destroying Message with id: %p[%s]", this, m_id.c_str());
	}

	const std::string& getId() {
		return m_id;
	}

	MessageType getType() {
		return m_msgType;
	}

	ContentType getContentType() {
		return m_cntType;
	}
	void setContentType(ContentType cntType) {
		m_cntType = cntType;
	}

	Error getError() {
		return m_error;
	}
	int getErrorCode() {
		return m_errorCode;
	}
	const std::string& getErrorDescription() {
		return m_errorDescription;
	}
	void setError(Error error, int errorCode = 1, const char* errorDescription = NULL) {
		m_error = error;
		m_errorCode = errorCode;
		if (errorDescription) {
			m_errorDescription = errorDescription;
		}
	}

    long getDelayInterval() {
        return m_delay;
    }
	long getDelay() {

		struct timeval time;
		gettimeofday(&time, NULL);
        
        long long tv_sec = time.tv_sec;
        long long tv_usec = time.tv_usec;
        long long tv_millis = tv_sec * 1000 + tv_usec / 1000;
        
		long long delay = m_posttime - tv_millis;
		return (delay < 0 ? 0 : delay);
	}
	void setDelay(long delay, bool poll = false, bool nowait = false) {

		m_delay = delay;

        if (poll && m_msgType == MSG_P2P)
            m_msgType = MSG_P2P_SUB;
        
        if (!nowait)
            schedulePost();
	}
	void schedulePost() {

		struct timeval time;
		gettimeofday(&time, NULL);
        
        long long tv_sec = time.tv_sec;
        long long tv_usec = time.tv_usec;
        long long tv_millis = tv_sec * 1000 + tv_usec / 1000;

		m_posttime = tv_millis + m_delay;
	}
    
    long getPostCount() {
        return m_postCount;
    }
    void incPostCount() {
        m_postCount++;
    }
    void resetPostCount() {
        m_postCount = 0;
    }

	const std::string& getSubject() {
		return m_subject;
	}
    
	const std::string& getRespSubject() {
		return m_respSubject;
	}
	void setRespSubject(const char* respSubject) {
		m_respSubject = respSubject;
	}

	virtual void* getData() {
		return NULL;
	}
	virtual void setData(void* data) {
	}
    
	virtual void* getData(const char* name) {
		return NULL;
	}
	virtual void setData(const char* name, const void* value) {
	}

	/* Message meta data  */
	NameValueMap& getMetaData() {
		return m_msgMetaData;
	}
    
    /* Message attachment */
    MessagePtr getAttachment() {
        return m_attachment;
    }
    void setAttachment(const MessagePtr& message) {
        m_attachment = message;
    }
    void removeAttachment() {
        m_attachment.reset();
    }

	/* Unmarshaller to unmarshal response stream to a model */
	void setDataBinder(binding::DataBinderPtr dataBinder) {
        ++m_hasBinder;
		m_dataBinder = dataBinder;
	}
	binding::DataBinderPtr getDataBinder() {
		return m_dataBinder;
	}
    short hasBinder() {
        return m_hasBinder;
    }

    /* Message cleanup callback */
    void setCleanupCallback(MessageCleanupCallback callback) {
        m_cleanupCallback = callback;
    }

protected:

	std::string m_id;

	long long m_delay;
	long long m_posttime;
    
    long m_postCount;

	MessageType m_msgType;
	ContentType m_cntType;

	Error m_error;
	int m_errorCode;
	std::string m_errorDescription;

	std::string m_subject;
    std::string m_respSubject;

	binding::DataBinderPtr m_dataBinder;
    short m_hasBinder;

	NameValueMap m_msgMetaData;
    MessagePtr m_attachment;
    
    MessageCleanupCallback m_cleanupCallback;

    template<class T> friend class Datum;
	friend class Service;
	friend class MessageBusManager;
	friend class MessageQueue;
};

/* A message that contains name value pairs 
 * as its content.
 */
class NVMessage : public Message {
    
public:
    NVMessage() : Message() {
        m_cntType = Message::CNT_NVMAP;
    };
    NVMessage(Message* message) : Message(message) {
    };
    NVMessage(NVMessage* message) : Message(message) {
        m_data.insert(message->m_data.begin(), message->m_data.end());
    };
    virtual ~NVMessage() {
    }
    
    void* getData() {
        return &m_data;
    }
    void setData(void* data) {
        m_data.insert(((NameValueMap *) data)->begin(), ((NameValueMap *) data)->end());
    }
    void* getData(const char* name) {
		return &m_data[name];
	}
    void setData(const char* name, const void* value) {
        m_data[name] = (const char *) value;
	}
    
private:
    
    NameValueMap m_data;
};

/* A data message contains a data model
 * associated with the messsage's subject.
 */
class DataMessage : public Message {

public:
	DataMessage() : Message() {
        m_cntType = Message::CNT_MODEL;
        m_data = NULL;
	};
	DataMessage(Message* message) : Message(message) {
        m_data = NULL;
	};
	DataMessage(DataMessage* message) : Message(message) {
		m_data = message->m_data;
	};
	virtual ~DataMessage() {
	}

	void* getData() {
		return m_data;
	}
	void setData(void* data) {
		m_data = data;
	}
    
private:

	void* m_data;
    
    template<class T> friend class Datum;
};

/* In a string message the string data
 * member contains the message contents.
 */
class StringMessage : public Message {

public:
	StringMessage() : Message() {
	}
	StringMessage(Message* message) : Message(message) {
	}
	StringMessage(StringMessage* message) : Message(message) {
		m_data << message->m_data.str();
	};
	virtual ~StringMessage() {
	}

	void* getData() {
		return (void *) m_data.str().c_str();
	}
	void setData(void* data) {
		m_data.clear();
		m_data.str((const char *) data);
	}

	void append(char* buffer, size_t size) {
		m_data.write(buffer, size);
	}

private:
	std::ostringstream m_data;
};

/* A Stream message is a message that will be
 * delivered as collection of callbacks as the
 * streamed data is available.
 */
class StreamMessage : public Message {

public:
	StreamMessage() : Message() {
	};
	StreamMessage(Message* message) : Message(message) {
	}
	StreamMessage(StreamMessage* message) : Message(message) {
		std::list<DataCallbackHandle>* callbacks = &message->m_callbacks;
		m_callbacks.insert(m_callbacks.end(), callbacks->begin(), callbacks->end());
	};
	virtual ~StreamMessage() {
	}

	void setCallback(void* context, DataCallback callback) {
		m_callbacks.push_back(DataCallbackHandle(context, callback));
	}

	bool sendData(MessagePtr messsage, void* buffer, size_t size) {

		bool result = true;

		for (std::list<DataCallbackHandle>::iterator i = m_callbacks.begin(); i != m_callbacks.end(); i++) {

			DataCallbackHandle handle = *i;
			bool callbackResult = handle.callback(handle.context, messsage, buffer, size);
            result = result && callbackResult;
		}

		return result;
	}

private:

	struct DataCallbackHandle {

		DataCallbackHandle(void* context, DataCallback callback)
			: context(context), callback(callback) { }

		void* context;
		DataCallback callback;
	};

	std::list<DataCallbackHandle> m_callbacks;
};

/* A p2p message is a single delivery message
 * that is targeted to a particular provider.
 */
class P2PMessage : public Message, public Listener {

public:

	enum ControlAction {
		NONE,
		SUSPEND,
		RESUME,
		CANCEL,
        REMOVE,
        ADD
	};

public:
	P2PMessage() : Message() {
        m_context = NULL;
        m_callback = NULL;
		m_controlAction = NONE;
	}
	P2PMessage(Message* message) : Message(message) {
        m_context = NULL;
        m_callback = NULL;
		m_controlAction = NONE;
	}
	P2PMessage(P2PMessage* message) : Message(message) {
		m_context = message->m_context;
		m_callback = message->m_callback;
		m_controlAction = NONE;
	};
	virtual ~P2PMessage() {
	}

	ControlAction getControlAction() {
		return m_controlAction;
	}
	const std::string& getTargetMsgId() {
		return m_targetMsgId;
	}
	void setControlAction(ControlAction controlAction, const char* targetMsgId = NULL) {

		m_controlAction = controlAction;
        m_msgType = MSG_P2P_SUB;
        
        if (targetMsgId)
            m_targetMsgId = targetMsgId;
	}
    
    bool hasCallback() {
        return (m_callback != NULL);
    }
	void setCallback(void* context, MessageCallback replyCallback) {

		m_context = context;
		m_callback = replyCallback;
	}

	virtual void onMessage(MessagePtr message) {

		if (m_callback)
			m_callback(m_context, message);
	}

private:
	void* m_context;
	MessageCallback m_callback;

	ControlAction m_controlAction;
    std::string m_targetMsgId;
};
    
/* A p2p Name Value Message
 */
class P2PNVMessage : public P2PMessage {
    
public:
    typedef boost::unordered_map<std::string, MessagePtr> MessageMap;

public:
	P2PNVMessage() : P2PMessage() {
	}
	P2PNVMessage(P2PNVMessage* message) : P2PMessage(message) {
        m_args.insert(message->m_args.begin(), message->m_args.end());
	};
    
    void* getData() {
		return &m_args;
	}
    void setData(void* data) {
        m_args.insert(((NameValueMap *) data)->begin(), ((NameValueMap *) data)->end());
    }
    void* getData(const char* name) {
		return &m_args[name];
	}
    void setData(const char* name, const void* value) {
        m_args[name] = (const char *) value;
	}

private:
    
    NameValueMap m_args;
};
    
/* Template class that provides a type and pointer safe
 * mechanism to access the message data instance.
 */
template <class T>
class Datum {
    
public:
    Datum(MessagePtr message) {
        
        TRACE("Setting up datum shared pointer for message with id: %s", message->getId().c_str());
        message->setCleanupCallback(Datum::CleanupCallback);
        
        void* data = message->getData();
        if (data) {
            m_data = (boost::shared_ptr<T> *) data;
            TRACE("Creating Datum shared pointer: %p[%d]", m_data->get(), m_data->use_count());
        }
    }
    
    boost::shared_ptr<T> getData() {
        return *m_data;
    }
    
    T* operator->() const {
        return m_data->get();
    }
    
    T* operator&() const {
        return m_data->get();
    }
    
    T& operator*() const {
        return *(m_data->get());
    }
    
private:
    
    static void CleanupCallback(Message* message) {
        
        if (message->m_cntType == Message::CNT_MODEL) {
            
            boost::shared_ptr<T>* dataPtr = (boost::shared_ptr<T> *) ((DataMessage *) message)->m_data;
            
            if (dataPtr) {
                
                TRACE("Deleting Datum shared pointer: %p[%d], message id:%s", 
                      dataPtr->get(), dataPtr->use_count(), message->getId().c_str());
                delete dataPtr;
            }
        }
    }
    
    boost::shared_ptr<T>* m_data;
};

/* A Service is an instance of a Listener and Provider. A
 * service listens for a particular type of message whose
 * structure is owned by that service. Services also have
 * a life cycle that is managed by the Message Bus.
 */
class Service : public Provider, public Listener {

public:
	virtual ~Service() { }

	/* Checks if this service is associated with the
	 * given service type identifier.
	 */
	bool isType(const char* type) {
		return (m_types.find(type) != m_types.end());
	}

	/* A service will always be associated
	 * with only one subject and this cannot
	 * be changed over its lifetime.
	 */
	virtual const char* getSubject() = 0;

	/* Called when a service is
	 * added to the MessageBus
	 */
	virtual void intialize() = 0;

	/* Called when a service is
	 * removed from the MessageBus
	 */
	virtual void destroy() = 0;

	/* Called by the MessageBus when the host
	 * application is sent to the background.
	 * The service can use the output stream
	 * to save any state that is required to
	 * resume.
	 */
	virtual void pause(std::ostream* output = NULL) = 0;

	/* Called by the MessageBus when the host
	 * application is brought to the foreground.
	 * The input stream should be used to read
	 * any state that was saved when the service
	 * was paused.
	 */
	virtual void resume(std::istream* input = NULL) = 0;

	/* Dynamic binding configuration for services
	 * that a configured for binding using a xml
	 * binding rule set.
	 */
	virtual void setBindingConfig(binding::DynaModelBindingConfigPtr bindingConfig) { }

	/* Logs state of the service.
	 */
	friend std::ostream &operator<< (std::ostream& cout, const Service& service) {
		cout << "Service with subject \"" << ((Service *) &service)->getSubject() << ": " << std::endl;
		((Service *) &service)->log(cout);
		return cout;
	}

protected:

	void initMessage( Message* message,
		Message::MessageType type = Message::MSG_P2P,
		Message::ContentType cntType = Message::CNT_UNKNOWN,
        const char* subject = NULL ) {

		message->m_subject = (subject ? subject : this->getSubject());
		message->m_msgType = type;
		message->m_cntType = cntType;
	}
	void initMessage( Message* request, Message* response,
		Message::MessageType msgType = Message::MSG_RESP,
        Message::ContentType cntType = Message::CNT_UNKNOWN,
        const char* subject = NULL ) {

		response->m_subject = (subject ? subject : this->getSubject());
		response->m_msgType = msgType;
		response->m_cntType = cntType;
		response->m_dataBinder = request->m_dataBinder;
	}

	void setType(const char* type) {
		m_types.insert(m_types.begin(), type);
	}

	virtual void log(std::ostream& cout) {

		cout << "\tType - ";
		for (boost::unordered_set<std::string>::iterator i = m_types.begin(); i != m_types.end(); i++) {
			cout << *i << "::";
		}
		cout << std::endl;
	}

private:

	boost::unordered_set<std::string> m_types;
};


}  // namespace : mb

#endif /* SERVICE_H_ */
