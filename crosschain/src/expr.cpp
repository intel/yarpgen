/*
Copyright (c) 2015-2016, Intel Corporation

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

#include <crosschain/expr.h>

using namespace crosschain;


IDX::IDX (sll i) {
    this->shift = i;
}


IDX::IDX (std::shared_ptr<IterVar> it_) {
    this->shift = 0;
    this->it = it_;
    assert(this->it.use_count() != 0);
}


IDX::IDX (std::shared_ptr<IterVar> it_, sll i) {
    this->shift = i;
    this->it = it_;
}


std::shared_ptr<rl::Expr> IDX::getExprPtr () {
    std::shared_ptr<rl::Expr> to_return;
    if (this->it.use_count() == 0)
        to_return = std::make_shared<rl::ConstExpr>(rl::IntegerType::IntegerTypeID::INT, (uint64_t)this->shift);
    else if (this->shift == 0)
        to_return = std::make_shared<rl::VarUseExpr>(this->it);
    else
        to_return = std::make_shared<rl::BinaryExpr>(rl::BinaryExpr::Op::Add, std::make_shared<rl::VarUseExpr>(this->it),
                    std::make_shared<rl::ConstExpr>(rl::IntegerType::IntegerTypeID::INT, (uint64_t)this->shift));
    if (this->idx.use_count() == 0)
        return to_return;
    return std::make_shared<rl::BinaryExpr>(this->idx_op, to_return, this->idx->getExprPtr());
}


IDX& IDX::operator+=(sll rhs) {
    this->shift += rhs;
    return *this;
}


IDX& IDX::operator-=(sll rhs) {
    this->shift -= rhs;
    return *this;
}


IDX& IDX::operator+=(IDX rhs) {
    this->shift += rhs.getShift();
    rhs -= rhs.getShift();
    if (this->idx.use_count() == 0){
        this->idx_op = rl::BinaryExpr::Op::Add;
        this->idx = std::make_shared<IDX>(rhs.copy());
    }
    else {
        if(this->idx_op == rl::BinaryExpr::Op::Add)
            *(this->idx) += rhs;
        if(this->idx_op == rl::BinaryExpr::Op::Sub)
            *(this->idx) -= rhs;
    }
    return *this;
}


IDX& IDX::operator-=(IDX rhs) {
    this->shift -= rhs.getShift();
    rhs -= rhs.getShift();
    if (this->idx.use_count() == 0){
        this->idx_op = rl::BinaryExpr::Op::Sub;
        this->idx = std::make_shared<IDX>(rhs.copy());
    }
    else {
        if(this->idx_op == rl::BinaryExpr::Op::Add)
            *(this->idx) -= rhs;
        if(this->idx_op == rl::BinaryExpr::Op::Sub)
            *(this->idx) += rhs;
    }
    return *this;
}


sll IDX::getStep(std::shared_ptr<rl::Context> ctx) {
    sll step = 0;
    if (this->it.use_count() != 0)
        step += this->it->getStep(ctx);
    if (this->idx.use_count() != 0)
        if (this->idx_op == rl::BinaryExpr::Op::Add)
            step += this->idx->getStep(ctx);
        if (this->idx_op == rl::BinaryExpr::Op::Sub)
            step -= this->idx->getStep(ctx);
    return step;
}


IDX IDX::copy() {
    IDX ret(this->it, this->shift);
    if (this->idx.use_count() != 0) {
        if (this->idx_op == rl::BinaryExpr::Op::Add)
            ret += this->idx->copy();
        if (this->idx_op == rl::BinaryExpr::Op::Sub)
            ret -= this->idx->copy();
    }
    return ret;
}


void IDX::clearup_tree() {
    while ((this->idx.use_count() != 0) && this->idx->isDummy()){
        if (this->idx->getChildSign() == this->idx_op)
            this->idx_op = rl::BinaryExpr::Op::Add;
        else
            this->idx_op = rl::BinaryExpr::Op::Sub;
        this->idx = this->idx->getChild();
    }
    if (this->idx.use_count() != 0)
        this->idx->clearup_tree();
}
