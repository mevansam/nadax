// The MIT License
//
// Copyright (c) 2012 Mevan Samaratunga
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
 * @file objectpool.cpp
 * @author Mevan Samaratunga
 *
 * @brief Implements ObjectPoolEvictionService which manages ObjectPool instances' management thread.
 **/

#include "objectpool.h"

#include "log.h"

/// Temporary time interval used when setting up a boost::asio::deadline_timer
// which an ObjectPool will use to schedule its management funtion execution.
#define TEMP_INTERVAL  3600

/// Singleton object pool eviction service
boost::shared_ptr<ObjectPoolEvictionService> _service = boost::shared_ptr<ObjectPoolEvictionService>(new ObjectPoolEvictionService());
/// Mutex to guard creation/deletion of timers and management thread (if not running).
boost::mutex _lock;


ObjectPoolEvictionService::ObjectPoolEvictionService() {
}

ObjectPoolEvictionService::~ObjectPoolEvictionService() {

	TRACE("Stopping object pool eviction services...");
	m_ioService.stop();
	TRACE("Done stopping object pool eviction services...");
}

boost::asio::deadline_timer* ObjectPoolEvictionService::startEvictTimer() {

	boost::mutex::scoped_lock lock(_lock);

	boost::asio::deadline_timer* timer = new boost::asio::deadline_timer(_service->m_ioService, boost::posix_time::seconds(TEMP_INTERVAL));
	_service->m_timers.push_back(boost::shared_ptr<boost::asio::deadline_timer>(timer));

	if (!_service->m_evictThread.get())
		_service->m_evictThread = boost::shared_ptr<boost::thread>(new boost::thread(&ObjectPoolEvictionService::evictTimer, _service.get()));

	return timer;
}

void ObjectPoolEvictionService::stopEvictTimer(boost::asio::deadline_timer* timer) {

	boost::mutex::scoped_lock lock(_lock);

	timer->cancel();

	std::vector<boost::shared_ptr<boost::asio::deadline_timer> >::iterator i = _service->m_timers.begin();
	std::vector<boost::shared_ptr<boost::asio::deadline_timer> >::iterator end = _service->m_timers.end();

	while (i < end) {

		if (i->get() == timer) {

			_service->m_timers.erase(i);
			break;
		}
		++i;
	}
}

void ObjectPoolEvictionService::evictTimer() {

	while (true) {

		TRACE("Running current object pool eviction timers...");

		m_ioService.run();
		m_ioService.reset();

		{ boost::mutex::scoped_lock lock(_lock);

			// If there are no more ObjectPool scheduled timers then
			// end the thread execution. The thread will be recreated
			// next time a managed ObjectPool comes into context.
			if (!_service->m_timers.size()) {

				TRACE("Object pool eviction timer thread ended...");
				_service->m_evictThread.reset();
				break;
			}
		}
	}
}
