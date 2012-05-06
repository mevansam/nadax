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
 * @file objectpool.h
 * @author Mevan Samaratunga
 *
 * @brief Object pool template class.
 * @details	This template implements an object pool.
 **/

#ifndef OBJECTPOOL_H_
#define OBJECTPOOL_H_

#include <deque>

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"


template <class T>
class ObjectPool
{
public:

	ObjectPool() {
	}

	virtual ~ObjectPool() {
	}

	void setPoolSize(size_t size, size_t max = -1, long timeout = -1);

	void setPoolManagement(long reapInterval, long lingerTime = 30);

	boost::shared_ptr<T> getObject() {

		boost::shared_ptr<T> object;

		{ boost::mutex::scoped_lock lock(m_poolLock);

			if (m_pool.size()) {

				object = m_pool.front();
				m_pool.pop_front();
			}
		}

		if (!object.get())
			object = boost::shared_ptr<T>(this->create());

		this->activate(object.get());
		return object;
	}

	void returnObject(boost::shared_ptr<T> object) {

		this->passivate(object.get());

		{ boost::mutex::scoped_lock lock(m_poolLock);

			m_pool.push_back(object);
		}
	}

protected:

	virtual T* create() = 0;
	virtual void activate(T* object) { };
	virtual void passivate(T* object) { };

private:

	boost::mutex m_poolLock;
	std::deque<boost::shared_ptr<T> > m_pool;
};


#endif /* OBJECTPOOL_H_ */
