#ifndef HEAPMANAGER_H
#define HEAPAMANGER_H

#include <stdint.h>

class HeapManager
{
public:
	static HeapManager* Create(void* i_memoryAddr, size_t i_memorySize);
	void Destroy();

	inline void* Alloc(const size_t i_size);
	void* Alloc(const size_t i_size, const unsigned int i_alignment);
	
	void Free(void* const i_memoryAddr);

	void GarbageCollect();

	inline bool Contains(void* const i_memoryAddr) const;
	bool IsAllocated(void* const i_memoryAddr) const;
	size_t GetLargestFreeBlock() const;
	size_t GetTotalFreeMemory() const;

private:
	HeapManager(void* const i_memoryAddr, const size_t i_size);
	HeapManager(const HeapManager& i_heapManager);
	HeapManager operator=(const HeapManager& i_heapManager);

	~HeapManager();

	struct Descriptor
	{
		size_t m_blockSize;
		Descriptor* m_next;
		Descriptor* m_prev;
	};
	
	void AddToAllocatedList(Descriptor* const i_descriptor);
	void AddToFreeList(Descriptor* const i_descriptor);
	bool IsValidDescriptor(const Descriptor* const i_descriptor);
	void MergeSort(Descriptor** const i_descriptor);
	Descriptor* SortedMerge(Descriptor* const i_descriptor1, Descriptor* const i_descriptor2);
	void SplitList(const Descriptor* const i_source, Descriptor** const io_front, Descriptor** const io_back);

	inline bool IsPowerOfTwo(const unsigned int i_value);
	inline void* RoundUp(void* const i_memoryAddr, const unsigned int i_align);
	inline void* RoundDown(void* const i_memoryAddr, const unsigned int i_align);
	
	const size_t m_heapSize;
	void* const m_heapBase;
	Descriptor* m_allocatedList;
	Descriptor* m_freeList;

#if _DEBUG
	static const uint8_t m_gaurdBandSize;
	static const uint8_t m_gaurdBandValue;
#endif
};

#include "HeapManager-inl.h"

#endif