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

	this->addEnvVars(m_envVars);
	this->start();
}

void HttpService::destroy() {

	this->stop();
}

void HttpService::onMessage(MessagePtr message) {
    
    try {
        
        http::HttpMessage* httpMessage = (http::HttpMessage *) message.get();

        NameValueMap variables;
        std::list<Message::NameValue>::iterator i;

        for (i = m_envVars.begin(); i != m_envVars.end(); i++) {
            variables[i->name] = i->value;
        }

        std::list<Message::NameValue> params = httpMessage->getParams();
        for (i = params.begin(); i != params.end(); i++) {
            variables[i->name] = i->value;
        }

        std::list<Message::NameValue> tmplVars = httpMessage->getTmplVars();
        for (i = tmplVars.begin(); i != tmplVars.end(); i++) {
            variables[i->name] = i->value;
        }

        std::ostringstream output;
        
        bool isVar = false;
        std::list<std::string>::iterator j;
        for (j = m_templateTokens.begin(); j != m_templateTokens.end(); j++) {
            
            if (isVar)
                output << variables[*j];
            else
                output << *j;
            
            isVar = !isVar;
        }

        std::string result(output.str());
        this->execute(message, result);
        
    } catch (...) {
        
        ERROR( "Caught unknown exception while handling message for HTTP service with subject '%s'.",
            this->getSubject() );
    }
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
		cout << "\t\t" << i->value << '=' << i->name << std::endl;
	}

	cout << std::endl;
	cout << "**** Begin Request Template =>" << std::endl;
	cout << m_template << std::endl;
	cout << "<= End Request Template ****" << std::endl << std::endl;
}

// Configuration callbacks

ADD_BEGIN_CONFIG_BINDING(initService, "messagebus-config/service/httpConfig", HttpService::initService);
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

ADD_BEGIN_CONFIG_BINDING(addHeader, "messagebus-config/service/headers/header", HttpService::addHeader);
void HttpService::addHeader(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

    GET_BINDER(mb::ServiceConfig);
    Service* service = (Service *) dataBinder->getService();

    if (service->isType("http"))
    	((HttpService *) service)->m_headers.push_back(Message::NameValue(attribs["name"], attribs["value"]));
}

ADD_END_CONFIG_BINDING(addRequestTemplate, "messagebus-config/service/requestTemplate", HttpService::addRequestTemplate);
void HttpService::addRequestTemplate(void* binder, const char* element, const char* body) {

    GET_BINDER(mb::ServiceConfig);
    Service* service = (Service *) dataBinder->getService();

    if (service->isType("http"))
    	((HttpService *) service)->m_template = body;
}


	}  // namespace : http
}  // namespace : mb
