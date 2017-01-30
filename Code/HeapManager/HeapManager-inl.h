#include "../DebugInfo/DebugInfo.h"

inline void* HeapManager::Alloc(const size_t i_size)
{
	assert(i_size != 0);
	
	const unsigned int defaultAlign = 4;
	return Alloc(i_size, defaultAlign);
}

inline bool HeapManager::IsPowerOfTwo(const unsigned int i_value)
{
	return i_value && !(i_value & (i_value - 1));
}

inline void* HeapManager::RoundUp(void* const i_memAddr, const unsigned int i_alignment)
{
	assert(i_memAddr != nullptr);
	assert(IsPowerOfTwo(i_alignment));

	return reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(i_memAddr) + (i_alignment - 1)) & ~uintptr_t(i_alignment - 1));
}

inline void* HeapManager::RoundDown(void* const i_memAddr, const unsigned int i_alignment)
{
	assert(i_memAddr != nullptr);
	assert(IsPowerOfTwo(i_alignment));

	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(i_memAddr) & ~uintptr_t((i_alignment - 1)));
}

inline bool HeapManager::Contains(void* const i_memory) const
{
	assert(i_memory != nullptr);

	return (i_memory >= m_heapBase) && (i_memory < (reinterpret_cast<uint8_t*>(m_heapBase) + m_heapSize));
}