# Lab4 实验报告

小组成员 姓名 学号
* 队长
* 队员：PB19051035周佳豪

## 实验要求

* 完成常量传播以及死代码删除的优化
* 完成循环不变式外移的优化
* 完成活跃变量分析的优化

## 实验难点

实验中遇到哪些挑战
* 常量传播与死代码删除难点
   * 对`store`、`load`指令的处理。
   * 如何删除死代码块。
   * 删除死代码块要考虑`phi`指令。
   * 对于`replace_all_use_with()`函数的理解
* 循环不变式外提难点
  * 关于指令本身就是左值的理解
  * 循环不变式外提时前置块末尾是br指令的处理
* 活跃变量分析难点

## 实验设计

* 常量传播
   * 实现思路：
     * 遍历每一个函数，广度优先遍历每一个函数内部的基本块，由于每个基本块的结尾均为跳转指令，即br，故将可能跳转的基本块加入队列。
     *  遍历基本块内部的每一个指令，并进行常量传播
        * 对于基本运算和基本比较指令，例如`add``fadd``cmp`等，通过`compute`函数计算右值是否为常数。若为常数，则通过该指令的`use`链表将所有使用到该指令左值的变量替换为常数。并将该指令加入到待删除指令的集合中。
        * 对于`si2fp`、`fp2si`和`zext`指令，通过`cast_constantint`与`cast_constantfp`函数判断右值是否为常数。若为常数，则通过该指令的`use`链表将所有使用到该指令左值的变量替换为常数。并将该指令加入到待删除指令的集合中。
        * 对于`store`与`load`指令。由于store指令没有左值，故无法通过use链表找到使用者，只能通过一个map容器记录`value`与`ConstantInt`或`ConstantFP`的对应关系。只有`load`指令才会用到`store`指令储存的常量，故map容器在`store`指令更新，在`load`指令使用。
        * 对于`br`指令。只需要处理条件跳转指令，先判断跳转条件变量`cmp`是否为常数。若为常数，通过`replace_all_use_with`函数创建一个新的无条件跳转指令，并将原条件跳转指令加入到待删除指令的集合中，并将跳向的基本块入队列。若不为常数，则将跳向的两个基本块都入队列。
        * 其余指令不会得到一定是常量的左值，故不进行处理。
     *  死代码删除：广度优先搜索维护了一个`visited`容器,储存了所有可能访问到的基本块。故再次遍历所有基本块，并与`visited`比较，即可得到死代码块，删除死代码块即可。
     *  删除基本块的注意事项：删除死代码块要考虑`phi`指令。因为phi指令可能会用到来自死代码块的变量。通过遍历`delete_bb`的use链表，如果是`phi`指令，并且使用了来自`delete_bb`的变量，则删除`phi`指令中对应的操作数。
   * 相应代码：
    ```c++
    //ConstPropagation.hpp
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
    ```
    ```C++
    #include "ConstPropagation.hpp"
    #include "logging.hpp"
    #include <queue>
    #include <algorithm>
    // 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
    // 当然如果同学们有更好的方式，不强求使用下面这种方式
    ConstantInt *ConstFolder::compute(
        Instruction::OpID op,
        ConstantInt *value1,
        ConstantInt *value2)
    {

        int c_value1 = value1->get_value();
        int c_value2 = value2->get_value();
        switch (op)
        {
        case Instruction::add:
            return ConstantInt::get(c_value1 + c_value2, module_);

            break;
        case Instruction::sub:
            return ConstantInt::get(c_value1 - c_value2, module_);
            break;
        case Instruction::mul:
            return ConstantInt::get(c_value1 * c_value2, module_);
            break;
        case Instruction::sdiv:
            return ConstantInt::get((int)(c_value1 / c_value2), module_);
            break;
        default:
            return nullptr;
            break;
        }
    }
    /*fadd,
            fsub,
            fmul,
            fdiv,*/
    //浮点型的常数折叠
    ConstantFP *ConstFolder::computeFP(
        Instruction::OpID op,
        ConstantFP *value1,
        ConstantFP *value2)
    {
        float c_value1 = value1->get_value();
        float c_value2 = value2->get_value();
        switch (op)
        {
        case Instruction::fadd:
            return ConstantFP::get(c_value1 + c_value2, module_);

            break;
        case Instruction::fsub:
            return ConstantFP::get(c_value1 - c_value2, module_);
            break;
        case Instruction::fmul:
            return ConstantFP::get(c_value1 * c_value2, module_);
            break;
        case Instruction::fdiv:
            return ConstantFP::get(c_value1 / c_value2, module_);
            break;
        default:
            return nullptr;
            break;
        }
    }

    //比较类型的常数折叠
    //整型数比较
    ConstantInt *ConstFolder::computeCMP(
        CmpInst::CmpOp op,
        ConstantInt *value1,
        ConstantInt *value2)
    {
        int c_value1 = value1->get_value();
        int c_value2 = value2->get_value();
        int cmp = 0;
        switch (op)
        {
        case CmpInst::EQ:
            cmp = (int)(c_value1 == c_value2);
            break;
        case CmpInst::NE:
            cmp = (int)(c_value1 != c_value2);
            break;
        case CmpInst::GT:
            cmp = (int)(c_value1 > c_value2);
            break;
        case CmpInst::GE:
            cmp = (int)(c_value1 >= c_value2);
            break;
        case CmpInst::LT:
            cmp = (int)(c_value1 < c_value2);
            break;
        case CmpInst::LE:
            cmp = (int)(c_value1 <= c_value2);
            break;
        default:
            return nullptr;
            break;
        }
        return ConstantInt::get(cmp, module_);
    }
    //浮点数比较
    ConstantInt *ConstFolder::computeFCMP(
        FCmpInst::CmpOp op,
        ConstantFP *value1,
        ConstantFP *value2)
    {
        float c_value1 = value1->get_value();
        float c_value2 = value2->get_value();
        int cmp = 0;
        switch (op)
        {
        case CmpInst::EQ:
            cmp = (int)(c_value1 == c_value2);
            break;
        case CmpInst::NE:
            cmp = (int)(c_value1 != c_value2);
            break;
        case CmpInst::GT:
            cmp = (int)(c_value1 > c_value2);
            break;
        case CmpInst::GE:
            cmp = (int)(c_value1 >= c_value2);
            break;
        case CmpInst::LT:
            cmp = (int)(c_value1 < c_value2);
            break;
        case CmpInst::LE:
            cmp = (int)(c_value1 <= c_value2);
            break;
        default:
            return nullptr;
            break;
        }
        return ConstantInt::get(cmp, module_);
    }

    // 用来判断value是否为ConstantFP，如果不是则会返回nullptr
    ConstantFP *cast_constantfp(Value *value)
    {
        auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
        if (constant_fp_ptr)
        {
            return constant_fp_ptr;
        }
        else
        {
            return nullptr;
        }
    }
    ConstantInt *cast_constantint(Value *value)
    {
        auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
        if (constant_int_ptr)
        {
            return constant_int_ptr;
        }
        else
        {
            return nullptr;
        }
    }

    ConstantInt *ConstPropagation::get_value_to_Int(Value *value)
    {
        auto global_value = dynamic_cast<GlobalVariable *>(value);
        if (global_value != nullptr)
        {
            auto iter = IntMap.find(global_value);
            if (iter != IntMap.end())
            {
                return iter->second;
            }
        }
        return nullptr;
    }
    ConstantFP *ConstPropagation::get_value_to_Float(Value *value)
    {
        auto global_value = dynamic_cast<GlobalVariable *>(value);
        if (global_value != nullptr)
        {
            auto iter = FloatMap.find(global_value);
            if (iter != FloatMap.end())
            {
                return iter->second;
            }
        }
        return nullptr;
    }

    ConstantInt *ConstPropagation::push_back_Int(Value *value, ConstantInt *const_int)
    {
        auto global_value = dynamic_cast<GlobalVariable *>(value);
        if (global_value != nullptr)
        {
            auto iter = IntMap.find(global_value);
            if (iter != IntMap.end())
            {
                iter->second = const_int;
                return iter->second;
            }
            else
            {
                IntMap.insert({global_value, const_int});
                return IntMap[global_value];
            }
        }
        return nullptr;
    }
    ConstantFP *ConstPropagation::push_back_Fp(Value *value, ConstantFP *const_float)
    {
        auto global_value = dynamic_cast<GlobalVariable *>(value);
        if (global_value != nullptr)
        {
            auto iter = FloatMap.find(global_value);
            if (iter != FloatMap.end())
            {
                iter->second = const_float;
                return iter->second;
            }
            else
            {
                FloatMap.insert({global_value, const_float});
                return FloatMap[global_value];
            }
        }
        return nullptr;
    }

    void ConstPropagation::clear_Int_map()
    {
        IntMap.clear();
    }
    void ConstPropagation::clear_float_map()
    {
        FloatMap.clear();
    }

    void ConstPropagation::run()
    {
        // 从这里开始吧！
        ConstFolder *folder = new ConstFolder(m_);
        std::vector<Instruction *> delete_instr;
        std::vector<BasicBlock *> visited_bb;
        std::queue<BasicBlock *> q_bb;
        clear_float_map();
        clear_Int_map();
        for (auto Fun : m_->get_functions())
        {
            printf("enter a function\n");
            visited_bb.clear();
            while (!q_bb.empty())
                q_bb.pop();
            if (Fun->get_num_basic_blocks() == 0)
            {
                printf("exit a function\n");
                continue;
            }
            //采用广度优先搜索遍历访问到的块，最后删除未访问到的块，即删除死代码
            q_bb.push(Fun->get_entry_block());
            while (!q_bb.empty())
            {
                delete_instr.clear();
                clear_float_map();
                clear_Int_map();
                auto bb = q_bb.front();
                q_bb.pop();
                visited_bb.push_back(bb);
                for (auto ir : bb->get_instructions())
                {
                    bool is_int_basic_operation = ir->is_add() || ir->is_sub() || ir->is_mul() || ir->is_div();
                    bool is_float_basic_operation = ir->is_fadd() || ir->is_fsub() || ir->is_fmul() || ir->is_fdiv();
                    if (is_int_basic_operation || is_float_basic_operation)
                    {
                        printf("enter basic operation\n");
                        auto int_v0 = cast_constantint(ir->get_operand(0));
                        auto float_v0 = cast_constantfp(ir->get_operand(0));
                        auto int_v1 = cast_constantint(ir->get_operand(1));
                        auto float_v1 = cast_constantfp(ir->get_operand(1));
                        ConstantInt *result_int = nullptr;
                        ConstantFP *result_float = nullptr;
                        if (is_int_basic_operation)
                        {
                            printf("enter int basic operation\n");
                            if (int_v0 == nullptr && float_v0 != nullptr)
                            {
                                int_v0 = ConstantInt::get(int(float_v0->get_value()), m_);
                            }
                            if (int_v1 == nullptr && float_v1 != nullptr)
                            {
                                int_v1 = ConstantInt::get(int(float_v1->get_value()), m_);
                            }
                            if (int_v0 != nullptr && int_v1 != nullptr)
                            {
                                printf("int_v0 and int_v1 are ready\n");
                                printf("int_v0 = %d\n", int_v0->get_value());
                                printf("int_v1=%d\n", int_v1->get_value());
                                result_int = folder->compute(ir->get_instr_type(), int_v0, int_v1);
                                printf("result is %d\n", result_int->get_value());
                                for (auto _use : ir->get_use_list())
                                {
                                    auto _user = dynamic_cast<User *>(_use.val_);
                                    _user->set_operand(_use.arg_no_, result_int);
                                }
                                delete_instr.push_back(ir);
                            }
                            printf("exit int basic operation\n");
                        }
                        else if (is_float_basic_operation)
                        {
                            printf("enter float basic operation\n");
                            if (float_v0 == nullptr && int_v0 != nullptr)
                            {
                                float_v0 = ConstantFP::get(float(int_v0->get_value()), m_);
                            }
                            if (float_v1 == nullptr && int_v1 != nullptr)
                            {
                                float_v1 = ConstantFP::get(float(int_v1->get_value()), m_);
                            }
                            if (float_v0 != nullptr && float_v1 != nullptr)
                            {
                                printf("float_v0 and float_v1 are ready\n");
                                printf("float_v0 = %f\n", float_v0->get_value());
                                printf("float_v1=%f\n", float_v1->get_value());
                                result_float = folder->computeFP(ir->get_instr_type(), float_v0, float_v1);
                                if (result_float == nullptr)
                                {
                                    printf("result is a nullptr\n");
                                }
                                else
                                {
                                    printf("result is not a nullptr\n");
                                }
                                printf("the value of result are ready:%f\n", result_float->get_value());
                                std::cout << "result of value is :" << result_float->get_value() << std::endl;
                                for (auto _use : ir->get_use_list())
                                {
                                    auto _user = dynamic_cast<User *>(_use.val_);
                                    _user->set_operand(_use.arg_no_, result_float);
                                }
                                printf("All use of value are over\n");
                                delete_instr.push_back(ir);
                            }
                            printf("exit float basic operation\n");
                        }
                        printf("exit basic operation\n");
                    }
                    else if (ir->is_fp2si())
                    {
                        printf("enter float to int\n");

                        auto float_value = cast_constantfp(ir->get_operand(0));
                        if (float_value == nullptr)
                        {
                            printf("float_value is nullptr\n");
                            //std::cout<<ir->get_operand(0)->get_type()<<std::endl;
                        }
                        else
                        {
                            printf("float_val is not nullptr\n");
                        }
                        if (float_value != nullptr)
                        {
                            auto int_value = ConstantInt::get(int(float_value->get_value()), m_);
                            for (auto _use : ir->get_use_list())
                            {
                                auto _user = dynamic_cast<User *>(_use.val_);
                                _user->set_operand(_use.arg_no_, int_value);
                            }
                            delete_instr.push_back(ir);
                        }

                        printf("exit float to int\n");
                    }
                    else if (ir->is_si2fp())
                    {
                        printf("enter int to float\n");
                        auto int_value = cast_constantint(ir->get_operand(0));
                        if (int_value == nullptr)
                        {
                            printf("int_value is nullptr\n");
                            //std::cout<<ir->get_operand(0)->get_type()<<std::endl;
                        }
                        else
                        {
                            printf("int_val is not nullptr\n");
                        }
                        if (int_value != nullptr)
                        {
                            auto float_value = ConstantFP::get(float(int_value->get_value()), m_);
                            printf("int_value:%d -> float_value:%f\n", int_value->get_value(), float_value->get_value());
                            for (auto _use : ir->get_use_list())
                            {
                                auto _user = dynamic_cast<User *>(_use.val_);
                                _user->set_operand(_use.arg_no_, float_value);
                            }
                            delete_instr.push_back(ir);
                        }

                        printf("exit int to float\n");
                    }
                    else if (ir->is_fcmp() || ir->is_cmp())
                    {
                        printf("enter a cmp operation\n");
                        auto int_v0 = cast_constantint(ir->get_operand(0));
                        auto float_v0 = cast_constantfp(ir->get_operand(0));
                        auto int_v1 = cast_constantint(ir->get_operand(1));
                        auto float_v1 = cast_constantfp(ir->get_operand(1));
                        ConstantInt *result = nullptr;
                        CmpInst *cmp = dynamic_cast<CmpInst *>(ir);
                        FCmpInst *fcmp = dynamic_cast<FCmpInst *>(ir);
                        if (cmp != nullptr)
                        {
                            if (int_v0 == nullptr && float_v0 != nullptr)
                            {
                                int_v0 = ConstantInt::get(int(float_v0->get_value()), m_);
                            }
                            if (int_v1 == nullptr && float_v1 != nullptr)
                            {
                                int_v1 = ConstantInt::get(int(float_v1->get_value()), m_);
                            }
                            if (int_v0 != nullptr && int_v1 != nullptr)
                            {
                                result = folder->computeCMP(cmp->get_cmp_op(), int_v0, int_v1);
                                for (auto _use : cmp->get_use_list())
                                {
                                    auto _user = dynamic_cast<User *>(_use.val_);
                                    _user->set_operand(_use.arg_no_, result);
                                }
                                delete_instr.push_back(ir);
                            }
                        }
                        else if (fcmp != nullptr)
                        {
                            if (float_v0 == nullptr && int_v0 != nullptr)
                            {
                                float_v0 = ConstantFP::get(float(int_v0->get_value()), m_);
                            }
                            if (float_v1 == nullptr && int_v1 != nullptr)
                            {
                                float_v1 = ConstantFP::get(float(int_v1->get_value()), m_);
                            }
                            if (float_v0 != nullptr && float_v1 != nullptr)
                            {
                                result = folder->computeFCMP(fcmp->get_cmp_op(), float_v0, float_v1);
                                for (auto _use : ir->get_use_list())
                                {
                                    auto _user = dynamic_cast<User *>(_use.val_);
                                    _user->set_operand(_use.arg_no_, result);
                                }
                                delete_instr.push_back(ir);
                            }
                        }

                        printf("exit a cmp operation\n");
                    }
                    else if (ir->is_store())
                    {
                        //store指令没有左值，找不到它的use链表，故需要储存新更改的变量，以及变量值（先判断是否为常数）
                        printf("enter a store instruction\n");
                        StoreInst *_store = dynamic_cast<StoreInst *>(ir);
                        auto _key = _store->get_lval();
                        auto _value = _store->get_rval();
                        auto int_value = cast_constantint(_value);
                        auto float_value = cast_constantfp(_value);
                        if (_key != nullptr && _value != nullptr)
                        {
                            if (_key->get_type()->is_integer_type())
                            {
                                if (int_value == nullptr && float_value != nullptr)
                                {
                                    int_value = ConstantInt::get(int(float_value->get_value()), m_);
                                }
                                if (int_value != nullptr)
                                    push_back_Int(_key, int_value);
                            }
                            else if (_key->get_type()->is_float_type())
                            {
                                if (float_value == nullptr && int_value != nullptr)
                                {
                                    float_value = ConstantFP::get(float(int_value->get_value()), m_);
                                }
                                if (float_value != nullptr)
                                    push_back_Fp(_key, float_value);
                            }
                        }
                        printf("exit a store instruction\n");
                    }
                    else if (ir->is_load())
                    {
                        printf("enter a load instruction\n");
                        LoadInst *_load = dynamic_cast<LoadInst *>(ir);
                        auto _key = _load->get_lval();
                        auto int_v = get_value_to_Int(_key);
                        auto float_v = get_value_to_Float(_key);
                        if (int_v != nullptr)
                        {
                            for (auto _use : _load->get_use_list())
                            {
                                auto _user = dynamic_cast<User *>(_use.val_);
                                _user->set_operand(_use.arg_no_, int_v);
                            }
                            delete_instr.push_back(ir);
                        }
                        else if (float_v != nullptr)
                        {
                            for (auto _use : _load->get_use_list())
                            {
                                auto _user = dynamic_cast<User *>(_use.val_);
                                _user->set_operand(_use.arg_no_, float_v);
                            }
                            delete_instr.push_back(ir);
                        }
                        printf("exit a load instruction\n");
                    }
                    else if (ir->is_zext())
                    {
                        printf("enter a zext instruction\n");
                        auto zext_val = cast_constantint(ir->get_operand(0));
                        if (zext_val != nullptr)
                        {
                            auto int_val = ConstantInt::get(int(zext_val->get_value()), m_);
                            for (auto _use : ir->get_use_list())
                            {
                                auto _user = dynamic_cast<User *>(_use.val_);
                                _user->set_operand(_use.arg_no_, int_val);
                            }
                            delete_instr.push_back(ir);
                        }
                        printf("exit a zext instruction\n");
                    }
                    else if (ir->is_br())
                    {
                        printf("enter a br instruction\n");
                        BranchInst *_br = dynamic_cast<BranchInst *>(ir);
                        if (_br->is_cond_br())
                        {
                            //条件跳转
                            auto cmp = _br->get_operand(0);
                            BasicBlock *trueBB = dynamic_cast<BasicBlock *>(_br->get_operand(1));
                            BasicBlock *falseBB = dynamic_cast<BasicBlock *>(_br->get_operand(2));
                            auto Constint_cmp = cast_constantint(cmp);
                            auto ConstFloat_cmp = cast_constantfp(cmp);
                            bool is_br;
                            bool is_const = Constint_cmp != nullptr || ConstFloat_cmp != nullptr;
                            if (is_const)
                            { //是常数跳转
                                if (Constint_cmp != nullptr)
                                {
                                    is_br = Constint_cmp->get_value() != 0;
                                }
                                else if (ConstFloat_cmp != nullptr)
                                {
                                    is_br = ConstFloat_cmp->get_value() != (float)0;
                                }
                                if (is_br)
                                {
                                    _br->replace_all_use_with(BranchInst::create_br(trueBB, bb));
                                    if (std::find(visited_bb.begin(), visited_bb.end(), trueBB) == visited_bb.end())
                                    {
                                        q_bb.push(trueBB);
                                    }
                                    delete_instr.push_back(ir);
                                }
                                else
                                {
                                    _br->replace_all_use_with(BranchInst::create_br(falseBB, bb));
                                    if (std::find(visited_bb.begin(), visited_bb.end(), falseBB) == visited_bb.end())
                                    {
                                        q_bb.push(falseBB);
                                    }
                                    delete_instr.push_back(ir);
                                }
                            }
                            else
                            { //不是常数跳转,故trueBB和falseBB均有可能访问到
                                if (std::find(visited_bb.begin(), visited_bb.end(), falseBB) == visited_bb.end())
                                    q_bb.push(falseBB);
                                if (std::find(visited_bb.begin(), visited_bb.end(), trueBB) == visited_bb.end())
                                    q_bb.push(trueBB);
                            }
                        }
                        else
                        {
                            //无条件跳转
                            BasicBlock *next_block = dynamic_cast<BasicBlock *>(_br->get_operand(0));
                            if (std::find(visited_bb.begin(), visited_bb.end(), next_block) == visited_bb.end())
                            {
                                q_bb.push(next_block);
                            }
                        }
                        printf("exit a br instruction\n");
                    }
                }
                for (auto ir : delete_instr)
                {
                    bb->delete_instr(ir);
                }
            }
            //删除死代码块
            std::vector<BasicBlock *> delete_bb;
            for (auto bb : Fun->get_basic_blocks())
            {
                if (std::find(visited_bb.begin(), visited_bb.end(), bb) == visited_bb.end())
                {
                    delete_bb.push_back(bb);
                }
            }
            for (auto bb : delete_bb)
            {
                for (auto _use : bb->get_use_list())
                {
                    _use.val_->remove_use(bb);
                    //phi指令可能引用了死代码块，故需要删除phi指令的引用
                    PhiInst *phi = dynamic_cast<PhiInst *>(_use.val_);
                    if (phi != nullptr)
                    {
                        int use_index = 0;
                        for (int i = 0; i < phi->get_operands().size(); i++)
                        {
                            if (phi->get_operand(i) == bb)
                            {
                                use_index = i;//记录死代码块在操作链表中的位置
                                break;
                            }
                        }
                        if (use_index > 0)
                            phi->remove_operands(use_index - 1, use_index);
                    }
                }
                Fun->remove(bb);
            }
            printf("exit a function\n");
        }
    }

    ```
   * 优化前后的IR对比（举一个例子）并辅以简单说明：
   * 优化前：
     ```llvm
        ; ModuleID = 'cminus'
        source_filename = "testcase-1.cminus"

        declare i32 @input()

        declare void @output(i32)

        declare void @outputFloat(float)

        declare void @neg_idx_except()

        define void @main() {
        label_entry:
        br label %label2
        label2:                                                ; preds = %label_entry, %label7
        %op78 = phi i32 [ 0, %label_entry ], [ %op73, %label7 ]
        %op79 = phi i32 [ 0, %label_entry ], [ %op40, %label7 ]
        %op4 = icmp slt i32 %op78, 100000000
        %op5 = zext i1 %op4 to i32
        %op6 = icmp ne i32 %op5, 0
        br i1 %op6, label %label7, label %label74
        label7:                                                ; preds = %label2
        %op8 = add i32 1, 1
        %op9 = add i32 %op8, 1
        %op10 = add i32 %op9, 1
        %op11 = add i32 %op10, 1
        %op12 = add i32 %op11, 1
        %op13 = add i32 %op12, 1
        %op14 = add i32 %op13, 1
        %op15 = add i32 %op14, 1
        %op16 = add i32 %op15, 1
        %op17 = add i32 %op16, 1
        %op18 = add i32 %op17, 1
        %op19 = add i32 %op18, 1
        %op20 = add i32 %op19, 1
        %op21 = add i32 %op20, 1
        %op22 = add i32 %op21, 1
        %op23 = add i32 %op22, 1
        %op24 = add i32 %op23, 1
        %op25 = add i32 %op24, 1
        %op26 = add i32 %op25, 1
        %op27 = add i32 %op26, 1
        %op28 = add i32 %op27, 1
        %op29 = add i32 %op28, 1
        %op30 = add i32 %op29, 1
        %op31 = add i32 %op30, 1
        %op32 = add i32 %op31, 1
        %op33 = add i32 %op32, 1
        %op34 = add i32 %op33, 1
        %op35 = add i32 %op34, 1
        %op36 = add i32 %op35, 1
        %op37 = add i32 %op36, 1
        %op38 = add i32 %op37, 1
        %op39 = add i32 %op38, 1
        %op40 = add i32 %op39, 1
        %op44 = mul i32 %op40, %op40
        %op46 = mul i32 %op44, %op40
        %op48 = mul i32 %op46, %op40
        %op50 = mul i32 %op48, %op40
        %op52 = mul i32 %op50, %op40
        %op54 = mul i32 %op52, %op40
        %op56 = mul i32 %op54, %op40
        %op59 = mul i32 %op40, %op40
        %op61 = mul i32 %op59, %op40
        %op63 = mul i32 %op61, %op40
        %op65 = mul i32 %op63, %op40
        %op67 = mul i32 %op65, %op40
        %op69 = mul i32 %op67, %op40
        %op71 = mul i32 %op69, %op40
        %op72 = sdiv i32 %op56, %op71
        %op73 = add i32 %op78, %op72
        br label %label2
        label74:                                                ; preds = %label2
        %op77 = mul i32 %op79, %op79
        call void @output(i32 %op77)
        ret void
        }
     ```
  * 优化后：
    ```llvm
    ; ModuleID = 'cminus'
    source_filename = "testcase-1.cminus"

    declare i32 @input()

    declare void @output(i32)

    declare void @outputFloat(float)

    declare void @neg_idx_except()

    define void @main() {
    label_entry:
    br label %label2
    label2:                                                ; preds = %label_entry, %label7
    %op78 = phi i32 [ 0, %label_entry ], [ %op73, %label7 ]
    %op79 = phi i32 [ 0, %label_entry ], [ 34, %label7 ]
    %op4 = icmp slt i32 %op78, 100000000
    %op5 = zext i1 %op4 to i32
    %op6 = icmp ne i32 %op5, 0
    br i1 %op6, label %label7, label %label74
    label7:                                                ; preds = %label2
    %op73 = add i32 %op78, 1
    br label %label2
    label74:                                                ; preds = %label2
    %op77 = mul i32 %op79, %op79
    call void @output(i32 %op77)
    ret void
    }
    ```
    可见优化后，中间代码大大简化，

* 循环不变式外提
    * 实现思路：
      * 调用`loop_search`中的`get_loops_in_func`即可得到一个函数的所有循环
      * 对于每个循环，使用while循环遍历循环中基本块，直到没有循环不变式
      * 循环中基本块的过程中，储存每一条ir，即左值，即循环内部变量
      * 再次遍历循环中的基本块，遍历每一条指令的操作数序列，判断是否全部都不是内部变量，若指令的全部操作数都不是内部变量，则该指令就是循环不变式，将该循环不变式和其所在的基本块储存起来，等待外移。并将该指令从左值集合中删去。
      * 循环不变式外移：
        * 对于一个循环不变式和其所在的基本块，通过`get_inner_loop`得到此循环集合，再通过`get_loop_base`得到循环的入口，由于入口至多只有两个前置块，其中有一个前置块属于循环内部，另一个属于循环内部。
        * 在`loop_search`建立CFG时，是从上向下遍历基本块，并更新基本块的后置块的前置基本块指针和前置块的后继基本块指针。故循环入口的前置基本块指针的第一个就是循环外部基本块。
        * 将指令从原来的基本块中删除，并插入到前置块的最后一条指令(br)的前面，可通过先删除br，然后插入指令，再插入br；
    * 相应代码：
     ```C++
    void LoopInvHoist::run()
    {
        // 先通过LoopSearch获取循环的相关信息
        LoopSearch loop_searcher(m_, false);
        loop_searcher.run();
        std::vector<Value *> left_var;
        std::vector<std::pair<BasicBlock *, Instruction *>> move_outside;
        bool is_move_outside = true;
        for (auto func : m_->get_functions())
        {
            is_move_outside = true;
            auto loop_set = loop_searcher.get_loops_in_func(func);
            if (loop_set.size() == 0)
                continue;
            left_var.clear();
            long i = 0;
            while (is_move_outside)
            {
                i++;
                if (i > 100000000)
                    break;
                is_move_outside = false;
                for (auto block_set : loop_set)
                {
                    left_var.clear();
                    move_outside.clear();
                    //第一次循环，储存左值，即储存内部变量
                    for (auto bb : *block_set)
                    {
                        for (auto ir : bb->get_instructions())
                        {
                            left_var.push_back(ir);
                        }
                    }
                    //第二次循环，判断右值是否属于左值集合，即判断右值是否为内部变量
                    //当一个指令的所有右值均不是内部变量，则该指令为循环不变式
                    for (auto bb : *block_set)
                    {
                        for (auto ir : bb->get_instructions())
                        {
                            bool is_int_basic_operation = ir->is_add() || ir->is_sub() || ir->is_mul() || ir->is_div();
                            bool is_float_basic_operation = ir->is_fadd() || ir->is_fsub() || ir->is_fmul() || ir->is_fdiv();
                            bool is_have_right_val = is_int_basic_operation || is_float_basic_operation || ir->is_cmp() || ir->is_fcmp() || ir->is_fp2si() || ir->is_si2fp() || ir->is_zext();
                            if (!is_have_right_val)
                                continue;
                            if (ir->get_operands().size() > 0)
                            {
                                is_move_outside = true;
                                for (auto right_val : ir->get_operands())
                                {
                                    if (std::find(left_var.begin(), left_var.end(), right_val) != left_var.end())
                                    {
                                        is_move_outside = false;

                                        break;
                                    }
                                }
                                if (is_move_outside)
                                {
                                    std::cout << "will move outside an instruction\n name is :\n";
                                    std::cout << "size is " << ir->get_operands().size() << std::endl;
                                    for (auto right_val : ir->get_operands())
                                    {
                                        std::cout << right_val->get_name() << std::endl;
                                    }
                                    //储存要外移的bb和ir
                                    move_outside.push_back({bb, ir});
                                    //当一条指令是循环不变式时，左值也为外部变量，故需要把该值删去
                                    for (auto iter = left_var.begin(); iter != left_var.end(); iter++)
                                    {
                                        if (*iter == ir)
                                        {
                                            left_var.erase(iter);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    //循环不变式外移
                    BasicBlock *bb;
                    Instruction *ir;
                    for (auto pair : move_outside)
                    {
                        std::cout << "enter move outside an instruction\n";
                        bb = pair.first;
                        ir = pair.second;
                        BBset_t *loop = loop_searcher.get_inner_loop(bb);
                        BasicBlock *loop_entry = loop_searcher.get_loop_base(loop);
                        BasicBlock *pre_bb = nullptr;
                        //根据建立CFG的顺序，是从上往下遍历基本块，故循环入口的前置外部块就在pre链表的起始位置
                        pre_bb = *loop_entry->get_pre_basic_blocks().begin();
                        if (loop->find(pre_bb) == loop->end())
                        {
                            std::cout << "It is ok\n";
                        }
                        if (pre_bb != nullptr)
                        {
                            //插入到br指令之前，故需要先删除br指令，插入新指令后再添加原来的br
                            Instruction *br_ir = pre_bb->get_terminator();
                            if (br_ir != nullptr)
                            {
                                pre_bb->get_instructions().remove(br_ir);
                                ir->set_parent(pre_bb);
                                pre_bb->add_instruction(ir);
                                pre_bb->add_instruction(br_ir);
                                bb->get_instructions().remove(ir);
                            }
                            else
                            {
                                ir->set_parent(pre_bb);
                                pre_bb->add_instruction(ir);
                                bb->get_instructions().remove(ir);
                            }
                            std::cout << "move outside a ir\n";
                        }
                    }
                }
            }
            std::cout << "total loop num is :" << i << std::endl;
        }

        
    }

     ``` 
    * 优化前后的IR对比（举一个例子）并辅以简单说明：
     优化前：
     ```llvm
    ; ModuleID = 'cminus'
    source_filename = "testcase-1.cminus"

    declare i32 @input()

    declare void @output(i32)

    declare void @outputFloat(float)

    declare void @neg_idx_except()

    define void @main() {
    label_entry:
    br label %label3
    label3:                                                ; preds = %label_entry, %label58
    %op61 = phi i32 [ %op64, %label58 ], [ undef, %label_entry ]
    %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58 ]
    %op63 = phi i32 [ %op65, %label58 ], [ undef, %label_entry ]
    %op5 = icmp slt i32 %op62, 10000
    %op6 = zext i1 %op5 to i32
    %op7 = icmp ne i32 %op6, 0
    br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
    br label %label11
    label9:                                                ; preds = %label3
    call void @output(i32 %op61)
    ret void
    label11:                                                ; preds = %label8, %label16
    %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
    %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
    %op13 = icmp slt i32 %op65, 10000
    %op14 = zext i1 %op13 to i32
    %op15 = icmp ne i32 %op14, 0
    br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
    %op19 = mul i32 %op62, %op62
    %op21 = mul i32 %op19, %op62
    %op23 = mul i32 %op21, %op62
    %op25 = mul i32 %op23, %op62
    %op27 = mul i32 %op25, %op62
    %op29 = mul i32 %op27, %op62
    %op31 = mul i32 %op29, %op62
    %op33 = mul i32 %op31, %op62
    %op35 = mul i32 %op33, %op62
    %op37 = sdiv i32 %op35, %op62
    %op39 = sdiv i32 %op37, %op62
    %op41 = sdiv i32 %op39, %op62
    %op43 = sdiv i32 %op41, %op62
    %op45 = sdiv i32 %op43, %op62
    %op47 = sdiv i32 %op45, %op62
    %op49 = sdiv i32 %op47, %op62
    %op51 = sdiv i32 %op49, %op62
    %op53 = sdiv i32 %op51, %op62
    %op55 = sdiv i32 %op53, %op62
    %op57 = add i32 %op65, 1
    br label %label11
    label58:                                                ; preds = %label11
    %op60 = add i32 %op62, 1
    br label %label3
    }

     ``` 
     优化后：
     ```llvm
     ; ModuleID = 'cminus'
    source_filename = "testcase-1.cminus"

    declare i32 @input()

    declare void @output(i32)

    declare void @outputFloat(float)

    declare void @neg_idx_except()

    define void @main() {
    label_entry:
    br label %label3
    label3:                                                ; preds = %label_entry, %label58
    %op61 = phi i32 [ %op64, %label58 ], [ undef, %label_entry ]
    %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58 ]
    %op63 = phi i32 [ %op65, %label58 ], [ undef, %label_entry ]
    %op5 = icmp slt i32 %op62, 10000
    %op6 = zext i1 %op5 to i32
    %op7 = icmp ne i32 %op6, 0
    br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
    %op19 = mul i32 %op62, %op62
    %op21 = mul i32 %op19, %op62
    %op23 = mul i32 %op21, %op62
    %op25 = mul i32 %op23, %op62
    %op27 = mul i32 %op25, %op62
    %op29 = mul i32 %op27, %op62
    %op31 = mul i32 %op29, %op62
    %op33 = mul i32 %op31, %op62
    %op35 = mul i32 %op33, %op62
    %op37 = sdiv i32 %op35, %op62
    %op39 = sdiv i32 %op37, %op62
    %op41 = sdiv i32 %op39, %op62
    %op43 = sdiv i32 %op41, %op62
    %op45 = sdiv i32 %op43, %op62
    %op47 = sdiv i32 %op45, %op62
    %op49 = sdiv i32 %op47, %op62
    %op51 = sdiv i32 %op49, %op62
    %op53 = sdiv i32 %op51, %op62
    %op55 = sdiv i32 %op53, %op62
    br label %label11
    label9:                                                ; preds = %label3
    call void @output(i32 %op61)
    ret void
    label11:                                                ; preds = %label8, %label16
    %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
    %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
    %op13 = icmp slt i32 %op65, 10000
    %op14 = zext i1 %op13 to i32
    %op15 = icmp ne i32 %op14, 0
    br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
    %op57 = add i32 %op65, 1
    br label %label11
    label58:                                                ; preds = %label11
    %op60 = add i32 %op62, 1
    br label %label3
    }

     ```
    课件优化后label18中的语句增多，而这些语句原来在更内一层的循环中，故减轻了编译器的负担
    
* 活跃变量分析
    实现思路：
    * 遍历函数中的每个基本块，遍历基本块中的每条指令，得到每个基本块的useBB与defBB集合。
    * 对于每条指令，先更新useBB，再更新defBB。更新useBB时需要该变量不在defBB中。由于可能会有`a=a+b`这种指令，此时a既属于useBB，也属于defBB。故先更新useBB，再更新defBB代码会更简单一些。
    * 对不同指令的处理： 
      * 基本运算指令与基本比较指令。左值为defBB，右值为useBB。
      * store指令。第0个操作数为useBB，第1个操作数为defBB
      * 强制类型转换指令。左值为defBB，右值为useBB。
      * 跳转指令(br)。没有左值，即没有defBB。第0个操作数为useBB。
      * call指令。若调用的函数不是void类型，则左值为defBB。右值从第1个操作数开始到结束均为useBB。第0个操作数为调用的函数，不属于活跃变量范围。
      * ret指令。`void_ret`没有右值，其余类型useBB为第0个操作数。没有defBB
      * load指令。左值为defBB，右值为useBB。
      * phi指令。操作数链中依次是value和bb，故每隔一个操作数是useBB。左值为defBB
      * gep指令。左值为defBB，右值为useBB。
      * alloca指令。左值为defBB，无useBB。
    * 找到每个块的use与def集合后，即可调用书中算法求每个块的in和out集合。
    * 由于之前没有考虑`phi_use`，故求in和out集合之前，要先处理一下phi指令。
    * 在while循环中遍历每个基本块的后继块，删除后继块中phi指令中来自其他块的活跃变量（删除`union_suc_bb`集合中的相关变量），并把删除的活跃变量储存起来。由于可能删错，比如%2 = phi i32 [ %1, %label_entry ], [ %1, %label14 ]，此时在中会把%1从活跃变量中删去，但%1有可能从bb传下去，故此时要进行修正
    * 再次遍历bb的后继块中每个phi，将从bb传下去的变量再加入`union_suc_bb`集合。
    * 调用算法更新每个基本块的in和out集合，并将in与原来的live_in比较是否发生了改变，改变了就更新live_in。若有一个in发生改变，则会再需要一次迭代。别忘了live_out也可能要更新。 
    相应的代码：
    ```C++
    void ActiveVars::run()
    {
        std::ofstream output_active_vars;
        output_active_vars.open("active_vars.json", std::ios::out);
        output_active_vars << "[";
        for (auto &func : this->m_->get_functions())
        {
            if (func->get_basic_blocks().empty())
            {
                continue;
            }
            else
            {
                func_ = func;

                func_->set_instr_name();
                live_in.clear();
                live_out.clear();

                // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内
                std::map<BasicBlock *, std::set<Value *>> use_bb, def_bb;
                for (auto bb : func_->get_basic_blocks())
                {
                    printf("enter a bb\n");
                    for (auto ir : bb->get_instructions())
                    {
                        bool is_int_basic_operation = ir->is_add() || ir->is_sub() || ir->is_mul() || ir->is_div();
                        bool is_float_basic_operation = ir->is_fadd() || ir->is_fsub() || ir->is_fmul() || ir->is_fdiv();
                        if (is_int_basic_operation || is_float_basic_operation || ir->is_cmp() || ir->is_fcmp())
                        {
                            printf("enter basic operation ir\n");

                            for (auto val : ir->get_operands())
                            {
                                if (dynamic_cast<ConstantInt *>(val) == nullptr && dynamic_cast<ConstantFP *>(val) == nullptr)
                                {
                                    if (def_bb[bb].find(val) == def_bb[bb].end() && use_bb[bb].find(val) == use_bb[bb].end())
                                        use_bb[bb].insert(val);
                                }
                            }
                            if (def_bb[bb].find(ir) == def_bb[bb].end())
                                def_bb[bb].insert(ir);
                        }
                        else if (ir->is_store())
                        {
                            printf("enter store ir\n");
                            auto val_0 = ir->get_operand(0);
                            auto val_1 = ir->get_operand(1);
                            if (dynamic_cast<ConstantFP *>(val_0) == nullptr && dynamic_cast<ConstantInt *>(val_0) == nullptr)
                            {
                                if (def_bb[bb].find(val_0) == def_bb[bb].end() && use_bb[bb].find(val_0) == use_bb[bb].end())
                                    use_bb[bb].insert(val_0);
                            }
                            if (def_bb[bb].find(val_1) == def_bb[bb].end())
                                def_bb[bb].insert(val_1);
                        }
                        else if (ir->is_fp2si() || ir->is_fp2si() || ir->is_zext())
                        {
                            //类型转换均只有一个操作数，左值为转换后的内容
                            printf("enter type conversion ir\n");
                            auto right_val = ir->get_operand(0);
                            if (dynamic_cast<ConstantFP *>(right_val) == nullptr && dynamic_cast<ConstantInt *>(right_val) == nullptr)
                            {
                                if (def_bb[bb].find(right_val) == def_bb[bb].end() && use_bb[bb].find(right_val) == use_bb[bb].end())
                                    use_bb[bb].insert(right_val);
                            }
                            if (def_bb[bb].find(ir) == def_bb[bb].end())
                                def_bb[bb].insert(ir);
                        }
                        else if (ir->is_br())
                        {
                            printf("enter br ir\n");
                            auto _br = dynamic_cast<BranchInst *>(ir);
                            if (_br->is_cond_br())
                            {
                                auto cmp = _br->get_operand(0);
                                if (dynamic_cast<ConstantFP *>(cmp) == nullptr && dynamic_cast<ConstantInt *>(cmp) == nullptr)
                                {
                                    if (def_bb[bb].find(cmp) == def_bb[bb].end() && use_bb[bb].find(cmp) == use_bb[bb].end())
                                        use_bb[bb].insert(cmp);
                                }
                            }
                        }
                        else if (ir->is_call())
                        {
                            printf("enter call ir\n");

                            for (int i = 1; i < ir->get_operands().size(); i++)
                            {
                                //call 的第0个操作数为fun，故从1开始遍历
                                auto temp_val = ir->get_operand(i);
                                if (dynamic_cast<ConstantFP *>(temp_val) == nullptr && dynamic_cast<ConstantInt *>(temp_val) == nullptr)
                                {
                                    if (def_bb[bb].find(temp_val) == def_bb[bb].end() && use_bb[bb].find(temp_val) == use_bb[bb].end())
                                        use_bb[bb].insert(temp_val);
                                }
                            }
                            if (!ir->is_void())
                            {
                                if (def_bb[bb].find(ir) == def_bb[bb].end())
                                    def_bb[bb].insert(ir);
                            }
                        }
                        else if (ir->is_ret())
                        {
                            printf("enter ret ir\n");
                            if (ir->get_operands().size() == 0)
                            {
                                printf("exit ret ir\n");
                                continue;
                            }
                            auto ret_value = ir->get_operand(0);
                            if (dynamic_cast<ConstantFP *>(ret_value) == nullptr && dynamic_cast<ConstantInt *>(ret_value) == nullptr)
                            {
                                if (def_bb[bb].find(ret_value) == def_bb[bb].end() && use_bb[bb].find(ret_value) == use_bb[bb].end())
                                    use_bb[bb].insert(ret_value);
                            }
                            printf("exit ret ir\n");
                        }
                        else if (ir->is_load())
                        {
                            printf("enter load ir\n");

                            auto loaded_value = ir->get_operand(0);
                            if (dynamic_cast<ConstantFP *>(loaded_value) == nullptr && dynamic_cast<ConstantInt *>(loaded_value) == nullptr)
                            {
                                if (def_bb[bb].find(loaded_value) == def_bb[bb].end() && use_bb[bb].find(loaded_value) == use_bb[bb].end())
                                    use_bb[bb].insert(loaded_value);
                            }
                            if (def_bb[bb].find(ir) == def_bb[bb].end())
                                def_bb[bb].insert(ir);
                        }
                        else if (ir->is_phi())
                        {
                            printf("enter phi ir\n");

                            for (int i = 0; i < ir->get_operands().size(); i = i + 2)
                            {
                                auto phi_val = ir->get_operand(i);
                                if (dynamic_cast<ConstantFP *>(phi_val) == nullptr && dynamic_cast<ConstantInt *>(phi_val) == nullptr)
                                {
                                    if (def_bb[bb].find(phi_val) == def_bb[bb].end() && use_bb[bb].find(phi_val) == use_bb[bb].end())
                                        use_bb[bb].insert(phi_val);
                                }
                            }
                            if (def_bb[bb].find(ir) == def_bb[bb].end())
                                def_bb[bb].insert(ir);
                        }
                        else if(ir->is_gep())
                        {
                            printf("enter gep ir\n");
                            for (auto val : ir->get_operands())
                            {
                                if (dynamic_cast<ConstantInt *>(val) == nullptr && dynamic_cast<ConstantFP *>(val) == nullptr)
                                {
                                    if (def_bb[bb].find(val) == def_bb[bb].end() && use_bb[bb].find(val) == use_bb[bb].end())
                                        use_bb[bb].insert(val);
                                }
                            }
                            if (def_bb[bb].find(ir) == def_bb[bb].end())
                                def_bb[bb].insert(ir);
                            printf("exit gep ir\n");
                        }
                        else if(ir->is_alloca())
                        {
                            printf("enter alloca ir\n");
                            if (def_bb[bb].find(ir) == def_bb[bb].end())
                                def_bb[bb].insert(ir);
                            printf("exit alloca ir\n");
                        }
                    }
                    
                    printf("exit a bb\n");
                }
                bool is_in_change = true;
                while (is_in_change)
                {
                    printf("enter a while loop\n");
                    is_in_change = false;
                    for (auto bb : func_->get_basic_blocks())
                    {
                        printf("enter a bb\n");
                        std::set<Value *> in, out, union_suc_in;
                        in.clear();
                        out.clear();
                        union_suc_in.clear();
                        for (auto suc_bb : bb->get_succ_basic_blocks())
                        {
                            printf("enter a suc_bb\n");
                            for (auto _in : live_in[suc_bb])
                                union_suc_in.insert(_in);
                            
                            //第一次遍历，删除phi指令中来自其他块的活跃变量
                            for (auto ir : suc_bb->get_instructions())
                            {

                                if (ir->is_phi())
                                {

                                    for (int i = 0; i < ir->get_operands().size(); i = i + 2)
                                    {
                                        auto _val = ir->get_operand(i);
                                        auto _bb = ir->get_operand(i + 1);
                                        if (_bb != bb && union_suc_in.find(_val) != union_suc_in.end())
                                        {
                                            union_suc_in.erase(_val);
                                            
                                        }
                                    }
                                }
                            }
                            //第二次遍历，防止删错phi指令中的活跃变量
                            //比如%2 = phi i32 [ %1, %label_entry ], [ %1, %label14 ]
                            //此时在前一次遍历中会把%1从活跃变量中删去，但%1有可能从bb传下去，故此时要进行修正
                            for (auto ir : suc_bb->get_instructions())
                            {
                                
                                if (ir->is_phi())
                                {
                                    for (int i = 0; i < ir->get_operands().size(); i = i + 2)
                                    {
                                        auto _val = ir->get_operand(i);
                                        auto _bb = ir->get_operand(i + 1);
                                        if (_bb == bb  && union_suc_in.find(_val) == union_suc_in.end())
                                        {
                                            if(dynamic_cast<ConstantInt *>(_val) == nullptr && dynamic_cast<ConstantFP *>(_val) == nullptr)
                                        {
                                            union_suc_in.insert(_val);
                                            
                                        }
                                        }
                                    }
                                }
                                
                            }
                            //此时已得到正确的union_suc_in
                            //OUT[B]=union_suc_in
                            //IN[B]=useB U (OUT[B]-defB)
                            for (auto val : union_suc_in)
                            {
                                if (out.find(val) == out.end())
                                    out.insert(val);
                                if (in.find(val) == in.end())
                                    in.insert(val);
                            }

                            printf("exit a suc_bb\n");
                        }
                        
                        for (auto _def : def_bb[bb])
                        {
                            if (in.find(_def) != in.end())
                                in.erase(_def);
                        }
                        for (auto _use : use_bb[bb])
                        {
                            if (in.find(_use) == in.end())
                                in.insert(_use);
                        }
                        //比较新的in与live_in[bb]是否不同
                        //in集合不会出现用新的元素去替换原来的元素的情况，故只需要比较size
                        if (in.size() != live_in[bb].size())
                        {
                            is_in_change = true;
                        }
                        for (auto _in : in)
                        {
                            if (live_in[bb].find(_in) == live_in[bb].end())
                                live_in[bb].insert(_in);
                        }
                        for (auto _out : out)
                        {
                            if (live_out[bb].find(_out) == live_out[bb].end())
                                live_out[bb].insert(_out);
                        }
                        printf("exit a bb\n");
                    }
                    printf("finish a while loop\n");
                }

                output_active_vars << print();
                output_active_vars << ",";
            }
        }
        output_active_vars << "]";
        output_active_vars.close();
        return;
    }
    ```

### 实验总结

* 理解了为什么指令本身的值就是左值
* 对常量传播、循环不变式外移以及活跃变量在phi指令的处理有了更好的理解
* debug能力提高
* 对于强制转换`dynamic_cast`有了更好的理解
* 掌握了更多新的容器，如set、map
* 对于不知道将循环不变式移动到哪个外层循环时，可采取while循环，每次只移动一层，直到不能再移动。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
