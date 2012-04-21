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

#ifndef CURLHTTPSERVICE_H_
#define CURLHTTPSERVICE_H_

#include <string>

#include "Number.h"
#include "HttpService.h"
#include "ServiceConfigManager.h"

namespace mb {
	namespace http {


class CurlHttpService : public HttpService {

STATIC_INIT_DECLARATION(CurlHttpService)

public:
	CurlHttpService(const std::string& subject, const std::string& url);
	virtual ~CurlHttpService();

    const char* getSubject() {
        return m_subject.c_str();
    }

    const char* getTemplate() {
        return m_template.c_str();
    }

    Message* createMessage();
    void addEnvVars(std::list<Message::NameValue>& envVars);
    void execute(MessagePtr message, std::string& request);

    // XML Configuration bindings

    static void createService(void* binder, const char* element, std::map<std::string, std::string>& attribs);
    static void initService(void* binder, const char* element, std::map<std::string, std::string>& attribs);

private:

    std::string m_subject;

    std::string m_url;
    int m_timeout;

    HttpMessage::HttpMethod m_method;
    Message::ContentType m_contentType;

    std::string m_template;

    std::list<Message::NameValue> m_headers;

    std::string m_streamSubject;
    std::string m_streamKey;
    std::string m_streamDoNotSnap;
    bool m_subscribeAndSnap;

    Number<bool> m_subscriptionEnabled;
};

STATIC_INIT_CALL(CurlHttpService)

	}  // namespace : http
}  // namespace : mb


#endif /* CURLHTTPSERVICE_H_ */