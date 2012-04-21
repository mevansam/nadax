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

#ifndef DATABINDER_H_
#define DATABINDER_H_

#include <list>
#include <map>
#include "boost/unordered_map.hpp"
#include <string>
#include <sstream>

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"

#include "Path.h"

#define GET_BINDER(BinderType) \
	BinderType* dataBinder = (BinderType *) binder;
#define GET_BINDING_ROOT(name, RootType) \
	RootType* name = (RootType *) dataBinder->getRoot();


namespace binding {


class DataBinder;
typedef boost::shared_ptr<DataBinder> DataBinderPtr;

    
typedef void (*BeginElementCallback)(void* m_binder, const char *element, std::map<std::string, std::string>& attribs);
typedef void (*EndElementCallback)(void* m_binder, const char *element, const char *m_body);

class BeginRule {

public:
	BeginRule(const char* pathStr, BeginElementCallback callback) : path(pathStr), callback(callback) { };
	BeginRule(const BeginRule& rule) : path(rule.path), callback(rule.callback) { };

	Path path;
	BeginElementCallback callback;
};


class EndRule {

public:
	EndRule(const char* pathStr, EndElementCallback callback) : path(pathStr), callback(callback) { };
	EndRule(const EndRule& rule) : path(rule.path), callback(rule.callback) { };

	Path path;
	EndElementCallback callback;
};

class DataBinder {

public:
	DataBinder();
	virtual ~DataBinder();

    virtual bool lock();
	virtual void reset();

	virtual void beginBinding() { };
	virtual void endBinding() { };

	virtual void startElement(const char *name, const char** attribs);
	virtual void endElement(const char *name);
	virtual void characters(const char *text, int len);

	virtual void startCDataSection();
	virtual void endCDataSection();

	const char* getBody() { 
        return m_body.str().c_str(); 
    }

	void skipParent(short level = 1) { 
        m_path.tag(level); 
    }

	virtual void* getRoot() { 
        return m_root; 
    }
	virtual void setRoot(void* root) { 
        m_root = root; 
    }

	void* detachRoot() { 
        void* root = m_root;
        m_root = NULL;
        return root; 
    }

	const char* getVariable(const char* name) { 
        return m_variables[name].c_str(); 
    }
	void setVariable(const char* name, const char* value) { 
        m_variables[name] = value; 
    }

	void setTrimBody(bool trim) { 
        m_trimBody = trim; 
    }

	void debug(const char* msg);

protected:

    void addBeginRule(const char *pathStr, BeginElementCallback callback);
    void addEndRule(const char *pathStr, EndElementCallback callback);

    void* m_root;

    Path m_path;
    Path* m_rulePath;

private:

    boost::unordered_map<std::string, std::list<BeginRule> > m_beginRules;
    boost::unordered_map<std::string, std::list<EndRule> > m_endRules;

    std::stringstream m_body;

    bool m_trimBody;
    bool m_addTextToBody;
    bool m_bodyIsCData;

    boost::unordered_map<std::string, std::string> m_variables;
    
    boost::shared_mutex m_bindingLock;
    bool m_binding;
};

template <class T>
class TypedDataBinder : public DataBinder {

public:
	TypedDataBinder() {
		m_root = NULL;
	}
	virtual ~TypedDataBinder() {

		if (m_root)
			delete ((boost::shared_ptr<T> *) DataBinder::m_root);
	}

	virtual void reset() {

		if (m_root)
			delete ((boost::shared_ptr<T> *) DataBinder::m_root);

		DataBinder::reset();
	}

	void* getRoot() {
        if (m_root)
            return ((boost::shared_ptr<T> *) DataBinder::m_root)->get();
        else
            return NULL;
	}

	void setRoot(void* root) {

		if (m_root)
			delete ((boost::shared_ptr<T> *) DataBinder::m_root);

		DataBinder::m_root = new boost::shared_ptr<T>((T *) root);
	}

	void setRoot(boost::shared_ptr<T> root) {

		if (m_root)
			delete ((boost::shared_ptr<T> *) DataBinder::m_root);

		DataBinder::m_root = new boost::shared_ptr<T>();
		*((boost::shared_ptr<T> *) DataBinder::m_root) = root;
	}

	boost::shared_ptr<T> getRootPtr() {
		return *((boost::shared_ptr<T> *) DataBinder::m_root);
	}
};


}  // namespace : binding

#endif /* DATABINDER_H_ */
