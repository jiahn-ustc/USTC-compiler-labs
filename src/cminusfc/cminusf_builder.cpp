#include "cminusf_builder.hpp"

// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_INT(num) \
    ConstantInt::get(num, module.get())


// You can define global variables here
// to store state

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) { 
    //visit every declaration in turn
    for(int i=0;i<node.declarations.size();i++){
        node.declarations[i]->accept(*this);
    }
}

Value *temp_Var;//store the address of the assignment variable 
Value *temp_expression;//store the load of expression,same as copy
int Block_number=1;
void CminusfBuilder::visit(ASTNum &node) { 
    if(node.type == TYPE_INT){//INTEGER
        temp_expression = CONST_INT(node.i_val);
        printf("value = %d\n",node.i_val);
    }
    else if(node.type == TYPE_FLOAT){//FLOAT
        temp_expression = CONST_FP(node.f_val);
        printf("value = %f\n",node.f_val);
    }

}

void CminusfBuilder::visit(ASTVarDeclaration &node) { 
    Type *type_declaration;
    if(node.type==TYPE_INT){
        type_declaration = Type::get_int32_type(module.get());
    }
    else if(node.type==TYPE_FLOAT){
        type_declaration = Type::get_float_type(module.get());
    }
    auto initializer = ConstantZero::get(type_declaration,module.get());
    if(scope.in_global()){//global var or array
        if(node.num==nullptr){//global var
            
            auto global_var=GlobalVariable::create(node.id,module.get(),type_declaration,false,initializer);
            scope.push(node.id,global_var);
        }
        else{//global array
            auto *arrayType = ArrayType::get(type_declaration,node.num->i_val);
            auto global_array = GlobalVariable::create(node.id,module.get(),arrayType,false,initializer);
            scope.push(node.id,global_array);
        }
    }
    else{//local var or array
        if(node.num==nullptr){//local var
            auto local_var = builder->create_alloca(type_declaration);
            scope.push(node.id,local_var);
        }
        else{//local array
            auto *arrayType = ArrayType::get(type_declaration,node.num->i_val);
            auto local_array = builder->create_alloca(arrayType);
            scope.push(node.id,local_array);
        }
    }
}

std::vector <Type *> type_params;
void CminusfBuilder::visit(ASTFunDeclaration &node) {
    Type *type_fun;
    //?????????start??????void????????????????????????builder->create_void_ret()??????
    int start=0;
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
    //????????????type_params???????????????????????????????????????????????????????????????
    while (!type_params.empty())
    {
        type_params.pop_back();
    }
    printf("function params size is %d\n",node.params.size());
    //??????????????????
    for(int i=0;i<node.params.size();i++){
        node.params[i]->accept(*this);
    }
    printf("type_params size is %d\n",type_params.size());
    auto FunTy = FunctionType::get(type_fun,type_params);
    auto Fun = Function::create(FunTy,node.id,module.get());
    //????????????id??????????????????
    scope.push(node.id,Fun);

    //?????????????????????
    scope.enter();
    auto bb = BasicBlock::create(module.get(),node.id+"_entry"+std::to_string(Block_number++),Fun);
    printf("this is a function,args num is %d\n",Fun->get_num_of_args());
    builder->set_insert_point(bb);
    std::vector<Value *> args;

    //???????????????????????????
    for(auto arg = Fun->arg_begin();arg!=Fun->arg_end();arg++){
        args.push_back(*arg);
    }
    printf("have push_back args\n");
    for(int i=0;i<node.params.size();i++){
        //?????????????????????????????????????????????????????????????????????
        //????????????int float???????????????????????????????????????
        //?????????????????????????????????point??????????????????point???????????????????????????
        //param????????????arg?????????
        auto param = node.params[i];
        auto arg = args[i];
        auto Int32Type = Type::get_int32_type(module.get());
        auto FloatType = Type::get_float_type(module.get());
        auto Int32PtrType = Type::get_int32_ptr_type(module.get());
        auto FloatPtrType = Type::get_float_ptr_type(module.get());
        if(param->isarray){
            //????????????????????????????????????point???????????????
            //?????????????????????int??????float?????????????????????
            //??????????????????????????????????????????
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
        }
        else{
            //???????????????int ???float????????????????????????int ???float??????
            //???????????????????????????
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
        }
    }
    //???????????????
    node.compound_stmt->accept(*this);
    //void????????????????????????????????????ret??????
    if(start == 1)
        builder->create_void_ret();
    //?????????????????????
    scope.exit();
    printf("Function is over!\n");
 }

void CminusfBuilder::visit(ASTParam &node) {
    Type *type_param;
    //?????????????????????????????????????????????????????????????????????????????????int or float
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
    
 }

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    scope.enter();
    //???????????????local_declaration???statement_expression????????????
    //???????????????????????????????????????
    for(int i=0; i<node.local_declarations.size();i++){
        node.local_declarations[i]->accept(*this);
    }
    for(int i=0; i<node.statement_list.size();i++){
        node.statement_list[i]->accept(*this);
    }
    scope.exit();
 }

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    if(node.expression!= nullptr)
        node.expression->accept(*this);
 }

void CminusfBuilder::visit(ASTSelectionStmt &node) { 
    if(node.else_statement!=nullptr){
        node.expression->accept(*this);
        auto if_expression = temp_expression;
        auto Int32Type = Type::get_int32_type(module.get());
        if(temp_expression->get_type()->is_float_type()){
            //??????????????????0??????????????????
            //???????????????????????????????????????float????????????????????????????????????
            if_expression = builder->create_fptosi(temp_expression,Int32Type);
        }
        auto icmp = builder->create_icmp_eq(if_expression,CONST_INT(0));
        auto ifBB = BasicBlock::create(module.get(), "ifBB"+std::to_string(Block_number),builder->get_insert_block()->get_parent());
        auto elseBB = BasicBlock::create(module.get(), "elseBB"+std::to_string(Block_number),builder->get_insert_block()->get_parent());
        //next_block???????????????if_else??????????????????
        BasicBlock *next_block;
        
        Block_number++;
        //?????????????????????????????????????????????????????????else??????????????????????????????????????????0??????
        builder->create_cond_br(icmp,elseBB,ifBB);

        builder->set_insert_point(ifBB);
        node.if_statement->accept(*this);
        //is_if_br??????if??????????????????return???goto???????????????
        //?????????????????????is_if_br???????????????????????????if?????????????????????next_block???
        auto is_if_br = builder->get_insert_block()->get_terminator();
        //?????????start??????????????????????????????next_block???????????????????????????
        int start = 0;
        if(is_if_br == nullptr){
            next_block = BasicBlock::create(module.get(), std::to_string(Block_number++),builder->get_insert_block()->get_parent());
            builder->create_br(next_block);
            start = 1;
            printf("--------------if------------\n");
        }
            

        builder->set_insert_point(elseBB);
        node.else_statement->accept(*this);
        //is_else_br???is_if_br????????????
        auto is_else_br = builder->get_insert_block()->get_terminator();
        if(is_else_br == nullptr){
            if(start==0){
                next_block = BasicBlock::create(module.get(), std::to_string(Block_number++),builder->get_insert_block()->get_parent());
                builder->create_br(next_block);
                start = 1;
            }
            else{
                builder->create_br(next_block);
                printf("--------------else------------\n");
            }   
        }
            
        //??????if_else??????????????????????????????????????????????????????????????????next_block?????????next_block?????????builder???
        if(is_if_br==nullptr || is_else_br== nullptr){
            builder->set_insert_point(next_block);
            printf("----something is nullptr\n");
        }
            
    }
    else{
        //??????????????????????????????elseBB???
        node.expression->accept(*this);
        auto if_expression = temp_expression;
        auto Int32Type = Type::get_int32_type(module.get());
        if(temp_expression->get_type()->is_float_type()){
            if_expression = builder->create_fptosi(temp_expression,Int32Type);
        }
        auto icmp = builder->create_icmp_eq(if_expression,CONST_INT(0));
        auto ifBB = BasicBlock::create(module.get(), "ifBB"+std::to_string(Block_number++),builder->get_insert_block()->get_parent());
        auto next_block = BasicBlock::create(module.get(), std::to_string(Block_number++),builder->get_insert_block()->get_parent());
        builder->create_cond_br(icmp,next_block,ifBB);
        builder->set_insert_point(ifBB);
        node.if_statement->accept(*this);
        auto is_if_br = builder->get_insert_block()->get_terminator();
        if(is_if_br ==nullptr){
            builder->create_br(next_block);
        }
        builder->set_insert_point(next_block);
    }
    
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    auto Fun = builder->get_insert_block()->get_parent();
    //while?????????????????????whilecond??????????????????whilebody???????????????whileend???????????????
    auto whilecond = BasicBlock::create(module.get(),"whilecond"+std::to_string(Block_number),Fun);
    auto whilebody = BasicBlock::create(module.get(),"whilebody"+std::to_string(Block_number),Fun);
    auto whileend = BasicBlock::create(module.get(),"whileend"+std::to_string(Block_number++),Fun);
    //???????????????????????????
    builder->create_br(whilecond);
    builder->set_insert_point(whilecond);
    node.expression->accept(*this);
    auto cond_expression = temp_expression;
    auto Int32Type = Type::get_int32_type(module.get());

    //?????????????????????float???????????????int???
    if(temp_expression->get_type()->is_float_type()){
            cond_expression = builder->create_fptosi(temp_expression,Int32Type);
    }
    auto icmp = builder->create_icmp_eq(cond_expression,CONST_INT(0));
    builder->create_cond_br(icmp,whileend,whilebody);
    //?????????
    builder->set_insert_point(whilebody);
    node.statement->accept(*this);
    //is_whilebody_b??????whilebody???????????????return???goto???????????????
    auto is_whilebody_br = builder->get_insert_block()->get_terminator();
    if(is_whilebody_br== nullptr){
        builder->create_br(whilecond);
    }
    //???????????????
    builder->set_insert_point(whileend);
    //??????????????????????????????????????????????????????next_block???
    auto next_block = BasicBlock::create(module.get(),std::to_string(Block_number++),Fun);
    builder->create_br(next_block);
    builder->set_insert_point(next_block);
}
            
            
            
            
void CminusfBuilder::visit(ASTReturnStmt &node) { 
    printf("a return statement\n");
    if(node.expression==nullptr){
        //??????return ; ,?????????????????????
        printf("return-stmt -> return;\n");
        builder->create_void_ret();
    }
    else {
        printf("return-stmt -> return expression;\n");
        node.expression->accept(*this);
        auto right_expression = temp_expression;
        auto Fun = builder->get_insert_block()->get_parent();
        auto FunReturnType = Fun->get_return_type();
        auto Int32Type = Type::get_int32_type(module.get());
        auto FloatType = Type::get_float_type(module.get());
        if(FunReturnType!=right_expression->get_type()){
            //??????????????????
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
    }
    printf("return is over\n");
}

void CminusfBuilder::visit(ASTVar &node) {
    printf("a Var\n");
    if(node.expression==nullptr){//variable
        printf("var -> ID \n");
        temp_Var = scope.find(node.id);
        temp_expression = builder->create_load(temp_Var);
    }
    else{//array variable
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
        auto icmp = builder->create_icmp_lt(array_index,CONST_INT(0));
        auto errorBB = BasicBlock::create(module.get(),node.id+"_error"+std::to_string(Block_number),builder->get_insert_block()->get_parent());
        auto correctBB = BasicBlock::create(module.get(),node.id+"_correct"+std::to_string(Block_number++),builder->get_insert_block()->get_parent());
        auto br = builder->create_cond_br(icmp,errorBB,correctBB);
        //errorBB
        builder->set_insert_point(errorBB);
        auto error_function = scope.find("neg_idx_except");
        builder->create_call(error_function,{});
        builder->create_br(correctBB);
        
        //correctBB
        printf("will enter correct\n");
        builder->set_insert_point(correctBB);
        
        printf("have insert basicblock\n");
        
        if(array_var->get_type()->get_pointer_element_type()->is_array_type()){
            //array
            printf("*****************************\n");
            printf("a array!\n");
            temp_Var = builder->create_gep(array_var, {CONST_INT(0), array_index});    
        }
        else{
            //pointer
            printf("-------------------------\n");
            //printf("a pointer\n");
            auto array_load = builder->create_load(array_var);
            temp_Var = builder->create_gep(array_load, {array_index});
        }
        
        
        printf("Have gotten temp_Var\n");
        temp_expression = builder->create_load(temp_Var);
        printf("have gotten temp_expression\n");
    }
 }

void CminusfBuilder::visit(ASTAssignExpression &node) {
    printf("a AssignExpression : var = expression\n");
    node.var->accept(*this);
    Value *left_var = temp_Var;
    Value *left_expression = temp_expression;
    printf("var is ready\n");
    node.expression->accept(*this);
    Value *right_expression = temp_expression;
    printf("right_expression is ready\n");
    auto Int32Type = Type::get_int32_type(module.get());
    auto FloatType = Type::get_float_type(module.get());
    //??????????????????
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
    printf("123456\n");
    builder->create_store(right_expression,left_var);
    temp_expression = builder->create_load(left_var);
 }

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    printf("a simple-expression \n");
    if(node.additive_expression_r!= nullptr){
        printf("a simple-expression -> additive-expression relop additive-expression\n");
        node.additive_expression_l->accept(*this);
        auto addit_l_expression = temp_expression;
        node.additive_expression_r->accept(*this);
        auto addit_r_expression = temp_expression;
        auto Int32Type = Type::get_int32_type(module.get());
        auto FloatType = Type::get_float_type(module.get());
        //??????????????????
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
        temp_expression = builder->create_zext(temp_expression,Int32Type);
    }
    else{
        printf("a simple-expression -> additive-expression\n");
        node.additive_expression_l->accept(*this);
    }
 }

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    printf("a additiveExpression \n");
    if(node.additive_expression!= nullptr){
        printf("additive-expression -> additive-expression addop term\n");
        node.additive_expression->accept(*this);
        auto addit_expression = temp_expression;
        node.term->accept(*this);
        auto term_expression = temp_expression;
        printf("have two expressions and will addop them\n");
        auto Int32Type = Type::get_int32_type(module.get());
        auto FloatType = Type::get_float_type(module.get());
        if(addit_expression->get_type()->is_float_type() 
            && term_expression->get_type()->is_integer_type()
            || addit_expression->get_type()->is_integer_type()
            && term_expression->get_type()->is_float_type()){
            //two float expressions
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
    }
    else{
        printf("additive-expression -> term\n");
        node.term->accept(*this);
    }
 }

void CminusfBuilder::visit(ASTTerm &node) {
    printf("a term \n");
    if(node.term!= nullptr){
        printf("term -> term mulop factor\n");
        node.term->accept(*this);
        auto term_expression = temp_expression;
        node.factor->accept(*this);
        auto Int32Type = Type::get_int32_type(module.get());
        auto FloatType = Type::get_float_type(module.get());
        auto factor_expression = temp_expression;
        
        if(factor_expression->get_type()->is_float_type() 
            && term_expression->get_type()->is_integer_type()
                || factor_expression->get_type()->is_integer_type()
                    && term_expression->get_type()->is_float_type()){
            //??????????????????
            if(term_expression->get_type()->is_float_type()){
                factor_expression = builder->create_sitofp(factor_expression,FloatType);
            }
            else if(factor_expression->get_type()->is_float_type()){
                term_expression = builder->create_sitofp(term_expression,FloatType);
            }
            switch(node.op){
                case OP_MUL: 
                    temp_expression = builder->create_fmul(term_expression, factor_expression);
                    break;
                case OP_DIV:
                    temp_expression = builder->create_fdiv(term_expression, factor_expression);
                    break;
            }
        }
        else if(term_expression->get_type()->is_float_type() 
                    && factor_expression->get_type()->is_float_type()){
            //two float expressions
            switch(node.op){
                case OP_MUL: 
                    temp_expression = builder->create_fmul(term_expression, factor_expression);
                    break;
                case OP_DIV:
                    temp_expression = builder->create_fdiv(term_expression, factor_expression);
                    break;
            }
        }
        else if(term_expression->get_type()->is_integer_type()
                    && factor_expression->get_type()->is_integer_type()){
            //two integer expressions
            switch(node.op){
                case OP_MUL: 
                    temp_expression = builder->create_imul(term_expression, factor_expression);
                    break;
                case OP_DIV:
                    temp_expression = builder->create_isdiv(term_expression, factor_expression);
                    break;
            }
        }
        
    }
    else{
        printf("term -> factor \n");
        node.factor->accept(*this);
    }
}
void CminusfBuilder::visit(ASTCall &node) {
    printf("call a function\n");
    auto Fun = scope.find(node.id);
    //??????????????????????????????????????????
    auto FunType = static_cast<FunctionType*>(Fun->get_type());
    std::vector<Value *> real_args;
    auto Int32Type = Type::get_int32_type(module.get());
    auto FloatType = Type::get_float_type(module.get());
    printf("function args num is %d\n",node.args.size());
    for(int i=0;i<node.args.size();i++){
        //??????????????????
        printf("will accept a arg\n");
        node.args[i]->accept(*this);
        printf("accpet a arg\n");
        //value_expression???????????????????????????????????????int???float??????
        //value_address????????????????????????????????????????????????
        auto value_expression = temp_expression;
        auto value_address = temp_Var;
        auto Int32PtrType = Type::get_int32_ptr_type(module.get()); 
        auto FloatPtrType = Type::get_float_ptr_type(module.get());
        if(FunType->get_param_type(i)->is_pointer_type()){
            //????????????
            auto value_load = builder->create_load(value_address);
            if(value_load->get_type()->is_pointer_type()){
                //?????????????????????
                real_args.push_back(value_load);
            }
            else{
                //?????????????????????
                printf("Call Function: this is a array!!!\n");
                auto arg = builder->create_gep(value_address, {CONST_INT(0), CONST_INT(0)});
                real_args.push_back(arg);
            }
            printf("Call Function : have store the pointer---------------- \n");
        }

        else{
            //int???float??????
            if(FunType->get_param_type(i)!=value_expression->get_type()){
                //???????????? 
                if(FunType->get_param_type(i)->is_float_type()){
                    value_expression = builder->create_sitofp(temp_expression,FloatType);
                }
                else if(FunType->get_param_type(i)->is_integer_type()){
                    value_expression = builder->create_fptosi(temp_expression,Int32Type);
                }
            }
            real_args.push_back(value_expression);
        }
    }
    auto call = builder->create_call(Fun,real_args);
    temp_expression = call;
    printf("call function is over\n");
 }
