// int main() {
//     int a[10];
//     a[0] = 10;
//     a[1] = a[0] * 2;
//     return a[1];
// }
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"

#include <iostream>
#include <memory>

#define CONST_INT(num) \
    ConstantInt::get(num,module)
int main() {
    auto module = new Module();
    auto builder = new IRBuilder(nullptr,module);

    Type *Int32Type = module->get_int32_type();
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);

    auto retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0),retAlloca);
    auto *array_a = ArrayType::get(Int32Type,10);
    auto a = builder->create_alloca(array_a);
    auto a0GEP = builder->create_gep(a,{CONST_INT(0),CONST_INT(0)});
    builder->create_store(CONST_INT(10),a0GEP);
    auto a1GEP = builder->create_gep(a,{CONST_INT(0),CONST_INT(1)});
    auto a0Load = builder->create_load(a0GEP);
    auto mul = builder->create_imul(a0Load,CONST_INT(2));
    builder->create_store(mul,a1GEP);
    
    auto a1Load = builder->create_load(a1GEP);
    builder->create_store(a1Load,retAlloca);
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    std::cout<<module->print()<<std::endl;
    return 0;
}
