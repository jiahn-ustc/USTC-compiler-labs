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
    Type *FloatType = Type::get_float_type(module);
    //function main
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);

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

    //tureBB
    builder->set_insert_point(trueBB);
    builder->create_store(CONST_INT(233), retAlloca);
    builder->create_br(retBB); // br retBB

    //falseBB
    builder->set_insert_point(falseBB);
    builder->create_store(CONST_INT(0), retAlloca);
    builder->create_br(retBB); // br retBB

    //retBB
    builder->set_insert_point(retBB); // ret分支
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    std::cout << module->print();
    delete module;
    return 0;
}