#pragma once

#include <atomic>
#include <stdint.h>
#include <thread>

namespace gx
{
	enum lock_mode { shared, exclusive };

	class shared_mutex
	{
	public:
		enum strategy { yield, wait, burn };
		shared_mutex(strategy strategy = yield, uint64_t delay_microseconds = 100)
			: m_strategy(strategy)
			, m_delay_microseconds(delay_microseconds)
			, m_priority_exclusive(false)
			, m_exclusive(false)
			, m_counter(0)
		{
		}
		~shared_mutex()
		{
		}
		void lock_shared()
		{
			while (true)
			{
				m_counter.fetch_add(1);
				if (m_exclusive.load())
					m_counter.fetch_sub(1);
				else
					break;
				while (m_exclusive.load()) delay();
			}
		}
		void unlock_shared()
		{
			m_counter.fetch_sub(1, std::memory_order_release);
		}
		void lock()
		{
			bool expected = false;
			while (!m_exclusive.compare_exchange_strong(expected, true))
			{
				delay();
				expected = false;
			}
			while (m_counter.load() > 0) delay();
		}
		void unlock()
		{
			if (m_priority_exclusive.load() > 0)
			{
				m_priority_exclusive.fetch_sub(1, std::memory_order_release);
				m_counter.fetch_sub(1, std::memory_order_release);
			}
			else
				m_exclusive.store(false, std::memory_order_release);
		}
		void unlock_shared_and_lock()
		{
			bool expected = false;
			if (m_exclusive.compare_exchange_strong(expected, true))
			{
				m_counter.fetch_sub(1, std::memory_order_release);
				while (m_counter.load() > 0) delay();
			}
			else // if some thread is claimed m_exclusive already and waits for m_counter == 0
			{
				int64_t num = m_priority_exclusive.fetch_add(1) + 1;
				while (m_counter.load() > num) delay();
			}
		}
		void unlock_and_lock_shared()
		{
			if (m_priority_exclusive.load() > 0)
			{
				m_priority_exclusive.fetch_sub(1, std::memory_order_release);
			}
			else
			{
				m_counter.fetch_add(1);
				m_exclusive.store(false, std::memory_order_release);
			}
		}
		lock_mode mode() const
		{
			if (m_exclusive)
				return lock_mode::exclusive;
			if (m_counter)
				return lock_mode::shared;
			return lock_mode(0xFFFF);
		}
	private:
		void delay()
		{
			if (m_strategy == yield)
				std::this_thread::yield();
			else if (m_strategy == wait)
				std::this_thread::sleep_for(std::chrono::duration<uint64_t, std::micro>(m_delay_microseconds));
		}
	private:
		strategy m_strategy;
		uint64_t m_delay_microseconds;
		std::atomic<int64_t> m_priority_exclusive;
		std::atomic<bool> m_exclusive;
		std::atomic<int64_t> m_counter;
	};

	struct shared_lock_t
	{
	};
	struct exclusive_lock_t
	{
	};

	struct defer_lock_t
	{
	};
	struct adopt_lock_t
	{
	};

	template <typename T = at::shared_lock_t>
	class shareable_lock_template
	{
	private:
		shareable_lock_template(const shareable_lock_template&);
	public:
		shareable_lock_template(shareable_lock_template&& l)
		{
			m_mutex = l.m_mutex; l.m_mutex = nullptr;
			m_mode = l.m_mode;
			is_locked = l.is_locked; l.is_locked = false;
		}
		shareable_lock_template() // invalid lock
			: m_mutex(nullptr), m_mode(lock_mode::exclusive), is_locked(false) {}

		shareable_lock_template(shared_mutex& mutex);
		shareable_lock_template(shared_mutex& mutex, defer_lock_t);

		shareable_lock_template(shared_mutex& mutex, lock_mode mode)
			: m_mutex(&mutex), m_mode(mode), is_locked(false)
		{
			if (m_mode == lock_mode::shared)
				m_mutex->lock_shared();
			else
				m_mutex->lock();
			is_locked = true;
		}
		shareable_lock_template(shared_mutex& mutex, adopt_lock_t)
			: m_mutex(&mutex), is_locked(true), m_mode(mutex.mode())
		{
		}
		void upgrade_to_exclusive()
		{
			if (is_locked && m_mode == lock_mode::shared)
			{
				m_mode = lock_mode::exclusive;
				m_mutex->unlock_shared_and_lock();
			}
		}
		void downgrade_to_shared()
		{
			if (is_locked && m_mode == lock_mode::exclusive)
			{
				m_mutex->unlock_and_lock_shared();
				m_mode = lock_mode::shared;
			}
		}
		void lock()
		{
			if (m_mutex && !is_locked)
			{
				if (m_mode == lock_mode::shared)
					m_mutex->lock_shared();
				else
					m_mutex->lock();

				is_locked = true;
			}
		}
		void release()
		{
			if (m_mutex)
			{
				m_mutex = nullptr;
				is_locked = false;
			}
		}
		bool owns_lock() const
		{
			return m_mutex && is_locked;
		}
		shared_mutex* mutex()
		{
			return m_mutex;
		}
		const shared_mutex* mutex() const
		{
			return m_mutex;
		}
		~shareable_lock_template()
		{
			if (m_mutex && is_locked)
			{
				if (m_mode == lock_mode::shared)
					m_mutex->unlock_shared();
				else
					m_mutex->unlock();
			}
		}
	private:
		shared_mutex* m_mutex;
		lock_mode m_mode;
		bool is_locked;
	};

	template <>
	inline shareable_lock_template<exclusive_lock_t>::shareable_lock_template(shared_mutex & mutex)
		: m_mutex(&mutex), m_mode(lock_mode::exclusive), is_locked(true)
	{
		m_mutex->lock();
	}

	template <>
	inline shareable_lock_template<exclusive_lock_t>::shareable_lock_template(shared_mutex & mutex, defer_lock_t)
		: m_mutex(&mutex), m_mode(lock_mode::exclusive), is_locked(false)
	{
	}

	template <>
	inline shareable_lock_template<shared_lock_t>::shareable_lock_template(shared_mutex & mutex)
		: m_mutex(&mutex), m_mode(lock_mode::shared), is_locked(true)
	{
		m_mutex->lock_shared();
	}

	template <>
	inline shareable_lock_template<shared_lock_t>::shareable_lock_template(shared_mutex & mutex, defer_lock_t)
		: m_mutex(&mutex), m_mode(lock_mode::shared), is_locked(false)
	{
	}

	using shareable_lock = shareable_lock_template<shared_lock_t>;
	using shared_lock = shareable_lock_template<shared_lock_t>;
	using exclusive_lock = shareable_lock_template<exclusive_lock_t>;
}
