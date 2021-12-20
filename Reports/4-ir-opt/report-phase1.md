# Lab4 实验报告-阶段一

小组成员 姓名 学号

* 组长：朱文冬 PB19111714
* 组员：周佳豪 PB19051035

## 实验要求

请按照自己的理解，写明本次实验需要干什么

深入阅读`Mem2Reg`与`LoopSearch`两个优化 pass 的代码。

理解 SSA （静态单赋值）格式，实现三个 pass ：常量传播与死代码删除，循环不变式外提，活跃变量分析。

## 思考题
### LoopSearch

1. * 在`LoopSearch.cpp`中，函数内部的基本块全为`CFG`结点的一个元素`BasicBlock *bb`,

     通过计算图的强连通分量可得到循环中基本块的集合，储存在`std::unordered_set<CFGNode *> &result`中。

   * 最后把强连通分量中储存的块再储存进`loop_set`中,故直接用于描述循环的数据结构为`loop_set`,其类型为`std::unordered_set<BBset_t *>`,其中`BBset_t`类型为`std::unordered_set<BasicBlock *>`。

2. * `LoopSearch.cpp`中的`CFGNodePtr LoopSearch::find_loop_base(CFGNodePtrSet *set,CFGNodePtrSet &reserved)`函数能获取一个循环的入口。

   * 其中`set`为一个强连通分量的所有节点的集合，`reserved`为已经找到的循环的入口块集合。

   * ```cpp
     CFGNodePtr base = nullptr;
         for (auto n : *set)
         {
             for (auto prev : n->prevs)
             {
                 if (set->find(prev) == set->end())
                 {
                     base = n;
                 }
             }
         }
         if (base != nullptr)
             return base;
     ```

     这部分代码通过遍历循环中的所有节点的所有前驱，若某个结点的前驱为循环外部结点，则该节点即为循环的入口。

   * ```c++
     for (auto res : reserved)
         {
             for (auto succ : res->succs)
             {
                 if (set->find(succ) != set->end())
                 {
                     base = succ;
                 }
             }
         }
     
         return base;
     ```

     * 若循环中的所有节点的所有前驱均为循环内部结点，首先说明这种情况存在的可能性，由于通常的`Tarjan`算法求的是图中的最大强连通量，故一个包含嵌套循环的循环只会被认为是一个强连通量，即一个循环。
     * 此时只能使用改良的`Tarjan`算法，即找到一个循环的入口后，用`reserved`储存该入口结点，然后删除该结点以及指向该节点的指针，最后再次使用`Tarjan`算法，即可得到内层循环，重复上述操作即可得到层次更深的循环。
     * 所以若一个循环是内层循环，此循环入口有可能没有指向外部结点的指针，此时只能借助`reserved`的力量来判断循环入口，即遍历reserved中所有元素的后继结点，若后继结点在循环内部，此后继结点即为循环入口。

3. 思路在问题2已说明。

   具体代码如下,即删除该结点以及指向该节点的指针：

   ```c++
   nodes.erase(base);
   for (auto su : base->succs)
   {
   su->prevs.erase(base);
   }
   for (auto prev : base->prevs)
   {
   prev->succs.erase(base);
   }
   ```

   

4. * ```c++
     //base2_loop的维护
     for (auto n : *scc)
     {
     bb_set->insert(n->bb);
     node_set_string = node_set_string + n->bb->get_name() + ',';
     }
     loop_set.insert(bb_set);
     func2loop[func].insert(bb_set);
     base2loop.insert({base->bb, bb_set});
     loop2base.insert({bb_set, base->bb});
     ```

   * 其中`scc`为图的强连通分量，`base->bb`为循环的入口块，通过遍历scc中的结点可得到储存循环基本块的集合`bb_set`,最后将结果储存在`func2loop`、`base2loop`和`loop2base`中。

   * ```c++
     //bb2_base的维护
     for (auto bb : *bb_set)
                             {
                                 if (bb2base.find(bb) == bb2base.end())
                                     bb2base.insert({bb, base->bb});
                                 else
                                     bb2base[bb] = base->bb;
                             }
     ```

   * 其中bb_set为刚刚得到的循环中基本块的集合，然后更新bb2base（新创建一个或直接覆盖原来的数据）。由于代码处理多层循环是自外由内，故bb2base最后储存的是循环中的块与该块所在最内侧循环入口的映射关系。

   * 故通过`bb2base`可找到基本块所属的最内层的循环的入口，然后再通过`base2loop`可找到最内层循环。

### Mem2reg
1. 简述概念

   **支配性**

   若从初始结点起，每条到达 m 的路径都要经过 n ，则 n 支配 m 。记作 n ∈ Dom(m)。其中一个结点也是它本身的支配结点，即 m ∈ Dom(m)。

   **严格支配性**

   当且仅当 n ∈ Dom(m) - {m} 时，n 严格支配 m。

   即 m 的支配集 Dom(m) 中不含 m 本身的结点，为其严格支配结点。

   **直接支配性**

   n 是 m 的严格支配集 Dom(m) - {m} 中与 m 最接近的结点，则 n 为 m 的直接支配结点。记作 IDom(m)。

   流图的入口结点没有直接支配结点。

   **支配边界**

   严格定义如下：

   (1) n 支配 m 的一个前驱。即：q ∈ preds(m) 且 n ∈ Dom(q)

   (2) n 并不严格支配 m。即：n ∉ Dom(m) - {m}

   我们将相对于 n 具有这种性质的结点 m 的集合称为 n 的支配边界，记作 DF(n)。

   也就是说，n 的支配终止于其支配边界。在离开 n 的每条 CFG 路径上，从结点 n 可达但不能支配的第一个结点。

   

2. `phi`节点

   phi 节点代表一个 phi 函数。

   在具有多个前趋的每个程序块起始处，为当前过程中定义或使用的每个名字，插入一个 phi 函数，对于 CFG 中的每一个前驱块，phi 函数有一个参数与之对应。phi 函数的定义要求位于程序块顶部的所有 phi 函数并发执行。在运行时，phi 函数根据在当前结点之前执行的是哪一个前驱结点来得到相应的值。

   之所以引入 phi 节点，是因为需要通过 phi 函数来确定前驱结点，进而获得相应的值，从而将 alloca 申请变量再进行 store 和 load 值传递过程转换为直接利用中间变量的静态单赋值形式，就例如题 4 中将

   ```c++
   %op1 = alloca i32
   store i32 %arg0, i32* %op1
   %op2 = load i32, i32* %op1
   ```

   类语句精简掉。和通过 phi 直接

   ```c++
   %op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]
   ```

   对前驱的判断（entry or 6）来直接取得对应的值。

   即：实现了代码的优化。

   

3. 使用了不同的临时变量名。（？）

   a 的空间 alloca 为 %op0，在第一次赋值时，0 赋值给 %op0。

   而在后续的赋值时，第二次、第三次赋值分别先赋值给了 %op1，%op3，再 store 到 %op0 中。

   

4. 部分发生了变化

   在函数 func 中

   ```c++
   %op1 = alloca i32
   store i32 %arg0, i32* %op1
   %op2 = load i32, i32* %op1
   ```

   删除了 alloca 代码，不再申请空间来存放函数参数值。消除了 store 和 load，直接使用 %arg0 传递函数参数值。

   在处理下面条件语句时

   ```c++
   if(x > 0){
       x = 0;
   }
   return x;
   ```

   消除了 store 和 load，加入 phi 指令来对于当前基本块的前驱进行判断，来获得相应的值。

   在该例中，该部分变化为：

   ```c++
   label6:                                                ; preds = %label_entry
     br label %label7
   label7:                                                ; preds = %label_entry, %label6
     %op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]
     ret i32 %op9
   ```

   其核心

   ```c++
   %op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]
   ```

   表示：若前驱为基本块 entry，则 %op9 的值取 %arg0，即参数 x。若前驱为基本块 6，则 %op9 的值取 0。实现了该想要的 if 条件语句。

   在函数 main 中，对局部变量 b，也删除了 alloca 代码，消除了 store 和 load，直接使用 2333 在函数调用 func(b) 中。即删去

   ```c++
   %op1 = alloca i32
   store i32 2333, i32* %op1
   %op6 = load i32, i32* %op1
   ```

   而对于数组 arr 和全局变量 globVar ，其 store 和 load 指令没变。

   

   

5. ...

   用到的代码块为：

   ```c++
   void Mem2Reg::generate_phi()
   ```

   首先计算结点的支配边界

   ```c++
   // step 1: find all global_live_var_name x and get their blocks 
   std::set<Value *> global_live_var_name;
   std::map<Value *, std::set<BasicBlock *>> live_var_2blocks;
   for ( auto bb : func_->get_basic_blocks() )
   {
       std::set<Value *> var_is_killed;
       for ( auto instr : bb->get_instructions() )
       {
           if ( instr->is_store() )
           {
               // store i32 a, i32 *b
               // a is r_val, b is l_val
               auto r_val = static_cast<StoreInst *>(instr)->get_rval();
               auto l_val = static_cast<StoreInst *>(instr)->get_lval();
   
               if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val))
               {
                   global_live_var_name.insert(l_val);
                   live_var_2blocks[l_val].insert(bb);
               }
           }
       }
   }
   ```

   然后，放置 phi 函数

   ```c++
   // step 2: insert phi instr
   std::map<std::pair<BasicBlock *,Value *>, bool> bb_has_var_phi; // bb has phi for var
   for (auto var : global_live_var_name )
   {
       std::vector<BasicBlock *> work_list;
       work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
       for (int i =0 ; i < work_list.size() ; i++ )
       {   
           auto bb = work_list[i];
           for ( auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb))
           {
               if ( bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end() )
               { 
                   // generate phi for bb_dominance_frontier_bb & add bb_dominance_frontier_bb to work list
                   auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
                   phi->set_lval(var);
                   bb_dominance_frontier_bb->add_instr_begin( phi );
                   work_list.push_back( bb_dominance_frontier_bb );
                   bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
               }
           }
       }
   }
   ```

   在第一步也是使用了支配树来计算支配边界的相关信息的。不过题意中放置 phi 结点应该是指的是狭义的第二部分。故详细讨论第二部分对支配树的利用。

   在第二步中，编译器可以轻松地找到全局名字集合。对每个全局名字 x，

   ```c++
   var : global_live_var_name
   ```

   算法首先对该名字申请一个 work_list，初始化为该全局名字 x 被定义或赋值的基本块集合。
   
   ```c++
   work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
   ```
   
   对于 work_list 上的每个基本块 b，算法通过
   
   ```c++
   dominators_->get_dominance_frontier(bb))
   ```
   
   利用支配树找到该基本块 b 的支配边界。在 b 的支配边界中，每个程序块 d 的起始处插入 phi 函数。然后将相应的关系存入 bb_has_var_phi 中。在向 d 插入对于 x 的 phi 节点后，将基本块 d 加入到 work_list， 循环进行上述操作，直到 work_list 中的每个基本块都访问完成。



### 代码阅读总结

此次实验有什么收获

* 了解了更多的C++容器
* 掌握了计算图的强连通分量的算法，以及一种找出强连通分量内部嵌套的强连通分量的思路。
* 掌握了构造静态单赋值形式的简单方法，理解了相关算法。
* 理解并接受了 phi 函数存在的意义和必要性。
* 强化了附加材料中伪代码算法与实现代码的对应阅读理解能力。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
