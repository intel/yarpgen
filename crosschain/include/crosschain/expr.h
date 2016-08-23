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

#ifndef EXPR_H
#define EXPR_H

#include <crosschain/typedefs.h>
#include <crosschain/types.h>
#include <node.h>

namespace crosschain {


class IDX {
protected:
    // IDX = it + shift +/- IDX'
    std::shared_ptr<IDX> idx;
    rl::BinaryExpr::Op idx_op;
    std::shared_ptr<IterVar> it;
    sll shift;

public:
    explicit IDX (sll i);
    explicit IDX (std::shared_ptr<IterVar> it_);
    explicit IDX (std::shared_ptr<IterVar> it_, sll i);

    virtual sll getShift() {return this->shift;}
    virtual std::shared_ptr<IDX> getChild() {return this->idx; }
    virtual rl::BinaryExpr::Op getChildSign() {return this->idx_op; }

    // Some indexes do not have iterators in them, and some do
    // like a[0] and a[i + 3]
    virtual std::shared_ptr<rl::Expr> getExprPtr ();

    IDX& operator+=(sll rhs);
    IDX& operator-=(sll rhs);
    IDX& operator+=(IDX rhs);
    IDX& operator-=(IDX rhs);

    virtual IDX copy();
    virtual sll getStep(std::shared_ptr<rl::Context> ctx);
    virtual bool isDummy() {return ((this->it.use_count() == 0) && (this->shift == 0)); }

    virtual void clearup_tree();
};
}


#endif
