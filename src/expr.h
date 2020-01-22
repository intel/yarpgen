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

#pragma once

#include <memory>
#include <map>
#include <utility>

#include "ir_value.h"
#include "ir_node.h"
#include "data.h"

namespace yarpgen {

class EvalCtx;

// Common ancestor for all classes that represent various expressions
class Expr : public IRNode {
  public:
    explicit Expr(std::shared_ptr<Data> _value) : value(std::move(_value)) {}
    Expr() = default;

    // This type represent result of computation. We keep it simple for now,
    // but it might change in the future.
    using EvalResType = DataType;

    // This function does type conversions required by the language standard (implicit cast,
    // integral promotion or usual arithmetic conversions) to existing child nodes.
    // As a result, it inserts required TypeCastExpr between existing child nodes and
    // current node.
    //TODO: do we care about CV-qualifiers?
    virtual bool propagateType() = 0;

    // This function calculates value of current node, based on its child nodes.
    // Also it detects UB (for more information, see rebuild() method
    // in inherited classes). It requires propagate_type() to be called first.
    virtual EvalResType evaluate(EvalCtx& ctx) = 0;

   // Similar to evaluate method, but it eliminates UB by rebuilding the tree.
    virtual EvalResType rebuild(EvalCtx& ctx) = 0;

    virtual IRNodeKind getKind() { return IRNodeKind::MAX_EXPR_KIND; }
    virtual std::shared_ptr<Data> getValue();

  protected:
    std::shared_ptr<Data> value;

  private:
    //TODO: add complexity tracker
    /*
    uint32_t complexity;

    // Count of expression over all test program
    static uint32_t total_expr_count;
    // Count of expression per single test function
    static uint32_t func_expr_count;
    */
};

// Constant representation
class ConstantExpr : public Expr {
  public:
    explicit ConstantExpr(IRValue _value);
    IRNodeKind getKind() final { return IRNodeKind::CONST; }

    bool propagateType() final { return true; }
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::ostream& stream, std::string offset = "") final;

  private:
    //TODO: add various buffers to increase tests expressiveness
};

// Abstract class that represents access to all sorts of variables
class VarUseExpr : public Expr {
  public:
    explicit VarUseExpr(std::shared_ptr<Data> _val) : Expr(std::move(_val)) {}
    virtual void setValue(std::shared_ptr<Expr> _expr) = 0;
};

class ScalarVarUseExpr : public VarUseExpr {
  public:
    // No one is supposed to call this constructor directly.
    // It is left public in order to use std::make_shared
    explicit ScalarVarUseExpr(std::shared_ptr<Data> _val) : VarUseExpr(std::move(_val)) {}
    static std::shared_ptr<ScalarVarUseExpr> init(std::shared_ptr<Data> _val);
    IRNodeKind getKind() final { return IRNodeKind::SCALAR_VAR_USE; }

    void setValue(std::shared_ptr<Expr> _expr) final;

    bool propagateType() final { return true; }
    EvalResType evaluate(EvalCtx& ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::ostream& stream, std::string offset = "") final { stream << offset << value->getName(); };

  private:
    static std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<ScalarVarUseExpr>> scalar_var_use_set;
};

class IterUseExpr : public VarUseExpr {
  public:
    explicit IterUseExpr(std::shared_ptr<Data> _val) : VarUseExpr(std::move(_val)) {}
    static std::shared_ptr<IterUseExpr> init(std::shared_ptr<Data> _iter);
    IRNodeKind getKind() final { return IRNodeKind::ITER_USE; }

    void setValue(std::shared_ptr<Expr> _expr) final;

    bool propagateType() final { return true; }
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::ostream& stream, std::string offset = "") final { stream << offset << value->getName(); };

  private:
    static std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<IterUseExpr>> iter_use_set;
};

class TypeCastExpr : public Expr {
  public:
    TypeCastExpr(std::shared_ptr<Expr> _expr, std::shared_ptr<Type> _to_type, bool _is_implicit);
    IRNodeKind getKind() final { return IRNodeKind::TYPE_CAST; }

    bool propagateType() final { return true; }
    // We assume that if we cast between compatible types we can't cause UB.
    EvalResType evaluate(EvalCtx &ctx) final { return value; }
    EvalResType rebuild(EvalCtx &ctx) final { return value; }

    void emit(std::ostream& stream, std::string offset = "") final;

  private:
    std::shared_ptr<Expr> expr;
    std::shared_ptr<Type> to_type;
    bool is_implicit;
};

class ArithmeticExpr : public Expr {
  public:
    explicit ArithmeticExpr(std::shared_ptr<Data> value) : Expr(std::move(value)) {}
    ArithmeticExpr() = default;

  protected:
    // Top-level recursive function for expression tree generation
    // static std::shared_ptr<Expr> gen_level (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, uint32_t par_depth);

    std::shared_ptr<Expr> integralProm(std::shared_ptr<Expr> arg);
    std::shared_ptr<Expr> convToBool(std::shared_ptr<Expr> arg);
};

class BinaryExpr : public ArithmeticExpr {
  public:
    BinaryExpr (std::shared_ptr<Expr> _lhs, std::shared_ptr<Expr> _rhs, BinaryOp _op) :
        op(_op), lhs(std::move(_lhs)), rhs(std::move(_rhs)) {}
    IRNodeKind getKind() final { return IRNodeKind::BINARY; }

    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::ostream& stream, std::string offset = "") final;

  private:
    void arithConv();

    BinaryOp op;
    std::shared_ptr<Expr> lhs;
    std::shared_ptr<Expr> rhs;
};
}
