#include "HeapManager.h"

#include <stdint.h>
#include <new>

#include "../DebugInfo/DebugInfo.h"

#ifdef _DEBUG
	const uint8_t HeapManager::m_gaurdBandSize = sizeof(void*);
	const uint8_t HeapManager::m_gaurdBandValue = 0xFD;
#endif

HeapManager* HeapManager::Create(void* i_pMemory, size_t i_memorySize)
{
	assert(i_pMemory != nullptr);
	assert(i_memorySize > (sizeof(HeapManager) + (sizeof(void*) * 2)));

	void* heapManagerMem = i_pMemory;
	i_pMemory = static_cast<uint8_t*>(i_pMemory) + sizeof(HeapManager);
	i_memorySize -= sizeof(HeapManager);

	HeapManager* m_heapManager = new (heapManagerMem) HeapManager(i_pMemory, i_memorySize);
	DEBUG_PRINT("Created HeapManager with size %d", i_memorySize);

	return m_heapManager;
}

void HeapManager::Destroy()
{

}

HeapManager::HeapManager(void* const i_memory, const size_t i_size) : m_heapBase(i_memory), m_heapSize(i_size), m_allocatedList(nullptr), m_freeList(nullptr)
{
	assert(i_memory != nullptr);
	assert(i_size != 0);

	DEBUG_PRINT("Called constructor of Memory Manager");

	m_freeList = reinterpret_cast<Descriptor*>(RoundUp(m_heapBase, alignof(Descriptor)));
	size_t initialOffset = reinterpret_cast<uint8_t*>(m_freeList) - static_cast<uint8_t*>(m_heapBase);

#ifdef _DEBUG
	uint8_t* gaurdbandStart = reinterpret_cast<uint8_t*>(m_freeList) + sizeof(Descriptor);
	uint8_t* gaurdbandEnd = reinterpret_cast<uint8_t*>(m_freeList) + m_heapSize - initialOffset;

	for (uint8_t i = 0; i < m_gaurdBandSize; i++)
	{
		gaurdbandStart = m_gaurdBandValue;
		gaurdbandStart++;
		gaurdbandEnd--;
		gaurdbandEnd = m_gaurdBandValue;
	}

	m_freeList->m_blockSize = m_heapSize - (2 * m_gaurdBandSize);
#endif

	m_freeList->m_blockSize = m_heapSize - sizeof(Descriptor) - initialOffset;
	m_freeList->m_next = nullptr;
	m_freeList->m_prev = nullptr;
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

#ifdef _DEBUG
		memoryBlock = static_cast<uint8_t*>(memoryBlock) + m_gaurdBandSize;
#endif
		
		if ((iterFreeList->m_blockSize >= i_size) && (iterFreeList->m_blockSize < (i_size + extraBytesAlloc)) && (alignof(memoryBlock) == i_alignment))
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

			void* alignedStartOfMemBlock = RoundDown(startOfMemBlock, i_alignment);
			size_t offsetForAlignment = startOfMemBlock - static_cast<uint8_t*>(alignedStartOfMemBlock);

#ifdef _DEBUG
			uint8_t* gaurdbandStart = static_cast<uint8_t*>(alignedStartOfMemBlock) - m_gaurdBandSize;
			uint8_t* gaurdbandEnd = static_cast<uint8_t*>(alignedStartOfMemBlock) + i_size + offsetForAlignment;

			for (uint8_t i = 0; i < m_gaurdBandSize; i++)
			{
				gaurdbandStart = m_gaurdBandValue;
				gaurdbandEnd = m_gaurdBandValue;
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

bool HeapManager::Free(void* const i_memory)
{
	assert(i_memory != nullptr);

	uint8_t* descriptorAddr = static_cast<uint8_t*>(i_memory) - sizeof(Descriptor);

#ifdef _DEBUG
	descriptorAddr = descriptorAddr - m_gaurdBandSize;
#endif

	Descriptor* iterAllocList = reinterpret_cast<Descriptor*>(descriptorAddr);
	
	if ((iterAllocList->m_next != nullptr) && (iterAllocList->m_prev != nullptr))
	{
		iterAllocList->m_prev->m_next = iterAllocList->m_next;
		iterAllocList->m_next->m_prev = iterAllocList->m_prev;

		DEBUG_PRINT("Freed the memory");

		AddToFreeList(iterAllocList);

		return true;
	}
	else if ((iterAllocList->m_next != nullptr) && (iterAllocList->m_prev == nullptr))
	{
		iterAllocList->m_next->m_prev = nullptr;

		DEBUG_PRINT("Freed the memory at the first block in the list");

		AddToFreeList(iterAllocList);

		return true;
	}
	else if ((iterAllocList->m_prev != nullptr) && (iterAllocList->m_next == nullptr))
	{
		m_allocatedList = iterAllocList->m_prev;
		m_allocatedList->m_next = nullptr;
		
		DEBUG_PRINT("Freed the memory pointed by m_allocatedList");

		AddToFreeList(iterAllocList);

		return true;
	}
	else if ((iterAllocList->m_prev == nullptr) && (iterAllocList->m_next == nullptr))
	{
		DEBUG_PRINT("Freed the memory block in the only node in the m_allocatedList");

		AddToFreeList(iterAllocList);

		return true;
	}

	DEBUG_PRINT("Wrong pointer passed for free");

	return false;
}

void HeapManager::GarbageCollect()
{
	DEBUG_PRINT("Performing Garbage collection");
	
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
}

bool HeapManager::IsAllocated(void* const i_memory) const
{
	assert(i_memory != nullptr);
	
	Descriptor* iterAllocList = m_allocatedList;
	while (iterAllocList != nullptr)
	{
		uint8_t* memoryBlock = reinterpret_cast<uint8_t*>(iterAllocList) + sizeof(Descriptor);

#ifdef _DEBUG
		memoryBlock = memoryBlock + m_gaurdBandSize;
#endif 

		if (memoryBlock == i_memory)
		{
			DEBUG_PRINT("%p has been allocated", i_memory);
			
			return true;
		}
		
		DEBUG_PRINT("%p has not been allocated", i_memory);

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