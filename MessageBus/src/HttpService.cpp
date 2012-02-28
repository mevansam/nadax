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


// **** HttpService Implementation ****

HttpService::HttpService() {
}

HttpService::~HttpService() {
}

void HttpService::intialize() {
    
    std::string tmpl = this->getTemplate();
    
    size_t len = tmpl.length();
    size_t k, j, i = 0;
    
    while (len) {
        
        if ((j = tmpl.find(TOKEN_BEGIN, i)) == std::string::npos) {
            
            m_template.push_back(tmpl.substr(i, len));
            break;
        }
        if ((k = tmpl.find(TOKEN_END, j + 2)) == std::string::npos) {
            
            m_template.push_back(tmpl.substr(i, len));
            break;
        }
        
        // Template characters
        m_template.push_back(tmpl.substr(i, j - i));
        // Template variable name
        m_template.push_back(tmpl.substr(j + 2, k - j - 2));
        
        i = len - (k - i + 2);
        i = k + 2;
    }
    
#ifdef LOG_LEVEL_TRACE
    std::ostringstream output;
    
    bool isVar = false;
    std::list<std::string>::iterator t;
    
    output << std::endl << std::endl << "\tTemplate tokens for service " << this->getSubject() << " : " << std::endl;
    for (t = m_template.begin(); t != m_template.end(); t++) {
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
        for (j = m_template.begin(); j != m_template.end(); j++) {
            
            if (isVar)
                output << variables[*j];
            else
                output << *j;
            
            isVar = !isVar;
        }

        std::string result(output.str());
        this->execute(message, result);
        
    } catch (...) {
        
        ERROR( 
            "Caught unknown exception while handling message for HTTP service with subject '%s'.", 
            this->getSubject() );
    }
}


	}  // namespace : http
}  // namespace : mb
