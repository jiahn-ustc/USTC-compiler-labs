# lab2 实验报告
PB19051035 周佳豪

## 问题1: cpp 与 .ll 的对应
请说明你的 cpp 代码片段和 .ll 的每个 BasicBlock 的对应关系。

* `assign_generator.cpp`与`stu_assign_generator`

  * cpp代码中的如下片段

    ```cpp
        auto bb = BasicBlock::create(module, "entry", mainFun);
        builder->set_insert_point(bb);
        auto *arrayType = ArrayType::get(Int32Type, 10);//create arrayType
        auto aAlloca = builder->create_alloca(arrayType);//create a[10]
        auto a0GEP = builder->create_gep(aAlloca, {CONST_INT(0),CONST_INT(0)}); // get the arrdess of a[0]
        builder->create_store(CONST_INT(10), a0GEP); //store 10 into a[0]
        auto a0Load = builder->create_load(a0GEP);//load a[0]
        auto a1GEP = builder->create_gep(aAlloca, {CONST_INT(0),CONST_INT(1)});
        auto mul = builder->create_imul(a0Load, CONST_INT(2));//mul=a[0]*2
        builder->create_store(mul,a1GEP);//a[1]=mul
        auto a1Load = builder->create_load(a1GEP);
        auto retAlloca = builder->create_alloca(Int32Type);// 在内存中分配返回值的位置
        builder->create_store(a1Load,retAlloca);
        auto retLoad = builder->create_load(retAlloca);
        builder->create_ret(retLoad);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_entry:
      %op0 = alloca [10 x i32]
      %op1 = getelementptr [10 x i32], [10 x i32]* %op0, i32 0, i32 0
      store i32 10, i32* %op1
      %op2 = load i32, i32* %op1
      %op3 = getelementptr [10 x i32], [10 x i32]* %op0, i32 0, i32 1
      %op4 = mul i32 %op2, 2
      store i32 %op4, i32* %op3
      %op5 = load i32, i32* %op3
      %op6 = alloca i32
      store i32 %op5, i32* %op6
      %op7 = load i32, i32* %op6
      ret i32 %op7
    ```

* `fun_generator.cpp`与`stu_fun_generator`

  * cpp代码中的如下片段

    ```cpp
        auto bb = BasicBlock::create(module, "entry", calleeFun);
        builder->set_insert_point(bb);
        auto retAlloca = builder->create_alloca(Int32Type); //ret
        auto aAlloca = builder->create_alloca(Int32Type);
    
        std::vector<Value *> args; // 获取gcd函数的形参,通过Function中的iterator
        for (auto arg = calleeFun->arg_begin(); arg != calleeFun->arg_end(); arg++)
        {
            args.push_back(*arg); // * 号运算符是从迭代器中取出迭代器当前指向的元素
        }
        builder->create_store(args[0], aAlloca);
        auto aLoad = builder->create_load(aAlloca);
        auto mul = builder->create_imul(aLoad, CONST_INT(2));
        builder->create_store(mul, retAlloca);
        auto retLoad = builder->create_load(retAlloca);
        builder->create_ret(retLoad);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_entry:
      %op1 = alloca i32
      %op2 = alloca i32
      store i32 %arg0, i32* %op2
      %op3 = load i32, i32* %op2
      %op4 = mul i32 %op3, 2
      store i32 %op4, i32* %op1
      %op5 = load i32, i32* %op1
      ret i32 %op5
    ```

  * cpp代码中的如下片段

    ```cpp
        bb = BasicBlock::create(module, "entry", mainFun);
        builder->set_insert_point(bb);
        auto call = builder->create_call(calleeFun,{CONST_INT(110)});
        builder->create_ret(call);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_entry:
      %op0 = call i32 @callee(i32 110)
      ret i32 %op0
    ```

* `if_generator.cpp` 与`stu_if_generator`

  * cpp代码中的如下片段

    ```cpp
    	auto bb = BasicBlock::create(module, "entry", mainFun);
        builder->set_insert_point(bb);
        auto retAlloca = builder->create_alloca(Int32Type);
        auto aAlloca = builder->create_alloca(FloatType);
        builder->create_store(CONST_FP(5.555), aAlloca);
        auto aLoad = builder->create_load(aAlloca);
        auto fcmp = builder->create_fcmp_gt(aLoad, CONST_FP(1));
        auto trueBB = BasicBlock::create(module, "trueBB", mainFun);   // true分支
        auto falseBB = BasicBlock::create(module, "falseBB", mainFun); // false分支
        auto retBB = BasicBlock::create(
            module, "", mainFun); // return分支,提前create,以便true分支可以br
        builder->create_cond_br(fcmp, trueBB, falseBB);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_entry:
      %op0 = alloca i32
      %op1 = alloca float
      store float 0x40163851e0000000, float* %op1
      %op2 = load float, float* %op1
      %op3 = fcmp ugt float %op2,0x3ff0000000000000
      br i1 %op3, label %label_trueBB, label %label_falseBB
    ```

  * cpp代码中的如下片段

    ```cpp
        builder->set_insert_point(trueBB);
        builder->create_store(CONST_INT(233), retAlloca);
        builder->create_br(retBB); // br retBB
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_trueBB:                                                ; preds = %label_entry
      store i32 233, i32* %op0
      br label %label4
    ```

  * cpp代码中的如下片段

    ```cpp
        builder->set_insert_point(falseBB);
        builder->create_store(CONST_INT(0), retAlloca);
        builder->create_br(retBB); // br retBB
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_falseBB:                                                ; preds = %label_entry
      store i32 0, i32* %op0
      br label %label4
    ```

  * cpp代码中的如下片段

    ```cpp
        builder->set_insert_point(retBB); // ret分支
        auto retLoad = builder->create_load(retAlloca);
        builder->create_ret(retLoad);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label4:                                                ; preds = %label_trueBB, %label_falseBB
      %op5 = load i32, i32* %op0
      ret i32 %op5
    ```

* `while_generator.cpp` 与`stu_while_generator`

  * cpp代码中的如下片段

    ```cpp
        auto bb = BasicBlock::create(module, "entry", mainFun);
        builder->set_insert_point(bb);
        auto retAlloca = builder->create_alloca(Int32Type);
        auto aAlloca = builder->create_alloca(Int32Type);
        auto iAlloca = builder->create_alloca(Int32Type);
        builder->create_store(CONST_INT(10), aAlloca);
        builder->create_store(CONST_INT(0), iAlloca);
        auto whilecond = BasicBlock::create(module, "whilecond", mainFun);
        auto whilebody = BasicBlock::create(module, "whilebody", mainFun);
        auto whileend = BasicBlock::create(module, "whileend", mainFun);
        builder->create_br(whilecond);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_entry:
      %op0 = alloca i32
      %op1 = alloca i32
      %op2 = alloca i32
      store i32 10, i32* %op1
      store i32 0, i32* %op2
      br label %label_whilecond
    ```

  * cpp代码中的如下片段

    ```c++
        builder->set_insert_point(whilecond);
        auto iLoad = builder->create_load(iAlloca);
        auto icmp = builder->create_icmp_lt(iLoad,CONST_INT(10));
        auto br = builder->create_cond_br(icmp, whilebody, whileend);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_whilecond:                                                ; preds = %label_entry, %label_whilebody
      %op3 = load i32, i32* %op2
      %op4 = icmp slt i32 %op3, 10
      br i1 %op4, label %label_whilebody, label %label_whileend
    ```

  * cpp代码中的如下片段

    ```c++
        builder->set_insert_point(whilebody);
        iLoad = builder->create_load(iAlloca);
        auto add_i = builder->create_iadd(iLoad,CONST_INT(1));
        builder->create_store(add_i, iAlloca);
        auto aLoad = builder->create_load(aAlloca);
        iLoad = builder->create_load(iAlloca);
        auto add_a = builder->create_iadd(aLoad,iLoad);
        builder->create_store(add_a, aAlloca);
        builder->create_br(whilecond);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_whilebody:                                                ; preds = %label_whilecond
      %op5 = load i32, i32* %op2
      %op6 = add i32 %op5, 1
      store i32 %op6, i32* %op2
      %op7 = load i32, i32* %op1
      %op8 = load i32, i32* %op2
      %op9 = add i32 %op7, %op8
      store i32 %op9, i32* %op1
      br label %label_whilecond
    ```

  * cpp代码中的如下片段

    ```c++
        builder->set_insert_point(whileend);
        aLoad = builder->create_load(aAlloca);
        builder->create_store(aLoad,retAlloca);
        auto retLoad = builder->create_load(retAlloca);
        builder->create_ret(retLoad);
    ```

    对应于.ll中的如下BasicBlock

    ```tex
    label_whileend:                                                ; preds = %label_whilecond
      %op10 = load i32, i32* %op1
      store i32 %op10, i32* %op0
      %op11 = load i32, i32* %op0
      ret i32 %op11
    ```

## 问题2: Visitor Pattern
分析 `calc` 程序在输入为 `4 * (8 + 4 - 1) / 2` 时的行为：
1. 请画出该表达式对应的抽象语法树（使用 `calc_ast.hpp` 中的 `CalcAST*` 类型和在该类型中存储的值来表示），并给节点使用数字编号。
2. 请指出示例代码在用访问者模式遍历该语法树时的遍历顺序。

序列请按如下格式指明（序号为问题 2.1 中的编号）：  
3->2->5->1

答：

![](figs/1.jpg)

遍历顺序为：1->2->3->4->5->6->7->23->8->9->10->11->12->13->14->25->15->16->17->26->18->19->20->24->21->22

## 问题3: getelementptr
请给出 `IR.md` 中提到的两种 getelementptr 用法的区别,并稍加解释:
  - `%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 %0`
  - `%2 = getelementptr i32, i32* %1, i32 %0`

答：

第一种用法已知数组的大小i32 x 10，第二种用法未知数组的大小(若访问的空间未分配，则会越界)。

第一种用法默认指针%1指向的是数组的首地址，若指针%1指向数组中间元素的某个地址则推荐用第二种。

考虑如下的两句代码

```c
int a[10];
int *p=&a[5];
```

则获得a[6]的地址可以由两种方法：

* `%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 6`，其中%1为a数组的首地址
* `%2 = getelementptr i32, i32* %1 i32 1`，其中%1=p

## 实验思考题

`gcd_array_generator.cpp`文件中

```c++
  auto x0GEP = builder->create_gep(x, {CONST_INT(0), CONST_INT(0)});  // GEP: 这里为什么是{0, 0}呢? (实验报告相关)
  builder->create_store(CONST_INT(90), x0GEP);
  auto y0GEP = builder->create_gep(y, {CONST_INT(0), CONST_INT(0)});  // GEP: 这里为什么是{0, 0}呢? (实验报告相关)
  builder->create_store(CONST_INT(18), y0GEP);

  x0GEP = builder->create_gep(x, {CONST_INT(0), CONST_INT(0)});
  y0GEP = builder->create_gep(y, {CONST_INT(0), CONST_INT(0)});
  call = builder->create_call(funArrayFun, {x0GEP, y0GEP});           // 为什么这里传的是{x0GEP, y0GEP}呢?
```

答：

对于`auto x0GEP = builder->create_gep(x, {CONST_INT(0), CONST_INT(0)});`第一个0代表数组的偏移量,表示是对当前数组进行操作，若为1则是指向下一个数组，在多维数组时很有用。第二个0代表数组元素的偏移量为0，即指向数组的第一个元素a[0]，若为1则指向数组的第二个元素a[1]。

`auto y0GEP = builder->create_gep(y, {CONST_INT(0), CONST_INT(0)});`原理和上述相同。

`call = builder->create_call(funArrayFun, {x0GEP, y0GEP});`此时函数调用传入的实参是一个数组，即数组的首地址，即数组第一个元素的地址。而x0GEP，y0GEP分别为x[0],y[0]的地址，所以这里传的是{x0GEP, y0GEP}。

## 实验难点
* 助教所给样例不够全面，还需要自己去搜索，这里推荐个我觉得很好的网站[IR API](https://blog.csdn.net/qq_42570601/category_10200372.html)
* 问题2有关语法树结点描述不清晰，我只能按照自己的理解做。

## 实验反馈
* 这次实验体验非常好，注释很友好，要求写的几个例子太简单，后面全是无聊的复制粘贴了...

