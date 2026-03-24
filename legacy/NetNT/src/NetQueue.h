#ifndef NETLIB_NET_QUEUE_H
#define NETLIB_NET_QUEUE_H

#include "NetCommon.h"

namespace Codex::Net
{
	template<typename T>
	class NetQueue
	{
	protected:
		std::mutex m_Mutex;
		std::deque<T> m_Queue;
		std::condition_variable m_CvBlocking;
		std::mutex m_MutexLocker;

	public:
		NetQueue() = default;
		NetQueue(const NetQueue<T>&) = delete;

	public:
		inline T& Front()
		{
			std::scoped_lock lock(m_Mutex);
			return m_Queue.front();
		}
		inline T& Back()
		{
			std::scoped_lock lock(m_Mutex);
			return m_Queue.back();
		}
		inline void Push(const T& item)
		{
			std::scoped_lock lock(m_Mutex);
			m_Queue.emplace_back(std::move(item));
		}
		inline void PushFront(const T& item)
		{
			std::scoped_lock lock(m_Mutex);
			m_Queue.emplace_front(std::move(item));
		}
		inline T Pop()
		{
			std::scoped_lock lock(m_Mutex);
			auto t = std::move(m_Queue.back());
			m_Queue.pop_back();
			std::unique_lock<std::mutex> mutex_lock(m_MutexLocker);
			m_CvBlocking.notify_one();
			return t;
		}
		inline T PopFront()
		{
			std::scoped_lock lock(m_Mutex);
			auto t = std::move(m_Queue.front());
			m_Queue.pop_front();
			std::unique_lock<std::mutex> mutex_lock(m_MutexLocker);
			m_CvBlocking.notify_one();
			return t;
		}
		inline bool IsEmpty()
		{
			std::scoped_lock lock(m_Mutex);
			return m_Queue.empty();
		}
		inline size_t Size()
		{
			std::scoped_lock lock(m_Mutex);
			return m_Queue.size();
		}
		inline void Clear()
		{
			std::scoped_lock lock(m_Mutex);
			return m_Queue.clear();
		}
		inline void Wait()
		{
			while (this->IsEmpty())
			{
				std::unique_lock<std::mutex> lock(m_MutexLocker);
				m_CvBlocking.wait(lock);
			}
		}
	};
}

#endif // NETLIB_NET_QUEUE_H