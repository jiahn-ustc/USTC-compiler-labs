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

    //function main
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
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
    


    //whilecond
    builder->set_insert_point(whilecond);
    auto iLoad = builder->create_load(iAlloca);
    auto icmp = builder->create_icmp_lt(iLoad,CONST_INT(10));
    auto br = builder->create_cond_br(icmp, whilebody, whileend);

    //whilebody
    builder->set_insert_point(whilebody);
    iLoad = builder->create_load(iAlloca);
    auto add_i = builder->create_iadd(iLoad,CONST_INT(1));
    builder->create_store(add_i, iAlloca);
    auto aLoad = builder->create_load(aAlloca);
    iLoad = builder->create_load(iAlloca);
    auto add_a = builder->create_iadd(aLoad,iLoad);
    builder->create_store(add_a, aAlloca);
    builder->create_br(whilecond);

    //whileend
    builder->set_insert_point(whileend);
    aLoad = builder->create_load(aAlloca);
    builder->create_store(aLoad,retAlloca);
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    std::cout << module->print();
    delete module;
    return 0;
}