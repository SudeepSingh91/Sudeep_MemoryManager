#include "HeapManager/HeapManager.h"

#if _DEBUG
#include <crtdbg.h>
#endif

#include <malloc.h>
#include <stdlib.h>

#include "DebugInfo/DebugInfo.h"
void main()
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(79);
#endif
	
	const size_t size = 400;
	const size_t cacheLineWidth = 64;
	void* memoryBlock = _aligned_malloc(size, cacheLineWidth);

	HeapManager* heapManager = HeapManager::Create(memoryBlock, size);

	heapManager->Destroy();
}