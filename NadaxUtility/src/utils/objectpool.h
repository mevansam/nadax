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
 * @file objectpool.h
 * @author Mevan Samaratunga
 *
 * @brief Object pool template definition which implements a managed pool of objects.
 **/

#ifndef OBJECTPOOL_H_
#define OBJECTPOOL_H_

#include <vector>
#include <deque>

#include "boost/exception/all.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"
#include "boost/asio.hpp"

#include "number.h"

/** @defgroup errorCodes Pool error codes
 *
 *  @{
 */

/// Thrown if all objects are allocated and a new object
/// is requested from a pool which is configured with timeout=0
#define ERROR_ALL_OBJECTS_ALLOCATED  1
/// Thrown if an object cannot be allocated from a pool
/// which is at maximum capacity after waiting the given timeout
/// for an object to be returned to the pool.
#define ERROR_TIMED_OUT_WAITING_FOR_OBJECT  2
/// Thrown if the callback to create an instance of an object
/// to be pooled returns NULL.
#define ERROR_CREATING_OBJECT  3


/** @} */

/// Object pool exception type
typedef struct _pool_error : virtual std::exception, virtual boost::exception { } pool_error;
/// Exception message type which can be used to retrieve the error message from boost::exception.
typedef boost::error_info<struct tag_pool_error_msg, std::string> pool_error_msg;

template <class T> class ObjectPool;

/**
 * @brief Singleton service that implements the eviction
 * thread. This class should never be instantiated directly.
 */
class ObjectPoolEvictionService {

public:
	ObjectPoolEvictionService();
	virtual ~ObjectPoolEvictionService();

private:
	static boost::asio::deadline_timer* startEvictTimer();
	static void stopEvictTimer(boost::asio::deadline_timer* timer);
	void evictTimer();

private:
	boost::asio::io_service m_ioService;
	std::vector<boost::shared_ptr<boost::asio::deadline_timer> > m_timers;

	Number<bool> m_runEvictThread;
	boost::shared_ptr<boost::thread> m_evictThread;

	template <class T> friend class ObjectPool;
};

/**
 * @brief Object pool for managing the lifecycle of a collection of pooled objects of type <T>.
 *
 * @detail This implementation is different to
 * <a href="http://www.boost.org/doc/libs/1_49_0/libs/pool/doc/html/index.html">boost::pool</a>,
 * which is a memory management solution vs. pooling objects that are costly to instantiate
 * such as connections to a remote system. If the pool is configured with an evict interval
 * it will schedule a pool clean up via the shared evict thread. The evict thread ensures a
 * minimum number of instantiated objects are always available for instant retrieval via
 * getObject(). The pool management function may be configured to keep objects around for
 * a particular time interval to work as efficiently as possible without unnecessary creation
 * and destruction of objects when the pool is operating under high throughput scenarios.
 *
 */
template <class T>
class ObjectPool
{
public:

	/**
	 * Constructor creates a default pool with no management
	 * capability. The default pool configuration will grow
	 * indefinitely. Objects returned to the pool will simply
	 * be cached and reallocated but will not be destroyed
	 * until the pool is destroyed.
	 */
	ObjectPool() {

		m_size = -1;
		m_max = -1;
		m_allocated = 0;

		m_timeout = -1;
		m_lingerTime = -1;
		m_evictChecks = -1;

		m_evictTimer = NULL;
	}

	/**
	 * The destructor will cancel any pool management timers.
	 * If there are no more pool thread is not tracking timers
	 * from any other pools then the thread itself will end.
	 * when the pool is destroyed.
	 */
	virtual ~ObjectPool() {

		if (m_evictTimer)
			ObjectPoolEvictionService::stopEvictTimer(m_evictTimer);
	}

	/// Returns the number of objects that have been allocated for use.
	int getAllocatedSize() {
		return m_allocated;
	}

	/// Returns the number of instantiated and available objects in the pool.
	int getUnallocatedPoolSize() {
		return m_pool.size();
	}

	/**
	 * @brief Sets the pool size.
	 *
	 * @param size The default size of the pool. If the pool managment thread
	 * 		is running the thread will always make sure the number of unallocated
	 * 		and available objects are equal to this parameter as long as the
	 * 		maximum size condition is met.
	 * @param max This is the maximum number of objects that can be pooled and
	 * 		allocated. If this value is -1 then there is no maximum size limit
	 * 		and the pool will grow indefinitely.
	 * 	@param timeout This is the amount of milliseconds to wait for an object
	 * 		to be returned to the pool if the pool has reached its maximum size.
	 * 		If this parameter is 0 then this method will return immediately with
	 * 		an exception. If it is -1 then requesting for an object from a pool
	 * 		that is reached the maximum allocation size then the caller will
	 * 		wait indefinitely.
	 */
	void setPoolSize(int size, int max = -1, long timeout = -1) {

		boost::lock_guard<boost::mutex> lock(m_poolM);

		m_size = size;
		m_max = max;
		m_timeout = timeout;

		while ((int) m_pool.size() < size)
			m_pool.push_back(PooledObject<T>(createSafe()));
	}

	/**
	 * @brief Sets pool management configuration.
	 *
	 * @param evictInterval This is the interval in millis that the pool eviction
	 * 		or management functionality will kick in. When the timer is setup if
	 * 		the singleton pool management service/thread is not running it will
	 * 		be started.
	 * @param lingerTime This is the amount of time in millis an object that has
	 * 		been returned to the pool will be kept around until it becomes elligible
	 * 		for destruction. If the object's linger time has expired then it will
	 * 		be destroyed next time the pool's management thread determines it needs
	 * 		to remove pooled and available objects in order to shrink the number of
	 * 		available objects to the default size.
	 * @param evictChecks This value sets the number of objects that will be checked
	 * 		for eviction eligibility when the number of available objects needs to
	 * 		shrink. Setting this value to -1 will mean the number of objects checked
	 * 		for eviction will be same as the default size.
	 */
	void setPoolManagement(long evictInterval, long lingerTime = 30000, int evictChecks = -1) {

		if (evictInterval > 0) {

			boost::lock_guard<boost::mutex> lock(m_poolM);

			m_lingerTime = lingerTime;
			m_evictChecks = (evictChecks == -1 ? m_size : evictChecks);

			// Initializes a boost::asio::deadline_time which is run via the boost::asio::io_service
			// within the singleton pool management thread. This service keeps track of all timers
			// configured by all object pools.
			m_evictInterval = boost::posix_time::time_duration(boost::posix_time::milliseconds(evictInterval));
			m_evictTimer = ObjectPoolEvictionService::startEvictTimer();

			scheduleNextEvict();

			typename std::deque<PooledObject<T> >::iterator i = m_pool.begin();
			typename std::deque<PooledObject<T> >::iterator end = m_pool.end();

			while (i != end) {

				i->setExpiry(lingerTime);
				++i;
			}

		}
	}

	/**
	 * @brief Returns a pooled object for use.
	 *
	 * This method retrieves an instance of an object for use. It first checks the front of the
	 * queue that maintains pooled available objects. If there are no pooled instances then it
	 * will create one.
	 *
	 * This method may return 3 boost::exception conditions via ::pool_error. The error
	 * code of a thrown exception can be queried via boost::errinfo_errno.
	 *
	 * If this method is called to retrieve an object from a pool which has reached its
	 * maximum size then it will:
	 * 		@li immediately throw an exception with error code #ERROR_ALL_OBJECTS_ALLOCATED if timeout=0.
	 * 		@li throw error code #ERROR_TIMED_OUT_WAITING_FOR_OBJECT, if timeout>0 and no object becomes
	 * 			available within the given timeout.
	 * 		@li wait indefinitely until an object becomes available if timeout=-1.
	 *
	 * This method may also throw #ERROR_CREATING_OBJECT if an error occurs while instantiating an object
	 * when there are no more available pooled instances but the pool size < maximum.
	 *
	 * @return A boost::shared pointer to an instance of the template type.
	 */
	boost::shared_ptr<T> getObject() {

		bool create = true;
		boost::shared_ptr<T> object;

		{ boost::unique_lock<boost::mutex> lock(m_poolM);

			if (m_max > 0) {

				while (m_allocated == m_max) {

					if (m_timeout == 0) {

						throw _pool_error()
							<< pool_error_msg("All pooled objects have been allocated.")
							<< boost::errinfo_errno(ERROR_ALL_OBJECTS_ALLOCATED);

					} if (m_timeout > 0) {

						const boost::system_time timeout = boost::get_system_time() + boost::posix_time::milliseconds(m_timeout);

						if (!m_poolC.timed_wait(lock, timeout)) {

							throw _pool_error()
								<< pool_error_msg("Timed out waiting for pooled object.")
								<< boost::errinfo_errno(ERROR_TIMED_OUT_WAITING_FOR_OBJECT);
						}

					} else
						m_poolC.wait(lock);
				}
			}

			if (m_pool.size()) {

				object = m_pool.front().object;
				m_pool.pop_front();
				create = false;
			}

			++m_allocated;
		}

		try {

			if (create)
				object = boost::shared_ptr<T>(createSafe());

			this->activate(object.get());
			return object;

		} catch (...) {

			removeReference();
			throw;
		}
	}

	/**
	 * @brief Returns an object to the pool.
	 *
	 * Returns the object to the pool. This object is wrapped in a ObjectPool#PooledObject
	 * structure which contains both a boost::shared pointer to the freed object and the
	 * time it will expire. The ObjectPool#PooledObject is then placed at the back of the
	 * std::deque that holds all available objects. This ensures that the objects that
	 * expire the soonest will always be at the front of the queue and will be allocated
	 * first. It also eliminates unnecessary searching when the pool management thread
	 * needs to select expired objects to evict to shrink the pool to maintain the
	 * default size.
	 */
	void returnObject(boost::shared_ptr<T> object) {

		if (object.get()) {

			try {

				this->passivate(object.get());

			} catch (...) {

				removeReference();
				throw;
			}

			{ boost::lock_guard<boost::mutex> lock(m_poolM);

				m_pool.push_back(PooledObject<T>(object, m_lingerTime));
				--m_allocated;
				m_poolC.notify_one();
			}
		}
	}

	/**
	 * @brief This method determines if the pool needs to shrink or grow to maintain
	 * the default size.
	 *
	 * If the pool of available objects < default size then this method will add more
	 * objects until the default size is reached or the pool reaches its specified
	 * maximum. If the available objects > default then this method will remove objects
	 * until the number of pooled objects is equal to the default size. The number of
	 * objects are instantiated or destroyed will always be within the given number of
	 * evict checks to ensure this method finishes within a reasonable finite amount
	 * of time that will not affect performance.
	 *
	 * This method can be overridden to provide custom pool management logic. However,
	 * this method must always be called by the overriding method to ensure the
	 * integrity of the pool.
	 */
	virtual void evict() {

		int poolSize = m_pool.size();

		if (poolSize < m_size) {

			// The size to grow by is the minimum of default size
			// and remaining size to pool max. This value is further
			// restricted by the number of checks that have been
			// configured. Hence, on each call the pool will grow
			// by the number of checks until default size is reached.

			int i, size, delta = m_size - poolSize;

			if (m_max > 0) {

				size = m_max - m_allocated - poolSize;
				if (size > m_size) size = m_size;
 			} else
 				size = m_size;

			if (size > m_evictChecks) size = m_evictChecks;
			if (size > delta) size = delta;

			for (i = 0; i < size; i++)
				m_pool.push_back(PooledObject<T>(createSafe(), m_lingerTime));

		} else {

			// To evict objects simply cycle through the queue of
			// pooled objects the number of configured checks,
			// and remove any expired objects from the front until
			// the default pool size is reached. This ensures only
			// expired objects at the front of a queue that is
			// larger than the default size are removed. Since
			// objects are returned to the back of the queue
			// the pre-condition for this algorithm that the pool
			// will be in ascending order of expiration time is
			// satisfied.

			int i;
			for (i = 0; i < m_evictChecks; i++) {

				if ((int) m_pool.size() > m_size) {

					if (m_pool.front().hasExpired())
						m_pool.pop_front();
					else
						break;

				} else
					break;
			}
		}
	}

protected:

	/**
	 * @brief Callback to create an instance of the template type.
	 *
	 * This is a pure virtual function that must be implemented to return a valid type.
	 * It may run some initialization but any initialization that needs to be done
	 * before this object can be used must always be done in the ObjectPool::activate()
	 * callback method.
	 *
	 * @return An instance of the object to be pooled or NULL if it could not be created.
	 */
	virtual T* create() = 0;

	/// Callback to activate an instance of the template type so that it can be used.
	virtual void activate(T* object) { }

	/// Callback to cleanup an instance which is being returned to the pool.
	virtual void passivate(T* object) { }

private:

	T* createSafe() {

		// Calls the callback to create an instance of an object. If a NULL is returned
		// then ::pool_error exception with error code #ERROR_CREATING_OBJECT is thrown.
		T* o = this->create();
		if (!o) {

			throw _pool_error()
				<< pool_error_msg("Unable to create pooled object.")
				<< boost::errinfo_errno(ERROR_CREATING_OBJECT);
		}
		return o;
	}

	void removeReference() {

		// Removes an object from list of objects that have been allocated. This
		// is a convenience method for decrementing the count of objects that are
		// being tracked.
        boost::lock_guard<boost::mutex> lock(m_poolM);
        --m_allocated;
        m_poolC.notify_one();
	}

	void scheduleNextEvict() {

		// Schedules the next execution of the pool management function ObjectPool::evict().
		m_evictTimer->expires_from_now(m_evictInterval);
		m_evictTimer->async_wait(boost::bind(&ObjectPool<T>::evictTimer, this, _1));
	}

	void evictTimer(const boost::system::error_code& error) {

		// Asynchronous call back by the boost::asio::deadline_timer instance which
		// is the scheduled execution of this pools management function ObjectPool::evict().

		if (error != boost::asio::error::operation_aborted) {

			boost::lock_guard<boost::mutex> lock(m_poolM);

			try {
				this->evict();
			} catch (...) {
			}

			scheduleNextEvict();
		}
	}

private:

	int m_size;
	int m_max;
	int m_allocated;

	long m_timeout;
	long m_lingerTime;

	int m_evictChecks;

	boost::mutex m_poolM;
	boost::condition_variable m_poolC;

	/**
	 * @brief Container for the pooled object used to wrap it before adding
	 * it to the internal queue of instantiated and available objects. This
	 * is an internal private structure.
	 */
	template <class PT>
	struct PooledObject {

		PooledObject() { }

		PooledObject(PT* o)
			: object(o) { }

		PooledObject(PT* o, long lingerTime)
			: object(o),  expiry(boost::get_system_time() + boost::posix_time::milliseconds(lingerTime)) { }

		PooledObject(boost::shared_ptr<PT> o, long lingerTime)
			: object(o),  expiry(boost::get_system_time() + boost::posix_time::milliseconds(lingerTime)) { }

		PooledObject(const PooledObject& po)
			: object(po.object), expiry(po.expiry) { }

		void setExpiry(long lingerTime) {

			expiry = boost::get_system_time() + boost::posix_time::milliseconds(lingerTime);
		}

		bool hasExpired() {

			return (boost::get_system_time() > expiry);
		}

		PooledObject& operator= (const PooledObject& po) {

			if (this != &po) {

				object = po.object;
				expiry = po.expiry;
			}
			return *this;
		}

		boost::shared_ptr<PT> object;
		boost::system_time expiry;
	};

	std::deque<PooledObject<T> > m_pool;

	boost::posix_time::time_duration m_evictInterval;
	boost::asio::deadline_timer* m_evictTimer;
};


#endif /* OBJECTPOOL_H_ */
