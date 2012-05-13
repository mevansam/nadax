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

#include <iostream>
#include <iomanip>

#include <boost/test/unit_test.hpp>

#include "objectpool.h"
#include "number.h"
#include "log.h"

#define POOL_SIZE             3
#define POOL_MAX1             12
#define POOL_MAX2             6
#define POOL_NUM_EVICT_CHECK  2

Number<int> _counter(0);

class TestObject {

public:

	TestObject() {

		id = _counter++;
		isValid = true;

		TRACE("Constructing object: %d", id);
	}

	virtual ~TestObject() {

		TRACE("Destroying object: %d (%s)", id, (isValid ? "valid" : "invalid"));
	}

	int id;
	bool isValid;
};

class TestObjectPool : public ObjectPool<TestObject> {

public:
	TestObjectPool(const char* n)
		: name(n) {
	}

	virtual ~TestObjectPool() {
	}

protected:

	virtual TestObject* create() {

		TestObject* o = new TestObject();;
		TRACE("Returning new object to be pooled from pool '%s': %p", name.c_str(), o);
		return o;
	}

	virtual void activate(TestObject* object) {

		TRACE("Activating pooled object in pool '%s': %p", name.c_str(), object);
	}

	virtual void passivate(TestObject* object) {

		TRACE("Passivating pooled object in pool '%s': %p", name.c_str(), object);
	}

	virtual void evict() {

		int size = ObjectPool<TestObject>::getUnallocatedPoolSize();
		int allocated = ObjectPool<TestObject>::getAllocatedSize();

		TRACE("Begin running eviction thread for '%s': allocated = %d, pool size = %d", name.c_str(), allocated, size);
		ObjectPool<TestObject>::evict();
		TRACE("End running eviction thread for '%s': pool size = %d", name.c_str(), ObjectPool<TestObject>::getUnallocatedPoolSize());
	}

private:

	std::string name;
};

BOOST_AUTO_TEST_CASE( object_pool_test ) {

	std::cout << std::endl << "Begin object pooling tests..." << std::endl;

	TestObjectPool pool1("testPool1");
	pool1.setPoolSize(POOL_SIZE, POOL_MAX1, 5000);
	pool1.setPoolManagement(2000, 1000, POOL_NUM_EVICT_CHECK);

	TestObjectPool pool2("testPool2");
	pool2.setPoolSize(POOL_SIZE, POOL_MAX2, 0);
	pool2.setPoolManagement(2000, 1000, POOL_NUM_EVICT_CHECK);

	boost::shared_ptr<TestObject> o[20];

	std::cout << std::endl << "Allocating 5 (0,2,4,6,8) objects from pool1 and (1,3,5,7) from pool 2." << std::endl;
	o[0] = pool1.getObject();
	o[1] = pool2.getObject();
	o[2] = pool1.getObject();
	o[3] = pool2.getObject();
	o[4] = pool1.getObject();
	o[5] = pool2.getObject();
	o[6] = pool1.getObject();
	o[7] = pool2.getObject();
	o[8] = pool1.getObject();
	std::cout << "curr allocated size = " << pool1.getAllocatedSize() << std::endl;
	BOOST_REQUIRE_MESSAGE(pool1.getAllocatedSize() == 5, "Number allocated from pool is not consistent.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(5);
	BOOST_REQUIRE_MESSAGE(pool1.getUnallocatedPoolSize() == POOL_SIZE, "Pool did not grow to minimum allocated size.");

	std::cout << std::endl << "Allocating 5 (9,10,11,12,13) more objects from pool1." << std::endl;
	o[9] = pool1.getObject();
	o[10] = pool1.getObject();
	o[11] = pool1.getObject();
	o[12] = pool1.getObject();
	o[13] = pool1.getObject();
	std::cout << "curr allocated size = " << pool1.getAllocatedSize() << std::endl;
	BOOST_REQUIRE_MESSAGE(pool1.getAllocatedSize() == 10, "Number allocated from pool is not consistent.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(5);
	BOOST_REQUIRE_MESSAGE(pool1.getUnallocatedPoolSize() == 2, "Pool did not grow to fill in remaining slots to max size.");

	std::cout << std::endl << "Allocating another 2 (14,15) more objects from pool1." << std::endl;
	o[14] = pool1.getObject();
	o[15] = pool1.getObject();
	std::cout << "curr allocated size = " << pool1.getAllocatedSize() << std::endl;
	BOOST_REQUIRE_MESSAGE(pool1.getAllocatedSize() == 12, "Number allocated from pool is not consistent.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(5);
	BOOST_REQUIRE_MESSAGE(pool1.getUnallocatedPoolSize() == 0, "Pool size grew even though max size objects have been allocated.");

	try {

		std::cout << std::endl << "Attempting to allocate an object from a maxed out pool." << std::endl;
		o[16] = pool1.getObject();

		BOOST_REQUIRE_MESSAGE(false, "Get on maxed pool did not timeout.");

	} catch (boost::exception& e) {

		int errorCode = *boost::get_error_info<boost::errinfo_errno>(e);
		BOOST_REQUIRE_MESSAGE(errorCode == ERROR_TIMED_OUT_WAITING_FOR_OBJECT, "An exception was thrown when attempting a get on a maxed pool but it was not a timeout exception.");
	}

	std::cout << std::endl << "Returning 3 (11) objects to pool1." << std::endl;
	pool1.returnObject(o[11]); o[11].reset();
	std::cout << "curr allocated size = " << pool1.getAllocatedSize() << std::endl;
	BOOST_REQUIRE_MESSAGE(pool1.getAllocatedSize() == 11, "Number allocated from pool is not consistent.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(5);
	BOOST_REQUIRE_MESSAGE(pool1.getUnallocatedPoolSize() == 1, "Pool size is not consistent with remaining slots to max size.");

	std::cout << std::endl << "Returning 3 (15,13,10) objects to pool1." << std::endl;
	pool1.returnObject(o[15]); o[15].reset();
	pool1.returnObject(o[13]); o[13].reset();
	pool1.returnObject(o[10]); o[10].reset();
	std::cout << "curr allocated size = " << pool1.getAllocatedSize() << std::endl;
	BOOST_REQUIRE_MESSAGE(pool1.getAllocatedSize() == 8, "Number allocated from pool is not consistent.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(5);
	BOOST_REQUIRE_MESSAGE(pool1.getUnallocatedPoolSize() == POOL_SIZE, "Unexpected pool size.");

	std::cout << std::endl << "Returning 4 (9,8,6,4,2) more objects to pool1." << std::endl;
	pool1.returnObject(o[9]); o[9].reset();
	pool1.returnObject(o[8]); o[8].reset();
	pool1.returnObject(o[6]); o[6].reset();
	pool1.returnObject(o[4]); o[4].reset();
	pool1.returnObject(o[2]); o[2].reset();
	std::cout << "curr allocated size = " << pool1.getAllocatedSize() << std::endl;
	BOOST_REQUIRE_MESSAGE(pool1.getAllocatedSize() == 3, "Number allocated from pool is not consistent.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(10);
	BOOST_REQUIRE_MESSAGE(pool1.getUnallocatedPoolSize() == POOL_SIZE, "Unexpected pool size.");

	std::cout << std::endl << "Returning another 2 (0) more objects to pool1." << std::endl;
	pool1.returnObject(o[0]); o[0].reset();
	std::cout << "curr allocated size = " << pool1.getAllocatedSize() << std::endl;
	BOOST_REQUIRE_MESSAGE(pool1.getAllocatedSize() == 2, "Number allocated from pool is not consistent.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(5);
	BOOST_REQUIRE_MESSAGE(pool1.getUnallocatedPoolSize() == POOL_SIZE, "Unexpected pool size.");

	std::cout << "Pause 5 secs." << std::endl;
	sleep(5);
	std::cout << std::endl << "End object pooling tests..." << std::endl;
}
