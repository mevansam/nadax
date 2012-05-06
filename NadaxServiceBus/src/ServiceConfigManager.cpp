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
 * @file ServiceConfigManager.cpp
 * @author Mevan Samaratunga
 *
 * @brief Message bus service configuration manager implementation.
 **/

#include "ServiceConfigManager.h"

#include "boost/iostreams/device/file.hpp"
#include "boost/iostreams/filter/regex.hpp"
#include "boost/regex.hpp"

#include "DynaModel.h"
#include "XmlStreamParser.h"
#include "MessageBusManager.h"

#define BUFFER_SIZE  1024

using namespace binding;
using namespace parser;


namespace mb {


class ServiceConfigBinder : public ServiceConfig {

public:
	ServiceConfigBinder() {

		unsigned int i;
		for (i = 0; i < ServiceConfigManager::_beginConfigBindings.size(); i++) {
			BeginConfigBinding* binding = ServiceConfigManager::_beginConfigBindings[i];
			TRACE("Adding begin config binding rule for path: %s", binding->m_pathStr.c_str());
			ServiceConfigBinder::addBeginRule(binding->m_pathStr.c_str(), binding->m_callback);
		}
		for (i = 0; i < ServiceConfigManager::_endConfigBindings.size(); i++) {
			EndConfigBinding* binding = ServiceConfigManager::_endConfigBindings[i];
			TRACE("Adding end config binding rule for path: %s", binding->m_pathStr.c_str());
			ServiceConfigBinder::addEndRule(binding->m_pathStr.c_str(), binding->m_callback);
		}

		ServiceConfigBinder::addBeginRule("*/bindings", beginBindingsConfig);
		ServiceConfigBinder::addBeginRule("*/bind", beginBindConfig);
		ServiceConfigBinder::addBeginRule("*/bind/parse", beginBindParseRule);
		ServiceConfigBinder::addBeginRule("*/bind/parse/mapping", beginBindParseValueMapping);
		ServiceConfigBinder::addEndRule("*/bind", endBindConfig);
		ServiceConfigBinder::addEndRule("*/bindings", endBindingsConfig);
		ServiceConfigBinder::addEndRule("messagebus-config/service", endServiceConfig);
	}

    void beginBinding() {

        m_error = false;
    }

    void addService(Service* service) {

    	TRACE("Binding service '%s' configuration details...", service->getSubject());
    	((ServiceConfigManager *) this->getRoot())->m_services[service->getSubject()] = boost::shared_ptr<Service>(service);
    	m_service = service;
    }

    static void beginBindingsConfig(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

        GET_BINDER(ServiceConfigBinder);

        dataBinder->m_bindingConfig = boost::shared_ptr<DynaModelBindingConfig>(new DynaModelBindingConfig());
    }

    static void beginBindConfig(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

        GET_BINDER(ServiceConfigBinder);

        dataBinder->m_bindingConfig->beginBindingConfigElement(attribs);
    }

    static void beginBindParseRule(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

        GET_BINDER(ServiceConfigBinder);
        dataBinder->m_bindingConfig->beginParseRule(attribs);
    }

    static void beginBindParseValueMapping(void* binder, const char* element, std::map<std::string, std::string>& attribs) {

        GET_BINDER(ServiceConfigBinder);
        dataBinder->m_bindingConfig->beginParseValueMapping(attribs);
    }

    static void endBindConfig(void* binder, const char* element, const char* body) {

        GET_BINDER(ServiceConfigBinder);

        dataBinder->m_bindingConfig->endBindingConfigElement();
    }

    static void endBindingsConfig(void* binder, const char* element, const char* body) {

        GET_BINDER(ServiceConfigBinder);

        Service* service = dataBinder->getService();
        if (service)
        	service->setDynaModelBindingConfig(dataBinder->m_bindingConfig);

        dataBinder->m_bindingConfig.reset();
    }

    static void endServiceConfig(void* binder, const char* element, const char* body) {

        GET_BINDER(ServiceConfigBinder);

        Service* service = dataBinder->getService();

#ifdef LOG_LEVEL_TRACE
        std::ostringstream output;
        output << *service;
        TRACE("Registering service:\n%s", output.str().c_str());
#endif

        MessageBusManager* mb = MessageBusManager::instance();
        mb->registerService(service);
    }

private:

    DynaModelBindingConfigPtr m_bindingConfig;
};

// **** Callback for token filter ****

/**
 * Boost wrapper for resolving token values when reading the stream.
 * The boost callback simply calls out to the provided token value
 * callback.
 */
struct TokenResolver {

	std::string operator()(const boost::match_results<const char*>& match) {

		std::string name(match[0].first + 2, match[0].length() - 3);

		std::string value;
		if (m_manager->lookupTokenValue(name, value))
			return value;
		else
			return std::string(match[0].first, match[0].length());
	}

	TokenResolver(ServiceConfigManager* manager) {
		m_manager = manager;
	}

	ServiceConfigManager* m_manager;
};


// **** ServiceConfigManager Singleton ****

STATIC_INIT_NULL_IMPL(ServiceConfigManager)

SINGLETON_MANAGER_IMPLEMENTATION(ServiceConfigManager, ServiceConfigException)

// **** Implementation ****

std::vector<BeginConfigBinding*> ServiceConfigManager::_beginConfigBindings;
std::vector<EndConfigBinding*> ServiceConfigManager::_endConfigBindings;

static boost::regex __tokenPattern("\\$\\{[-+_a-zA-Z0-9]+\\}");

ServiceConfigManager::ServiceConfigManager() {

	// Start up message bus manager
	mb::MessageBusManager::initialize();
}

ServiceConfigManager::~ServiceConfigManager() {

	// Destroy message bus manager
	mb::MessageBusManager::destroy();
}

void ServiceConfigManager::addBeginConfigElementBinding(BeginConfigBinding* binding) {

	ServiceConfigManager::_beginConfigBindings.push_back(binding);
}

void ServiceConfigManager::addEndConfigElementBinding(EndConfigBinding* binding) {

	ServiceConfigManager::_endConfigBindings.push_back(binding);
}

void ServiceConfigManager::foreground() {
}

bool ServiceConfigManager::background() {

	return true;
}

void ServiceConfigManager::monitorConfigUri(const char* uri, int refresh) {
}

void ServiceConfigManager::loadConfigFile(const char* fileName) {

	TRACE("Loading service configuration file '%s'.", fileName);

	boost::iostreams::filtering_istream input;
	input.push(boost::iostreams::regex_filter(__tokenPattern, TokenResolver(this)));
	input.push(boost::iostreams::file_source(fileName));

	parseConfig(input);
}

void ServiceConfigManager::loadConfigData(const void* data, int len) {
}

void ServiceConfigManager::parseConfig(boost::iostreams::filtering_istream& input) {

    try {

    	ServiceConfigBinder binder;
        binder.setRoot(this);

        XmlBinder xmlBinder(&binder);
        char* buffer = (char *) xmlBinder.initialize(BUFFER_SIZE);
        unsigned int readlen;

    	while ( input.good() ) {

    		input.read(buffer, BUFFER_SIZE);
    		readlen = input.gcount();

#ifdef LOG_LEVEL_TRACE
    		std::string filecontent(buffer, readlen);
        	TRACE("Parsing config data line: %s", filecontent.c_str());
#endif
            xmlBinder.parse(readlen, !input.good());
    	}

#ifdef LOG_LEVEL_TRACE
    	MessageBusManager::instance()->debug("Message bus state after loading/refreshing config: ");
#endif

    } catch (CException* e) {

        ERROR("Exception parsing service config: %s", e->getMessage());
    }
}

} /* namespace mb */
