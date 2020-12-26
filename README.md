# Memory-Allocators

source：https://arjunsreedharan.org/post/148675821737/write-a-simple-memory-allocator#_=_

使用C语言实现内存分配，包括malloc()，calloc()，realloc()，free()

内存布局（memory layout）：  
    进程在自己的虚拟地址空间中运行，与其他进程的虚拟地址空间不同。 虚拟地址空间通常由5个部分组成：  
    1、文本段（Text section）：包含由处理器执行的二进制指令。  
    2、数据段（Data section）：包含非零初始化静态数据。  
    3、以符号为始的块（BSS-Block Started by Symbol）：包含零初始化静态数据。 程序中未初始化的静态数据被初始化为0，然后转到这里。  
    4、堆（Heap）：包含动态分配的数据。  
    5、栈（Stack）：包含自动变量、函数参数、基本指针副本等。  
