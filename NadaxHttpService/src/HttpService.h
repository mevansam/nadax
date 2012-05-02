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

#ifndef HTTPSERVICE_H_
#define HTTPSERVICE_H_

#include <string>
#include <iostream>
#include <list>

#include "staticinit.h"
#include "number.h"

#include "ServiceConfigManager.h"
#include "Service.h"


namespace mb {
    namespace http {


/* Optional callback to retrieve request body from caller
 * TODO: Refactor to enable file uploads
 */
typedef void (*GetRequestBodyCallback)(MessagePtr message, std::ostream& buffer);

/* HTTP GET/POST message type. This type of message
   is handled by an HTTP service instance. Such a
   service will typically handle a particular REST,
   SOAP or plain HTTP request/response call to a
   server.
*/
class HttpMessage : public P2PMessage {

public:
	/* HTTP Request Method */
	enum HttpMethod {
		GET,
		POST
	};

public:
	HttpMessage(HttpMethod method) {
		m_method = method;
		m_getBodyCallback = NULL;
	}
	HttpMessage(HttpMessage* message) : P2PMessage(message) {

		m_method = message->m_method;
		m_getBodyCallback = message->m_getBodyCallback;

		std::list<Message::NameValue>* list;

		list = &message->m_headers;
		m_headers.insert(m_headers.end(), list->begin(), list->end());
		list = &message->m_params;
		m_params.insert(m_params.end(), list->begin(), list->end());
		list = &message->m_tmplVars;
		m_tmplVars.insert(m_tmplVars.end(), list->begin(), list->end());
	};
	virtual ~HttpMessage() {
	}

	/* Optional request body callback */
	void setGetBodyCallback(GetRequestBodyCallback getBodyCallback) {
		m_getBodyCallback = getBodyCallback;
	}

	/* Http Method */
	HttpMethod getMethod() {
		return m_method;
	}

	/* Default data is request param */
    void* getData(const char* name) {
        
        std::list<Message::NameValue>::iterator param;
        std::list<Message::NameValue>::iterator end = m_params.end();

        for (param = m_params.begin(); param != end; param++) {
            
            if (param->name == name)
                return &param->name;
        }
		return NULL;
	}
	void setData(const char* name, const void* value) {
        
        std::list<Message::NameValue>::iterator param;
        std::list<Message::NameValue>::iterator end = m_params.end();
        
        for (param = m_params.begin(); param != end; param++) {
            
            if (param->name == name)
                m_params.erase(param);
        }
		m_params.push_back(Message::NameValue(name, (const char *) value));
	}

	/* Request header */
	void setHeader(const char* name, const char* value) {
		m_headers.push_back(Message::NameValue(name, value));
	}
	std::list<Message::NameValue>& getHeaders() {
		return m_headers;
	}
	/* Request get/post param */
	void setParam(const char* name, const char* value) {
		m_params.push_back(Message::NameValue(name, value));
	}
	std::list<Message::NameValue>& getParams() {
		return m_params;
	}
	/* Request template variable */
	void setTmplVar(const char* name, const char* value) {
		m_tmplVars.push_back(Message::NameValue(name, value));
	}
	std::list<Message::NameValue>& getTmplVars() {
		return m_tmplVars;
	}

private:

	HttpMethod m_method;

	std::list<Message::NameValue> m_headers;
	std::list<Message::NameValue> m_params;
	std::list<Message::NameValue> m_tmplVars;

	GetRequestBodyCallback m_getBodyCallback;
};

/* A HTTP Service class implements an HTTP service endpoint.
 */
class HttpService : public Service
{

STATIC_INIT_DECLARATION(HttpService)

public:
	HttpService(const std::string& subject, const std::string& url);
	virtual ~HttpService();
    
	virtual Message* createMessage() {
		Message* message = new HttpMessage(HttpMessage::POST);
		Service::initMessage(message, Message::MSG_P2P, Message::CNT_XML);
		return message;
	}

	virtual const char* getSubject() {
        return m_subject.c_str();
    }

    void intialize();
    void destroy();
    
	virtual void start() { };
	virtual void stop() { };

	virtual void pause(std::ostream* output = NULL) { };
	virtual void resume(std::istream* input = NULL) { };

    void onMessage(MessagePtr message);
    
    virtual const char* getTemplate() {
        return m_template.c_str();
    }

    virtual void addEnvVars(std::list<Message::NameValue>& envVars) = 0;
    
    virtual void execute(MessagePtr message, std::string& request) = 0;

    // XML Configuration bindings

    static void initService(void* binder, const char* element, std::map<std::string, std::string>& attribs);
    static void addHeader(void* binder, const char* element, std::map<std::string, std::string>& attribs);
    static void addRequestTemplate(void* binder, const char* element, const char* body);

protected:

    void log(std::ostream& cout);

    std::string m_subject;
    std::string m_url;
    int m_timeout;

    HttpMessage::HttpMethod m_method;
    Message::ContentType m_contentType;

    std::string m_template;

    std::list<Message::NameValue> m_envVars;
    std::list<std::string> m_templateTokens;

    std::list<Message::NameValue> m_headers;

    std::string m_streamSubject;
    std::string m_streamKey;
    std::string m_streamDoNotSnap;
    bool m_subscribeAndSnap;

    Number<bool> m_subscriptionEnabled;

};

STATIC_INIT_CALL(HttpService)


    }  // namespace : http
}  // namespace : mb


#endif /* HTTPSERVICE_H_ */
