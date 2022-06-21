#pragma once

#include "Common.h"

class CentralCache
{
public:
	// 从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size); 

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t size);

	// 从spanlist 或者 page cache获取一个span
	Span* GetOneSpan(size_t size);

	static CentralCache& GetInsatnce()
	{
		static CentralCache inst;
		return inst;
	}

private:
	//单例：保证创建出一个对象
	CentralCache() 
	{}

	CentralCache(const CentralCache&) = delete; 
	
	// 中心缓存span双向自由链表
	SpanList _spanLists[NFREE_LIST];
};
