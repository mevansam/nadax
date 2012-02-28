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

#endif /* _MANAGER_H__INCLUDED_ */
