#ifndef HEAPMANAGER_H
#define HEAPAMANGER_H

struct Descriptor
{
	size_t m_blockSize;
	Descriptor* m_next;
	void* m_blockAddress;
};

class HeapManager
{
public:
	HeapManager(void* i_memory, const size_t i_size);

	~HeapManager();

	void* Alloc(const size_t i_size);
	void* Alloc(const size_t i_size, const unsigned int i_alignment);
	
	bool Free(void* i_memory);

	void GarbageCollect();

	bool Contains(const void* const i_memory) const;
	bool IsAllocated(const void* const i_memory) const;
	size_t GetLargestFreeBlock() const;
	size_t GetTotalFreeMemory() const;

private:
	size_t m_heapSize;
	Descriptor* m_freeList;
	Descriptor* m_allocatedList;
	void* m_heapBase;
};

#endif