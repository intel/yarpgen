/*
Copyright (c) 2015-2020, Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////////////////

#include "context.h"
#include "data.h"
#include "expr.h"

#include <utility>

using namespace yarpgen;

void ScalarVar::dbgDump() {
    std::cout << "Scalar var: " << name << std::endl;
    std::cout << "Type info:" << std::endl;
    type->dbgDump();
    std::cout << "Init val: " << init_val << std::endl;
    std::cout << "Current val: " << cur_val << std::endl;
    std::cout << "Was changed: " << changed << std::endl;
}

std::shared_ptr<ScalarVar> ScalarVar::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    IntTypeID type_id = rand_val_gen->getRandId(gen_pol->int_type_distr);
    IRValue init_val = rand_val_gen->getRandValue(type_id);
    auto int_type = IntegralType::init(type_id);
    NameHandler& nh = NameHandler::getInstance();
    return std::make_shared<ScalarVar>(nh.getVarName(), int_type, init_val);
}

void Array::dbgDump() {
    std::cout << "Array: " << name << std::endl;
    std::cout << "Type info:" << std::endl;
    type->dbgDump();
    init_vals->dbgDump();
    cur_vals->dbgDump();
}

Array::Array(std::string _name,
                      const std::shared_ptr<ArrayType> &_type,
                      std::shared_ptr<Data> _val)
    : Data(std::move(_name), _type), init_vals(_val), cur_vals(_val),
      was_changed(false) {
    if (!type->isArrayType())
        ERROR("Array variable should have an ArrayType");

    auto array_type = std::static_pointer_cast<ArrayType>(type);
    if (array_type->getBaseType() != init_vals->getType())
        ERROR("Can't initialize array with variable of wrong type");

    ub_code = init_vals->getUBCode();
}
void Array::setValue(std::shared_ptr<Data> _val) {
    auto array_type = std::static_pointer_cast<ArrayType>(type);
    if (array_type->getBaseType() != _val->getType())
        ERROR("Can't initialize array with variable of wrong type");
    cur_vals = std::move(_val);
    ub_code = cur_vals->getUBCode();
    was_changed = true;
}

void Iterator::setParameters(std::shared_ptr<Expr> _start,
                                      std::shared_ptr<Expr> _end,
                                      std::shared_ptr<Expr> _step) {
    start = std::move(_start);
    end = std::move(_end);
    step = std::move(_step);
}

std::shared_ptr<Iterator> Iterator::create(std::shared_ptr<GenCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();

    IntTypeID type_id = rand_val_gen->getRandId(gen_pol->int_type_distr);
    auto start = std::make_shared<ConstantExpr>(IRValue{type_id, {false, 0}});
    size_t end_val = rand_val_gen->getRandValue(gen_pol->iters_end_limit_min, gen_pol->iter_end_limit_max);
    auto end = std::make_shared<ConstantExpr>(IRValue(type_id, {false, end_val}));
    size_t step_val = rand_val_gen->getRandId(gen_pol->iters_step_distr);
    auto step = std::make_shared<ConstantExpr>(IRValue{type_id, {false, step_val}});
    auto type = IntegralType::init(type_id);

    NameHandler& nh = NameHandler::getInstance();

    auto iter = std::make_shared<Iterator>(nh.getIterName(), type, start, end, step);
    return iter;
}

void Iterator::dbgDump() {
    std::cout << name << std::endl;
    type->dbgDump();
    start->emit(std::cout);
    end->emit(std::cout);
    end->emit(std::cout);
}
