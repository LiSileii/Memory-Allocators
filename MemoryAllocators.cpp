#include <iostream>
#include <cunistd.h>
using namespace std;

/*
* 初版malloc()
void* malloc(size_t size)
{
	void* blovk;
	block = sbrk(size);
	if (block == (void*)-1)
		return NULL;
	return blobk;
}
*/

typedef char ALIGN[16];
union header
{
	struct
	{
		size_t size;
		unsigned is_free;
		union header* next;
	} s;
	ALIGN stub; //用于字节对齐，union的大小等于最大成员的大小，这里的union对齐为16字节，
				//因此header的大小为16字节。
};
typedef union header header_t;

header_t* head, * tail; //内存块链表

pthread_mutex_t global_malloc_lock; //全局锁。防止多个线程同时访问内存
									//pthread_mutex_t是Linux线程互斥量

void* malloc(size_t size)
{
	size_t total_size;
	void* block;
	header_t* header;
	if (!size)
		return NULL;
	pthread_mutex_lock(&global_malloc_lock); //pthread_mutex_lock()上锁
	header = get_free_block(size); //get_free_block()函数会遍历空闲内存块链表（以前标记为释放的内存），
								//检查是否存在已被标记为空闲并且大于申请大小的内存块,
								//header指针指向遍历链表所找到的内存块.
	if (header) //存在空内存块的大小大于所需内存大小
	{
		header->s.is_free = 0;
		pthread_mutex_unlock(&global_malloc_lock);
		return (void*)(header + 1); //对于上层应用需要隐藏header的存在。
									//内存块包括header和实际内存块两个部分。
									//当执行(header + 1)，指针指向header之后的位置，
									//也即是真正内存块的起始位置。
	}
	total_size = sizeof(header_t) + size;
	block = sbrk(total_size); //没有空闲块的大小能容纳所需内存，所以要调用sbrk()扩展堆
							//堆扩展的大小为申请大小加header的大小
	if (block == (void*)-1)
	{
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}
	//在刚才向操作系统申请得到的内存块上，我们首先写入header
	//block指向的内存包括header和实际内存两部分
	header = block;
	header->s.size = size;
	header->s.is_free = 0;
	header->s.next = NULL;
	//将新申请的内存块加入内存块链表
	if (!head)
		head = header;
	if (tail)
		tail->s.next = header;
	tail = header;
	pthread_mutex_unlock(&global_malloc_lock);
	return (void*)(header + 1);
}

header_t* get_free_block(size_t size) //搜寻空闲块用于malloc
{
	header_t* curr = head;
	while (curr)
	{
		if (curr->s.is_free && curr->s.size >= size)
			return curr;
		curr = curr->s.next;
	}
	return NULL;
}

void free(void* block)
{
	/*
	* 先判断要释放的内存是否在堆的末尾，若是，则让系统回收内存；若不是，则做标记表示空闲，
	* 之后可被重用
	*/
	header_t* header, * tmp;
	void* programbreak;
	if (!block)
		return;
	pthread_mutex_lock(&global_malloc_lock);
	header = (header_t*)block - 1; //得到对应header的地址
								//先将block转换成header_t*指针的形式，再减1，就能跨过一个header
	programbreak = sbrk(0); //sbrk(0)返回当前brk的位置（堆末尾）
	//为了检查被释放的内存块是否在堆尾部，首先找到被释放的内存块的尾部，然后用它和brk位置比较
	if ((char*)block + header->s.size == programbreak) //在堆的末尾
	{
		if (head == tail)
			head = tail = NULL;
		else
		{
			tmp = head;
			while (tmp)
			{
				if (tmp->s.next == tail)
				{
					tmp->s.next == NULL;
					tail = tmp;
				}
				tmp = tmp->s.next; //堆尾部内存块
			}
		}
		sbrk(0 - sizeof(header_t) - header->s.size);
		pthread_mutex_unlock(&global_malloc_lock);
		return;
	}
	//不在堆末尾
	header->s.is_free = 1;
	pthread_mutex_unlock(&global_malloc_lock);
}

void* calloc(size_t num, size_t nsize)
{
	/*
	* 申请一个数组的内存，数组元素个数为num，每个元素的大小为nsize字节，
	* 返回指向被申请内存的指针。并且，这块内存内部的值都被初始化为0。
	*/
	size_t size;
	void* block;
	//处理边界情况
	if (!num || !nsize)
		return NULL;
	size = num * nsize;
	//检查是否乘法溢出
	if (nsize != size / num)
		return NULL;
	//申请内存
	block = malloc(size);
	if (!block)
		return NULL;
	//内存置0
	memset(block, 0, size);
	return block;
}

void* readlloc(void *block, size_t size)
{
	/*
	* 修改已分配内存块大小为指定大小
	*/
	header_t* header;
	void* ret;
	//处理边界情况
	if (!block || !size)
		return malloc(size);
	header = (header_t*)block - 1; //内存块的header部分
	//若修改的大小小于当前内存块大小，则不用修改
	if (header->s.size >= size) 
		return block;
	//否则重新malloc一块更大的内存
	ret = malloc(size);
	if (ret)
	{
		memcpy(ret, block, header->s.size);
		free(block);
	}
	return ret;
}