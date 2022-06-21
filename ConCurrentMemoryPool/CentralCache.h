#pragma once

#include "Common.h"

class CentralCache
{
public:
	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size); 

	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t size);

	// ��spanlist ���� page cache��ȡһ��span
	Span* GetOneSpan(size_t size);

	static CentralCache& GetInsatnce()
	{
		static CentralCache inst;
		return inst;
	}

private:
	//��������֤������һ������
	CentralCache() 
	{}

	CentralCache(const CentralCache&) = delete; 
	
	// ���Ļ���span˫����������
	SpanList _spanLists[NFREE_LIST];
};