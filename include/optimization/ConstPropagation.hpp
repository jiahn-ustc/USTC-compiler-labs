#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>

// tips: 用来判断value是否为ConstantFP/ConstantInt
ConstantFP* cast_constantfp(Value *value);
ConstantInt* cast_constantint(Value *value);


// tips: ConstFloder类

class ConstFolder
{
public:
    ConstFolder(Module *m) : module_(m) {}
    ConstantInt *compute(
        Instruction::OpID op,
        ConstantInt *value1,
        ConstantInt *value2);
    // ...
    ConstantFP *computeFP(
        Instruction::OpID op,
        ConstantFP *value1,
        ConstantFP *value2
    );
    ConstantInt *computeCMP(
        CmpInst::CmpOp op,
        ConstantInt *value1,
        ConstantInt *value2
    );
    ConstantInt *computeFCMP(
        FCmpInst::CmpOp OP,
        ConstantFP *value1,
        ConstantFP *value2
    );
private:
    Module *module_;
};

class ConstPropagation : public Pass
{
public:
    ConstPropagation(Module *m) : Pass(m) {}
    void run();
    //通过get_value得到store指令对应变量的常数值
    //通过push_back更新store指令对应变量的常数值
    //通过clear初始化Map
    ConstantInt *get_value_to_Int(Value *value);
    ConstantFP *get_value_to_Float(Value *value);
    ConstantInt *push_back_Int(Value *value,ConstantInt *const_int);
    ConstantFP *push_back_Fp(Value *value,ConstantFP *const_float);
    void clear_Int_map();
    void clear_float_map();
private:
    //IntMap、FloatMap记录全局变量在块中的变化，具体指store、load指令
    //由于store指令并没有给某个中间变量定值，故不能找到它的user
    //只能将store用IntMap、FloatMap记录下来
    std::unordered_map<Value *,ConstantInt *> IntMap;
    std::unordered_map<Value *,ConstantFP *> FloatMap;
};

#endif
