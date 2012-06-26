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

#include "HttpService.h"

#include "DynaModel.h"
#include "MessageBusManager.h"

#define TOKEN_BEGIN  "{{"
#define TOKEN_END    "}}"


namespace mb {
    namespace http {


STATIC_INIT_NULL_IMPL(HttpService)

// **** HttpService Implementation ****

HttpService::HttpService(const std::string& subject, const std::string& url) {

	Service::setType("http");

	m_subject = subject;
	m_url = url;
	m_timeout = 10;

	m_method = http::HttpMessage::GET;
	m_contentType = Message::CNT_UNKNOWN;

    m_streamDoNotSnap = CSTR_TRUE;

    m_subscriptionEnabled = false;
    m_subscribeAndSnap =false;
}

HttpService::~HttpService() {
}

void HttpService::intialize() {
    
    std::string tmpl = this->getTemplate();
    
    size_t len = tmpl.length();
    size_t k, j, i = 0;
    
    while (i < len) {
        
        if ((j = tmpl.find(TOKEN_BEGIN, i)) == std::string::npos) {
            
        	m_templateTokens.push_back(tmpl.substr(i, len));
            break;
        }
        if ((k = tmpl.find(TOKEN_END, j + 2)) == std::string::npos) {
            
        	m_templateTokens.push_back(tmpl.substr(i, len));
            break;
        }
        
        // Template characters
        m_templateTokens.push_back(tmpl.substr(i, j - i));
        // Template variable name
        m_templateTokens.push_back(tmpl.substr(j + 2, k - j - 2));
        
        i = k + 2;
    }
    
#ifdef LOG_LEVEL_TRACE
    std::ostringstream output;
    
    bool isVar = false;
    std::list<std::string>::iterator t;
    
    output << std::endl << std::endl << "\tTemplate tokens for service " << this->getSubject() << " : " << std::endl;
    for (t = m_templateTokens.begin(); t != m_templateTokens.end(); t++) {
        output << "\t\tTemplate " << (isVar ? "variable" : "characters") << " : " << *t << std::endl;
        isVar = !isVar;
    }
    
    TRACE(output.str().c_str());
#endif

	this->start();
}

void HttpService::destroy() {

	this->stop();
}

void HttpService::onMessage(MessagePtr message) {
    
    MessageBusManager* manager = MessageBusManager::instance();
    http::HttpMessage* httpMessage = (http::HttpMessage *) message.get();

    P2PMessage::ControlAction controlAction = httpMessage->getControlAction();

    bool isControlNone = (controlAction == P2PMessage::NONE);
    bool isSubscription = (httpMessage->getType() == Message::MSG_P2P_SUB);
    bool isFirstPost = (httpMessage->getPostCount() == 0);

    if (!m_subscriptionEnabled && !isFirstPost)
        return;

    const char* respSubject = (httpMessage->getRespSubject().length() > 0 ? httpMessage->getRespSubject().c_str() : NULL);

    MessagePtr completion;

	if (isSubscription && m_streamSubject.length() > 0) {

		// Send a subscription call to the streaming service

		MessagePtr subRequest = manager->createMessage(m_streamSubject.c_str());
		NameValueMap& metaData = subRequest->getMetaData();
		metaData[DO_NOT_SNAP] = m_streamDoNotSnap;

		if (respSubject)
			subRequest->setRespSubject(respSubject);
		else
			subRequest->setRespSubject(message->getSubject().c_str());

		TRACE("Initiating subscription on service '%s'.", subRequest->getSubject().c_str());

		if (m_streamKey.length() > 0) {

			std::list<Message::NameValue> params = httpMessage->getParams();
			std::list<Message::NameValue>::iterator param;

			for (param = params.begin(); param != params.end(); param++) {

				if (m_streamKey == param->name) {

					TRACE("The subscription key data is: %s", param->value.c_str());

					subRequest->setData(param->name.c_str(), (void *) param->value.c_str());
					break;
				}
			}
		}

		((P2PMessage *) subRequest.get())->setControlAction(controlAction);

		if (!isFirstPost || !isControlNone || m_subscribeAndSnap) {

			MessagePtr subResponse = manager->sendMessage(subRequest);
			if (!subResponse->getError()) {

				Service::initMessage(
					message.get(),
					subResponse.get(),
					subResponse->getType(),
					subResponse->getContentType(),
					respSubject );

				if (m_subscribeAndSnap) {

					// If subscription just activated then continue and snap, else
					// return as subscription will stream subsequent updates.
					if (subResponse->getMetaData()[SUBSCRIPTION_RESULT_CODE] == SUBSCRIPTION_RESULT_ACTIVE) {

						POST_RESPONSE(subResponse, message);
						return;
					}

				} else {

					TRACE( "Streaming subscription is active. Continuing without polling for service '%s'",
						subRequest->getSubject().c_str() );

					POST_RESPONSE(subResponse, message);
					return;
				}
			}

		} else {

			// If this is the first message and not a control message
			// and "subscribe asap" is false, then the subcription
			// request to the streaming service should be sent
			// after the snap request/response is completed. So
			// the message is passed on to the http response handler
			// which posts this message once the http response
			// is complete.

			completion = subRequest;
		}
	}

	// Only messages that do not have a control action associated
	// with them should continue to be processed as a request response,
	// as control actions are subscription specific.
	if (!isControlNone)
		return;

	MessagePtr response(new StreamMessage());
	Service::initMessage(
		message.get(),
		response.get(),
		Message::MSG_RESP_STREAM,
		Message::CNT_UNKNOWN,
		respSubject );

	NameValueMap& metaData = response->getMetaData();

	if (Service::hasDynaModelBindingConfig() && message->hasBinder() == 1) {

		metaData[DATA_IS_DYNA_MODEL] = "true";

		// Sets up clean up of data when message is deleted
		Datum<binding::DynaModel> datum(response);

	} else
		metaData[DATA_IS_DYNA_MODEL] = "false";

	metaData[REQUEST_ID] = httpMessage->getId();

	if (isSubscription && isFirstPost) {
		// Return a subscription ID only with the first
		// response message. The receiver needs to keep
		// track of this so they can control subscriptions.
		metaData[SUBSCRIPTION_ID] = httpMessage->getId();
	}

	if (POST_RESPONSE(response, message) > 0) {

        std::ostringstream output;

        // Build request body

		try {

            NameValueMap variables;
            std::list<Message::NameValue>::iterator i;

            std::list<Message::NameValue> params = httpMessage->getParams();
            for (i = params.begin(); i != params.end(); i++) {
                variables[i->name] = i->value;
            }

            std::list<Message::NameValue> tmplVars = httpMessage->getTmplVars();
            for (i = tmplVars.begin(); i != tmplVars.end(); i++) {
                variables[i->name] = i->value;
            }

            ServiceConfigManager* scm = ServiceConfigManager::instance();

            bool isVar = false;
            std::list<std::string>::iterator j;
            for (j = m_templateTokens.begin(); j != m_templateTokens.end(); j++) {

                if (isVar) {

                	if (variables.find(*j) != variables.end()) {

                		output << variables[*j];

                	} else {

                		std::string value;
                		if (scm->lookupTokenValue(*j, value))
                    		output << value;
                		else
                			output << TOKEN_BEGIN << *j << TOKEN_END;
                	}

                } else
                    output << *j;

                isVar = !isVar;
            }

	    } catch (...) {

	        ERROR( "Error while formating request body for HTTP service with subject '%s'.",
	            this->getSubject() );

            response->setError(Message::ERR_CONNECTION_ERROR, 1, "Error formatting request body.");
            SEND_DATA(response, NULL, 0);
	    }

		std::string result(output.str());
		this->execute(message, result);

	} else
		SEND_DATA(response, NULL, 0);
}

void HttpService::log(std::ostream& cout) {

	Service::log(cout);

	cout << "\tURL - " << m_url << std::endl;
	cout << "\tTimeout - " << m_timeout << std::endl;

	cout << "\tHTTP Method - ";
	switch (m_method) {
		case http::HttpMessage::GET:
			cout << "GET";
			break;
		case http::HttpMessage::POST:
			cout << "POST";
			break;
		default:
			cout << "UNKNOWN";
			break;
	}
	cout << std::endl;

	cout << "\tContent Type - ";
	switch (m_contentType) {
		case Message::CNT_XML:
			cout << "text/xml";
			break;
		case Message::CNT_JSON:
			cout << "application/json";
			break;
		default:
			cout << "UNKNOWN";
			break;
	}
	cout << std::endl;

	cout << "\tSubscription enabled - " << (m_subscriptionEnabled ? 'Y' : 'N') << std::endl;
	cout << "\tSubscribe and snap - " << (m_subscribeAndSnap ? 'Y' : 'N') << std::endl;
	cout << "\tSnap override - " << m_streamDoNotSnap << std::endl;

	cout << "\tHeaders - " << std::endl;
	for (std::list<Message::NameValue>::iterator i = m_headers.begin(); i != m_headers.end(); i++) {
		cout << "\t\t" << i->name << '=' << i->value << std::endl;
	}

	cout << std::endl;
	cout << "**** Begin Request Template =>" << std::endl;
	cout << m_template << std::endl;
	cout << "<= End Request Template ****" << std::endl << std::endl;
}

// Configuration callbacks

ADD_BEGIN_CONFIG_BINDING("messagebus-config/service/httpConfig", HttpService, initService);
void HttpService::initService(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

    GET_BINDER(mb::ServiceConfig);
    Service* service = (Service *) dataBinder->getService();

    if (service->isType("http")) {

        HttpService* httpService = (HttpService *) service;
		std::map<std::string, std::string>::iterator attribsEnd = attribs.end();

		if (attribs.find("timeout") != attribsEnd)
			httpService->m_timeout = atoi(attribs["timeout"].c_str());
		if (attribs.find("httpMethod") != attribsEnd) {

			std::string method = attribs["httpMethod"];
			httpService->m_method = (method == "GET" ? http::HttpMessage::GET : http::HttpMessage::POST);

		} if (attribs.find("contentType") != attribsEnd) {

			std::string contentType = attribs["contentType"];
			httpService->m_contentType = (
				contentType == "text/xml" ? Message::CNT_XML :
				contentType == "application/json" ? Message::CNT_JSON : Message::CNT_UNKNOWN );
		}
    }
}

ADD_BEGIN_CONFIG_BINDING("messagebus-config/service/headers/header", HttpService, addHeader);
void HttpService::addHeader(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

    GET_BINDER(mb::ServiceConfig);
    Service* service = (Service *) dataBinder->getService();

    if (service->isType("http"))
    	((HttpService *) service)->m_headers.push_back(Message::NameValue(attribs["name"], attribs["value"]));
}

ADD_END_CONFIG_BINDING("messagebus-config/service/requestTemplate", HttpService, addRequestTemplate);
void HttpService::addRequestTemplate(void* binder, const char* element, const char* body) {

    GET_BINDER(mb::ServiceConfig);
    Service* service = (Service *) dataBinder->getService();

    if (service->isType("http"))
    	((HttpService *) service)->m_template = body;
}


	}  // namespace : http
}  // namespace : mb
