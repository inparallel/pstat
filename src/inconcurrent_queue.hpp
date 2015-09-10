#ifndef INCONCURRENT_QUEUE_HPP
#define	INCONCURRENT_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

namespace tbb
{
	/**
	 * \brief An bad implementation of a concurrent (thread-safe) queue that wraps std::queue with locks. Useful as
	 * a (rather naive) alternative to tbb::concurrent_queue. This is what you get when you don't use TBB.
	 * 
	 * Reference: https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
	 */
	template<typename T>
	class concurrent_queue
	{
		std::queue<T> m_Queue;
		std::mutex m_Mutex;
		std::condition_variable m_cv;
	public:

		/**
		 * \brief Pushes the specified element to the back of the queue. Thread safe.
		 */
		void push(T const& data)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Queue.push(data);
			lock.unlock();
			m_cv.notify_one();
		}

		/**
		 * \brief Emplaces the specified element to the back of the queue. Thread safe.
		 */
		template<typename... Arguments>
		void emplace(Arguments&&... args)
		{
			this->push(T(std::forward<Arguments>(args)...));
		}

		/**
		 * \brief Tries to pop an element off the queue. Thread safe.
		 * \param element On success, the popped element is stored here
		 * \return True if something is popped, false otherwise
		 */
		bool try_pop(T& popped_value)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			if (m_Queue.empty())
			{
				return false;
			}

			popped_value = m_Queue.front();
			m_Queue.pop();
			return true;
		}

	};
}


#endif	/* INCONCURRENT_QUEUE_HPP */

