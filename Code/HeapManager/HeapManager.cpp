#include "HeapManager.h"

#include <new>

#include "../DebugInfo/DebugInfo.h"

#ifdef _DEBUG
	const uint8_t HeapManager::m_gaurdBandSize = sizeof(void*);
	const uint8_t HeapManager::m_gaurdBandValue = 0xFD;
#endif

HeapManager* HeapManager::Create(void* i_memoryAddr, size_t i_memorySize)
{
	assert(i_memoryAddr != nullptr);
	assert(i_memorySize > (sizeof(HeapManager) + (sizeof(void*) * 2)));

	void* heapManagerMem = i_memoryAddr;
	i_memoryAddr = static_cast<uint8_t*>(i_memoryAddr) + sizeof(HeapManager);
	i_memorySize -= sizeof(HeapManager);

	HeapManager* m_heapManager = new (heapManagerMem) HeapManager(i_memoryAddr, i_memorySize);
	
	DEBUG_PRINT("Created HeapManager with size %d", i_memorySize);

	return m_heapManager;
}

void HeapManager::Destroy()
{
	void* memoryToFree = static_cast<uint8_t*>(m_heapBase) - sizeof(HeapManager);
	this->~HeapManager();
	_aligned_free(memoryToFree);
}

HeapManager::HeapManager(void* const i_memoryAddr, const size_t i_size) : m_heapSize(i_size), m_heapBase(i_memoryAddr), m_allocatedList(nullptr), m_freeList(nullptr)
{
	assert(i_memoryAddr != nullptr);
	assert(i_size > (sizeof(void*) * 2));

	DEBUG_PRINT("Called constructor of Heap Manager");

	m_freeList = reinterpret_cast<Descriptor*>(RoundUp(m_heapBase, alignof(Descriptor)));
	size_t initialOffset = reinterpret_cast<uint8_t*>(m_freeList) - static_cast<uint8_t*>(m_heapBase);

#ifdef _DEBUG
	uint8_t* gaurdbandStart = reinterpret_cast<uint8_t*>(m_freeList) + sizeof(Descriptor);
	uint8_t* gaurdbandEnd = reinterpret_cast<uint8_t*>(m_freeList) + m_heapSize - initialOffset;

	for (short i = 0; i < m_gaurdBandSize; i++)
	{
		*gaurdbandStart = m_gaurdBandValue;
		gaurdbandStart++;
		gaurdbandEnd--;
		*gaurdbandEnd = m_gaurdBandValue;
	}

	m_freeList->m_blockSize = m_heapSize - (2 * m_gaurdBandSize);
#endif

	m_freeList->m_blockSize = m_heapSize - sizeof(Descriptor) - initialOffset;
	m_freeList->m_next = nullptr;
	m_freeList->m_prev = nullptr;
}

HeapManager::~HeapManager()
{

}

void HeapManager::AddToFreeList(Descriptor* const i_descriptor)
{
	assert(i_descriptor != nullptr);

	DEBUG_PRINT("Inserted block into free list");

	if (m_freeList != nullptr)
	{
		m_freeList->m_next = i_descriptor;
		i_descriptor->m_next = nullptr;
		i_descriptor->m_prev = m_freeList;
		m_freeList = i_descriptor;
	}
	else
	{
		m_freeList = i_descriptor;
		i_descriptor->m_next = nullptr;
		i_descriptor->m_prev = nullptr;
	}
}

void HeapManager::AddToAllocatedList(Descriptor* const i_descriptor)
{
	assert(i_descriptor != nullptr);

	if (m_allocatedList != nullptr)
	{
		m_allocatedList->m_next = i_descriptor;
		i_descriptor->m_next = nullptr;
		i_descriptor->m_prev = m_allocatedList;
		m_allocatedList = i_descriptor;
	}
	else
	{
		m_allocatedList = i_descriptor;
		i_descriptor->m_next = nullptr;
		i_descriptor->m_prev = nullptr;
	}

	DEBUG_PRINT("Inserted block into allocated list");
}

void* HeapManager::Alloc(const size_t i_size, const unsigned int i_alignment)
{
	assert(i_size != 0);
	assert(IsPowerOfTwo(i_alignment));
	assert(i_size <= (m_heapSize - sizeof(Descriptor)));
	
	Descriptor* iterFreeList = m_freeList;
	
	while (iterFreeList != nullptr);
	{
		const uint8_t extraBytesAlloc = 64;
		void* memoryBlock = reinterpret_cast<uint8_t*>(iterFreeList) + sizeof(Descriptor);
		void* alignedStartOfMemBlock = nullptr;
		
#ifdef _DEBUG
		memoryBlock = static_cast<uint8_t*>(memoryBlock) + m_gaurdBandSize; 
#endif
		
		if ((iterFreeList->m_blockSize >= i_size) && (iterFreeList->m_blockSize < (i_size + extraBytesAlloc)) && ((reinterpret_cast<uintptr_t>(memoryBlock) % i_alignment) == 0))
		{
			if ((iterFreeList->m_prev != nullptr) && (iterFreeList->m_next != nullptr))
			{
				iterFreeList->m_prev->m_next = iterFreeList->m_next;
				iterFreeList->m_next->m_prev = iterFreeList->m_prev;
			}
			else if ((iterFreeList->m_prev == nullptr) && (iterFreeList->m_next != nullptr))
			{
				iterFreeList->m_next->m_prev = nullptr;
			}
			else if ((iterFreeList->m_next == nullptr) && (iterFreeList->m_prev != nullptr))
			{
				m_freeList = iterFreeList->m_prev;
				m_freeList->m_next = nullptr;
			}

			AddToAllocatedList(iterFreeList);

			DEBUG_PRINT("Found block of almost equal size in free list to allocate memory requested");

			return memoryBlock;
		}
		else if (iterFreeList->m_blockSize >= (i_size + extraBytesAlloc))
		{
			uint8_t* startOfMemBlock = reinterpret_cast<uint8_t*>(iterFreeList) + sizeof(Descriptor) + iterFreeList->m_blockSize - i_size;

#ifdef _DEBUG
			startOfMemBlock = startOfMemBlock - m_gaurdBandSize;
			m_freeList->m_blockSize = m_freeList->m_blockSize - (2 * m_gaurdBandSize);
#endif

			alignedStartOfMemBlock = RoundDown(startOfMemBlock, i_alignment);
			size_t offsetForAlignment = startOfMemBlock - static_cast<uint8_t*>(alignedStartOfMemBlock);

#ifdef _DEBUG
			uint8_t* gaurdbandStart = static_cast<uint8_t*>(alignedStartOfMemBlock) - m_gaurdBandSize;
			uint8_t* gaurdbandEnd = static_cast<uint8_t*>(alignedStartOfMemBlock) + i_size + offsetForAlignment;

			for (short i = 0; i < m_gaurdBandSize; i++)
			{
				*gaurdbandStart = m_gaurdBandValue;
				*gaurdbandEnd = m_gaurdBandValue;
				gaurdbandStart++;
				gaurdbandEnd++;
			}

			alignedStartOfMemBlock = gaurdbandStart;
#endif 

			Descriptor* descOfMemBlock = reinterpret_cast<Descriptor*>(reinterpret_cast<uint8_t*>(alignedStartOfMemBlock) - sizeof(Descriptor));
			descOfMemBlock->m_blockSize = i_size + offsetForAlignment;
			iterFreeList->m_blockSize = iterFreeList->m_blockSize - descOfMemBlock->m_blockSize - sizeof(Descriptor);

			AddToAllocatedList(descOfMemBlock);

			DEBUG_PRINT("Segmented an existing free block to allocate memory requested");

			return alignedStartOfMemBlock;
		}

		iterFreeList = iterFreeList->m_next;
	}

	DEBUG_PRINT("Could not find block to allocate memory from");

	return nullptr;
}

void HeapManager::Free(void* const i_memoryAddr)
{
	assert(i_memoryAddr != nullptr);

	uint8_t* descriptorAddr = static_cast<uint8_t*>(i_memoryAddr) - sizeof(Descriptor);

#ifdef _DEBUG
	descriptorAddr = descriptorAddr - m_gaurdBandSize;
#endif

	Descriptor* iterAllocList = reinterpret_cast<Descriptor*>(descriptorAddr);

	assert(IsValidDescriptor(iterAllocList));
	
	if ((iterAllocList->m_next != nullptr) && (iterAllocList->m_prev != nullptr))
	{
		iterAllocList->m_prev->m_next = iterAllocList->m_next;
		iterAllocList->m_next->m_prev = iterAllocList->m_prev;

		DEBUG_PRINT("Freed the memory");

		AddToFreeList(iterAllocList);
	}
	else if ((iterAllocList->m_next != nullptr) && (iterAllocList->m_prev == nullptr))
	{
		iterAllocList->m_next->m_prev = nullptr;

		DEBUG_PRINT("Freed the memory at the first block in the list");

		AddToFreeList(iterAllocList);
	}
	else if ((iterAllocList->m_prev != nullptr) && (iterAllocList->m_next == nullptr))
	{
		m_allocatedList = iterAllocList->m_prev;
		m_allocatedList->m_next = nullptr;
		
		DEBUG_PRINT("Freed the memory pointed by m_allocatedList");

		AddToFreeList(iterAllocList);
	}
	else if ((iterAllocList->m_prev == nullptr) && (iterAllocList->m_next == nullptr))
	{
		DEBUG_PRINT("Freed the memory block in the only node in the m_allocatedList");

		AddToFreeList(iterAllocList);
	}
}

HeapManager::Descriptor* HeapManager::SortedMerge(Descriptor* const i_descriptor1, Descriptor* const i_descriptor2)
{
	Descriptor* result = nullptr;

	if (i_descriptor1 == nullptr)
		return i_descriptor2;
	else if (i_descriptor2 == nullptr)
		return i_descriptor1;

	if (i_descriptor1 >= i_descriptor2)
	{
		result = i_descriptor1;
		result->m_prev = SortedMerge(i_descriptor1->m_prev, i_descriptor2);
	}
	else
	{
		result = i_descriptor2;
		result->m_prev = SortedMerge(i_descriptor1, i_descriptor2->m_prev);
	}

	return result;
}

void HeapManager::SplitList(const Descriptor* const i_source, Descriptor** const io_front, Descriptor** const io_back)
{
	if ((i_source == nullptr) || (i_source->m_prev == nullptr))
	{
		*io_front = const_cast<Descriptor*>(i_source);
		*io_back = nullptr;
	}
	else
	{
		const Descriptor* fast = i_source->m_prev;
		Descriptor* slow = const_cast<Descriptor*>(i_source);

		while (fast != nullptr)
		{
			fast = fast->m_prev;
			if (fast != nullptr)
			{
				fast = fast->m_prev;
				slow = slow->m_prev;
			}
		}

		*io_front = const_cast<Descriptor*>(i_source);
		*io_back = slow->m_prev;
		slow->m_prev = nullptr;
	}
}

void HeapManager::MergeSort(Descriptor** const i_descriptor)
{
	assert(i_descriptor != nullptr);
	assert(*i_descriptor != nullptr);
	
	const Descriptor* const head = *i_descriptor;

	if ((head == nullptr) || (head->m_prev == nullptr))
	{
		return;
	}

	Descriptor* front = nullptr;
	Descriptor* back = nullptr;

	SplitList(head, &front, &back);

	MergeSort(&front);
	MergeSort(&back);

	*i_descriptor = SortedMerge(front, back);
}

void HeapManager::GarbageCollect()
{
	DEBUG_PRINT("Started Garbage collection");
	
	MergeSort(&m_freeList);

	DEBUG_PRINT("Sorted Free List");

	Descriptor* iterFreeList = m_freeList;
	while (iterFreeList->m_prev != nullptr)
	{
		while ((reinterpret_cast<uint8_t*>(iterFreeList) + sizeof(Descriptor) + iterFreeList->m_blockSize) == reinterpret_cast<uint8_t*>(iterFreeList->m_next))
		{
			iterFreeList->m_blockSize = iterFreeList->m_blockSize + sizeof(Descriptor) + iterFreeList->m_next->m_blockSize;
			iterFreeList->m_next = iterFreeList->m_next->m_next;
			iterFreeList->m_next->m_prev = iterFreeList;
		}
		
		iterFreeList = iterFreeList->m_prev;
	}

	DEBUG_PRINT("Completed Garbage Collection");
}

bool HeapManager::IsAllocated(void* const i_memoryAddr) const
{
	assert(i_memoryAddr != nullptr);
	
	Descriptor* iterAllocList = m_allocatedList;

	while (iterAllocList != nullptr)
	{
		uint8_t* memoryBlock = reinterpret_cast<uint8_t*>(iterAllocList) + sizeof(Descriptor);

#ifdef _DEBUG
		memoryBlock = memoryBlock + m_gaurdBandSize;
#endif 

		if (memoryBlock == i_memoryAddr)
		{
			DEBUG_PRINT("%p has been allocated", i_memoryAddr);
			
			return true;
		}
		
		DEBUG_PRINT("%p has not been allocated", i_memoryAddr);

		iterAllocList = iterAllocList->m_prev;
	}

	return false;
}

size_t HeapManager::GetLargestFreeBlock() const
{
	Descriptor* iterFreeList = m_freeList;
	size_t largeBlockSize = 0;
	while (iterFreeList != nullptr)
	{
		if (iterFreeList->m_blockSize > largeBlockSize)
		{
			largeBlockSize = iterFreeList->m_blockSize;
		}

		iterFreeList = iterFreeList->m_prev;
	}

	DEBUG_PRINT("Largest free block has size %d", largeBlockSize);

	return largeBlockSize;
}

size_t HeapManager::GetTotalFreeMemory() const
{
	Descriptor* iterFreeList = m_freeList;
	size_t totalFreeBlockSize = 0;
	while (iterFreeList != nullptr)
	{
		totalFreeBlockSize += iterFreeList->m_blockSize;

		iterFreeList = iterFreeList->m_prev;
	}

	DEBUG_PRINT("Total free memory has size %d", totalFreeBlockSize);

	return totalFreeBlockSize;
}

bool HeapManager::IsValidDescriptor(const Descriptor* const i_descriptor)
{
	assert(i_descriptor != nullptr);
	
	Descriptor* iterFreeList = m_freeList;
	while (iterFreeList != nullptr)
	{
		if (iterFreeList == i_descriptor)
		{
			DEBUG_PRINT("%p is a valid Descriptor", i_descriptor);
			
			return true;
		}

		iterFreeList = iterFreeList->m_prev;
	}

	DEBUG_PRINT("%p is not a valid Descriptor", i_descriptor);

	return false;
}