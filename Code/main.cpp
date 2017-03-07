#include "HeapManager/HeapManager.h"

#include <malloc.h>

void main()
{
	void* memoryBlock = _aligned_malloc(1024, 64);

	HeapManager* heapManager = HeapManager::Create(memoryBlock, 1024);

	heapManager->Destroy();
}