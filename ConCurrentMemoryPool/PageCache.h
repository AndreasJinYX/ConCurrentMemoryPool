#pragma once

#include "Common.h"

class PageCache
{
public:
	// 申请一个新span
	Span* _NewSpan(size_t numpage);
	Span* NewSpan(size_t numpage);

	// 释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCache(Span* span);

	Span* GetIdToSpan(PAGE_ID id);

	static PageCache& GetPageCacheInstance()
	{
		static PageCache inst;
		return inst;
	}

private:
	// 单例模式
	PageCache()
	{}

	PageCache(const PageCache&) = delete;

	SpanList _spanLists[MAX_PAGES];
	std::unordered_map<PAGE_ID, Span*>  _idSpanMap;

	std::mutex _mtx;
};
