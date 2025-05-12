#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRBuilder.hpp"
#include "Module.hpp"
#include "Type.hpp"

#include <iostream>
#include <memory>

// int callee(int a) { return 2 * a; }
// int main() { return callee(110); }

#define CONST_INT(num) \
    ConstantInt::get(num,module)
int main() {
    auto module = new Module();
    auto builder = new IRBuilder(nullptr,module);
    Type *Int32Type = module->get_int32_type();
    //function callee
    // int callee(int a) { return 2 * a; }
    std::vector<Type *> Int(1, Int32Type);
    auto calleeFunTy = FunctionType::get(Int32Type,Int);
    auto calleeFun = Function::create(calleeFunTy, "callee", module);
    auto bb = BasicBlock::create(module, "entry", calleeFun);
    builder->set_insert_point(bb);
    auto aAlloca = builder->create_alloca(Int32Type);
    std::vector<Value *> args;
    for(auto &arg: calleeFun->get_args())
    {
        args.push_back(&arg);
    }
    builder->create_store(args[0], aAlloca);
    auto aLoad = builder->create_load(aAlloca);
    auto mul = builder->create_imul(aLoad,CONST_INT(2));
    builder->create_ret(mul);

    //main
    // int main() { return callee(110); }
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    auto retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0),retAlloca);
    auto call = builder->create_call(calleeFun,{CONST_INT(110)});
    builder->create_ret(call);
    std::cout<< module->print();
    delete module;
    return 0;

}
