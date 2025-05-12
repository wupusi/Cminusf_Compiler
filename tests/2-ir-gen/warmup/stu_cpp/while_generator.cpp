#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"

#include <iostream>
#include <memory>

// 定义一个从常数值获取/创建 ConstantInt 类实例化的宏，方便多次调用
#define CONST_INT(num) \
    ConstantInt::get(num, module)

// 定义一个从常数值获取/创建 ConstantFP 类实例化的宏，方便多次调用
#define CONST_FP(num) \
    ConstantFP::get(num, module)

// int main() {
//     int a;
//     int i;
//     a = 10;
//     i = 0;
//     while (i < 10) {
//         i = i + 1;
//         a = a + i;
//     }
//     return a;
// }


int main() {
    auto module = new Module();
    auto builder = new IRBuilder(nullptr,module);
    Type *Int32Type = module->get_int32_type();
    
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto entryBB = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(entryBB);

    auto aAlloca = builder->create_alloca(Int32Type);
    auto iAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(10),aAlloca);
    builder->create_store(CONST_INT(0),iAlloca);

    auto condBB = BasicBlock::create(module,"cond", mainFun);
    auto bodyBB = BasicBlock::create(module,"body",mainFun);
    auto endBB = BasicBlock::create(module,"end", mainFun);

    builder->create_br(condBB);

    builder->set_insert_point(condBB);
    auto iload = builder->create_load(iAlloca);
    auto cmp = builder->create_icmp_lt(iload,CONST_INT(10));
    auto br = builder->create_cond_br(cmp,bodyBB,endBB);

    builder->set_insert_point(bodyBB);
    iload = builder->create_load(iAlloca);
    auto iadd = builder->create_iadd(iload,CONST_INT(1));
    builder->create_store(iadd,iAlloca);
    auto aload = builder->create_load(aAlloca);
    iload = builder->create_load(iAlloca);
    auto aadd = builder->create_iadd(aload,iload);
    builder->create_store(aadd,aAlloca);
    builder->create_br(condBB);

    builder->set_insert_point(endBB);
    aload = builder->create_load(aAlloca);
    builder->create_ret(aload);
    
    std::cout<<module->print();
    delete module;
    return 0;

}
