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

#include "Manager.h"
#include "DataBinder.h"
#include "Service.h"

/** @defgroup Class/Service static initializing macros
 *
 * These macros should be defined within the class declaration, outside the
 * class in the header and within the class implementation, to ensure that
 * static symbols are initialized. It also calls a static class method
 * _static_init() which can be implemented with additional static
 * initialization logic.
 *
 *  @{
 */

#define STATIC_INIT_DECLARATION(className) \
	public: \
		static bool __static_init() { \
			static bool _initialized = false; \
			static boost::mutex _mutex; \
			boost::mutex::scoped_lock lock(_mutex); \
			if (!_initialized) { \
				TRACE(#className " static state being initialized...");  \
				_initialized = className::_static_init(); \
			} \
			return _initialized; \
		}; \
		static bool _static_init();
#define STATIC_INIT_CALL(className) \
	static bool _init##className##Result = className::__static_init();
#define STATIC_INIT_NULL_IMPL(className) \
	bool className::_static_init() { return true; }

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

typedef void (*GetTokenValue)(const char* name, std::ostringstream& output); /**< @brief Callback to retrieve the value of a ${token} present in a configuration */

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
	 * Add a configuration URI to be loaded.
	 */
	void addConfig(const char* uri, int refresh = -1);

	/**
	 * Load configuration data from a string.
	 */
	void loadConfig(const char* data);

	/**
	 * Adds a begin config element binding trigger.
	 */
	static void addBeginConfigElementBinding(BeginConfigBinding* binding);

	/**
	 * Adds an end config element binding trigger.
	 */
	static void addEndConfigElementBinding(EndConfigBinding* binding);

protected:

	ServiceConfigManager();

	void foreground();
	bool background();

private:

	boost::unordered_map<std::string, boost::shared_ptr<Service> > m_services;

	static std::vector<BeginConfigBinding*> _beginConfigBindings;
	static std::vector<EndConfigBinding*> _endConfigBindings;

	friend class ServiceConfigBinder;
};

STATIC_INIT_CALL(ServiceConfigManager)

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

	friend class ServiceConfigManager;
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

	friend class ServiceConfigManager;
};


} // namespace : mb

#endif /* SERVICECONFIGMANAGER_H_ */
