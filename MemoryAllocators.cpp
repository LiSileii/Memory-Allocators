#include <iostream>
#include <cunistd.h>
using namespace std;

/*
* ����malloc()
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
	ALIGN stub; //�����ֽڶ��룬union�Ĵ�С��������Ա�Ĵ�С�������union����Ϊ16�ֽڣ�
				//���header�Ĵ�СΪ16�ֽڡ�
};
typedef union header header_t;

header_t* head, * tail; //�ڴ������

pthread_mutex_t global_malloc_lock; //ȫ��������ֹ����߳�ͬʱ�����ڴ�
									//pthread_mutex_t��Linux�̻߳�����

void* malloc(size_t size)
{
	size_t total_size;
	void* block;
	header_t* header;
	if (!size)
		return NULL;
	pthread_mutex_lock(&global_malloc_lock); //pthread_mutex_lock()����
	header = get_free_block(size); //get_free_block()��������������ڴ��������ǰ���Ϊ�ͷŵ��ڴ棩��
								//����Ƿ�����ѱ����Ϊ���в��Ҵ��������С���ڴ��,
								//headerָ��ָ������������ҵ����ڴ��.
	if (header) //���ڿ��ڴ��Ĵ�С���������ڴ��С
	{
		header->s.is_free = 0;
		pthread_mutex_unlock(&global_malloc_lock);
		return (void*)(header + 1); //�����ϲ�Ӧ����Ҫ����header�Ĵ��ڡ�
									//�ڴ�����header��ʵ���ڴ���������֡�
									//��ִ��(header + 1)��ָ��ָ��header֮���λ�ã�
									//Ҳ���������ڴ�����ʼλ�á�
	}
	total_size = sizeof(header_t) + size;
	block = sbrk(total_size); //û�п��п�Ĵ�С�����������ڴ棬����Ҫ����sbrk()��չ��
							//����չ�Ĵ�СΪ�����С��header�Ĵ�С
	if (block == (void*)-1)
	{
		pthread_mutex_unlock(&global_malloc_lock);
		return NULL;
	}
	//�ڸղ������ϵͳ����õ����ڴ���ϣ���������д��header
	//blockָ����ڴ����header��ʵ���ڴ�������
	header = block;
	header->s.size = size;
	header->s.is_free = 0;
	header->s.next = NULL;
	//����������ڴ������ڴ������
	if (!head)
		head = header;
	if (tail)
		tail->s.next = header;
	tail = header;
	pthread_mutex_unlock(&global_malloc_lock);
	return (void*)(header + 1);
}

header_t* get_free_block(size_t size) //��Ѱ���п�����malloc
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
	* ���ж�Ҫ�ͷŵ��ڴ��Ƿ��ڶѵ�ĩβ�����ǣ�����ϵͳ�����ڴ棻�����ǣ�������Ǳ�ʾ���У�
	* ֮��ɱ�����
	*/
	header_t* header, * tmp;
	void* programbreak;
	if (!block)
		return;
	pthread_mutex_lock(&global_malloc_lock);
	header = (header_t*)block - 1; //�õ���Ӧheader�ĵ�ַ
								//�Ƚ�blockת����header_t*ָ�����ʽ���ټ�1�����ܿ��һ��header
	programbreak = sbrk(0); //sbrk(0)���ص�ǰbrk��λ�ã���ĩβ��
	//Ϊ�˼�鱻�ͷŵ��ڴ���Ƿ��ڶ�β���������ҵ����ͷŵ��ڴ���β����Ȼ��������brkλ�ñȽ�
	if ((char*)block + header->s.size == programbreak) //�ڶѵ�ĩβ
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
				tmp = tmp->s.next; //��β���ڴ��
			}
		}
		sbrk(0 - sizeof(header_t) - header->s.size);
		pthread_mutex_unlock(&global_malloc_lock);
		return;
	}
	//���ڶ�ĩβ
	header->s.is_free = 1;
	pthread_mutex_unlock(&global_malloc_lock);
}

void* calloc(size_t num, size_t nsize)
{
	/*
	* ����һ��������ڴ棬����Ԫ�ظ���Ϊnum��ÿ��Ԫ�صĴ�СΪnsize�ֽڣ�
	* ����ָ�������ڴ��ָ�롣���ң�����ڴ��ڲ���ֵ������ʼ��Ϊ0��
	*/
	size_t size;
	void* block;
	//����߽����
	if (!num || !nsize)
		return NULL;
	size = num * nsize;
	//����Ƿ�˷����
	if (nsize != size / num)
		return NULL;
	//�����ڴ�
	block = malloc(size);
	if (!block)
		return NULL;
	//�ڴ���0
	memset(block, 0, size);
	return block;
}

void* readlloc(void *block, size_t size)
{
	/*
	* �޸��ѷ����ڴ���СΪָ����С
	*/
	header_t* header;
	void* ret;
	//����߽����
	if (!block || !size)
		return malloc(size);
	header = (header_t*)block - 1; //�ڴ���header����
	//���޸ĵĴ�СС�ڵ�ǰ�ڴ���С�������޸�
	if (header->s.size >= size) 
		return block;
	//��������mallocһ�������ڴ�
	ret = malloc(size);
	if (ret)
	{
		memcpy(ret, block, header->s.size);
		free(block);
	}
	return ret;
}