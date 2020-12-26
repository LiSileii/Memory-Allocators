# Memory-Allocators

source：https://arjunsreedharan.org/post/148675821737/write-a-simple-memory-allocator#_=_

使用C语言实现内存分配，包括malloc()，calloc()，realloc()，free()

内存布局（memory layout）：
    进程在自己的虚拟地址空间中运行，与其他进程的虚拟地址空间不同。 虚拟地址空间通常由5个部分组成：
    1、文本段（Text section）：
    2、数据段（Data section）：
    3、以符号为始的块（BSS）：
    4、堆（Heap）：
    5、栈（Stack）：
