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

#ifndef STATICINIT_H_
#define STATICINIT_H_


/** @defgroup static Static initialization
 *
 * @brief Macros for initializing the static state of a class.
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
		static bool __static_init_##className##_() { \
			static bool _initialized = false; \
			static boost::mutex _mutex; \
			boost::mutex::scoped_lock lock(_mutex); \
			if (!_initialized) { \
				TRACE(#className " static state being initialized...");  \
				_initialized = className::_static_init(); \
			} \
			return _initialized; \
		}; \
	private: \
		static bool _static_init();
#define STATIC_INIT_CALL(className) \
	static bool _init##className##Result = className::__static_init_##className##_();
#define STATIC_INIT_NULL_IMPL(className) \
	bool className::_static_init() { _init##className##Result = false; return true; }

/** }@ */

#endif /* STATICINIT_H_ */
