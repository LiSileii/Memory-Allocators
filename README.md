# Memory-Allocators

source：https://arjunsreedharan.org/post/148675821737/write-a-simple-memory-allocator#_=_

使用C语言实现内存分配，包括malloc()，calloc()，realloc()，free()

内存布局（memory layout）：  
进程在自己的虚拟地址空间中运行，与其他进程的虚拟地址空间不同。  
虚拟地址空间通常由5个部分组成：  
1、文本段（Text section）：包含由处理器执行的二进制指令。  
2、数据段（Data section）：包含非零初始化静态数据。  
3、以符号为始的块（BSS-Block Started by Symbol）：包含零初始化静态数据。 程序中未初始化的静态数据被初始化为0，然后转到这里。  
4、堆（Heap）：包含动态分配的数据。  
5、栈（Stack）：包含自动变量、函数参数、基本指针副本等。  
![内存布局](http)
其中堆和栈的地址增长方向相反。堆区的数据可能是从低地址位到高地址位分配的，而栈区的数据可能按照从高地址位到低地址位的方向分配。

数据段、BSS和堆常被统称为数据段，其末尾由名为程序中断或brk的指针标定，即brk指向堆的末尾。  
若想给堆分配更多的内存，则需要向系统请求增大brk；释放内存需要减小brk。  
Linux使用sbrk()系统调用进行内存分配。  
sbrk(0)：将brk置为当前地址  
sbrk(x)：brk增加x字节  
sbrk(-x)：brk减少x字节  
sbrk()失败时返回(void*)-1  
尽管sbrk()不再是最佳选择，mmap()是更好的替代，但是malloc依然使用sbrk()来分配大小不大的内存。  
使用sbrk()需要包含unistd.h头文件，该头文件只在Linux中有。  

每块内存块看起来像这样：  
![内存块](http)  
内存块链表看起来像这样：  
![内存块链表](http)  

编译并使用我们的内存分配器：  
	1，编译为库文件  
		$ gcc -o memalloc.so -fPIC -shared memalloc.c  
		gcc是一种编译器。  
		-o选项用来指定输出文件。  
		-fPIC和-shared选项用于确保编译输出的文件的是代码位置独立的并且是可被动态链接的动态库。  
	2，优先加载我们的库文件  
		$ export LD_PRELOAD=$PWD/memalloc.so  
		$PWD给出当前工作目录的绝对路径名。  
在Linux，如果你将一个动态库文件的路径设置在环境变量LD_PRELOAD中，这个库文件会优先被加载。  
