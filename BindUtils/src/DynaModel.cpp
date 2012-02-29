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

#include "DynaModel.h"

#include <iostream>
#include <iomanip>
#include <ext/hash_map>

#include "boost/algorithm/string.hpp"

#include "log.h"
#include "exception.h"
#include "number.h"

#define ATTEMPT_TO_ADD_KEY_TO_LIST        "You cannot add key to a list node."
#define ATTEMPT_TO_ADD_TO_MAP             "You cannot add a node to a map node."
#define ATTEMPT_TO_ADD_KEY_VALUE_TO_LIST  "You cannot add a key value pair to a list node"
#define ATTEMPT_TO_ADD_VALUE_TO_MAP       "You cannot add a value pair to a map node without a key"

#define INDENT  4

#ifdef LOG_LEVEL_TRACE
static Number<long> instanceCount(0);
#endif


namespace binding {


class DynaModelException : public CException {
public:
	DynaModelException(const char* source, int lineNumber) : CException(source, lineNumber) { }
    virtual ~DynaModelException() { }
};


class Node : public DynaModel {

public:
	Node(DynaModel::Type type);
	virtual ~Node();

	Type getType();

	int size();
	bool containsKey(const char* key);
	std::list<std::string> keys();

	DynaModelNode get(const char* key);
	DynaModelNode get(unsigned int index);

	void add(DynaModelNode node, const char* key = NULL);
	DynaModelNode add(const char* key, Type type = MAP);
	DynaModelNode add(Type type = MAP);

	void setValue(const char* key, const char* value);
	void addValue(const char* value);

	virtual void toJson(std::ostream& cout, int level = -1);

private:
	DynaModel::Type m_type;

	__gnu_cxx::hash_map<std::string, int, hashstr, eqstr> m_childRefs;
	std::vector<DynaModelNode> m_childNodes;

	friend class DynaModelBinder;
};

class ValueNode : public DynaModel {

public:
	ValueNode(const char* value) { m_value = value; }
	virtual ~ValueNode() { }

	DynaModel::Type getType() { return VALUE; }

	const char* value() { return m_value.c_str(); }

	virtual void toJson(std::ostream& cout, int level = -1) { cout << '"' << m_value << '"'; }

private:
	std::string m_value;

	friend class DynaModelBinder;
};


// **** DataMap Implementation ****

DynaModel::DynaModel() {
	TRACE("Constructed DataMap Node: %p (# in heap %d)", this, instanceCount.interlockedInc());
}

DynaModel::~DynaModel() {
	TRACE("Destroyed DataMap Node: %p (# in heap %d)", this, instanceCount.interlockedDec());
}

DynaModelNode DynaModel::create(Type type) {
	return DynaModelNode(new Node(type));
}

std::ostream& operator<< (std::ostream& cout, const DynaModelNode data) {

	data->toJson(cout);
	return cout;
}


// **** Node Implementation ****

Node::Node(DynaModel::Type type) {
	m_type = type;
}

Node::~Node() {
}

DynaModel::Type Node::getType() {
	return m_type;
}

bool Node::containsKey(const char* key) {
	return m_childRefs.find(key) != m_childRefs.end();
}

std::list<std::string> Node::keys() {

	std::list<std::string> keys;

	__gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator i;
	__gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator end = m_childRefs.end();

	for (i = m_childRefs.begin(); i != end; i++)
		keys.push_back(i->first);

	return keys;
}

int Node::size() {
	return m_childNodes.size();
}

DynaModelNode Node::get(const char* key) {

	if (m_childRefs.find(key) != m_childRefs.end())
		return m_childNodes[m_childRefs[key]];
	else
		return DynaModelNode();
}

DynaModelNode Node::get(unsigned int index) {

	if (index < m_childNodes.size())
		return m_childNodes[index];
	else
		return DynaModelNode();
}

void Node::add(DynaModelNode node, const char* key) {

	if (key) {

		__gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator ref = m_childRefs.find(key);
		if (ref != m_childRefs.end()) {

			m_childNodes.erase(m_childNodes.begin() + ref->second);
			m_childRefs.erase(ref);
		}
	}

	m_childNodes.push_back(node);
	int i = m_childNodes.size() - 1;

	if (key)
		m_childRefs[key] = i;

	TRACE( "Added DataMap Node: %s = %p @ index %d to %p",
		(key ? key : "-"), node.get(), i, this );
}

DynaModelNode Node::add(const char* key, Type type) {

	if (m_type == DynaModel::MAP) {

		__gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator ref = m_childRefs.find(key);
		if (ref == m_childRefs.end()) {

			DynaModelNode dataNode = DynaModelNode(new Node(type));

			m_childNodes.push_back(dataNode);
			int i = m_childNodes.size() - 1;
			m_childRefs[key] = i;

			TRACE( "Added DataMap Node: %s = %s @ index %d to %p",
				key, type == MAP ? "MAP" : "LIST", i, this );

			return dataNode;

		} else {

			DynaModelNode dataNode = m_childNodes[ref->second];
			if (dataNode->getType() == VALUE) {

				dataNode = DynaModelNode(new Node(type));
				m_childNodes[ref->second] = dataNode;

				TRACE( "Replacing value DataMap Node @ %d with new Node: %s = %s to %p",
					ref->second, key, type == MAP ? "MAP" : "LIST", this );
			}

			return dataNode;
		}
	} else
		THROW(DynaModelException, EXCEP_MSSG(ATTEMPT_TO_ADD_KEY_TO_LIST));

	return DynaModelNode();
}

DynaModelNode Node::add(Type type) {

	if (m_type == DynaModel::LIST) {

		DynaModelNode dataNode = DynaModelNode(new Node(type));
		m_childNodes.push_back(dataNode);

		TRACE("Added DataMap %s Node @ index %d to %p",
			m_type == MAP ? "MAP" : "LIST", m_childNodes.size() - 1, this );

		return dataNode;

	} else
		THROW(DynaModelException, EXCEP_MSSG(ATTEMPT_TO_ADD_TO_MAP));

	return DynaModelNode();
}

void Node::setValue(const char* key, const char* value) {

	if (m_type == DynaModel::MAP) {

		DynaModelNode dataNode = DynaModelNode(new ValueNode(value));

		__gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator ref = m_childRefs.find(key);
		if (ref == m_childRefs.end()) {

			m_childNodes.push_back(dataNode);
			int i = m_childNodes.size() - 1;
			m_childRefs[key] = i;

			TRACE( "Added DataMap VALUE Node: %s = %s @ index %d to %p",
				key, value, i, this );

		} else
			m_childNodes[ref->second] = dataNode;
	} else
		THROW(DynaModelException, EXCEP_MSSG(ATTEMPT_TO_ADD_KEY_VALUE_TO_LIST));
}

void Node::addValue(const char* value) {

	if (m_type == DynaModel::LIST) {

		DynaModelNode dataNode = DynaModelNode(new ValueNode(value));
		m_childNodes.push_back(dataNode);

		TRACE( "Added DataMap VALUE Node @ index %d to %p",
			m_childNodes.size() - 1, this);

	} else
		THROW(DynaModelException, EXCEP_MSSG(ATTEMPT_TO_ADD_VALUE_TO_MAP));
}

void Node::toJson(std::ostream& cout, int level) {

    if (level >= 0) {

        if (m_childRefs.size() > 0) {

            cout << '{' << std::endl;

            __gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator i = m_childRefs.begin();
            __gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator end = m_childRefs.end();

            while (i != end) {

                cout.width((level + 1) * INDENT);
                cout << '"' << i->first << "\": ";

                DynaModelNode node = m_childNodes[i->second];
                node->toJson(cout, level + 1);

                if (++i != end)
                    cout << ',' << std::endl;
                else
                    cout << std::endl;
            }

            cout.width(level * INDENT);
            cout << '}';

        } else {

            cout << '[';

            int size = m_childNodes.size();
            int i = 0;

            DynaModelNode node;

            while (i < size) {

                node = m_childNodes[i];

                if (node->getType() == DynaModel::VALUE) {
                    cout << std::endl;
                    cout.width((level + 1) * INDENT);
                } else if (i == 0)
                    cout << ' ';

                node->toJson(cout, level);

                if (++i != size)
                    cout << ", ";
                else
                    cout << ' ';
            }

            if (node.get() && node->getType() == DynaModel::VALUE) {
                cout << std::endl;
                cout.width(level * INDENT);
            }

            cout << ']';
        }

    } else {

        if (m_childRefs.size() > 0) {

            cout << '{';

            __gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator i = m_childRefs.begin();
            __gnu_cxx::hash_map<std::string, int, hashstr, eqstr>::iterator end = m_childRefs.end();

            while (i != end) {

                cout << '"' << i->first << "\":";

                DynaModelNode node = m_childNodes[i->second];
                node->toJson(cout);

                if (++i != end)
                    cout << ',';
            }

            cout << '}';

        } else {

            cout << '[';

            int size = m_childNodes.size();
            int i = 0;

            DynaModelNode node;

            while (i < size) {

                node = m_childNodes[i];

                node->toJson(cout);

                if (++i != size)
                    cout << ',';
            }

            cout << ']';
        }
    }
}


// **** DynaModelBindingConfig Implementation ****

void DynaModelBindingConfig::beginBindingConfigElement(std::map<std::string, std::string>& attribs) {

	const char* pathStr = attribs["path"].c_str();
	std::string& type = attribs["type"];

	Path path(pathStr);
	m_pathDepth.push(path.length());
	m_path.append(path);

	DynaModelBinding binding(
		m_path.str(),
		attribs["key"].c_str(),
		attribs["ref"].c_str(),
		attribs["index"] == "true",
		type == "map" ? DynaModel::MAP :
		type == "list" ? DynaModel::LIST : DynaModel::VALUE );

	m_bindings.push_back(binding);

	TRACE( "Binding config [%d]: path=%s, depth=%d, key=%s, ref=%s, isIdx=%s, type=%s",
        m_bindings.size(),
		m_path.str(),
		m_pathDepth.top(),
		binding.m_key.c_str(),
		binding.m_ref.c_str(),
		binding.m_isIdx ? "yes" : "false",
		binding.m_type == DynaModel::MAP ? "map" :
		binding.m_type == DynaModel::LIST ? "list" : "value" );
}

void DynaModelBindingConfig::endBindingConfigElement() {

	int depth = m_pathDepth.top();
	m_pathDepth.pop();

	while (depth--)
		m_path.pop();
}

void DynaModelBindingConfig::beginParseRule(std::map<std::string, std::string>& attribs) {
    
    std::map<std::string, std::string>::iterator attribEnd = attribs.end();
    
    DynaModelBinding& binding = m_bindings.back();
    binding.m_parseRules.push_back(DynaModelBinding::ParseRule());
    DynaModelBinding::ParseRule& parseRule = binding.m_parseRules.back();
    
    parseRule.key = attribs["key"];
    
    if (attribs.find("offset") != attribEnd)
        parseRule.offset = atoi(attribs["offset"].c_str());
    else
        parseRule.offset = -1;

    if (attribs.find("delim") != attribEnd && attribs["delim"].length() > 0)
        parseRule.delim = attribs["delim"].at(0);
    else
        parseRule.delim = -1;
    
    if (attribs.find("length") != attribEnd)
        parseRule.length = atoi(attribs["length"].c_str());
    else
        parseRule.length = -1;
    
    if (attribs.find("strip") != attribEnd && attribs["strip"].length() > 0)
        parseRule.strip = attribs["strip"];
    
    if (attribs.find("replace") != attribEnd && attribs["replace"].length() > 0)
        parseRule.replace = attribs["replace"];
}

void DynaModelBindingConfig::beginParseValueMapping(std::map<std::string, std::string>& attribs) {
    
    std::map<std::string, std::string>::iterator attribEnd = attribs.end();
    if (attribs.find("from") != attribEnd) {
        
        DynaModelBinding& binding = m_bindings.back();
        DynaModelBinding::ParseRule& parseRule = binding.m_parseRules.back();
        
        parseRule.valueMapping[attribs["from"]] = attribs["to"];
    }
}


// **** DynaModelBinder Implementation ****

DynaModelBinder::DynaModelBinder() {
}

DynaModelBinder::DynaModelBinder(DynaModelBindingConfig* config) {

    int i, size = config->m_bindings.size();
    for (i = 0; i < size; i++) {
        
        DynaModelBinding& binding = config->m_bindings[i];
        
        const char* path = binding.m_path.c_str();
        m_bindingMap[path] = binding;

        switch (binding.m_type) {
                
            case DynaModel::MAP:
                DataBinder::addBeginRule(path, beginMap);
                DataBinder::addEndRule(path, endMap);
                break;
                
            case DynaModel::LIST:
                DataBinder::addBeginRule(path, beginList);
                DataBinder::addEndRule(path, endList);
                break;
                
            case DynaModel::VALUE:
                DataBinder::addEndRule(path, bindValue);
                break;
                
            default:
                break;
        }
    }
}

void DynaModelBinder::addBinding(
	const char* path,
	DynaModel::Type type,
	const char* key,
	const char* refKey ) {

	m_bindingMap[path] = DynaModelBinding(path, key ? key : "", refKey ? refKey : "", false, type);

	switch (type) {

		case DynaModel::MAP:
			DataBinder::addBeginRule(path, beginMap);
			DataBinder::addEndRule(path, endMap);
			break;

		case DynaModel::LIST:
			DataBinder::addBeginRule(path, beginList);
			DataBinder::addEndRule(path, endList);
			break;

		case DynaModel::VALUE:
			DataBinder::addEndRule(path, bindValue);
			break;

		default:
			break;
	}
}

void DynaModelBinder::addBinding(
	const char* path,
	const char* key,
	bool isIdx ) {

	m_bindingMap[path] = DynaModelBinding(path, key ? key : "", "", isIdx, DynaModel::VALUE);
	DataBinder::addEndRule(path, bindValue);
}

void DynaModelBinder::beginBinding() {

	m_bindingNode.push(DynaModel::create());
	this->setRoot(m_bindingNode.top());

	m_listBindingProcessed = NULL;
}

void DynaModelBinder::endBinding() {
    
	finalizeListElemProcessing();
	while (!m_bindingNode.empty())
    {
		m_bindingNode.pop();
    }
}

void DynaModelBinder::reset() {

	while (!m_bindingNode.empty())
		m_bindingNode.pop();
    
    while (!m_index.empty())
        m_index.pop();

	m_listBindingProcessed = NULL;

	TypedDataBinder<DynaModel>::reset();
}

inline void DynaModelBinder::addNodeToParent(const DynaModelBinding* binding) {

    if(m_bindingNode.empty())
    {
        TRACE("Empty stack on addNodeToParent");
        return;
    }
    
	DynaModelNode curr = m_bindingNode.top();
	m_bindingNode.pop();

	if (binding->m_ref.length() == 0) {

        if(m_bindingNode.empty())
        {
            TRACE("Empty stack on addNodeToParent");
            return;
        }

		DynaModelNode top = m_bindingNode.top();
        if (top.get() == 0)
        {
            WARN("Can't add node to parent. Node at top is NULL");
            return;
        }
        
        switch (top->getType()) {
                
            case DynaModel::MAP:
                if (binding->m_key.length() > 0)
                    top->add(curr, binding->m_key.c_str());
                break;
                
            case DynaModel::LIST:
                top->add(curr);
                break;
                
            default:
                break;
        }

	} else if (binding->m_key.c_str() > 0) {

		std::vector<std::string> dynModelKeyPath;
		boost::split(dynModelKeyPath, binding->m_ref, boost::is_any_of("/"), boost::token_compress_on);

		int numKeys = dynModelKeyPath.size();
		int i = 0;

		DynaModelNode node = this->getRootPtr();

		while (node.get() && i < numKeys)
			node = node->get(dynModelKeyPath[i++].c_str());

		if (!m_index.empty()) {

            std::string index = m_index.top();
            m_index.pop();

            if (index.length() > 0)
                node = node->get(index.c_str());
		}

		if (node.get())
			node->add(curr, binding->m_key.c_str());

	} else {

		ERROR("Unable to bind referenced node %p as no binding key was not provided.", curr.get());
	}
}

inline void DynaModelBinder::finalizeListElemProcessing() {

	if (m_listBindingProcessed) {

		addNodeToParent(m_listBindingProcessed);
		m_listBindingProcessed = NULL;
	}
}

void DynaModelBinder::beginMap(void* binder, const char *element, std::map<std::string, std::string>& attribs) {

	GET_BINDER(DynaModelBinder);
	dataBinder->finalizeListElemProcessing();
	dataBinder->m_bindingNode.push(DynaModel::create());
}

void DynaModelBinder::endMap(void* binder, const char* element, const char* body) {

	GET_BINDER(DynaModelBinder);
	const DynaModelBinding* binding = &dataBinder->m_bindingMap[dataBinder->m_rulePath->str()];

	dataBinder->finalizeListElemProcessing();
	dataBinder->addNodeToParent(binding);

    dataBinder->m_lastBoundPath = dataBinder->m_path.str();
}

void DynaModelBinder::beginList(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

	GET_BINDER(DynaModelBinder);
    
	if (dataBinder->m_listBindingProcessed) {
        
        if (dataBinder->m_lastBoundPath == dataBinder->m_path.str()) {
            
            dataBinder->m_listBindingProcessed = NULL;
            dataBinder->m_index.push("");
            return;
        }
        
        dataBinder->finalizeListElemProcessing();
    }

    dataBinder->m_bindingNode.push(DynaModel::create(DynaModel::LIST));
    dataBinder->m_index.push("");
}

void DynaModelBinder::endList(void* binder, const char* element, const char* body) {

	GET_BINDER(DynaModelBinder);
	const DynaModelBinding* binding = &dataBinder->m_bindingMap[dataBinder->m_rulePath->str()];

    if (*body) {
        
		dataBinder->m_bindingNode.top()->addValue(body);
        
    } else {

        dataBinder->finalizeListElemProcessing();
        
        DynaModelNode curr = dataBinder->m_bindingNode.top();
		dataBinder->m_bindingNode.pop();

		DynaModelNode top = dataBinder->m_bindingNode.top();
		if (top->getType() == DynaModel::LIST) {

            std::string index = dataBinder->m_index.top();
            
            if (index.length() > 0)
                top->add(curr, index.c_str());
			else
                top->add(curr);

		} else if (binding->m_key.length() > 0)
			top->add(curr, binding->m_key.c_str());

	}

    dataBinder->m_index.pop();
    dataBinder->m_listBindingProcessed = binding;
    dataBinder->m_lastBoundPath = dataBinder->m_path.str();
}

void DynaModelBinder::bindValue(void* binder, const char* element, const char* body) {

	GET_BINDER(DynaModelBinder);

	dataBinder->finalizeListElemProcessing();

	DynaModelBinding& binding = dataBinder->m_bindingMap[dataBinder->m_rulePath->str()];
	DynaModelNode curr = dataBinder->m_bindingNode.top();
    
    bool parseValue = (binding.m_parseRules.size() > 0);

	if (binding.m_key.length() > 0 || parseValue) {

		if (curr->getType() == DynaModel::LIST) {

			curr = DynaModel::create();
			dataBinder->m_bindingNode.push(curr);
		}
        
        if (parseValue) {
            
            std::string value(body);
            int valueLen = value.length();
            
            int nextOffset, offset = 0;
            int len;
            
            std::list<DynaModelBinding::ParseRule>::iterator end = binding.m_parseRules.end();
            std::list<DynaModelBinding::ParseRule>::iterator parseRule;
            
            for (parseRule = binding.m_parseRules.begin(); offset < valueLen && parseRule != end; parseRule++) {

                if (parseRule->delim != -1) {
                    
                    size_t delimOffset = value.find(parseRule->delim, offset);
                    len = (delimOffset == std::string::npos ? valueLen - offset : delimOffset - offset);
                    
                    nextOffset = offset + (len + 1);
                    
                } else if (parseRule->offset != -1) {
                    
                    offset = parseRule->offset;
                    if (offset >= valueLen)
                        break;
                    
                    len = (parseRule->length == -1 ? valueLen - offset : parseRule->length);
                    nextOffset = offset + len;
                    
                } else
                    len = valueLen - offset;
                
                std::string parsedValue = value.substr(offset, len);
                
                int stripLen = parseRule->strip.length();
                if (stripLen != 0) {
                    
                	size_t stripOffset = 0;
                    
                    int replaceLen = parseRule->replace.length();
                    bool replace = (replaceLen != 0);
                    
                    while ((stripOffset = parsedValue.find(parseRule->strip, stripOffset)) != std::string::npos) {
                        
                        if (replace) {
                            
                            parsedValue.replace(stripOffset, stripLen, parseRule->replace, 0, replaceLen);
                            stripOffset += replaceLen;
                            
                        } else
                            parsedValue.erase(stripOffset, stripLen);
                    }
                }
                
                if ( parseRule->valueMapping.size() > 0 && 
                    parseRule->valueMapping.find(parsedValue) != parseRule->valueMapping.end() ) {
                    
                    curr->setValue(parseRule->key.c_str(), parseRule->valueMapping[parsedValue].c_str());
                } else
                    curr->setValue(parseRule->key.c_str(), parsedValue.c_str());
                
                offset = nextOffset;
            }
            
        } else
            curr->setValue(binding.m_key.c_str(), body);
        
        if (binding.m_isIdx) {
            dataBinder->m_index.pop();
            dataBinder->m_index.push(body);
        }
	}

    dataBinder->m_lastBoundPath = dataBinder->m_path.str();
}


}  // namespace : binding
