# CSAPP-archlab

#### 要求

Write a Y86-64 program `sum.ys` that iteratively sums the elements of a linked list.  Your program should consist  of  some  code  that  sets  up  the  stack  structure,  invokes  a  function,  and  then  halts. 

In  this  case, the function should be Y86-64 code for a function (`sum_list`) that is functionally equivalent to the C `sum_list` function in Figure 1. Test your program using the following three-element list:

> part1 汇编编写,这部分挺简单的，都是一些普通的语法

结构体如下

~~~c
typedef struct ELE {
    long val;
    struct ELE *next;
} *list_ptr;
~~~

### 源码

~~~c
/* sum_list - Sum the elements of a linked list */
long sum_list(list_ptr ls)
{
    long val = 0;
    while (ls) {
	val += ls->val;
	ls = ls->next;
    }
    return val;
}
~~~

## part 2

#### 要求

在 `sim/seq` 文件夹里，修改 `seq-full.hcl` 文件，添加 `iaddq` 指令

第一步分析 iaddq指令的作用：

|   state   |         do         |
| :-------: | :----------------: |
|   fetch   | icode:ifun<-M1[PC] |
|           |  rA,rB<-M1[PC+1]   |
|           |   valC<-M1[PC+2]   |
|           |    ValP<-PC+10     |
|  decode   |    valB<-R[rB]     |
|  execute  |  ValE<-ValB+ValC   |
|  memory   |                    |
| writeback |    R[rB]<-ValE     |
|           |      PC<-valP      |

