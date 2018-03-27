#pragma once

#include <random>
#include <thread>

inline int my_rand()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dis(1, 1000000000);
	return dis(gen);
}

inline void rand_wait()
{
	//return; // no wait

	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> dis(0, 1);

	std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
}
