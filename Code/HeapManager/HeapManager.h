#ifndef HEAPMANAGER_H
#define HEAPAMANGER_H

class HeapManager
{
public:
	static HeapManager* Create(void* i_pMemory, size_t i_memorySize);
	void Destroy();

	inline void* Alloc(const size_t i_size);
	void* Alloc(const size_t i_size, const unsigned int i_alignment);
	
	bool Free(const void* const i_memory);

	void GarbageCollect();

	bool Contains(const void* const i_memory) const;
	bool IsAllocated(const void* const i_memory) const;
	size_t GetLargestFreeBlock() const;
	size_t GetTotalFreeMemory() const;

private:
	HeapManager(void* i_memory, const size_t i_size);
	HeapManager(const HeapManager& i_heapManager);
	HeapManager operator=(const HeapManager& i_heapManager);

	struct Descriptor
	{
		size_t m_blockSize;
		Descriptor* m_next;
		Descriptor* m_prev;
	};
	
	void AddToAllocatedList(Descriptor* const i_descriptor);
	bool AddToFreeList(Descriptor* const i_descriptor);

	inline bool IsPowerOfTwo(const unsigned int i_value);
	inline void* RoundUp(const void* const i_memAddr, const unsigned int i_align);
	inline void* RoundDown(const void* const i_memAddr, const unsigned int i_align);

	void* m_heapBase;
	size_t m_heapSize;
	Descriptor* m_freeList;
	Descriptor* m_allocatedList;
};

#include "HeapManager-inl.h"

#endif