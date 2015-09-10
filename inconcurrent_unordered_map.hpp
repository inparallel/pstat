#ifndef INCONCURRENT_UNORDERED_MAP_HPP
#define	INCONCURRENT_UNORDERED_MAP_HPP

#include <unordered_map>
#include <mutex>

namespace tbb
{
	/**
	 * \brief An bad implementation of a concurrent (thread-safe) unordered map that wraps std::unordered_map with locks. Useful as
	 * a (rather naive) alternative to tbb::concurrent_unordered_map. This is what you get when you don't use TBB.
	 */
	template <typename Key, typename T>
	class concurrent_unordered_map
	{
		std::unordered_map<Key, T> m_Map;
		std::mutex m_Mutex;
		
	public:
		
		T& operator[](const Key& key)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			
			m_Map.insert(std::pair<Key, T>(key, T()));
			
			return m_Map[key];
		}
		
		size_t count(const Key& key)
		{
			return m_Map.count(key);
		}
		
		
	};
}


#endif	/* INCONCURRENT_UNORDERED_MAP_HPP */

