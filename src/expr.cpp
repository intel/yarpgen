/*
Copyright (c) 2015-2019, Intel Corporation

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
#include "expr.h"
#include <algorithm>
#include <utility>

using namespace yarpgen;

std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<ScalarVarUseExpr>> yarpgen::ScalarVarUseExpr::scalar_var_use_set;
std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<IterUseExpr>> yarpgen::IterUseExpr::iter_use_set;

std::shared_ptr<Data> Expr::getValue() {
    //TODO: it might cause some problems in the future, but it is good for now
    return value;
}

ConstantExpr::ConstantExpr(IRValue _value) {
    //TODO: maybe we need a constant data type rather than an anonymous scalar variable
    value = std::make_shared<ScalarVar>("", IntegralType::init(_value.getIntTypeID()), _value);
}

Expr::EvalResType ConstantExpr::evaluate(EvalCtx &ctx) {
    return value;
}

Expr::EvalResType ConstantExpr::rebuild(EvalCtx &ctx) {
    return evaluate(ctx);
}

void ConstantExpr::emit(std::ostream& stream, std::string offset) {
    assert(value->isScalarVar() && "ConstExpr can represent only scalar constant");
    auto scalar_var = std::static_pointer_cast<ScalarVar>(value);

    assert(scalar_var->getType()->isIntType() && "ConstExpr can represent only scalar integral constant");
    auto int_type = std::static_pointer_cast<IntegralType>(scalar_var->getType());

    IRValue val = scalar_var->getCurrentValue();
    IRValue min_val = int_type->getMin();
    if (!int_type->getIsSigned() || (val != min_val).getValueRef<bool>())
        stream << val << int_type->getLiteralSuffix();

    // INT_MIN is defined as -INT_MAX - 1, so we have to do the same
    IRValue one(val.getIntTypeID());
    //TODO: this is not an appropriate way to do it
    one.getValueRef<uint64_t>() = 1;
    IRValue min_one_val = min_val + one;
    stream << "(" << min_one_val << " - " << one << ")";
}

std::shared_ptr<ScalarVarUseExpr> ScalarVarUseExpr::init(std::shared_ptr<Data> _val) {
    assert(_val->isScalarVar() && "ScalarVarUseExpr accepts only scalar variables!");
    auto find_res = scalar_var_use_set.find(_val);
    if (find_res != scalar_var_use_set.end())
        return find_res->second;

    auto ret = std::make_shared<ScalarVarUseExpr>(_val);
    scalar_var_use_set[_val] = ret;
    return ret;
}

void ScalarVarUseExpr::setValue(std::shared_ptr<Expr> _expr) {
    std::shared_ptr<Data> new_val = _expr->getValue();
    assert(new_val->isScalarVar() && "Can store only scalar variables!");
    if (value->getType() != new_val->getType())
        ERROR("Can't assign different types!");

    std::static_pointer_cast<ScalarVar>(value)->setCurrentValue(std::static_pointer_cast<ScalarVar>(new_val)->getCurrentValue());
}

Expr::EvalResType ScalarVarUseExpr::evaluate(EvalCtx &ctx) {
    // This variable is defined and we can just return it.
    auto find_res = ctx.input.find(value->getName());
    if (find_res != ctx.input.end()) {
        return find_res->second;
    }
    return value;
}

Expr::EvalResType ScalarVarUseExpr::rebuild(EvalCtx &ctx) {
    return evaluate(ctx);
}

std::shared_ptr<IterUseExpr> IterUseExpr::init(std::shared_ptr<Data> _iter) {
    assert(_iter->isIterator() && "IterUseExpr accepts only iterators!");
    auto find_res = iter_use_set.find(_iter);
    if (find_res != iter_use_set.end())
        return find_res->second;

    auto ret = std::make_shared<IterUseExpr>(_iter);
    iter_use_set[_iter] = ret;
    return ret;
}

void IterUseExpr::setValue(std::shared_ptr<Expr> _expr) {
    std::shared_ptr<Data> new_val = _expr->getValue();
    assert(new_val->isIterator() && "IterUseExpr can store only iterators!");
    auto new_iter = std::static_pointer_cast<Iterator>(new_val);
    if (value->getType() != new_iter->getType())
        ERROR("Can't assign different types!");

    std::static_pointer_cast<Iterator>(value)->setParameters(new_iter->getStart(), new_iter->getEnd(), new_iter->getStep());
}

Expr::EvalResType IterUseExpr::evaluate(EvalCtx &ctx) {
    // This iterator is defined and we can just return it.
    auto find_res = ctx.input.find(value->getName());
    if (find_res != ctx.input.end()) {
        return find_res->second;
    }

    return value;
}

Expr::EvalResType IterUseExpr::rebuild(EvalCtx &ctx) {
    return evaluate(ctx);
}
