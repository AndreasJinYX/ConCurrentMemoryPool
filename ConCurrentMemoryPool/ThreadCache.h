#pragma once

#include "Common.h"

class ThreadCache
{
public:

	// ������thread cache�в���tls��thread local storage�����ֲ߳̾��洢������
	// ����֤һ���߳̽�ӵ��һ��ȫ�ֵ�thread cache���󣬴Ӷ�����ʹ�߳�����thread cache�����ڴ�����ʱ��ʵ����������
	// 
	// �����ڴ���ͷ��ڴ�
	void* Allocte(size_t size);
	void Deallocte(void* ptr, size_t size);

	// �����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index);

	// ������������ж��󳬹�һ�����Ⱦ�Ҫ�ͷŸ����Ļ���
	void ListTooLong(FreeList& freeList, size_t num, size_t size);
private:
	FreeList _freeLists[NFREE_LIST]; 

};


// �ֲ߳̾��洢��tls������һ�ֱ����Ĵ洢��������������������ڵ��߳�����ȫ�ֿɷ��ʵģ�
// ���ǲ��ܱ������̷߳��ʵ��������ͱ��������ݵ��̶߳����ԡ�
// ����֪��ȫ�ֱ������������̶߳����Է��ʵģ������Ͳ��ɱ�����Ҫ�������ƣ������˿��Ƴɱ��ʹ��븴�Ӷȡ�

_declspec (thread) static ThreadCache* pThreadCache = nullptr;