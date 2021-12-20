#include "ActiveVars.hpp"

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
                                    if (_bb == bb &&  union_suc_in.find(_val) == union_suc_in.end())
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

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars += "{\n";
    active_vars += "\"function\": \"";
    active_vars += func_->get_name();
    active_vars += "\",\n";

    active_vars += "\"live_in\": {\n";
    for (auto &p : live_in)
    {
        if (p.second.size() == 0)
        {
            continue;
        }
        else
        {
            active_vars += "  \"";
            active_vars += p.first->get_name();
            active_vars += "\": [";
            for (auto &v : p.second)
            {
                active_vars += "\"%";
                active_vars += v->get_name();
                active_vars += "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    },\n";

    active_vars += "\"live_out\": {\n";
    for (auto &p : live_out)
    {
        if (p.second.size() == 0)
        {
            continue;
        }
        else
        {
            active_vars += "  \"";
            active_vars += p.first->get_name();
            active_vars += "\": [";
            for (auto &v : p.second)
            {
                active_vars += "\"%";
                active_vars += v->get_name();
                active_vars += "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}