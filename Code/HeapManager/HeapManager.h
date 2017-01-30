#ifndef HEAPMANAGER_H
#define HEAPAMANGER_H

class HeapManager
{
public:
	static HeapManager* Create(void* const i_pMemory, size_t i_memorySize);
	void Destroy();

	inline void* Alloc(const size_t i_size);
	void* Alloc(const size_t i_size, const unsigned int i_alignment);
	
	bool Free(void* const i_memory);

	void GarbageCollect();

	inline bool Contains(void* const i_memory) const;
	bool IsAllocated(void* const i_memory) const;
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
	void AddToFreeList(Descriptor* const i_descriptor);
	void SortFreeList();

	inline bool IsPowerOfTwo(const unsigned int i_value);
	inline void* RoundUp(void* const i_memAddr, const unsigned int i_align);
	inline void* RoundDown(void* const i_memAddr, const unsigned int i_align);

	void* const m_heapBase;
	const size_t m_heapSize;
	Descriptor* m_allocatedList;
	Descriptor* m_freeList;
};

#include "HeapManager-inl.h"

#endif