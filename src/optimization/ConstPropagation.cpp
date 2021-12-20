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
                //删除可以删除的ir
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
