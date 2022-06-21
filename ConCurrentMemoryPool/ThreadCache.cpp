#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::Allocte(size_t size) //申请内存
{
	size_t index = SizeClass::ListIndex(size);
	FreeList& freeList = _freeLists[index];
	if (!freeList.Empty()) 
	{
		return freeList.Pop(); 
	}
	else 
	{
		return FetchFromCentralCache(SizeClass::RoundUp(size)); // ThreadCache中没有内存需要向CentralCache中申请内存
	}
}

void ThreadCache::Deallocte(void* ptr, size_t size) //释放内存
{
	size_t index = SizeClass::ListIndex(size); 
	FreeList& freeList = _freeLists[index]; 
	freeList.Push(ptr);  

	size_t num = SizeClass::NumMoveSize(size);
	if (freeList.Num() >= num) //达到一定的批量还给 CentralCache
	{
		ListTooLong(freeList, num, size); 
	}
}

void ThreadCache::ListTooLong(FreeList& freeList, size_t num, size_t size) //还给central cache中的span
{
	void* start = nullptr, *end = nullptr;
	freeList.PopRange(start, end, num);

	NextObj(end) = nullptr;
	CentralCache::GetInsatnce().ReleaseListToSpans(start, size); 
}

// 测试thread cache 
void* ThreadCache::FetchFromCentralCache(size_t size) //需要多少从中心缓存中获取
{
	size_t num = SizeClass::NumMoveSize(size); 

	//CentralCache中获取内存
	void* start = nullptr, *end = nullptr;
	size_t actualNum = CentralCache::GetInsatnce().FetchRangeObj(start, end, num, size);

	if (actualNum == 1)
	{
		return start;
	}
	else
	{
		size_t index = SizeClass::ListIndex(size);
		FreeList& list = _freeLists[index];
		list.PushRange(NextObj(start), end, actualNum - 1);

		return start;
	}
}