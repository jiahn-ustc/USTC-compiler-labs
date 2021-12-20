#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

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
                for (auto pair : move_outside)
                {
                    std::cout << "enter move outside an instruction\n";
                    BasicBlock *bb = pair.first;
                    Instruction *ir = pair.second;
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

    // 接下来由你来补充啦！
}
