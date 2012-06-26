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

#include "openssl_certs.h"
#include "staticinit.h"
#include "log.h"
#include "executor.h"

#include "OpenSSLInit.h"
#include "HttpService.h"


#define DEFAULT_POOL_SIZE            5
#define DEFAULT_POOL_MAX             10
#define DEFAULT_POOL_TIMEOUT         60000
#define DEFAULT_POOL_EVICT_INTERVAL  30000
#define DEFAULT_POOL_LINGER_TIME     30000
#define DEFAULT_POOL_EVICT_CHECKS    -1


namespace mb {
	namespace http {


std::string _proxyHost;
long _proxyPort;


int curlTrace(CURL* curl, curl_infotype infotype, char* text, size_t len, void* userdata);


class HttpConnection {

public:
	HttpConnection() {

		m_curlHandle = curl_easy_init();
		if (m_curlHandle) {

		    curl_easy_setopt(m_curlHandle, CURLOPT_ERRORBUFFER, m_error);
		    curl_easy_setopt(m_curlHandle, CURLOPT_NOSIGNAL, 1L);
		    curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGFUNCTION, curlTrace);
		    curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 1L);

		    if (_proxyHost.size()) {
				curl_easy_setopt(m_curlHandle, CURLOPT_PROXY, _proxyHost.c_str());
				curl_easy_setopt(m_curlHandle, CURLOPT_PROXYPORT, _proxyPort);
		    }

#ifdef SKIP_PEER_VERIFICATION
			/*
			 * If you want to connect to a site who isn't using a certificate that is
			 * signed by one of the certs in the CA bundle you have, you can skip the
			 * verification of the server's certificate. This makes the connection
			 * A LOT LESS SECURE.
			 *
			 * If you have a CA cert for the server stored someplace else than in the
			 * default bundle, then the CURLOPT_CAPATH option might come handy for
			 * you.
			 */
			curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
#else
			curl_easy_setopt(m_curlHandle, CURLOPT_CAPATH, __CACERTS_PATH);
#endif

#ifdef SKIP_HOSTNAME_VERFICATION
			/*
			 * If the site you're connecting to uses a different host name that what
			 * they have mentioned in their server certificate's commonName (or
			 * subjectAltName) fields, libcurl will refuse to connect. You can skip
			 * this check, but this will make the connection less secure.
			 */
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
		}
	}

	~HttpConnection() {

		if (m_curlHandle)
			curl_easy_cleanup(m_curlHandle);
	}

	void initialize() {

	}

	void operator() () {

	}

	void finalize() {

	}

public:
	CURL* m_curlHandle;

	int m_maxCachedConnections;

	char m_error[CURL_ERROR_SIZE];
};


class HttpConnectionPool : public ObjectPool<HttpConnection> {

public:
	HttpConnectionPool(int maxCachedConnections) {

		m_maxCachedConnections = maxCachedConnections;
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

private:
	int m_maxCachedConnections;
};


int curlTrace(CURL* curl, curl_infotype infotype, char* text, size_t len, void* userdata) {

	TRACE("cURL: %s", text);
	return 0;
}


boost::shared_ptr<Executor> _executor;
boost::shared_ptr<HttpConnectionPool> _pool;


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

ADD_BEGIN_CONFIG_BINDING("messagebus-config/curlhttpservice", CurlHttpService, configureServices);
void CurlHttpService::configureServices(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

	// cURL handle pool configuration
	int size = DEFAULT_POOL_SIZE;
	int max = DEFAULT_POOL_MAX;
	int timeout = DEFAULT_POOL_TIMEOUT;
	int evictInterval = DEFAULT_POOL_EVICT_INTERVAL;
	int lingerTime = DEFAULT_POOL_LINGER_TIME;
	int evictChecks = DEFAULT_POOL_EVICT_CHECKS;

	// cURL internal cache size (number of connections
	// cached within each cURL handle defaults to 5)
	int maxCachedConnections = -1;

	if (attribs.find("poolSize") != attribs.end())
		size = atoi(attribs["poolSize"].c_str());
	if (attribs.find("poolMax") != attribs.end())
		max = atoi(attribs["poolMax"].c_str());
	if (attribs.find("poolTimeout") != attribs.end())
		timeout = atoi(attribs["poolTimeout"].c_str());
	if (attribs.find("poolEvictInterval") != attribs.end())
		evictInterval = atoi(attribs["poolEvictInterval"].c_str());
	if (attribs.find("poolLingerTime") != attribs.end())
		lingerTime = atoi(attribs["poolLingerTime"].c_str());
	if (attribs.find("poolEvictChecks") != attribs.end())
		evictChecks = atoi(attribs["poolEvictChecks"].c_str());
	if (attribs.find("maxCachedConnections") != attribs.end())
		maxCachedConnections = atoi(attribs["maxCachedConnections"].c_str());

	_pool = boost::shared_ptr<HttpConnectionPool>(new HttpConnectionPool(maxCachedConnections));
	_pool->setPoolSize(size, max, timeout);
	_pool->setPoolManagement(evictInterval, lingerTime, evictChecks);

	if (attribs.find("concurrency") != attribs.end())
		// Concurrency is the number of threads the executor will make available for
		// asynchronous execution. The executor does not grow the thread count but will
		// instead queue work items if all threads are busy.
		_executor = boost::shared_ptr<Executor>(new Executor((size_t) atoi(attribs["concurrency"].c_str())));
	else
		_executor = boost::shared_ptr<Executor>(new Executor(size));

	if (attribs.find("proxyHost") != attribs.end())
		_proxyHost = atoi(attribs["poolSize"].c_str());
	if (attribs.find("proxyPort") != attribs.end())
		_proxyPort = atoi(attribs["proxyPort"].c_str());
}

ADD_BEGIN_CONFIG_BINDING("messagebus-config/service", CurlHttpService, createService);
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
