#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "test_util.h"

#include "shared_mutex.h"

#include <atomic>
#include <list>

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <thread>

#define THREADS 4
#define LOOPS 1000

int test1()
{
	char output[1024];

	printf("test 1:\n");
	printf("using gx::shared_mutex in multiple readers / multiple writers mode\n");

	volatile long long value = 0;
	volatile long long * ptr_value = &value;

	gx::shared_mutex m(gx::shared_mutex::yield); // yield is the default and seemingly the best mode

	auto thread1 = [&]
	{
		for (uint32_t i = 0; i < LOOPS; i++)
		{
			gx::shared_lock l(m);
			l.upgrade_to_exclusive();
			ptr_value = nullptr;
			rand_wait();
			ptr_value = &value;
			*ptr_value = my_rand();
			l.downgrade_to_shared();
			volatile long long intvalue = 0;
			for (uint32_t i = 0; i < 1000; i++)
			{
				intvalue += *ptr_value * my_rand();
			}
			l.upgrade_to_exclusive();
			*ptr_value = intvalue;
		}
	};

	auto thread2 = [&]
	{
		for (uint32_t i = 0; i < LOOPS; i++)
		{
			gx::exclusive_lock l(m);
			ptr_value = nullptr;
			rand_wait();
			ptr_value = &value;
			*ptr_value = my_rand();
			l.downgrade_to_shared();
			volatile long long intvalue = 0;
			for (uint32_t i = 0; i < 1000; i++)
			{
				intvalue += *ptr_value * my_rand();
			}
			l.upgrade_to_exclusive();
			*ptr_value = intvalue;
		}
	};

	auto thread3 = [&]
	{
		for (uint32_t i = 0; i < LOOPS; i++)
		{
			gx::shareable_lock l(m, gx::lock_mode::shared); // or gx::shareable_lock_template<gx::shared_lock_t> l(m);
			volatile long long intvalue = 0;
			for (uint32_t i = 0; i < 1000; i++)
			{
				intvalue += *ptr_value * my_rand();
			}
			l.upgrade_to_exclusive();
			ptr_value = nullptr;
			rand_wait();
			ptr_value = &value;
			*ptr_value = intvalue;
		}
	};

	sprintf(output, "starting %i x 3 threads with %i loops\n", THREADS, LOOPS);
	printf("%s", output);

	auto timeMark1 = std::chrono::high_resolution_clock::now();

	std::list<std::thread> insert_threads;
	for (auto i = 0; i < THREADS; ++i) insert_threads.emplace_back(std::thread(thread1));
	for (auto i = 0; i < THREADS; ++i) insert_threads.emplace_back(std::thread(thread2));
	for (auto i = 0; i < THREADS; ++i) insert_threads.emplace_back(std::thread(thread3));

	printf("wait for threads to end\n");
	for (auto& t : insert_threads) t.join();

	auto timeMark2 = std::chrono::high_resolution_clock::now();
	std::cout << "execution took " << std::chrono::duration_cast<std::chrono::milliseconds>(timeMark2 - timeMark1).count() << " milliseconds\n\n";

	insert_threads.clear();

	return 0;
}
