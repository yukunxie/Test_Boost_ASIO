// test_asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/pool/singleton_pool.hpp"
#include <time.h>
#include <atomic>
#include <iostream>
#include <sstream>
#include <random>
#include <thread>
#include <mutex>

class MicroLockerMutex {
	std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
	MicroLockerMutex() = default;
	MicroLockerMutex(const MicroLockerMutex&) = delete;
	MicroLockerMutex& operator= (const MicroLockerMutex&) = delete;
	void lock() {
		while (flag.test_and_set(std::memory_order_acquire));
	}
	void unlock() {
		flag.clear(std::memory_order_release);
	}
};

class MicroLocker
{
public:
	MicroLocker(MicroLockerMutex & mutex) : lock(mutex)
	{}
private:
	std::lock_guard<MicroLockerMutex> lock;
};


MicroLockerMutex sm;
boost::mutex global_stream_lock;

void job(int task_id);

void producer(boost::shared_ptr< boost::asio::io_service > io_service)
{
	{
		//std::lock_guard<MicroLockerMutex> lock(sm);
		MicroLocker lock(sm);
		std::cout << "producer" << std::endl;
	}
	
	//global_stream_lock.lock();
	//std::cout << "producer" << std::endl;
	//global_stream_lock.unlock();

	for (int i = 0; i < 10; i++)
	{
		io_service->post(boost::bind(job, i));
		std::random_device rd;
		std::chrono::milliseconds dura(rd() % 100);
		//std::this_thread::sleep_for(dura);
	}
}

void consumer(boost::shared_ptr< boost::asio::io_service > io_service)
{
	{
		MicroLocker lock(sm);
		std::cout << "consumer" << " threadid=" << std::this_thread::get_id() << std::endl;
	}

	//global_stream_lock.lock();
	//std::cout << "consumer" << " threadid=" << std::this_thread::get_id() << std::endl;
	//global_stream_lock.unlock();

	io_service->run();
}

void job(int task_id)
{
	{
		MicroLocker lock(sm);
		std::cout << "taskid: " << task_id << " threadid=" << std::this_thread::get_id() << std::endl;
	}
	//global_stream_lock.lock();
	//std::cout << "taskid: " << task_id << " threadid=" << std::this_thread::get_id()  << std::endl;
	//global_stream_lock.unlock();
	
	std::random_device rd;
	std::chrono::milliseconds dura(rd() % 1000);
	//std::this_thread::sleep_for(dura);
}

int main()
{

	int max = 10000000;

	clock_t start = clock();
	for (int i = 0; i < max; ++i)
	{
		global_stream_lock.lock();
		global_stream_lock.unlock();
	}
	std::cout << clock() - start << std::endl;

	start = clock();
	for (int i = 0; i < max; ++i)
	{
		//MicroLocker lock(sm);
		std::lock_guard<MicroLockerMutex> lock(sm);
	}
	std::cout << clock() - start << std::endl;

	start = clock();
	for (int i = 0; i < max; ++i)
	{
		//MicroLocker lock(sm);
		//std::lock_guard<MicroLockerMutex> lock(sm);
		sm.lock();
		sm.unlock();
	}
	std::cout << clock() - start << std::endl;

	std::thread::id id;
	start = clock();
	for (int i = 0; i < max; ++i)
	{
		//MicroLocker lock(sm);
		//std::lock_guard<MicroLockerMutex> lock(sm);
		id = std::this_thread::get_id();
	}
	std::cout << clock() - start << " " << id << std::endl;

	start = clock();
	for (int i = 0; i < max; ++i)
	{
	}
	std::cout << clock() - start << std::endl;

	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%dabc", (int)time_t());


	using namespace boost::posix_time;
	ptime now = second_clock::universal_time();

	std::wstring ws(FormatTime(now));
	std::wcout << ws << std::endl;
	

	time_t seconds;
	time(&seconds);
	std::string ts = boost::lexical_cast<std::string>(seconds);
	std::cout << ts << ":" << time(&seconds) << std::endl;;

	boost::shared_ptr< boost::asio::io_service > io_service(new boost::asio::io_service);

	std::thread _p(producer, io_service);
	std::thread _c(consumer, io_service);
	std::thread _c1(consumer, io_service);
	_p.join();
	_c.join();
	_c1.join();
	system("pause");
	return 0;
}
