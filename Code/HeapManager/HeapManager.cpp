#include "HeapManager.h"

#include <stdint.h>
#include <new>

#include "../DebugInfo/DebugInfo.h"

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
	m_freeList->m_blockSize = m_heapSize - sizeof(Descriptor);
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

	if (i_size >= ((m_heapSize) - sizeof(Descriptor)))
	{
		return nullptr;

		DEBUG_PRINT("Requested more memory then size of heap");
	}
	
	Descriptor* iterFreeList = m_freeList;
	while (iterFreeList != nullptr);
	{
		const uint8_t extraBytesAlloc = 64;

		if ((iterFreeList->m_blockSize >= i_size) && (iterFreeList->m_blockSize < (i_size + extraBytesAlloc)) && (alignof(reinterpret_cast<uint8_t*>(iterFreeList) + sizeof(Descriptor)) == i_alignment))
		{
			if ((iterFreeList->m_prev != nullptr) && (iterFreeList->m_next != nullptr))
			{
				iterFreeList->m_prev->m_next = iterFreeList->m_next;
				iterFreeList->m_next->m_prev = iterFreeList->m_prev;
			}
			else if ((iterFreeList->m_prev == nullptr) && (iterFreeList->m_next != nullptr))
			{
				iterFreeList->m_next->m_prev = iterFreeList->m_prev;
			}
			else if (iterFreeList->m_next == nullptr)
			{
				m_freeList = iterFreeList->m_prev;
				m_freeList->m_next = nullptr;
			}

			AddToAllocatedList(iterFreeList);

			DEBUG_PRINT("Found block of almost equal size in free list to allocate memory requested");

			return reinterpret_cast<uint8_t*>(m_allocatedList) + sizeof(Descriptor);
		}
		else if (iterFreeList->m_blockSize >= (i_size + extraBytesAlloc))
		{
			void* startOfMemBlock = reinterpret_cast<uint8_t*>(iterFreeList) + sizeof(Descriptor) + iterFreeList->m_blockSize - i_size;
			void* alignedStartOfMemBlock = RoundDown(startOfMemBlock, i_alignment);
			Descriptor* descOfMemBlock = reinterpret_cast<Descriptor*>(reinterpret_cast<uint8_t*>(alignedStartOfMemBlock) - sizeof(Descriptor));
			descOfMemBlock->m_blockSize = reinterpret_cast<uint8_t*>(iterFreeList) + sizeof(Descriptor) + iterFreeList->m_blockSize - static_cast<uint8_t*>(alignedStartOfMemBlock);
			iterFreeList->m_blockSize = reinterpret_cast<uint8_t*>(descOfMemBlock) - (reinterpret_cast<uint8_t*>(iterFreeList) + sizeof(Descriptor));

			AddToAllocatedList(descOfMemBlock);

			DEBUG_PRINT("Segmented an existing free block to allocate memory requested");

			return reinterpret_cast<uint8_t*>(m_allocatedList) + sizeof(Descriptor);
		}

		iterFreeList = iterFreeList->m_next;
	}

	DEBUG_PRINT("Could not find block to allocate memory from");

	return nullptr;
}

bool HeapManager::Free(void* const i_memory)
{
	assert(i_memory != nullptr);

	Descriptor* iterAllocList = reinterpret_cast<Descriptor*>(static_cast<uint8_t*>(i_memory) - sizeof(Descriptor));
	
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
		iterAllocList->m_next->m_prev = iterAllocList->m_prev;

		DEBUG_PRINT("Freed the memory at the first block in the list");

		AddToFreeList(iterAllocList);

		return true;
	}
	else if (iterAllocList->m_next == nullptr)
	{
		m_allocatedList = iterAllocList->m_prev;
		m_allocatedList->m_next = nullptr;

		DEBUG_PRINT("Freed the memory pointed by m_allocatedList");

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
		if ((reinterpret_cast<uint8_t*>(iterAllocList) + sizeof(Descriptor)) == i_memory)
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