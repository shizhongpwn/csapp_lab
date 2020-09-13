# arch-lab

> part1其实文档里面记录过了，所以这个从part2开始

## part2

这个其实就是为处理器添加汇编指令，我们可以根据其各个阶段求出（其实可以根据里面的注释填写）：

#### 取值阶段

```
Fetch Stage
```

当然是有效指令，希望实现的新增的有效指令

```
#是否是有效指令,新增IIADDQ在末尾
bool instr_valid = icode in 
	{ INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ,
	       IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ,IIADDQ };
```

用到了rB寄存器

```
# Does fetched instruction require a regid byte? 获取的指令需要regid字节吗？
bool need_regids =
	icode in { IRRMOVQ, IOPQ, IPUSHQ, IPOPQ, 
		     IIRMOVQ, IRMMOVQ, IMRMOVQ,IIADDQ };
```

用到了常数，值取出来在valC里面

```
# Does fetched instruction require a constant word? 获取指令需要一个常量字吗？
bool need_valC =
	icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL,IIADDQ };
```

#### 译码阶段

```
Decode Stage
```

只有B源，且rB用作B源

```
##  What register should be used as the B source? 什么寄存器应该用作B源？
word srcB = [
	icode in { IOPQ, IRMMOVQ, IMRMOVQ,IIADDQ  } : rB;
	icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
	1 : RNONE;  # Don't need register
];
```

结果放在rB中

```
## What register should be used as the E destination? 什么寄存器应该用作E目的地？
word dstE = [
	icode in { IRRMOVQ } && Cnd : rB;
	icode in { IIRMOVQ, IOPQ,IIADDQ} : rB;
	icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
	1 : RNONE;  # Don't write any register
];
```

#### 执行阶段

```
Execute Stage
```

valE = valC + valB

所以输入A是valC，输入B是valB

```
## Select input A to ALU 选择输入A到ALU
word aluA = [
	icode in { IRRMOVQ, IOPQ } : valA;
	icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ,IIADDQ } : valC;
	icode in { ICALL, IPUSHQ } : -8;
	icode in { IRET, IPOPQ } : 8;
	# Other instructions don't need ALU
];
## Select input B to ALU 选择输入B到ALU
word aluB = [
	icode in { IRMMOVQ, IMRMOVQ, IOPQ, ICALL, 
		      IPUSHQ, IRET, IPOPQ,IIADDQ } : valB;
	icode in { IRRMOVQ, IIRMOVQ } : 0;
	# Other instructions don't need ALU
];
## Should the condition codes be updated? 是否应更新条件代码？
bool set_cc = icode in { IOPQ,IIADDQ };
```

