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

#include "DynaModel.h"

using namespace binding;


namespace mb {


class ServiceConfigBinder : public ServiceConfig {

public:
	ServiceConfigBinder(ServiceConfigManager* manager) {

		m_manager = manager;

		ServiceConfigBinder::addBeginRule("*/bindings", beginBindingsConfig);
		ServiceConfigBinder::addBeginRule("*/bind", beginBindConfig);
		ServiceConfigBinder::addBeginRule("*/bind/parse", beginBindParseRule);
		ServiceConfigBinder::addBeginRule("*/bind/parse/mapping", beginBindParseValueMapping);
		ServiceConfigBinder::addEndRule("*/bind", endBindConfig);
		ServiceConfigBinder::addEndRule("*/bindings", endBindingsConfig);
	}

    void beginBinding() {

        m_error = false;
    }

    void addService(Service* service) {

    	TRACE("Binding service '%s' configuration details...", service->getSubject());
    	m_manager->m_services[service->getSubject()] = boost::shared_ptr<Service>(service);
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
        	service->setBindingConfig(dataBinder->m_bindingConfig);

        dataBinder->m_bindingConfig.reset();
    }

private:

    ServiceConfigManager* m_manager;
    DynaModelBindingConfigPtr m_bindingConfig;
};


// **** ServiceConfigManager Singleton ****

STATIC_INIT_NULL_IMPL(ServiceConfigManager)

SINGLETON_MANAGER_IMPLEMENTATION(ServiceConfigManager, ServiceConfigException)

// **** Implementation ****

std::vector<BeginConfigBinding*> ServiceConfigManager::_beginConfigBindings;
std::vector<EndConfigBinding*> ServiceConfigManager::_endConfigBindings;

ServiceConfigManager::ServiceConfigManager() {
}

ServiceConfigManager::~ServiceConfigManager() {
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

void ServiceConfigManager::addConfig(const char* uri, int refresh) {
}

void ServiceConfigManager::loadConfig(const char* data) {
}

} /* namespace mb */
