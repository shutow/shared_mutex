#include "shared_mutex.h"

#include <iostream>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <random>

namespace
{
	class Test {
	public:
		Test()
			: engine(std::random_device()()), uniform_dist(1, 10000)
		{
		}

		uint32_t getrand() { return uniform_dist(engine); }

		bool test()
		{
			gx::shared_lock l(m_cache_mutex);
			const auto number = getrand();
			auto it = m_cache.find(number);
			if (it == m_cache.end())
			{
				l.upgrade_to_exclusive();
				it = m_cache.find(number);
				if (it == m_cache.end())
				{
					auto ret = m_cache.emplace(std::make_pair(number, std::vector<uint32_t>()));
					for (uint32_t i = 0; i < number; ++i) ret.first->second.push_back(number);
					it = ret.first;
				}
				l.downgrade_to_shared();
			}
			bool res = true;
			if (it->second.size() != it->first)
			{
				return false;
			}
			else
			{
				for (auto n : it->second)
				{
					if (it->first != n)
					{
						res = false;
						break;
					}
				}
			}
			return res;
		}
	private:
		std::default_random_engine engine;
		std::uniform_int_distribution<uint32_t> uniform_dist;

		std::unordered_map<uint32_t, std::vector<uint32_t>> m_cache;
		gx::shared_mutex m_cache_mutex;
	};
}

int test2()
{
	printf("test 2:\n");
	printf("collision detection\n");

	Test t;
	std::vector<bool> rv(1000000, true);
	std::vector<std::thread> th;
	std::atomic<int> count;
	count = 0;
	const int numthreads = 12;
	for (int i = 0; i < numthreads; ++i)
	{
		th.emplace_back([&]()
		{
			while (true)
			{
				int c = count++;
				if (c >= (int)rv.size()) return;
				rv[c] = t.test();
			}
		});
	}
	for (int i = 0; i < numthreads; ++i) th[i].join();
	int res = 0;
	for (auto n : rv)
	{
		if (!n)
		{
			res = -1;
			break;
		}
	}
	if (res != 0)
		std::cout << "A collision between reading and writing threads had occured." << std::endl;
	else
		std::cout << "All went OK." << std::endl;
	return res;
}
