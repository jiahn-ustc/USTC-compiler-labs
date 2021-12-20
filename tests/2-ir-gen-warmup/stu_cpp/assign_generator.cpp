#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到
int main()
{
    auto module = new Module("Cminus code"); // module name是什么无关紧要
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    
    auto bb = BasicBlock::create(module, "entry", mainFun);//create "entry" basicblock
    builder->set_insert_point(bb);//insert the basicblock into IRbuilder
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
    std::cout << module->print();
    delete module;

    return 0;
}