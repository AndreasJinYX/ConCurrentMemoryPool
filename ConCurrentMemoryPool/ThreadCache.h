#pragma once

#include "Common.h"

class ThreadCache
{
public:

	// 不过在thread cache中采用tls（thread local storage）即线程局部存储技术，
	// 来保证一个线程仅拥有一个全局的thread cache对象，从而可以使线程在向thread cache申请内存对象的时候实现无锁访问
	// 
	// 申请内存和释放内存
	void* Allocte(size_t size);
	void Deallocte(void* ptr, size_t size);

	// 从中心缓存获取对象
	void* FetchFromCentralCache(size_t index);

	// 如果自由链表中对象超过一定长度就要释放给中心缓存
	void ListTooLong(FreeList& freeList, size_t num, size_t size);
private:
	FreeList _freeLists[NFREE_LIST]; 

};


// 线程局部存储（tls），是一种变量的存储方法，这个变量在它所在的线程内是全局可访问的，
// 但是不能被其他线程访问到，这样就保持了数据的线程独立性。
// 而熟知的全局变量，是所有线程都可以访问的，这样就不可避免需要锁来控制，增加了控制成本和代码复杂度。

_declspec (thread) static ThreadCache* pThreadCache = nullptr;