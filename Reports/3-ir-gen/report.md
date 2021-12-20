# Lab3 实验报告

朱文冬 PB19111714

周佳豪 PB19051035


## 实验难点

1. 首推指针和数组的区别！

2. 把函数从符号表中找到并进行强制类型转换。

3. if_else语句体中可能会直接跳转

4. while语句中，由于while_body中的数组越界检查会导致如下情况：

   ```tex
   whilecond:
       codes
       ...
       br whilebody or whileend
   whilebody:
   	codes
   	...
   	br error_BB or correct_BB
   whileend:
   
   error_BB:    ;数组不合法
   	codes:		;调用特定函数，程序终止
   
   correct_BB:			;数组合法
   	codes 			;whilebody之后的指令
   	br whilecode	
   
   ```

   whileend之后会直接进入error_BB，此时需要在函数末尾创建一个新的基本块，并在whileend中插入 br new_block

## 实验设计

请写明为了顺利完成本次实验，加入了哪些亮点设计，并对这些设计进行解释。
可能的阐述方向有:

1. 如何设计全局变量
2. 遇到的难点以及解决方案
3. 如何降低生成 IR 中的冗余
4. ...



按 declarations 的大小，依次访问每个 declaration

```c++
for(int i=0;i<node.declarations.size();i++){
    node.declarations[i]->accept(*this);
}
```

两个变量存储 assignment variable 的地址和 expression 的加载

```c++
Value *temp_Var;
Value *temp_expression;
```

对 ASTNum 类型 node ，分整型和浮点型分别处理

```c++
if(node.type == TYPE_INT){
    temp_expression = CONST_INT(node.i_val);
    printf("value = %d\n",node.i_val);
}
else if(node.type == TYPE_FLOAT){
    temp_expression = CONST_FP(node.f_val);
    printf("value = %f\n",node.f_val);
}
```

处理变量声明 ASTVarDeclaration 时，对整型和浮点型

```c++
Type *type_declaration;
if(node.type==TYPE_INT){
    type_declaration = Type::get_int32_type(module.get());
}
else if(node.type==TYPE_FLOAT){
    type_declaration = Type::get_float_type(module.get());
}
```

为初始化准备

```c++
auto initializer = ConstantZero::get(type_declaration,module.get());
```

用 scope.in_global() 判断该变量是否为全局变量（或数组）。

对全局变量、全局数组

```c++
auto global_var=GlobalVariable::create(node.id,module.get(),type_declaration,false,initializer);
scope.push(node.id,global_var);
```

```c++
auto *arrayType = ArrayType::get(type_declaration,node.num->i_val);
auto global_array = GlobalVariable::create(node.id,module.get(),arrayType,false,initializer);
scope.push(node.id,global_array);
```

对局部变量、局部数组

```c++
auto local_var = builder->create_alloca(type_declaration);
scope.push(node.id,local_var);
```

```c++
auto *arrayType = ArrayType::get(type_declaration,node.num->i_val);
auto local_array = builder->create_alloca(arrayType);
scope.push(node.id,local_array);
```

在函数声明 ASTFunDeclaration 部分

设置标志位 start 用于 void 类型函数最后执行 builder->create_void_ret() 指令

```c++
int start=0;
```

```c++
if(node.type==TYPE_INT){
    type_fun = Type::get_int32_type(module.get());
}
else if(node.type==TYPE_FLOAT){
    type_fun = Type::get_float_type(module.get());
}
else{
    type_fun = Type::get_void_type(module.get());
    start = 1;
}
```

```c++
if(start == 1)
    builder->create_void_ret();
```

全局变量 type_params 用于储存形参的类型

```c++
std::vector <Type *> type_params;
```

每次使用前，若不为空时需要清空

```c++
while (!type_params.empty())
{
    type_params.pop_back();
}
```

遍历每个形参

```c++
for(int i=0;i<node.params.size();i++){
    node.params[i]->accept(*this);
}
printf("type_params size is %d\n",type_params.size());
```

将声明后函数的 id 维护在符号表中

```c++
auto FunTy = FunctionType::get(type_fun,type_params);
auto Fun = Function::create(FunTy,node.id,module.get());
scope.push(node.id,Fun);
```

进入函数作用域

```c++
scope.enter();
auto bb = BasicBlock::create(module.get(),node.id+"_entry"+std::to_string(Block_number++),Fun);
printf("this is a function,args num is %d\n",Fun->get_num_of_args());
builder->set_insert_point(bb);
```

得到传入函数的实参

```c++
for(auto arg = Fun->arg_begin();arg!=Fun->arg_end();arg++){
    args.push_back(*arg);
}
printf("have push_back args\n");
```

遍历形参 param ，为形参分配空间，并储存对应实参 arg 的数据

若形参为数组，即对应于 point 变量的实参。为形参分配指向 int / float 变量的指针空间，并将空间的地址储存到符号表中。

```c++
if(param->type==TYPE_INT){
    auto IntArrayAlloca = builder->create_alloca(Int32PtrType);
    builder->create_store(arg,IntArrayAlloca);
    scope.push(param->id,IntArrayAlloca);
    printf("Have store a array\n");
}
else if(param->type==TYPE_FLOAT){
    auto FloatArrayAlloca = builder->create_alloca(FloatPtrType);
    builder->create_store(arg,FloatArrayAlloca);
    scope.push(param->id,FloatArrayAlloca);
}
```

若形参为一般 int / float 变量，则为其分配相应空间，并储存对应的实参的值。

```c++
if(param->type==TYPE_INT){
    auto IntAlloca = builder->create_alloca(Int32Type);
    builder->create_store(arg,IntAlloca);
    scope.push(param->id,IntAlloca);
}
else if(param->type==TYPE_FLOAT){
    auto FloatAlloca = builder->create_alloca(FloatType);
    builder->create_store(arg,FloatAlloca);
    scope.push(param->id,FloatAlloca);
}
```

对每一个形参 ASTParam 的访问，

先判断数组/普通变量，再判断 int / float 变量，无需多言。

```c++
if(node.isarray){
    if(node.type==TYPE_INT){
        type_param = Type::get_int32_ptr_type(module.get());    
    }
    else if(node.type==TYPE_FLOAT){
        type_param = Type::get_float_ptr_type(module.get());    
    }
    type_params.push_back(type_param);
}
else{
    if(node.type==TYPE_INT){
        type_param = Type::get_int32_type(module.get());
    }
    else if(node.type==TYPE_FLOAT){
        type_param = Type::get_float_type(module.get());
    }
    type_params.push_back(type_param);
}
```

对函数体 ASTCompoundStmt ，

其包含 local_declaration 与 statement_expression 两种语句，依次访问函数体的每一个语句。

```c++
scope.enter();
for(int i=0; i<node.local_declarations.size();i++){
    node.local_declarations[i]->accept(*this);
}
for(int i=0; i<node.statement_list.size();i++){
    node.statement_list[i]->accept(*this);
}
scope.exit();
```

对于条件选择 ASTSelectionStmt 模块

以 if ( expression ) statement else statement 模式为代表，不带 else 的情况同理。

由于要与整型 0 比较是否相等，故若判断的表达式为 float 型，先进行类型转换。

```c++
if(temp_expression->get_type()->is_float_type()){
    if_expression = builder->create_fptosi(temp_expression,Int32Type);
}
```

next_block 表示紧跟着 if_else 语句的基本块，基本块 ifBB 与 elseBB 用于条件跳转。

因为定义的 icmp 判断的是 expression 值是否为 0 （而不是是否为真），所以在 create_cond_br 中设计的 ifBB 与 elseBB 的位置同习惯略有不同。

```c++
auto icmp = builder->create_icmp_eq(if_expression,CONST_INT(0));
auto ifBB = BasicBlock::create(module.get(), "ifBB"+std::to_string(Block_number),builder->get_insert_block()->get_parent());
auto elseBB = BasicBlock::create(module.get(), "elseBB"+std::to_string(Block_number),builder->get_insert_block()->get_parent());
BasicBlock *next_block;
Block_number++;
builder->create_cond_br(icmp,elseBB,ifBB);
```

在 ifBB 块中，设计 is_if_br ，来表示 if 块内部是否有return、goto等跳转语句。

```c++
auto is_if_br = builder->get_insert_block()->get_terminator();
```

若无此类语句，is_if_br 为空指针，则需要在 if 块结束后跳转至 next_block 块。

设标志位 start 指示是否已经创建 next_block 块，避免创建两次。

```c++
int start = 0;
```

```c++
if(is_if_br == nullptr){
    next_block = BasicBlock::create(module.get(), std::to_string(Block_number++),builder->get_insert_block()->get_parent());
    builder->create_br(next_block);
    start = 1;
    printf("--------------if------------\n");
}
```

在 elseBB 中，同样设置 is_else_br ，作用与 is_if_br 相同。

如果 if_else 语句结束后仍需顺序执行，此时一定已经创建了 next_block，则将 next_block 插入到 builder 中。

```c++
if(is_if_br==nullptr || is_else_br== nullptr){
    builder->set_insert_point(next_block);
    printf("----something is nullptr\n");
}
```

对循环语句 ASTIterationStmt 部分

while语句可分为三个块，分别为：

循环判断块 whilecond，循环体块 whilebody，循环结束块 whileend。

```c++
auto Fun = builder->get_insert_block()->get_parent();
auto whilecond = BasicBlock::create(module.get(),"whilecond"+std::to_string(Block_number),Fun);
auto whilebody = BasicBlock::create(module.get(),"whilebody"+std::to_string(Block_number),Fun);
auto whileend = BasicBlock::create(module.get(),"whileend"+std::to_string(Block_number++),Fun);
```

判断时，同条件语句，需要先进行 float → int 强制类型转换

```c++
if(temp_expression->get_type()->is_float_type()){
    cond_expression = builder->create_fptosi(temp_expression,Int32Type);
}
```

icmp 判断 expression 值是否为 0 （而不是是否为真），所以与 ifBB 、 elseBB 的位置设置同理，将 whileend 放在了前面。

```
auto icmp = builder->create_icmp_eq(cond_expression,CONST_INT(0));
builder->create_cond_br(icmp,whileend,whilebody);
```

在循环体 whilebody 中，设 is_whilebody_b 表示 whilebody 块中是否有 return、goto 等跳转指令。

```c++
auto is_whilebody_br = builder->get_insert_block()->get_terminator();
if(is_whilebody_br== nullptr){
    builder->create_br(whilecond);
}
```

对于返回语句 ASTReturnStmt 部分，分为两种情况

若不带返回值，即  return-stmt → return ; 

```c++
if(node.expression==nullptr){
    printf("return-stmt -> return;\n");
    builder->create_void_ret();
}
```

若带返回值，即 return-stmt → return expression ；

如果表达式的值与函数返回值的类型冲突，则需要进行类型转换。

```c++
if(FunReturnType!=right_expression->get_type()){
    if(FunReturnType->is_float_type()){
        right_expression = builder->create_sitofp(right_expression,FloatType);
    }
    else if(FunReturnType->is_integer_type()){
        right_expression = builder->create_fptosi(right_expression,Int32Type);
    }
}
if(FunReturnType->is_float_type() || FunReturnType->is_integer_type()){
    builder->create_ret(right_expression);
}
else if(FunReturnType->is_void_type())
    builder->create_void_ret();
```

对变量 ASTVar 部分

若为一般变量，

```c++
if(node.expression==nullptr){
    printf("var -> ID \n");
    temp_Var = scope.find(node.id);
    temp_expression = builder->create_load(temp_Var);
}
```

若为数组变量，

获取 expression 作为数组索引，若非整型就进行类型转换。

```c++
printf("var -> ID [expression]\n");
auto array_var = scope.find(node.id);
printf("will accpet the expression\n");
node.expression->accept(*this);
printf("have accepted the expression\n");
auto array_index = temp_expression;
if(temp_expression->get_type()->is_float_type()){
    auto Int32Type = Type::get_int32_type(module.get());
    array_index = builder->create_fptosi(temp_expression,Int32Type);
    printf("array_index float -> int\n");
}
```

判断数组索引是否非法（是否 < 0 ），若非法则报错。

```c++
auto icmp = builder->create_icmp_lt(array_index,CONST_INT(0));
auto errorBB = BasicBlock::create(module.get(),node.id+"_error"+std::to_string(Block_number),builder->get_insert_block()->get_parent());
auto correctBB = BasicBlock::create(module.get(),node.id+"_correct"+std::to_string(Block_number++),builder->get_insert_block()->get_parent());
auto br = builder->create_cond_br(icmp,errorBB,correctBB);
```

```c++
builder->set_insert_point(errorBB);
auto error_function = scope.find("neg_idx_except");
builder->create_call(error_function,{});
builder->create_br(correctBB);
```

对数组和指针的处理

```c++
builder->set_insert_point(correctBB);
if(array_var->get_type()->get_pointer_element_type()->is_array_type()){
            //array
            temp_Var = builder->create_gep(array_var, {CONST_INT(0), array_index});    
        }
        else{
            //pointer
            auto array_load = builder->create_load(array_var);
            temp_Var = builder->create_gep(array_load, {array_index});
        }
        temp_expression = builder->create_load(temp_Var);
```

这里的array和point分别对应以下函数调用

```c++
//例1
void A(int a[]);
void main(){
    int a[10];
    A(a);
}
//函数调用，需要得到实参a
//用scope.find(a),返回数组a的地址
//故A(a)中的a是一个[10 x i32]指针，指向数组a
//在上述代码中array_var即为一个[10 x i32]指针，get_pointer_element_type()返回它指向元素的类型，即数组a，即is_array_type()返回true
```

```c++
//例2
void A(int a[]);
void B(int b[]){
    A(b);
}
//首先需要了解函数声明做了什么
//在函数B的声明中，实参为b0Gep，即一个指向数组b第一个元素的指针，为i32 *
//然后函数声明过程后，会创建一个类型为Int32Ptr的变量，把b0Gep store进去，最后把b push进符号表。
//在B中调用A(b)，先find(b)，得到b的地址，b中存储的是b0Gep，故对b使用get_pointer_element_type()即得到b0Gep的类型，即 i32 *，is_pointer_type()会返回true
```

对赋值表达式 ASTAssignExpression 部分

获取左侧变量

```c++
node.var->accept(*this);
Value *left_var = temp_Var;
Value *left_expression = temp_expression;
printf("var is ready\n");
```

获取右侧表达式

```c++
node.expression->accept(*this);
Value *right_expression = temp_expression;
printf("right_expression is ready\n");
```

若左右类型不同，进行类型转换。

```c++
if(left_expression->get_type()->is_float_type() 
            && right_expression->get_type()->is_integer_type()
            || left_expression->get_type()->is_integer_type()
            && right_expression->get_type()->is_float_type()) {
    if(left_expression->get_type()->is_float_type()){
        right_expression = builder->create_sitofp(right_expression,FloatType);
        printf("right_expression int to float\n");
    }
    else if(left_expression->get_type()->is_integer_type()){
        right_expression = builder->create_fptosi(right_expression,Int32Type);
        printf("right_expression float to int\n");
    }
}
```

```c++
builder->create_store(right_expression,left_var);
temp_expression = builder->create_load(left_var);
```

对简单表达式 ASTSimpleExpression 部分

若 node.additive_expression_r = nullptr ，即用产生式 simple-expression → additive-expression ，无需多言。

而若  simple-expression → additive-expression relop additive-expression ，

```c++
node.additive_expression_l->accept(*this);
auto addit_l_expression = temp_expression;
node.additive_expression_r->accept(*this);
auto addit_r_expression = temp_expression;
```

若关系运算符左右类型不同，统一转化为浮点型。并对六种关系算符进行处理。

```
if(addit_l_expression->get_type()->is_float_type() 
            && addit_r_expression->get_type()->is_integer_type()
            || addit_l_expression->get_type()->is_integer_type()
            && addit_r_expression->get_type()->is_float_type()){
    if(addit_l_expression->get_type()->is_float_type()){
        addit_r_expression = builder->create_sitofp(addit_r_expression,FloatType);
    }
    else if(addit_r_expression->get_type()->is_float_type()){
        addit_l_expression = builder->create_sitofp(addit_l_expression,FloatType);
    }
    switch(node.op){
        case OP_LE:
            temp_expression = builder->create_fcmp_le(addit_l_expression,addit_r_expression);
            break;
        case OP_LT:
            temp_expression = builder->create_fcmp_lt(addit_l_expression,addit_r_expression);
            break;
        case OP_GT:
            temp_expression = builder->create_fcmp_gt(addit_l_expression,addit_r_expression);
            break;
        case OP_GE:
            temp_expression = builder->create_fcmp_ge(addit_l_expression,addit_r_expression);
            break;
        case OP_EQ:
            temp_expression = builder->create_fcmp_eq(addit_l_expression,addit_r_expression);
            break;
        case OP_NEQ:
            temp_expression = builder->create_fcmp_ne(addit_l_expression,addit_r_expression);
            break;
    }
}
```

若左右均为浮点型或均为整型，按对应类型处理。

```c++
else if(addit_l_expression->get_type()->is_float_type()
                && addit_r_expression->get_type()->is_float_type()){
    switch(node.op){
        case OP_LE:
            temp_expression = builder->create_fcmp_le(addit_l_expression,addit_r_expression);
            break;
        case OP_LT:
            temp_expression = builder->create_fcmp_lt(addit_l_expression,addit_r_expression);
            break;
        case OP_GT:
            temp_expression = builder->create_fcmp_gt(addit_l_expression,addit_r_expression);
            break;
        case OP_GE:
            temp_expression = builder->create_fcmp_ge(addit_l_expression,addit_r_expression);
            break;
        case OP_EQ:
            temp_expression = builder->create_fcmp_eq(addit_l_expression,addit_r_expression);
            break;
        case OP_NEQ:
            temp_expression = builder->create_fcmp_ne(addit_l_expression,addit_r_expression);
            break;
    }
}                    
```

```c++
else if(addit_l_expression->get_type()->is_integer_type()
                && addit_r_expression->get_type()->is_integer_type()){
    switch(node.op){
        case OP_LE:
            temp_expression = builder->create_icmp_le(addit_l_expression,addit_r_expression);
            break;
        case OP_LT:
            temp_expression = builder->create_icmp_lt(addit_l_expression,addit_r_expression);
            break;
        case OP_GT:
            temp_expression = builder->create_icmp_gt(addit_l_expression,addit_r_expression);
            break;
        case OP_GE:
            temp_expression = builder->create_icmp_ge(addit_l_expression,addit_r_expression);
            break;
        case OP_EQ:
            temp_expression = builder->create_icmp_eq(addit_l_expression,addit_r_expression);
            break;
        case OP_NEQ:
            temp_expression = builder->create_icmp_ne(addit_l_expression,addit_r_expression);
            break;
    }
}
```

结果要整型

```c++
temp_expression = builder->create_zext(temp_expression,Int32Type);
```

对加法表达式 ASTAdditiveExpression 部分

若 node.additive_expression = nullptr ，即用产生式 additive-expression → term ，无需多言。

而若 additive-expression → additive-expression addop term ，

```c++
node.additive_expression->accept(*this);
auto addit_expression = temp_expression;
node.term->accept(*this);
auto term_expression = temp_expression;
printf("have two expressions and will addop them\n");
```

若加减运算符左右类型不同，统一转化为浮点型。并对加减运算进行处理。

```c++
if(addit_expression->get_type()->is_float_type() 
            && term_expression->get_type()->is_integer_type()
            || addit_expression->get_type()->is_integer_type()
            && term_expression->get_type()->is_float_type()){
    printf("1\n");
    if(addit_expression->get_type()->is_float_type()){
        term_expression = builder->create_sitofp(term_expression,FloatType);
    }
    else if(term_expression->get_type()->is_float_type()){
        addit_expression = builder->create_sitofp(addit_expression,FloatType);
    }
    switch(node.op){
        case OP_PLUS:
            temp_expression = builder->create_fadd(addit_expression, term_expression);
            break;
        case OP_MINUS:
            temp_expression= builder->create_fsub(addit_expression, term_expression);
            break;
    }
}
```

若左右均为浮点型或均为整型，按对应类型处理。

```c++
else if(addit_expression->get_type()->is_float_type()
                && term_expression->get_type()->is_float_type()){
    printf("2\n");
    printf("2.01\n");
    switch(node.op){
        case OP_PLUS:
            temp_expression = builder->create_fadd(addit_expression, term_expression);
            break;
        case OP_MINUS:
            temp_expression = builder->create_fsub(addit_expression, term_expression);
            break;
    }
    printf("2.1\n");
}
```

```c++
else if(addit_expression->get_type()->is_integer_type()
                && term_expression->get_type()->is_integer_type()){
    switch(node.op){
        case OP_PLUS:
            temp_expression = builder->create_iadd(addit_expression, term_expression);
            break;
        case OP_MINUS:
            temp_expression = builder->create_isub(addit_expression, term_expression);
            break;
    }
}
```

对乘法表达式（姑且这么叫） ASTTerm 部分

与加法表达式完全同理，不再赘述。

对调用 ASTCall 部分

强制类型转换，得到函数的类型

```c++
auto Fun = scope.find(node.id);
auto FunType = static_cast<FunctionType*>(Fun->get_type());
```

遍历每个实参：

value_expression 代表实参表达式的值，适用于 int 和 float 变量；value_address 代表实参的地址，适用于数组与指针。

```c++
auto value_expression = temp_expression;
auto value_address = temp_Var;
```

若为指针类型，分为指向指针的指针和指向数组的指针。

```c++
if(FunType->get_param_type(i)->is_pointer_type()){
    auto value_load = builder->create_load(value_address);
    if(value_load->get_type()->is_pointer_type()){
        real_args.push_back(value_load);
    }
    else{
        printf("Call Function: this is a array!!!\n");
        auto arg = builder->create_gep(value_address, {CONST_INT(0), CONST_INT(0)});
        real_args.push_back(arg);
    }
    printf("Call Function : have store the pointer---------------- \n");
}
```

若为普通类型（int / float），类型转换。

```c++
else{
    if(FunType->get_param_type(i)!=value_expression->get_type()){ 
        if(FunType->get_param_type(i)->is_float_type()){
            value_expression = builder->create_sitofp(temp_expression,FloatType);
        }
        else if(FunType->get_param_type(i)->is_integer_type()){
            value_expression = builder->create_fptosi(temp_expression,Int32Type);
        }
    }
    real_args.push_back(value_expression);
}
```

```c++
auto call = builder->create_call(Fun,real_args);
temp_expression = call;
printf("call function is over\n");
```



### 实验总结

* 真的理解了数组和指针的区别，具体我已在Var中进行了阐述，太爽了！
* 对C++的多态有了很深的理解
* printf调试大法yyds
* 对IterationStmt表达式的while_cond、while_body、while_end的细节有了很好的理解，具体见实验难点吧。

* 此次实验中，使用 `LightIR` 框架自动产生 `cminus-f` 语言的LLVM IR。

  在 lab2 中对最后一个部分的访问者模式当时理解的并不是很彻底，通过这次实验亲手实践对其有了更加深刻的体会，并利用访问者模式实现了 IR 的自动生成。同时，与同学间的协作能力有了很大的提高。

### 实验反馈 （可选 不会评分）

* 体验很好，lab2的样例在lab3有很大帮助
* 不会使用logging，助教应该说的更详细一些，比如具体到某个bug
* 也不会使用GDB，助教给了示例应该会很好，最后只能靠printf大法了

### 组间交流 （可选）

无
