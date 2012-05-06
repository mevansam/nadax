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

#ifndef DATAMAP_H_
#define DATAMAP_H_

#include <vector>
#include <list>
#include <stack>
#include <set>

#include "boost/shared_ptr.hpp"

#include "objectpool.h"

#include "DataBinder.h"

namespace binding {


class DynaModel;
class DynaModelBindingConfig;

typedef boost::shared_ptr<DynaModel> DynaModelNode;
typedef boost::shared_ptr<DynaModelBindingConfig> DynaModelBindingConfigPtr;


class DynaModel {

public:
	enum Type {
		NUL,
		MAP,
		LIST,
		VALUE
	};

public:
	DynaModel(DynaModelNode node) {
		m_node = node;
	};
	DynaModel(DynaModel& model) {
		m_node = model.m_node;
	};
	virtual ~DynaModel();

	static DynaModelNode create(Type type = MAP);

	bool isValid() { return this->getType() != NUL; }

	virtual Type getType() { return NUL; }

	virtual int size() { return 0; }
	virtual bool containsKey(const char* key) { return false; }
	virtual std::list<std::string> keys() { return std::list<std::string>(); }

	virtual const char* value() { return NULL; }

	virtual DynaModelNode get(const char* key) { return DynaModelNode(); }
	virtual DynaModelNode get(unsigned int index) { return DynaModelNode(); }

	virtual void add(DynaModelNode node, const char* key = NULL) { }
	virtual DynaModelNode add(const char* key, Type type = MAP) { return DynaModelNode(); }
	virtual DynaModelNode add(Type type = MAP) { return DynaModelNode(); }

	virtual void setValue(const char* key, const char* value) { };
	virtual void addValue(const char* value) { };

	virtual void toJson(std::ostream& cout, int level = -1) { };

	DynaModel operator[](const char* key) {
		DynaModel model(m_node->get(key));
		return model;
	}
	DynaModel operator[](int index) {
		DynaModel model(m_node->get(index));
		return model;
	}

	friend std::ostream &operator<< (std::ostream& cout, const DynaModelNode data);

protected:
	DynaModel();
    
private:
    DynaModelNode m_node;
};


class DynaModelBinding {

public:
	DynaModelBinding() { }

	DynaModelBinding(const char* path, const char* key, const char* ref, bool isIdx, DynaModel::Type type )
		: m_path(path), m_key(key), m_ref(ref), m_isIdx(isIdx), m_type(type) { }

	DynaModelBinding(const DynaModelBinding& binding)
		: m_path(binding.m_path), m_key(binding.m_key), m_ref(binding.m_ref), m_isIdx(binding.m_isIdx), m_type(binding.m_type) { 
        
        m_parseRules.insert(m_parseRules.begin(), binding.m_parseRules.begin(), binding.m_parseRules.end());
    }

private:
    std::string m_path;
	std::string m_key;
	std::string m_ref;
	bool m_isIdx;
	DynaModel::Type m_type;

	struct ParseRule {

		ParseRule& operator=(const ParseRule& value) const {

			ParseRule* rule = const_cast<ParseRule*>(&value);
			const_cast<ParseRule*>(this)->offset = rule->offset;
			const_cast<ParseRule*>(this)->delim = rule->delim;
			const_cast<ParseRule*>(this)->length = rule->length;
			const_cast<ParseRule*>(this)->strip = rule->strip;
			const_cast<ParseRule*>(this)->replace = rule->replace;
			const_cast<ParseRule*>(this)->valueMapping.insert(rule->valueMapping.begin(), rule->valueMapping.end());

			return *(const_cast<ParseRule*>(this));
		}

		void copy(ParseRule* rule) {

		}

	    int offset;
	    char delim;
	    int length;

	    std::string strip;
	    std::string replace;

	    std::string key;

	    boost::unordered_map<std::string, std::string> valueMapping;
	};
    
    std::list<ParseRule> m_parseRules;

	friend class DynaModelBindingConfig;
	friend class DynaModelBinder;
};


class DynaModelBindingConfig {

public:
    
    void beginBindingConfigElement(std::map<std::string, std::string>& attribs);
    void endBindingConfigElement();
    
	void beginParseRule(std::map<std::string, std::string>& attribs);
	void beginParseValueMapping(std::map<std::string, std::string>& attribs);
        
private:
    std::vector<DynaModelBinding> m_bindings;

    Path m_path;
    std::stack<int> m_pathDepth;

    friend class DynaModelBinding;
    friend class DynaModelBinder;
};


class DynaModelBinder : public TypedDataBinder<DynaModel> {

public:
	DynaModelBinder();
	DynaModelBinder(DynaModelBindingConfig* config);

	void addBinding(
		const char* path,
		DynaModel::Type type,
		const char* key,
		const char* refKey = NULL );

	void addBinding(
		const char* path,
		const char* key = NULL,
		bool isIdx = false);

	void beginBinding();
	void endBinding();

	void reset();

	void addNodeToParent(const DynaModelBinding* binding);
	void finalizeListElemProcessing();

	static void beginMap(void* binder, const char *element, std::map<std::string, std::string>& attribs);
	static void endMap(void* binder, const char *element, const char *body);

	static void beginList(void* binder, const char *element, std::map<std::string, std::string>& attribs);
	static void endList(void* binder, const char *element, const char *body);

	static void bindValue(void* binder, const char *element, const char *body);

private:
	boost::unordered_map<std::string, DynaModelBinding> m_bindingMap;

	std::stack<DynaModelNode> m_bindingNode;
	const DynaModelBinding* m_listBindingProcessed;
	std::stack<std::string> m_index;
    std::string m_lastBoundPath;
    
    friend class DynaModelBinding;
};

class DynaModelBinderPool : public ObjectPool<DynaModelBinder> {

public:

	DynaModelBinderPool() {
	};

	virtual ~DynaModelBinderPool() {
	};

	void setDynaModelBindingConfig(DynaModelBindingConfigPtr bindingConfig) {
		m_bindingConfig = bindingConfig;
	}

	bool hasDynaModelBindingConfig() {
		return (m_bindingConfig.get() != NULL);
	}

protected:

	virtual DynaModelBinder* create() {
		return new DynaModelBinder(m_bindingConfig.get());
	}

	virtual void passivate(DynaModelBinder* binder) {
		binder->reset();
	};

private:
	DynaModelBindingConfigPtr m_bindingConfig;
};


}  // namespace : binding

#endif /* DATAMAP_H_ */
