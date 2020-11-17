# CSAPP-第八章-异常

> ECF - 异常控制流。

处理器检测到有事件发生的时候，会有一张`异常表`的`跳转表`，进行间接过程调用，到`异常处理程序(exception handler)`.

## 异常

系统中可能的每一种类型的异常都分配了一个唯一的非负整数的`异常号`。

在操作系统启动的时候，操作系统分配和初始化一个叫`异常表`的`跳转表`。

![image-20201113104045072](CSAPP-第八章-异常.assets/image-20201113104045072.png)

运行的时候`处理器`检测到发生事件，确定`异常号`，处理器就会触发异常。

异常触发步骤：

1. 处理器检测到异常
2. 确定`异常号`
3. 通过`异常号k`在异常表（`异常表基址寄存器`）查找到相关表项
4. 转到相应的处理程序

![image-20201113185057940](CSAPP-第八章-异常.assets/image-20201113185057940.png)

异常处理完成之后，根据引起异常的事件的类型，会发生3种情况中的一种：

* 处理程序返回，事件发生时正在执行的指令。
* 处理程序返回，返回到没有发生异常将会执行的`下一条指令`。
* 处理程序被中断的程序（处理程序也可能被中断奥）。

`异常处理注意`

* 处理器在异常处理的时候也可能把一些额外的处理器状态压倒栈里面，处理程序返回的时候会恢复这些状态。
* 如果控制器从用户程序转移到内核，所有项目都被压倒内核栈里面，而不是压倒用户栈里面。
* 异常处理运行在内核态的时候拥有一切访问权限。

`异常的类别`

![image-20201113185740352](CSAPP-第八章-异常.assets/image-20201113185740352.png)

`中断`

* 中断时异步发生的,其它的都是同步发生的，中断如下：![image-20201114085613980](CSAPP-第八章-异常.assets/image-20201114085613980.png)
* 其他的异常统称为`故障指令`。

 `陷阱和系统调用`

> 陷阱最重要的用途是在用户程序和内核之间提供一个像过程一样的接口，叫做系统调用。

处理器提供`syscall`指令，它会导致一个到异常处理程序的陷阱，这个处理程序解析参数，调用合适的内核程序。

![image-20201114090306936](CSAPP-第八章-异常.assets/image-20201114090306936.png)

`故障`

它由错误引起，可能被故障处理程序修正。

*  修正成功：返回引起故障的指令再次执行。
* 修正失败：返回到内核`abort`例程，终止程序。

![image-20201114090727673](CSAPP-第八章-异常.assets/image-20201114090727673.png)

`终止`

不可恢复的致命性错误，这个就没啥好说的，直接`abort`终止程序。

`Linux/x86-64系统的异常`

其有256种不同的异常类型，0-31来自于`Intel架构师`，对任何`x86/64`一样，32~255操作系统定义。

经典异常号

![image-20201114091346176](CSAPP-第八章-异常.assets/image-20201114091346176.png)

`Linux/86-64系统调用`

对于系统调用，`系统调用号`对应了很多的`内核跳转表的偏移量（区别于异常表）`

## 进程

`基本概念`

* 多任务(multitasking)：一个进程和其它进程轮流执行。

`私有地址空间`

> 一般而言，和这个空间里面某个地址相关联的那个内存字节是不能被其他进程读或者写的，这个意义上说，这个地址空间是私有的。

![image-20201114100009488](CSAPP-第八章-异常.assets/image-20201114100009488.png)

`用户模式和内核模式`

处理器提供了一种机制，限制一个应用可以执行的命令以及它可以访问的地址空间范围。

* 寄存器模式位`mode bit`，其描述了当前享有的特权。
  * 设置模式位进入内核模式
* 用户态到内核态
* 通过`异常`，异常发生控制异常处理程序，将用户模式变为内核模式。

**Linux提供了一个重要的数据结构，/proc文件系统，进而允许用户模式访问内核数据结构的内容。/proc文件系统把很多内核数据结构输出为一个用户程序可读的文本文件层次结构。**

`上下文切换`

内核通过`上下文切换(context switch)`异常控制流实现多任务。

`上下文`：内核重新启动一个被抢占的进程所需的状态。

* 通用寄存器，用户栈之类的。。

![image-20201114125525170](CSAPP-第八章-异常.assets/image-20201114125525170.png)

## 进程控制

> 记录自己记得不清楚的点。

`init`进程：其在系统启动的时候创建，是所有进程的祖先，如果一个父进程没有回收它的僵死子进程就终止了，那么内核会安排`init进程`去回收他们。

`waitpid函数`

~~~c
#include<sys/types.h>
#include<sys/wait.h>
pid_t waitpid(pid_t pid,int *statusp,int options);
~~~

默认情况`options=0`，waitpid挂起调用进程的执行，直到它的`等待集合`中的一个子进程终止。如果等待集合里面的一个进程在调用的时候就已经终止了，那么`waitpid`就立刻返回，`waitpid`返回导致waitpid返回的已终止的子进程的`PID`,此时子进程已经被回收，内核会冲系统中删除掉它的所有痕迹。

1. 判定等待集合的成员（由参数pid来判定）
   1. 如果pid>0，等待集合就是一个单独的子进程，进程id=pid
   2. 如果pid=-1,那么等待集合就是由父进程所有的子进程组成的。
2. 修改默认行为
   1. `options`选项：
      1. ![image-20201116200115240](CSAPP-第八章-异常.assets/image-20201116200115240.png)
   2. `statusp`参数：
      1. ![image-20201116201210382](CSAPP-第八章-异常.assets/image-20201116201210382.png)
3. 如果没有调用子进程，`waitpid`返回-1，并且设置`errno`为`ECHILD`，如果`waitpid函数`被一个信号中断，它返回`-1`，设置`errno`为`EINTR`.

测试案例：

~~~c
#include<stdio.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<errno.h>
int main()
{
    int statu,i;
    pid_t pid;
    for(i=0;i<10;i++)
    {
        if((pid=fork())==0)
        {
            exit(100+i);
        }
    }
    while(((pid = waitpid(-1,&statu,NULL))>0))
    {
        if(WIFEXITED(statu))
        {
            printf("child process pid = %d over normally with statu  = %d\n",pid,WEXITSTATUS(statu));
        }
        else
        {
            printf("child process pid = %d over over over\n",pid);
        }
    }
    if(errno != ECHILD)
    {
        printf("waitpid error\n");
        exit(-1);
    }
    return 0;
}
~~~

![image-20201117075955246](CSAPP-第八章-异常.assets/image-20201117075955246.png)

看出程序不会按照特定顺序回收子进程。

测试2：

~~~c
#include<stdio.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<errno.h>
int main()
{
    int statu,i;
    pid_t pid[10];
    for(i=0;i<10;i++)
    {
        if((pid[i]=fork())==0)
        {
            exit(100+i);
        }
    }
    i=0;
    pid_t repid;
    while(((repid = waitpid(pid[i],&statu,NULL))>0))
    {
        if(WIFEXITED(statu))
        {
            printf("child process pid = %d over normally with statu  = %d\n",repid,WEXITSTATUS(statu));
        }
        else
        {
            printf("child process pid = %d over over over\n",repid);
        }
        i++;
    }
    if(errno != ECHILD)
    {
        printf("waitpid error\n");
        exit(-1);
    }
    return 0;
}
~~~

![image-20201117080641604](CSAPP-第八章-异常.assets/image-20201117080641604.png)

这样子进程可以按照特定顺序被回收。

`加载并运行程序`

~~~c
#include<unistd.h>
int execve(const char *filename,const char *argv[],const char *envp[]);
~~~

![image-20201117081509230](CSAPP-第八章-异常.assets/image-20201117081509230.png)

环境变量字符串都是`键值对`的形式。

![image-20201117081846673](CSAPP-第八章-异常.assets/image-20201117081846673.png)

这个点倒是没有注意到哎**在栈的顶部是系统启动函数libc_start_main**的栈帧。

`操作环境数组`

~~~c
#include<stdlib.h>
char *getenv(const char *name);
~~~

该函数搜索键值对里面的`name`返回一个执行`value`的指针，否则返回`NULL`.

~~~c
#include<stdlib.h>
int setenv(const char* name,const char* newvalue, int overwrite); //成功返回1，失败返回0
void unsetenv(const char  *name);
~~~

理解`envp`和`argv`

~~~c
#include<stdio.h>
#include<stdlib.h>
int main(int argc,char *argv[],char *envp[])
{
    printf("command line argv is :\n");
    for(int i=0;argv[i]!=NULL;i++)
    {
        printf("          argv[%d] =  %s\n",i,argv[i]);
    }

    printf("envp is :\n");
    for(int i=0;envp[i]!=NULL;i++)
    {
        printf("          envp[%d] =  %s\n",i,envp[i]);
    }
    return 0;
}
~~~

## 信号

![image-20201117094243473](CSAPP-第八章-异常.assets/image-20201117094243473.png)

底层的硬件异常都是由内核处理程序运行的，正常情况下，对用户而言是不可见的，信号提供了一种机制可以通知用户发生了这种异常。

值得注意的几个信号机制：

* `ctrl+C`内核会发送`SIGINT`给这个前台进程组里面的每个进程。
* 一个进程可以通过向另一个进程发送`SIGKILL`信号终止它。
* 当一个子进程终止的时候，内核会发送一个`SIGCHLD`信号给父进程。





















