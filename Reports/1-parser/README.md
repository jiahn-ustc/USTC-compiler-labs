# lab1 实验报告

**PB19051035周佳豪**

## 实验要求

* 完成一个完整的`Cminus-f`解析器，包括基于`flex`的词法分析器和基于 `bison` 的语法分析器。
* 词法分析器需要补全模式和动作，能够输出识别出的`token`,`text`,`line`,`pos_start`,`pos_end`。
* 语法分析器可以从`Cminus-f`代码分析得到一颗语法树。

## 实验难点

1. 实验文档的阅读，需要大量的stfw
2. 注释正则表达式的编写
3. 词法分析器中`_syntax_tree_node`结构体和`node()`函数的理解

## 实验设计

1. 根据实验文档在`syntax_analyzer.y`中编写`token`和`type`

2. 在`lexical_analyzer.l`文件中编写正则表达式和其对应的代码块
3. 根据`program`的例子在`syntax_analyzer.y`中编写规则
4. 进行结果验证

## 实验结果验证

测试代码如下：

``` c
/* 121332 */
int main(void){
    int a;
    int c;
    a=1;
    c=2;
    return 0;
}
```

词法分析：

```tex
Token	      Text	Line	Column (Start,End)
265  	       int	1	(1,4)
285  	      main	1	(5,9)
268  	         (	1	(9,10)
267  	      void	1	(10,14)
269  	         )	1	(14,15)
272  	         {	1	(15,16)
265  	       int	2	(5,8)
285  	         a	2	(9,10)
264  	         ;	2	(10,11)
265  	       int	3	(5,8)
285  	         c	3	(9,10)
264  	         ;	3	(10,11)
285  	         a	4	(5,6)
280  	         =	4	(6,7)
286  	         1	4	(7,8)
264  	         ;	4	(8,9)
285  	         c	5	(5,6)
280  	         =	5	(6,7)
286  	         2	5	(7,8)
264  	         ;	5	(8,9)
277  	    return	6	(5,11)
286  	         0	6	(12,13)
264  	         ;	6	(13,14)
273  	         }	7	(1,2)
```

语法分析:

```tex
>--+ program
|  >--+ declaration-list
|  |  >--+ declaration
|  |  |  >--+ fun-declaration
|  |  |  |  >--+ type-specifier
|  |  |  |  |  >--* int
|  |  |  |  >--* main
|  |  |  |  >--* (
|  |  |  |  >--+ params
|  |  |  |  |  >--* void
|  |  |  |  >--* )
|  |  |  |  >--+ compound-stmt
|  |  |  |  |  >--* {
|  |  |  |  |  >--+ local-declarations
|  |  |  |  |  |  >--+ local-declarations
|  |  |  |  |  |  |  >--+ local-declarations
|  |  |  |  |  |  |  |  >--* epsilon
|  |  |  |  |  |  |  >--+ var-declaration
|  |  |  |  |  |  |  |  >--+ type-specifier
|  |  |  |  |  |  |  |  |  >--* int
|  |  |  |  |  |  |  |  >--* a
|  |  |  |  |  |  |  |  >--* ;
|  |  |  |  |  |  >--+ var-declaration
|  |  |  |  |  |  |  >--+ type-specifier
|  |  |  |  |  |  |  |  >--* int
|  |  |  |  |  |  |  >--* c
|  |  |  |  |  |  |  >--* ;
|  |  |  |  |  >--+ statement-list
|  |  |  |  |  |  >--+ statement-list
|  |  |  |  |  |  |  >--+ statement-list
|  |  |  |  |  |  |  |  >--+ statement-list
|  |  |  |  |  |  |  |  |  >--* epsilon
|  |  |  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  |  |  >--+ expression-stmt
|  |  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  |  >--* a
|  |  |  |  |  |  |  |  |  |  |  >--* =
|  |  |  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ integer
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* 1
|  |  |  |  |  |  |  |  |  |  >--* ;
|  |  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  |  >--+ expression-stmt
|  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  >--+ var
|  |  |  |  |  |  |  |  |  |  |  >--* c
|  |  |  |  |  |  |  |  |  |  >--* =
|  |  |  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--+ integer
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* 2
|  |  |  |  |  |  |  |  |  >--* ;
|  |  |  |  |  |  >--+ statement
|  |  |  |  |  |  |  >--+ return-stmt
|  |  |  |  |  |  |  |  >--* return
|  |  |  |  |  |  |  |  >--+ expression
|  |  |  |  |  |  |  |  |  >--+ simple-expression
|  |  |  |  |  |  |  |  |  |  >--+ additive-expression
|  |  |  |  |  |  |  |  |  |  |  >--+ term
|  |  |  |  |  |  |  |  |  |  |  |  >--+ factor
|  |  |  |  |  |  |  |  |  |  |  |  |  >--+ integer
|  |  |  |  |  |  |  |  |  |  |  |  |  |  >--* 0
|  |  |  |  |  |  |  |  >--* ;
|  |  |  |  |  >--* }

```



## 实验反馈

1. 实验所给Cminus-f语法与所学C语言语法有很大区别，且未强调，造成了很多不必要的bug，浪费了很多精力。
2. 实验文档不够通俗易懂，觉得不如[C-Minus词法分析](https://blog.csdn.net/Gyc8787/article/details/103824742)和[flex与bison](https://blog.csdn.net/CrazyHeroZK/article/details/87359818)讲的透彻。

