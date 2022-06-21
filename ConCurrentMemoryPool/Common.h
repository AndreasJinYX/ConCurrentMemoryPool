#pragma once

#include <iostream>
#include <assert.h>
#include <unordered_map>
#include <thread>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

using std::endl;
using std::cout;

const size_t MAX_SIZE = 64 * 1024; 
const size_t NFREE_LIST = MAX_SIZE / 8; //FREE_LIST，以8字节间隔
const size_t MAX_PAGES = 129; //超过128需要向系统申请
const size_t PAGE_SHIFT = 12; // 4k


/* 
* 为了取出一块对象内存中的前4个字节（32位系统）或者8个字节（64位系统）的内存来存储一个指针指向下一块释放回来的自由对象内存，
* 将对象内存强转成void**的类型，那么再对这个二级指针类型解引用就可以取出当前对象的前4个字节（32位系统）或8个字节（64位系统），
* 由于这个操作之后会频繁使用，因此定义为内联函数放在common.h头文件中方便调用。
*/
inline void*& NextObj(void* obj)
{
	return *((void**)obj);   // 解引用获取当前对象的指针地址
}


class FreeList //自由链表类
{

private:
	void* _freelist = nullptr; //将释放的内存链接回收并管理
	size_t _maxNum = 1;//慢增长用于保住申请批次下限
	size_t _num = 0; 

public:
	void Push(void* obj) // 头插
	{
		NextObj(obj) = _freelist; 
		_freelist = obj; 
		++_num;
	}

	void* Pop() // 头删
	{	
		void* obj = _freelist; 
		_freelist = NextObj(obj); 
		--_num;
		return obj;
	}
	

	void PushRange(void* head, void* tail, size_t num) //将一个范围的串插入
	{
		NextObj(tail) = _freelist; 
		_freelist = head; 
		_num += num;
	}

	size_t PopRange(void*& start, void*& end, size_t num) //取一段spanlist链表长度
	{
		size_t actualNum = 0;
		void* prev = nullptr;
		void* cur = _freelist;

		for (; actualNum < num && cur != nullptr; ++actualNum)
		{
			prev = cur;
			cur = NextObj(cur);
		} 

		start = _freelist;
		end = prev;  

		_freelist = cur; // 把头部的指针移动到pre的下一个

		_num -= actualNum;

		return actualNum;
	}

	size_t Num()
	{
		return _num;
	}


	size_t MaxNum()
	{
		return _maxNum;
	}


	bool Empty() 
	{
		return _freelist == nullptr; // 自由链表已空
	}

	void Clear() 
	{
		_freelist = nullptr;
		_num = 0;
	}


};

class SizeClass //计算项目各种大小的类
{
public:

	// [9, 16] + 7 = [16, 23] -> 16 8 4 2 1 
	static size_t _RoundUp(size_t size, size_t alignment) //对齐
	{
		return (size + alignment - 1) & (~(alignment - 1));
	}

	// 控制在[1%，10%]左右的内碎片浪费
	// 整体控制在最多10%左右的内碎片浪费
	// [1,128] 8byte对齐       freelist[0,16)
	// [128+1,1024] 16byte对齐   freelist[16,72)
	// [1024+1,8*1024] 128byte对齐   freelist[72,128)
	// [8*1024+1,64*1024] 1024byte对齐     freelist[128,184)
	// [64*1024+1,256*1024] 8*1024byte对齐   freelist[184,208)

	static inline size_t RoundUp(size_t size)
	{
		assert(size <= MAX_SIZE);

		if (size <= 128) {
			return _RoundUp(size, 8);
		}
		else if (size <= 1024) {
			return _RoundUp(size, 16); // 128 * 8
		}
		else if (size <= 8192) {
			return _RoundUp(size, 128);
		}
		else if (size <= 65536) {
			return _RoundUp(size, 1024);
		}

		return -1;
	}

	// [9，16] + 7 = [16， 23]
	static size_t _ListIndex(size_t size, size_t align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1; // 计算对应哈希桶链桶的下标
	}

	static size_t ListIndex(size_t size)
	{
		assert(size <= MAX_SIZE);

		// 每个区间有多少个链
		static int group_array[4] = { 16, 56, 56, 56 }; 
		if (size <= 128)
		{
			return _ListIndex(size, 3); // 2^3 8字节对齐
		}
		else if (size <= 1024)
		{
			return _ListIndex(size - 128, 4) + group_array[0]; //2^4 16字节对齐
		}
		else if (size <= 8192)
		{
			return _ListIndex(size - 1024, 7) + group_array[1] + group_array[0]; //2^7 128字节对齐
		}
		else if (size <= 65536)
		{
			return _ListIndex(size - 8192, 10) + group_array[2] + group_array[1] + group_array[0]; //2^10 1024字节对齐
		}

		return -1;
	}

	// [2,512]个之间
	// 计算向CentralCache一次获取批次的数量
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0) 
			return 0;

		//计算合理的内存对象数量
		//根据批次计算获取的数量会在[1,32768]，范围过大，
		//因此控制获取的对象数量范围在区间[2, 512],较为合理
		//小对象获取的批次多，大对象获取的批次少

		int num = MAX_SIZE / size; 
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;

		return num;
	}

	// 计算一次向PageCache申请多少页
	// 单个对象的大小从8byte - 256kB
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num * size;

		npage >>= PAGE_SHIFT; // 页面大小是4KB
		if (npage == 0)
			npage = 1;

		return npage;
	}
};

#ifdef _WIN32
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;
#endif 


// 管理多个大块内存的跨度结构
struct Span //跨度类
{
	PAGE_ID _pageid = 0; // 页号
	PAGE_ID _pagesize = 0; // 页的数量

	FreeList _freeList;  // 对象自由链表
	size_t _objSize = 0; // 自由链表对象大小
	int _usecount = 0;   // 当前Span已经被分配给Thread Cache的内存数量

	Span* _next = nullptr; //链接span前后
	Span* _prev = nullptr;
};

class SpanList //双向链表：双向、带头、循环
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin() 
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	void PushFront(Span* newspan) //头插
	{
		Insert(_head->_next, newspan);
	}

	void PopFront() //头删
	{
		Erase(_head->_next);
	}

	void PushBack(Span* newspan) //尾插
	{
		Insert(_head, newspan);
	}

	void PopBack() //尾删
	{
		Erase(_head->_prev);
	}

	void Insert(Span* pos, Span* newspan) //插入
	{
		Span* prev = pos->_prev;

		prev->_next = newspan;
		newspan->_next = pos;
		pos->_prev = newspan;
		newspan->_prev = prev;
	}

	void Erase(Span* pos) //删除
	{
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	bool Empty()
	{
		return Begin() == End();
	}

	void Lock()
	{
		_mtx.lock();
	}

	void Unlock()
	{
		_mtx.unlock();
	}

private:
	Span* _head; 
	std::mutex _mtx;
};

inline static void* SystemAlloc(size_t num_page) //向系统申请内存
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, num_page*(1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// brk mmap等
#endif
	if (ptr == nullptr) 
		throw std::bad_alloc();

	return ptr;
}

inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
#endif
}