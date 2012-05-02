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

#if !defined(_MANAGER_H__INCLUDED_)
#define _MANAGER_H__INCLUDED_

#include <list>

#include <boost/thread/mutex.hpp>

#include "log.h"
#include "exception.h"


namespace mb {

/* Callback to perform platform specific thread startup
 */
typedef void* (*ThreadBeginCallback)();

/* Callback to perform platform specific thread cleanup
 */
typedef void (*ThreadEndCallback)(void* context);

//////////////////////////////////////////////////////////////////////
// This class defines a base instance for a collection of singletons
// that manage certain types of similar services or resources.

class Manager {

public:

	Manager();
	virtual ~Manager();

	static void destroy();

	static void bringToForeground();
	static void sendToBackground();
    
    static void setThreadCallbacks(ThreadBeginCallback beginCallback, ThreadEndCallback endCallback);

    static void* onBeginManagerThread();
    static void onEndManagerThread(void* context);

protected:

	virtual void foreground() = 0;
	virtual bool background() = 0;

private:

	static std::list<Manager*> _managers;
    
    static ThreadBeginCallback _threadBeginCallback;
    static ThreadEndCallback _threadEndCallback;
};
    

}  // namespace : mb


#define SINGLETON_MANAGER(type_name) \
	public: \
		static void initialize(); \
		static void destroy(); \
		static type_name* instance(); \
	private: \
		static type_name* _manager; \
		static boost::mutex _manager_access_lock;


#define SINGLETON_MANAGER_IMPLEMENTATION(type_name, exception_name) \
	class exception_name : public CException { \
	public: \
		exception_name(const char* source, int lineNumber) : CException(source, lineNumber) { } \
		virtual ~exception_name() { } \
	}; \
	type_name* type_name::_manager = NULL; \
	boost::mutex type_name::_manager_access_lock; \
	void type_name::initialize() { \
		boost::mutex::scoped_lock lock(type_name::_manager_access_lock); \
		if (type_name::_manager) { \
			TRACE(#type_name " singleton already initialized. Ignoring initialize()"); \
			return; \
		} \
		type_name::_manager = new type_name(); \
	} \
	void type_name::destroy() { \
		boost::mutex::scoped_lock lock(type_name::_manager_access_lock); \
		if (type_name::_manager) { \
			delete type_name::_manager; \
			type_name::_manager = NULL; \
		} \
	} \
	type_name* type_name::instance() { \
		if (!type_name::_manager) \
			THROW(exception_name, EXCEP_MSSG(#type_name " singleton has not been initialized.")); \
		return type_name::_manager; \
	}


#endif /* _MANAGER_H__INCLUDED_ */
