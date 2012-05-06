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

/**
 * @file ServiceConfigManager.h
 * @author Mevan Samaratunga
 *
 * @brief Message bus service configuration manager.
 * @details	This class defines a single entity for managing configurations of
 * services registered with the message bus. Service configuration resources
 * may be loaded from a file system resource or via a URI. If a configuration
 * resource is loaded with a refresh interval then the resource URI will be
 * queried periodically for updates.
 **/


#ifndef SERVICECONFIGMANAGER_H_
#define SERVICECONFIGMANAGER_H_

#include <sstream>
#include <string>
#include <vector>

#include "boost/iostreams/filtering_stream.hpp"

#include "staticinit.h"

#include "Manager.h"
#include "DataBinder.h"
#include "Service.h"

/** @} */

/** @defgroup configBindings Configuration Binding Macros
 *
 * The binding macros should be included in service implementation
 * source files. When the binding classes are defined its constructor
 * will add the itself to the vector of rules which is used to initialize
 * the binding configuration xml.
 *
 *  @{
 */

#define ADD_BEGIN_CONFIG_BINDING(name, path, callback) \
	static mb::BeginConfigBinding name(path, callback); /**< Adds config binding callback for configuring the start of a config XML element. */
#define ADD_END_CONFIG_BINDING(name, path, callback) \
	static mb::EndConfigBinding name(path, callback); /**< Adds config binding callback for configuring the end of a config XML element. */

/** @} */

namespace mb {

/**
 * @brief Callback to retrieve the value of a ${token} present in a configuration.
 * If a token value for the given name is available then the function should return
 * true.
 */
typedef bool (*TokenResolverCallback)(const std::string& name, std::string& value);

/**
 * @class ServiceConfigManager "ServiceConfigManager.h"
 * @brief Manages configuration and registration life-cycle of message bus services.
 */

class BeginConfigBinding;
class EndConfigBinding;

class ServiceConfigManager : protected Manager {

STATIC_INIT_DECLARATION(ServiceConfigManager)

SINGLETON_MANAGER(ServiceConfigManager)

public:
	virtual ~ServiceConfigManager();

	/**
	 * @brief Set callback function to retrieve a token value. If a
	 * token lookup map is also provided then this callback
	 * is called only if the token does not exist in the map.
	 */
	void setTokenResolverCallback(TokenResolverCallback tokenCallback) {
		m_tokenCallback = tokenCallback;
	}

	/**
	 * @brief Copies a collection of tokens from a map.
	 */
	void setTokenLookupMap(const boost::unordered_map<std::string, std::string>& tokens) {
		m_tokens.insert(tokens.begin(), tokens.end());
	}

	/**
	 * @brief Adds a token to the token lookup map
	 */
	void addToken(const char* name, const char* value) {
		m_tokens[name] = value;
	}

	/**
	 * @brief Adds a token to the token lookup map
	 */
	bool lookupTokenValue(const std::string& name, std::string& value);

	/**
	 * @brief Add a configuration URI to be loaded.
	 */
	void monitorConfigUri(const char* uri, int refresh = -1);

	/**
	 * @brief Load configuration data from a file.
	 */
	void loadConfigFile(const char* fileName);

	/**
	 * @brief Load configuration data from a buffer.
	 */
	void loadConfigData(const void* data, int len);

	/**
	 * @brief Adds a begin config element binding trigger.
	 */
	static void addBeginConfigElementBinding(BeginConfigBinding* binding);

	/**
	 * @brief Adds an end config element binding trigger.
	 */
	static void addEndConfigElementBinding(EndConfigBinding* binding);

protected:

	ServiceConfigManager();

	void foreground();
	bool background();

private:

	void parseConfig(boost::iostreams::filtering_istream& input);

	boost::unordered_map<std::string, boost::shared_ptr<Service> > m_services;

	static std::vector<BeginConfigBinding*> _beginConfigBindings;
	static std::vector<EndConfigBinding*> _endConfigBindings;

	TokenResolverCallback m_tokenCallback;
	boost::unordered_map<std::string, std::string> m_tokens;

	friend class ServiceConfigBinder;
};

STATIC_INIT_CALL(ServiceConfigManager)

/**
 * @brief Token lookup via ServiceConfigManager token references.
 */
inline bool ServiceConfigManager::lookupTokenValue(const std::string& name, std::string& value) {

	boost::unordered_map<std::string, std::string>::iterator token = m_tokens.find(name);
	if (token != m_tokens.end()) {
		value = token->second;
		return true;
	}

	if (m_tokenCallback) {
		return m_tokenCallback(name, value);
	}
	return false;
}

/**
 * @class ServiceConfig "ServiceConfigManager.h"
 * @brief Class for holding the instance of the service currently being
 * 		configured as the configuration XML is being parsed.
 */
class ServiceConfig : public binding::DataBinder {

public:
	virtual ~ServiceConfig() { }

	virtual void addService(Service* service) = 0;

	Service* getService() {
		return m_service;
	}

	bool getError() {
		return m_error;
	}

protected:

	Service* m_service;
	bool m_error;
};

/**
 * @class BeginConfigBinding "ServiceConfigManager.h"
 * @brief Class initialized using the config binding macros.
 */
class BeginConfigBinding {

public:
	BeginConfigBinding(const char* pathStr, binding::BeginElementCallback callback)
		: m_pathStr(pathStr), m_callback(callback) {
		ServiceConfigManager::addBeginConfigElementBinding(this);
	}
	~BeginConfigBinding() { }
private:
	std::string m_pathStr;
	binding::BeginElementCallback m_callback;

	friend class ServiceConfigBinder;
};

/**
 * @class EndConfigBinding "ServiceConfigManager.h"
 * @brief Class initialized using the config binding macros.
 */
class EndConfigBinding {

public:
	EndConfigBinding(const char* pathStr, binding::EndElementCallback callback)
		: m_pathStr(pathStr), m_callback(callback) {
		ServiceConfigManager::addEndConfigElementBinding(this);
	}
	~EndConfigBinding() { }
private:
	std::string m_pathStr;
	binding::EndElementCallback m_callback;

	friend class ServiceConfigBinder;
};


} // namespace : mb

#endif /* SERVICECONFIGMANAGER_H_ */
