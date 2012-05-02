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

#include "CurlHttpService.h"


namespace mb {
	namespace http {

STATIC_INIT_NULL_IMPL(CurlHttpService)

CurlHttpService::CurlHttpService(const std::string& subject, const std::string& url) {

	m_subject = subject;
	m_url = url;

    m_streamDoNotSnap = CSTR_TRUE;

    m_subscriptionEnabled = false;
    m_subscribeAndSnap =false;
}

CurlHttpService::~CurlHttpService() {
}

mb::Message* CurlHttpService::createMessage() {

	return NULL;
}

void CurlHttpService::addEnvVars(std::list<Message::NameValue>& envVars) {
}

void CurlHttpService::execute(MessagePtr message, std::string& request) {
}

void CurlHttpService::log(std::ostream& cout) {

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

ADD_BEGIN_CONFIG_BINDING(createService, "messagebus-config/service", CurlHttpService::createService);
void CurlHttpService::createService(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

    GET_BINDER(mb::ServiceConfig);

    std::string serviceName = attribs["name"];
    std::string serviceUrl = attribs["url"];
    std::string serviceType = attribs["type"];

    if (serviceType == "curlhttp") {

    	TRACE("Found CURL HTTP service configuration '%s' for url '%s'.", serviceName.c_str(), serviceUrl.c_str());
    	dataBinder->addService(new CurlHttpService(serviceName, serviceUrl));
    }
}

ADD_BEGIN_CONFIG_BINDING(initService, "messagebus-config/service/httpConfig", CurlHttpService::initService);
void CurlHttpService::initService(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

    GET_BINDER(mb::ServiceConfig);
    CurlHttpService* service = (CurlHttpService *) dataBinder->getService();

    std::map<std::string, std::string>::iterator attribsEnd = attribs.end();

    if (attribs.find("timeout") != attribsEnd)
    	service->m_timeout = atoi(attribs["timeout"].c_str());
    if (attribs.find("httpMethod") != attribsEnd) {

    	std::string method = attribs["httpMethod"];
        service->m_method = (method == "GET" ? http::HttpMessage::GET : http::HttpMessage::POST);

    } if (attribs.find("contentType") != attribsEnd) {

    	std::string contentType = attribs["contentType"];
    	service->m_contentType = (
    		contentType == "text/xml" ? Message::CNT_XML :
    		contentType == "application/json" ? Message::CNT_JSON : Message::CNT_UNKNOWN );
    }
}

ADD_BEGIN_CONFIG_BINDING(addHeader, "messagebus-config/service/headers/header", CurlHttpService::addHeader);
void CurlHttpService::addHeader(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

    GET_BINDER(mb::ServiceConfig);
    CurlHttpService* service = (CurlHttpService *) dataBinder->getService();

    service->m_headers.push_back(Message::NameValue(attribs["name"], attribs["value"]));
}

ADD_END_CONFIG_BINDING(addRequestTemplate, "messagebus-config/service/requestTemplate", CurlHttpService::addRequestTemplate);
void CurlHttpService::addRequestTemplate(void* binder, const char* element, const char* body) {

    GET_BINDER(mb::ServiceConfig);
    CurlHttpService* service = (CurlHttpService *) dataBinder->getService();

    service->m_template = body;
}


    }  // namespace : http
}  // namespace : mb
