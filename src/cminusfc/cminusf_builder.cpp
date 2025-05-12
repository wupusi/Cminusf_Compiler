#include "cminusf_builder.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

char labelName[32];

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

Value *CminusfBuilder::visit(ASTProgram &node)
{
    VOID_T = module->get_void_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();

    Value *ret_val = nullptr;
    for (auto &decl : node.declarations)
    {
        ret_val = decl->accept(*this);
    }
    return ret_val;
}

Value *CminusfBuilder::visit(ASTNum &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    if (node.type == TYPE_INT)
    {
        context.value = CONST_INT(node.i_val);
    }
    else if (node.type == TYPE_FLOAT)
    {
        context.value = CONST_FP(node.f_val);
    }
    return nullptr;
}

Value *CminusfBuilder::visit(ASTVarDeclaration &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    Type *vartype;
    if (node.num == nullptr)
    {
        if (node.type == TYPE_INT)
        {
            vartype = INT32_T;
        }
        else
        {
            vartype = FLOAT_T;
        }
    }
    else
    {
        int array_size = node.num->i_val; // 语法保证是整数常量
        vartype = ArrayType::get((node.type == TYPE_INT) ? INT32_T : FLOAT_T, array_size);
    }

    Value *newVar;
    if (scope.in_global())
    {
        newVar = GlobalVariable::create(node.id, module.get(), vartype, false, ConstantZero::get(vartype, module.get()));
    }
    else
    {
        newVar = builder->create_alloca(vartype);
    }
    scope.push(node.id, newVar);
    return nullptr;
}

Value *CminusfBuilder::visit(ASTFunDeclaration &node)
{
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto &param : node.params)
    {
        // TODO: Please accomplish param_types.
        if (param->isarray)
        {
            if (param->type == TYPE_INT)
                param_types.push_back(INT32PTR_T);
            else
                param_types.push_back(FLOATPTR_T);
        }
        else
        {
            if (param->type == TYPE_INT)
            {
                param_types.push_back(INT32_T);
            }
            else
            {
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto func = Function::create(fun_type, node.id, module.get());
    scope.push(node.id, func);
    context.func = func;
    auto funBB = BasicBlock::create(module.get(), "entry", func);
    builder->set_insert_point(funBB);
    scope.enter();
    std::vector<Value *> args;
    for (auto &arg : func->get_args())
    {
        args.push_back(&arg);
    }
    for (int i = 0; i < node.params.size(); ++i)
    {
        // TODO: You need to deal with params and store them in the scope.
        auto param = node.params[i];
        auto arg_val = args[i];
        Type *alloca_type;
        if (param->isarray)
            alloca_type = (param->type == TYPE_INT) ? INT32PTR_T : FLOATPTR_T;
        else
            alloca_type = (param->type == TYPE_INT) ? INT32_T : FLOAT_T;
        auto alloca = builder->create_alloca(alloca_type);
        builder->create_store(arg_val, alloca);
        scope.push(param->id, alloca);
    }
    node.compound_stmt->accept(*this);
    if (not builder->get_insert_block()->is_terminated())
    {
        if (context.func->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (context.func->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
    return nullptr;
}

Value *CminusfBuilder::visit(ASTParam &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    Type *param_type;
    if (node.isarray)
    {
        //param_type = ArrayType::get(INT32_T, 0); // 实际上参数是指针
        param_type = PointerType::get(INT32_T);  // 传递数组退化为指针
    }
    else
    {
        param_type = INT32_T;
    }

    auto *alloca = builder->create_alloca(param_type); // 为参数分配空间
    scope.push(node.id, alloca);                       // 注册到符号表
    builder->create_store(context.value, alloca);      // 保存函数调用时传入的实参
    return nullptr;
}

Value *CminusfBuilder::visit(ASTCompoundStmt &node)
{
    // TODO: This function is not complete.
    // You may need to add some code here
    // to deal with complex statements.
    scope.enter();
    //处理局部变量声明
    for (auto &decl : node.local_declarations)
    {
        decl->accept(*this);
    }
    //处理语句
    for (auto &stmt : node.statement_list)
    {
        stmt->accept(*this);
        //如果终止(return,break),退出执行
        if (builder->get_insert_block()->is_terminated())
            break;
    }
    scope.exit();
    return nullptr;
}

Value *CminusfBuilder::visit(ASTExpressionStmt &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    //处理表达式
    if (node.expression != nullptr)
    {
        node.expression->accept(*this);
    }
    return nullptr;
}

Value *CminusfBuilder::visit(ASTSelectionStmt &node)
{
    // TODO: This function is empty now.
    // Add some code here.

    auto trueBB = BasicBlock::create(module.get(), labelName, context.func);
    auto falseBB = BasicBlock::create(module.get(), labelName, context.func);
    auto endBB = BasicBlock::create(module.get(), labelName, context.func);
    //回调Value *CminusfBuilder::visit(ASTExpressionStmt &node)
    node.expression->accept(*this);

    //根据表达式类型生成比较指令
    if (context.value->get_type()->is_integer_type())
    {
        context.value = builder->create_icmp_ne(context.value, CONST_INT(0));
    }
    else
    {
        context.value = builder->create_fcmp_ne(context.value, CONST_FP(0.));
    }
    //条件跳转
    builder->create_cond_br(context.value, trueBB, falseBB);

    builder->set_insert_point(trueBB);
    //进入新作用域
    scope.enter();
    node.if_statement->accept(*this);
    /*
    退出作用域
    exit放在跳转指令生成之前:
        作用域的生命周期必须严格匹配
            局部变量会错误地存活到 if 语句之外（违反语言规范）。
            如果后续代码复用变量名，可能导致冲突。
        与控制流指令的交互
            跳转后的代码（如 endBB）已经不属于当前作用域。
            如果作用域未正确退出，可能导致符号表污染或资源泄漏。
    */
    scope.exit();

    //如果没有终止指令，创建endBB 如有，不会创建endBB
    if (!builder->get_insert_block()->is_terminated())
    {
        builder->create_br(endBB);
    }

    builder->set_insert_point(falseBB);
    //有else则跳转，无则创建endBB
    if (node.else_statement != nullptr)
    {
        scope.enter();
        node.else_statement->accept(*this);
        scope.exit();
        if (!builder->get_insert_block()->is_terminated())
        {
            builder->create_br(endBB);
        }
    }
    else
    {
        builder->create_br(endBB);
    }

    builder->set_insert_point(endBB);

    return nullptr;
}

Value *CminusfBuilder::visit(ASTIterationStmt &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    auto condBB = BasicBlock::create(module.get(), labelName, context.func);
    auto bodyBB = BasicBlock::create(module.get(), labelName, context.func);
    auto endBB = BasicBlock::create(module.get(), labelName, context.func);

    if (!builder->get_insert_block()->is_terminated())
    {
        builder->create_br(condBB);
    }
    builder->set_insert_point(condBB);
    node.expression->accept(*this);
    if (context.value->get_type()->is_integer_type())
    {
        context.value = builder->create_icmp_ne(context.value, CONST_INT(0));
    }
    else
    {
        context.value = builder->create_fcmp_ne(context.value, CONST_FP(0.0));
    }

    builder->create_cond_br(context.value, bodyBB, endBB);
    builder->set_insert_point(bodyBB);
    scope.enter();
    node.statement->accept(*this);
    scope.exit();
    if (!builder->get_insert_block()->is_terminated())
    {
        builder->create_br(condBB);
    }
    builder->set_insert_point(endBB);
    return nullptr;
}

Value *CminusfBuilder::visit(ASTReturnStmt &node)
{
    if (node.expression == nullptr)
    {
        builder->create_void_ret();
        return nullptr;
    }
    else
    {
        // TODO: The given code is incomplete.
        // You need to solve other return cases (e.g. return an integer).
        node.expression->accept(*this);
        //处理return 语句中的返回值类型
        auto retType = context.func->get_function_type()->get_return_type();
        //如果与返回值类型与函数返回值类型不匹配，类型转换
        if (retType != context.value->get_type())
        {
            // expression type is inconsistent with the function return type,
            // type conversion is performed
            if (context.value->get_type()->is_integer_type())
            {
                context.value = builder->create_sitofp(context.value, FLOAT_T);
            }
            else
            {
                context.value = builder->create_fptosi(context.value, INT32_T);
            }
        }
        builder->create_ret(context.value);
    }
    return nullptr;
}

//处理变量引用（包括数组索引访问）的中间代码生成
Value *CminusfBuilder::visit(ASTVar &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    //变量查找
    context.value = scope.find(node.id);
    if (node.expression == nullptr)
    {
        if (context.lvalue == 0)
        {
            //如果是右值
            if (context.value->get_type()->get_pointer_element_type()->is_array_type())
            {
                //数组用指针处理
                context.value = builder->create_gep(context.value, {CONST_INT(0), CONST_INT(0)});
            }
            else
            {
                //其他类型变量处理
                context.value = builder->create_load(context.value);
            }
        }
    }
    else
    {
        //有表达式情况
        //保存变量自身指针(如数组首地址)
        auto *lVal = context.value;
        //备份左值右值标记
        bool backup = context.lvalue;
        //强制视为右值
        context.lvalue = false;
        //处理索引表达式
        node.expression->accept(*this);
        //恢复标记
        context.lvalue = backup;
        //获取表达式计算结果
        auto *rVal = context.value;
        //类型转换
        if (rVal->get_type()->is_float_type())
        {
            rVal = builder->create_fptosi(rVal, INT32_T);
        }

        auto trueBB = BasicBlock::create(module.get(), labelName, context.func);//索引合法分支
        auto falseBB = BasicBlock::create(module.get(), labelName, context.func);
        auto endBB = BasicBlock::create(module.get(), labelName, context.func);
        auto *cond = builder->create_icmp_ge(rVal, CONST_INT(0));
        builder->create_cond_br(cond, trueBB, falseBB);
        builder->set_insert_point(trueBB);
        if (lVal->get_type()->get_pointer_element_type()->is_integer_type() || lVal->get_type()->get_pointer_element_type()->is_float_type())
        {
            //一维数组直接计算元素地址
            context.value = builder->create_gep(lVal, {rVal});
        }
        else if (lVal->get_type()->get_pointer_element_type()->is_pointer_type())
        {
            //指针数组，访问是纯地址计算，不涉及符号查找，无需scope.push
            lVal = builder->create_load(lVal);//解引用获取指针
            context.value = builder->create_gep(lVal, {rVal});//计算元素地址
        }
        else
        {
            //多维数组:固定首维度偏移为0
            context.value = builder->create_gep(lVal, {CONST_INT(0), rVal});
        }
        if (context.lvalue == 0)
        {
            //右值需要加载实际值
            context.value = builder->create_load(context.value);
        }
        builder->create_br(endBB);
        // False
        builder->set_insert_point(falseBB);//非法索引处理
        builder->create_call(scope.find("neg_idx_except"), {});//调用异常处理函数
        builder->create_br(endBB);
        // End
        builder->set_insert_point(endBB);
    }
    return nullptr;
}

//赋值表达式
Value *CminusfBuilder::visit(ASTAssignExpression &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    // auto lhs = node.var->accept(this);
    // auto rhs = node.expression->accept(this);
    // builder->create_store(rhs, lhs);
    // return rhs;

    //左值处理
    context.lvalue = true;
    node.var->accept(*this);
    context.lvalue = false;//恢复右值
    auto orig = context.value;//保存左值
    node.expression->accept(*this);//处理右值
    auto varAlloca = context.value;
    auto varType = varAlloca->get_type();

    //赋值左右两边类型不同，右侧进行转换
    if (orig->get_type()->get_pointer_element_type() != varType)
    {
        if (varType->is_integer_type())
        {
            varAlloca = builder->create_sitofp(varAlloca, FLOAT_T);
        }
        else
        {
            varAlloca = builder->create_fptosi(varAlloca, INT32_T);
        }
    }
    //进行赋值
    builder->create_store(varAlloca, orig);
    context.value = varAlloca;
    return nullptr;
}

//处理关系表达式 < > <= >=
Value *CminusfBuilder::visit(ASTSimpleExpression &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    node.additive_expression_l->accept(*this);//计算左表达式
    if (node.additive_expression_r == nullptr)
    { // Only left expression
        return nullptr;
    }
    auto *lVal = context.value;//保存左值结果
    node.additive_expression_r->accept(*this);
    auto *rVal = context.value;//获取右值结果
    // Data type conversion 若类型不一，统一转换为float
    if (lVal->get_type()->is_float_type() || context.value->get_type()->is_float_type())
    { // Has float
        if (lVal->get_type()->is_integer_type())
        {
            lVal = builder->create_sitofp(lVal, FLOAT_T);
        }
        if (rVal->get_type()->is_integer_type())
        {
            rVal = builder->create_sitofp(rVal, FLOAT_T);
        }
        switch (node.op)
        {
        case OP_LT:
            context.value = builder->create_fcmp_lt(lVal, rVal);
            break;
        case OP_GT:
            context.value = builder->create_fcmp_gt(lVal, rVal);
            break;
        case OP_LE:
            context.value = builder->create_fcmp_le(lVal, rVal);
            break;
        case OP_GE:
            context.value = builder->create_fcmp_ge(lVal, rVal);
            break;
        case OP_EQ:
            context.value = builder->create_fcmp_eq(lVal, rVal);
            break;
        case OP_NEQ:
            context.value = builder->create_fcmp_ne(lVal, rVal);
            break;
        }
    }
    else
    {
        switch (node.op)
        {
        case OP_LT:
            context.value = builder->create_icmp_lt(lVal, rVal);
            break;
        case OP_GT:
            context.value = builder->create_icmp_gt(lVal, rVal);
            break;
        case OP_LE:
            context.value = builder->create_icmp_le(lVal, rVal);
            break;
        case OP_GE:
            context.value = builder->create_icmp_ge(lVal, rVal);
            break;
        case OP_EQ:
            context.value = builder->create_icmp_eq(lVal, rVal);
            break;
        case OP_NEQ:
            context.value = builder->create_icmp_ne(lVal, rVal);
            break;
        }
    }
    context.value = builder->create_zext(context.value, INT32_T);//将1位bool扩展为32位int
    return nullptr;
}

//加减法表达式
Value *CminusfBuilder::visit(ASTAdditiveExpression &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    //单操作数
    if (node.additive_expression == nullptr)
    {
        node.term->accept(*this);
    }
    //双操作数
    else
    {
        //递归处理左表达式
        node.additive_expression->accept(*this);
        auto *lVal = context.value;
        //处理右表达式
        node.term->accept(*this);
        auto *rVal = context.value;

        if (lVal->get_type()->is_float_type() || context.value->get_type()->is_float_type())
        { // Has float
            if (lVal->get_type()->is_integer_type())
            {
                lVal = builder->create_sitofp(lVal, FLOAT_T);
            }
            if (rVal->get_type()->is_integer_type())
            {
                rVal = builder->create_sitofp(rVal, FLOAT_T);
            }
            if (node.op == OP_PLUS)
            {
                context.value = builder->create_fadd(lVal, rVal);
            }
            else if (node.op == OP_MINUS)
            {
                context.value = builder->create_fsub(lVal, rVal);
            }
        }
        else
        {
            if (node.op == OP_PLUS)
            {
                context.value = builder->create_iadd(lVal, rVal);
            }
            else if (node.op == OP_MINUS)
            {
                context.value = builder->create_isub(lVal, rVal);
            }
        }
    }
    return nullptr;
}

//处理乘除法表达式
Value *CminusfBuilder::visit(ASTTerm &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    if (node.term == nullptr)
    {
        node.factor->accept(*this);
        return nullptr;
    }
    node.term->accept(*this);
    auto *lVal = context.value;
    node.factor->accept(*this);
    auto *rVal = context.value;

    if (lVal->get_type()->is_float_type() || context.value->get_type()->is_float_type())
    { // Has float
        if (lVal->get_type()->is_integer_type())
        {
            lVal = builder->create_sitofp(lVal, FLOAT_T);
        }
        if (rVal->get_type()->is_integer_type())
        {
            rVal = builder->create_sitofp(rVal, FLOAT_T);
        }
        if (node.op == OP_MUL)
        {
            context.value = builder->create_fmul(lVal, rVal);
        }
        else if (node.op == OP_DIV)
        {
            context.value = builder->create_fdiv(lVal, rVal);
        }
    }
    else
    {
        if (node.op == OP_MUL)
        {
            context.value = builder->create_imul(lVal, rVal);
        }
        else if (node.op == OP_DIV)
        {
            context.value = builder->create_isdiv(lVal, rVal);
        }
    }
    return nullptr;
}

Value *CminusfBuilder::visit(ASTCall &node)
{
    // TODO: This function is empty now.
    // Add some code here.
    //符号表中查找id对应的函数对象
    auto *func = (Function *)(scope.find(node.id));
    //获得形参列表的起始位置
    auto param = func->get_function_type()->param_begin();
    //实参容器
    std::vector<Value *> args;
    for (auto &arg : node.args)
    {
        arg->accept(*this);//递归计算参数表达式
        auto *vType = context.value->get_type();
        //与形参列表不匹配并且非指针类型才进行类型转换
        if (vType != *param && !vType->is_pointer_type())
        {
            if (vType->is_integer_type())
            {
                context.value = builder->create_sitofp(context.value, *param);
            }
            else
            {
                context.value = builder->create_fptosi(context.value, *param);
            }
        }
        param++;//实现同步递归
        args.push_back(context.value);
    }
    context.value = builder->create_call(func, args);
    return nullptr;
}
