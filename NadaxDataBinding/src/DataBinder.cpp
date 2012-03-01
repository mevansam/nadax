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

#include "DataBinder.h"

#include <stdlib.h>
#include <iostream>
#include <iomanip>

#include "exception.h"
#include "log.h"


namespace binding {


class BindingException : public CException {
public:
	BindingException(const char* source, int lineNumber) : CException(source, lineNumber) { }
    virtual ~BindingException() { }
};

DataBinder::DataBinder() {

    m_binding = false;
    
    m_trimBody = true;
    m_addTextToBody = true;
    m_bodyIsCData = false;

    m_root = NULL;
}

DataBinder::~DataBinder() {
}
    
bool DataBinder::lock() {
    
    if (m_bindingLock.try_lock()) {
        
        if (m_binding) {
            
            m_bindingLock.unlock();
            return false;
            
        } else {
            
            m_binding = true;
            m_bindingLock.unlock();
            
            return true;
        }
        
    } else
        return false;
}

void DataBinder::reset() {

	m_path.reset();

    m_body.clear();
    m_body.str("");

    m_trimBody = true;
    m_addTextToBody = true;
    m_bodyIsCData = false;

    m_root = NULL;
    m_variables.clear();
    
    { boost::shared_lock<boost::shared_mutex> lock(m_bindingLock);
        
        m_binding = false;
    }
}

void DataBinder::addBeginRule(const char* pathStr, BeginElementCallback callback) {

	Path path(pathStr);
	std::list<BeginRule>* rules = &m_beginRules[path.leaf()];
	rules->push_back(BeginRule(pathStr, callback));
}

void DataBinder::addEndRule(const char* pathStr, EndElementCallback callback) {

	Path path(pathStr);
	std::list<EndRule>* rules = &m_endRules[path.leaf()];
	rules->push_back(EndRule(pathStr, callback));
}

void DataBinder::startElement(const char* name, const char** attribs) {

	const char* elemName = strchr(name, ':');
	elemName = (elemName ? elemName + 1 : name);

    m_path.push(elemName);

    TRACE("Begin parsing Element at path: %s", m_path.str());

    m_body.clear();
    m_body.str("");

    if (!m_path.isTagged())
    {
    	std::ostringstream attribPath;
    	std::string attribPathName;
        std::map<std::string, std::string> attribMap;

		const char* attribName;
		const char* attribValue;

		for (int i = 0; attribs[i]; i += 2) {

    		attribName = attribs[i];
    		attribValue = attribs[i + 1];

    		attribMap.insert(std::pair<std::string, std::string>(attribName, attribValue));
		}

    	if (m_beginRules.find(elemName) != m_beginRules.end()) {

        	std::list<BeginRule>* rules = &m_beginRules[elemName];
        	std::list<BeginRule>::iterator end = rules->end();

        	for (std::list<BeginRule>::iterator rule = rules->begin(); rule != end; rule++) {

    			m_rulePath = &rule->path;
        		if (m_path == *m_rulePath) {
        			TRACE("Triggering begin binding handler for: %s", m_path.str());
        			rule->callback(this, elemName, attribMap);
        		}
    			m_rulePath = NULL;
        	}
    	}

		for (int i = 0; attribs[i]; i += 2) {
            
            attribName = attribs[i];
            attribValue = attribs[i + 1];

    		attribPath << '@' << attribName;
    		attribPathName = attribPath.str();
    		m_path.push(attribPathName.c_str());

    		if (m_beginRules.find(attribPathName) != m_beginRules.end()) {

        		std::list<BeginRule>* rules = &m_beginRules[attribPathName];
            	std::list<BeginRule>::iterator end = rules->end();

            	for (std::list<BeginRule>::iterator rule = rules->begin(); rule != end; rule++) {

        			m_rulePath = &rule->path;
            		if (m_path == *m_rulePath) {
            		    TRACE("Triggering begin binding handler for attribute path: %s with value %s", m_path.str(), attribValue);
            			rule->callback(this, attribName, attribMap);
            		}
        			m_rulePath = NULL;
            	}
    		}

    		if (m_endRules.find(attribPathName) != m_endRules.end()) {

        		std::list<EndRule>* rules = &m_endRules[attribPathName];
            	std::list<EndRule>::iterator end = rules->end();

            	for (std::list<EndRule>::iterator rule = rules->begin(); rule != end; rule++) {

        			m_rulePath = &rule->path;
            		if (m_path == *m_rulePath) {
            		    TRACE("Triggering end binding handler for attribute path: %s with value %s", m_path.str(), attribValue);
            			rule->callback(this, attribName, attribValue);
            		}
        			m_rulePath = NULL;
            	}
    		}

    		m_path.pop();
    		attribPath.clear();
    		attribPath.str("");
    	}
    }
}

void DataBinder::endElement(const char* name) {

	const char* elemName = strchr(name, ':');
	elemName = (elemName ? elemName + 1 : name);

    if (!m_path.isTagged())
    {
    	if (m_endRules.find(elemName) != m_endRules.end()) {

    		std::list<EndRule>* rules = &m_endRules[elemName];
        	std::list<EndRule>::iterator end = rules->end();

        	std::string body = m_body.str();
        	int len = body.length();
    		char* text = (char *) body.c_str();

    		if (m_trimBody && !m_bodyIsCData) {

        		// Trim leading spaces
    			while (len > 0) {

    				if (*text==' ' || *text=='\t' || *text=='\r' || *text =='\n') {
    					text++;
    					len--;
    				} else {
    					break;
    				}
    			}
    			if (len > 0) {

    				// Trim trailing spaces
    				char* last = ((char* ) text) + len - 1;
    				while (len > 0) {

    					if (*last==' ' || *last=='\t' || *last=='\r' || *last =='\n') {
    						last--;
    						len--;
    					} else {
    						break;
    					}
    				}

    				*(text + len) = 0;
    			}
    		}

        	for (std::list<EndRule>::iterator rule = rules->begin(); rule != end; rule++) {

    			m_rulePath = &rule->path;
        		if (m_path == *m_rulePath) {
        		    TRACE("Triggering end binding handler for: %s with body %s", m_path.str(), text);
        			rule->callback(this, elemName, text);
        		}
    			m_rulePath = NULL;
        	}
    	}
    }

    m_addTextToBody = true;
    m_bodyIsCData = false;

    m_body.clear();
    m_body.str("");

    TRACE("End parsing Element at path: %s", m_path.str());

    m_path.pop();
}

void DataBinder::characters(const char* text, int len) {

	if (m_addTextToBody)
		m_body.write(text, len);
}

void DataBinder::startCDataSection() {

    m_body.clear();
    m_body.str("");
}
void DataBinder::endCDataSection() {

	m_addTextToBody = false;
	m_bodyIsCData = true;
}

void DataBinder::debug(const char* msg) {

	std::cout << std::endl;
	std::cout << "Debug output for DataBinder instance '" << msg;
	std::cout << "' : " << std::endl;

	__gnu_cxx::hash_map<std::string, std::list<BeginRule>, hashstr, eqstr>::iterator i;
	std::list<BeginRule>::iterator j;

	std::cout << "  Begin Rules: " << std::endl;
	for (i = m_beginRules.begin(); i != m_beginRules.end(); i++) {

		std::cout << "    Element Name: " << i->first << std::endl;
		for (j = i->second.begin(); j != i->second.end(); j++) {
			std::cout << "      Trigger On: " << j->path;
		}
	}

	__gnu_cxx::hash_map<std::string, std::list<EndRule>, hashstr, eqstr>::iterator k;
	std::list<EndRule>::iterator l;

	std::cout << std::endl << "  End Rules: " << std::endl;
	for (k = m_endRules.begin(); k != m_endRules.end(); k++) {

		std::cout << "    Element Name: " << k->first << std::endl;
		for (l = k->second.begin(); l != k->second.end(); l++) {
			std::cout << "      Trigger On: " << l->path;
		}
	}

	std::cout << std::endl;
}


}  // namespace : binding
