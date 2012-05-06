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
//
// Executor.h : Implementation of an executor that assigns a work item to
// a pooled thread.
//

#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/thread.hpp"

class Executor {
public:

	Executor(size_t n)
		: m_service(n), m_work(m_service) {

		std::size_t (boost::asio::io_service::*run)(void) = &boost::asio::io_service::run;

		for (size_t i = 0; i < n; i++) {
			m_pool.create_thread(boost::bind(run, &m_service));
		}
	}

	~Executor() {
		m_service.stop();
		m_pool.join_all();
	}

	template<typename F> void submit(F task) {
		m_service.post(task);
	}

protected:
	boost::thread_group m_pool;
	boost::asio::io_service m_service;
	boost::asio::io_service::work m_work;
};

#endif /* EXECUTOR_H_ */
