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

#include "curl/curl.h"

#include "staticinit.h"
#include "log.h"
#include "executor.h"

#include "OpenSSLInit.h"
#include "HttpService.h"


namespace mb {
	namespace http {


class HttpConnection {

public:
	HttpConnection() {

		m_curlHandle = curl_easy_init();
	}

	~HttpConnection() {

		if (m_curlHandle)
			curl_easy_cleanup(m_curlHandle);
	}

	void initialize() {

	}

	void operator()() {

	}

	void finalize() {

	}

public:
	CURL* m_curlHandle;
};


class HttpConnectionPool : public ObjectPool<HttpConnection> {

public:
	virtual ~HttpConnectionPool() {
	}

protected:

	virtual HttpConnection* create() {

		HttpConnection* connection = new HttpConnection();
		if (!connection->m_curlHandle) {

			FATAL("Unable to create cURL handle.");
			delete connection;
			return NULL;

		} else
			return connection;

	}

	virtual void activate(HttpConnection* object) {

	}

	virtual void passivate(HttpConnection* object) {

	}
};


STATIC_INIT_NULL_IMPL(CurlHttpService)

CurlHttpService::CurlHttpService(const std::string& subject, const std::string& url)
	: HttpService(subject, url) {

	Service::setType("curl");
}

CurlHttpService::~CurlHttpService() {
}

mb::Message* CurlHttpService::createMessage() {

	return NULL;
}

void CurlHttpService::execute(MessagePtr message, std::string& request) {
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


    }  // namespace : http
}  // namespace : mb
