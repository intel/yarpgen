/*
Copyright (c) 2015-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

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

#include <deque>
#include <map>
#include <memory>
#include <utility>

#include "data.h"
#include "ir_node.h"
#include "ir_value.h"

namespace yarpgen {

class EvalCtx;
class PopulateCtx;

// Common ancestor for all classes that represent various expressions
class Expr : public IRNode {
  public:
    explicit Expr(std::shared_ptr<Data> _value) : value(std::move(_value)) {}
    Expr() = default;

    // This type represent result of computation. We keep it simple for now,
    // but it might change in the future.
    using EvalResType = DataType;

    // This function does type conversions required by the language standard
    // (implicit cast, integral promotion or usual arithmetic conversions) to
    // existing child nodes. As a result, it inserts required TypeCastExpr
    // between existing child nodes and current node.
    // TODO: do we care about CV-qualifiers?
    virtual bool propagateType() = 0;

    // This function calculates value of current node, based on its child nodes.
    // Also it detects UB (for more information, see rebuild() method
    // in inherited classes). It requires propagate_type() to be called first.
    virtual EvalResType evaluate(EvalCtx &ctx) = 0;

    // Similar to evaluate method, but it eliminates UB by rebuilding the tree.
    virtual EvalResType rebuild(EvalCtx &ctx) = 0;

    virtual IRNodeKind getKind() { return IRNodeKind::MAX_EXPR_KIND; }
    virtual std::shared_ptr<Data> getValue();

  protected:
    std::shared_ptr<Data> value;

  private:
    // TODO: add complexity tracker
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

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<ConstantExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  private:
    static std::vector<std::shared_ptr<ConstantExpr>> used_consts;
};

// Abstract class that represents access to all sorts of variables
class VarUseExpr : public Expr {
  public:
    explicit VarUseExpr(std::shared_ptr<Data> _val) : Expr(std::move(_val)) {}
    void setIsDead(bool val) { value->setIsDead(val); }
};

class ScalarVarUseExpr : public VarUseExpr {
  public:
    // No one is supposed to call this constructor directly.
    // It is left public in order to use std::make_shared
    explicit ScalarVarUseExpr(std::shared_ptr<Data> _val)
        : VarUseExpr(std::move(_val)) {}
    static std::shared_ptr<ScalarVarUseExpr> init(std::shared_ptr<Data> _val);
    IRNodeKind getKind() final { return IRNodeKind::SCALAR_VAR_USE; }

    void setValue(std::shared_ptr<Expr> _expr);

    bool propagateType() final { return true; }
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final {
        stream << offset << value->getName(ctx);
    };
    static std::shared_ptr<ScalarVarUseExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  private:
    static std::unordered_map<std::shared_ptr<Data>,
                              std::shared_ptr<ScalarVarUseExpr>>
        scalar_var_use_set;
};

class ArrayUseExpr : public VarUseExpr {
  public:
    explicit ArrayUseExpr(std::shared_ptr<Data> _val)
        : VarUseExpr(std::move(_val)) {}
    static std::shared_ptr<ArrayUseExpr> init(std::shared_ptr<Data> _val);
    IRNodeKind getKind() final { return IRNodeKind::ARRAY_USE; }

    void setValue(std::shared_ptr<Expr> _expr, std::deque<size_t> &span,
                  std::deque<size_t> &steps);

    bool propagateType() final { return true; }
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final {
        stream << offset << value->getName(ctx);
    };

  private:
    static std::unordered_map<std::shared_ptr<Data>,
                              std::shared_ptr<ArrayUseExpr>>
        array_use_set;
};

class IterUseExpr : public VarUseExpr {
  public:
    explicit IterUseExpr(std::shared_ptr<Data> _val)
        : VarUseExpr(std::move(_val)) {}
    static std::shared_ptr<IterUseExpr> init(std::shared_ptr<Data> _iter);
    IRNodeKind getKind() final { return IRNodeKind::ITER_USE; }

    void setValue(std::shared_ptr<Expr> _expr);

    bool propagateType() final { return true; }
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final {
        stream << offset << value->getName(ctx);
    };

  private:
    static std::unordered_map<std::shared_ptr<Data>,
                              std::shared_ptr<IterUseExpr>>
        iter_use_set;
};

class TypeCastExpr : public Expr {
  public:
    TypeCastExpr(std::shared_ptr<Expr> _expr, std::shared_ptr<Type> _to_type,
                 bool _is_implicit);
    IRNodeKind getKind() final { return IRNodeKind::TYPE_CAST; }

    bool propagateType() final;
    // We assume that if we cast between compatible types we can't cause UB.
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<TypeCastExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  private:
    std::shared_ptr<Expr> expr;
    std::shared_ptr<Type> to_type;
    bool is_implicit;
};

class ArithmeticExpr : public Expr {
  public:
    explicit ArithmeticExpr(std::shared_ptr<Data> value)
        : Expr(std::move(value)) {}
    ArithmeticExpr() = default;

    static std::shared_ptr<Expr> create(std::shared_ptr<PopulateCtx> ctx);

  protected:
    std::shared_ptr<Expr> integralProm(std::shared_ptr<Expr> arg);
    std::shared_ptr<Expr> convToBool(std::shared_ptr<Expr> arg);
    static void arithConv(std::shared_ptr<Expr> &lhs,
                          std::shared_ptr<Expr> &rhs);
    static void varyingPromotion(std::shared_ptr<Expr> &lhs,
                                 std::shared_ptr<Expr> &rhs);
};

class UnaryExpr : public ArithmeticExpr {
  public:
    UnaryExpr(UnaryOp _op, std::shared_ptr<Expr> _expr);
    IRNodeKind getKind() final { return IRNodeKind::UNARY; }

    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<UnaryExpr> create(std::shared_ptr<PopulateCtx> ctx);

  private:
    UnaryOp op;
    std::shared_ptr<Expr> arg;
};

class BinaryExpr : public ArithmeticExpr {
  public:
    BinaryExpr(BinaryOp _op, std::shared_ptr<Expr> _lhs,
               std::shared_ptr<Expr> _rhs);
    IRNodeKind getKind() final { return IRNodeKind::BINARY; }

    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<BinaryExpr> create(std::shared_ptr<PopulateCtx> ctx);

  private:
    BinaryOp op;
    std::shared_ptr<Expr> lhs;
    std::shared_ptr<Expr> rhs;
};

class TernaryExpr : public ArithmeticExpr {
  public:
    TernaryExpr(std::shared_ptr<Expr> _cond, std::shared_ptr<Expr> _true_br,
                std::shared_ptr<Expr> _false_br);
    IRNodeKind getKind() final { return IRNodeKind::TERNARY; }

    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<TernaryExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  private:
    std::shared_ptr<Expr> cond;
    std::shared_ptr<Expr> true_br;
    std::shared_ptr<Expr> false_br;
};

class ArrayStencilParams;

class SubscriptExpr : public Expr {
  public:
    SubscriptExpr(std::shared_ptr<Expr> _arr, std::shared_ptr<Expr> _idx);
    IRNodeKind getKind() final { return IRNodeKind::SUBSCRIPT; }

    size_t getActiveDim() { return active_dim; }

    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<SubscriptExpr>
    init(std::shared_ptr<Array> arr, std::shared_ptr<PopulateCtx> ctx);
    static std::vector<std::shared_ptr<Array>>
    getSuitableArrays(std::shared_ptr<PopulateCtx> ctx);
    static std::shared_ptr<SubscriptExpr>
    create(std::shared_ptr<PopulateCtx> ctx);
    void setValue(std::shared_ptr<Expr> _expr, std::deque<size_t> &span,
                  std::deque<size_t> &steps);

    void setIsDead(bool val);

  private:
    static std::shared_ptr<SubscriptExpr>
    initImpl(ArrayStencilParams array_params, std::shared_ptr<PopulateCtx> ctx);
    bool inBounds(size_t dim, std::shared_ptr<Data> idx_val, EvalCtx &ctx);

    void setOffset(int64_t _offset) { stencil_offset = _offset; }
    int64_t getOffset() { return stencil_offset; }

    std::shared_ptr<Expr> array;
    std::shared_ptr<Expr> idx;
    size_t active_dim;
    // Auxiliary fields that prevents double computation
    size_t active_size;
    IntTypeID idx_int_type_id;
    // It is a hack for stencil
    int64_t stencil_offset;
};

class AssignmentExpr : public Expr {
  public:
    AssignmentExpr(std::shared_ptr<Expr> _to, std::shared_ptr<Expr> _from,
                   bool _taken = true)
        : to(std::move(_to)), from(std::move(_from)), taken(_taken) {}
    IRNodeKind getKind() final { return IRNodeKind::ASSIGN; }

    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<AssignmentExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  private:
    std::shared_ptr<Expr> to;
    std::shared_ptr<Expr> from;
    bool taken;
};

class CallExpr : public Expr {
  public:
    IRNodeKind getKind() final { return IRNodeKind::CALL; }

  private:
};

class LibCallExpr : public CallExpr {
  public:
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  protected:
    // Ternary operator need access to ispc promotion casts.
    // TODO: should we move them somewhere else?
    friend class TernaryExpr;

    // Utility functions to simplify type conversions
    // You should call propagateType on the arguments beforehand
    // CXX conversions should be performed before any other
    static IntTypeID getTopIntID(std::vector<std::shared_ptr<Expr>> args);
    static void cxxArgPromotion(std::shared_ptr<Expr> &arg, IntTypeID type_id);
    static bool isAnyArgVarying(std::vector<std::shared_ptr<Expr>> args);
    static void ispcArgPromotion(std::shared_ptr<Expr> &arg);
};

class MinMaxCallBase : public LibCallExpr {
  public:
    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final {
        a->rebuild(ctx);
        b->rebuild(ctx);
        return evaluate(ctx);
    }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") override;

  protected:
    MinMaxCallBase(std::shared_ptr<Expr> _a, std::shared_ptr<Expr> _b,
                   LibCallKind _kind);
    static std::shared_ptr<LibCallExpr>
    createHelper(std::shared_ptr<PopulateCtx> ctx, LibCallKind kind);
    static void emitCDefinitionImpl(std::shared_ptr<EmitCtx> ctx,
                                    std::ostream &stream, std::string offset,
                                    LibCallKind kind);
    std::shared_ptr<Expr> a;
    std::shared_ptr<Expr> b;
    LibCallKind kind;
};

class MinCall : public MinMaxCallBase {
  public:
    MinCall(std::shared_ptr<Expr> _a, std::shared_ptr<Expr> _b)
        : MinMaxCallBase(std::move(_a), std::move(_b), LibCallKind::MIN) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return createHelper(std::move(ctx), LibCallKind::MIN);
    }
    static void emitCDefinition(std::shared_ptr<EmitCtx> ctx,
                                std::ostream &stream, std::string offset = "") {
        emitCDefinitionImpl(ctx, stream, offset, LibCallKind::MAX);
    }
};

class MaxCall : public MinMaxCallBase {
  public:
    MaxCall(std::shared_ptr<Expr> _a, std::shared_ptr<Expr> _b)
        : MinMaxCallBase(std::move(_a), std::move(_b), LibCallKind::MAX) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return createHelper(std::move(ctx), LibCallKind::MAX);
    }
    static void emitCDefinition(std::shared_ptr<EmitCtx> ctx,
                                std::ostream &stream, std::string offset = "") {
        emitCDefinitionImpl(ctx, stream, offset, LibCallKind::MIN);
    }
};

class SelectCall : public LibCallExpr {
  public:
    SelectCall(std::shared_ptr<Expr> _cond, std::shared_ptr<Expr> _true_arg,
               std::shared_ptr<Expr> _false_arg);
    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final;
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  private:
    std::shared_ptr<Expr> cond;
    std::shared_ptr<Expr> true_arg;
    std::shared_ptr<Expr> false_arg;
};

class LogicalReductionBase : public LibCallExpr {
  public:
    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final {
        arg->rebuild(ctx);
        return evaluate(ctx);
    }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;

  protected:
    LogicalReductionBase(std::shared_ptr<Expr> _arg, LibCallKind _kind);
    static std::shared_ptr<LibCallExpr>
    createHelper(std::shared_ptr<PopulateCtx> ctx, LibCallKind kind);
    std::shared_ptr<Expr> arg;
    LibCallKind kind;
};

class AnyCall : public LogicalReductionBase {
  public:
    explicit AnyCall(std::shared_ptr<Expr> _arg)
        : LogicalReductionBase(std::move(_arg), LibCallKind::ANY) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return LogicalReductionBase::createHelper(std::move(ctx),
                                                  LibCallKind::ANY);
    }
};

class AllCall : public LogicalReductionBase {
  public:
    explicit AllCall(std::shared_ptr<Expr> _arg)
        : LogicalReductionBase(std::move(_arg), LibCallKind::ALL) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return LogicalReductionBase::createHelper(std::move(ctx),
                                                  LibCallKind::ALL);
    }
};

class NoneCall : public LogicalReductionBase {
  public:
    explicit NoneCall(std::shared_ptr<Expr> _arg)
        : LogicalReductionBase(std::move(_arg), LibCallKind::NONE) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return LogicalReductionBase::createHelper(std::move(ctx),
                                                  LibCallKind::NONE);
    }
};

class MinMaxEqReductionBase : public LibCallExpr {
  private:
    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final {
        arg->rebuild(ctx);
        return evaluate(ctx);
    }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;

  protected:
    MinMaxEqReductionBase(std::shared_ptr<Expr> _arg, LibCallKind _kind);
    static std::shared_ptr<LibCallExpr>
    createHelper(std::shared_ptr<PopulateCtx> ctx, LibCallKind kind);
    std::shared_ptr<Expr> arg;
    LibCallKind kind;
};

class ReduceMinCall : public MinMaxEqReductionBase {
  public:
    ReduceMinCall(std::shared_ptr<Expr> _arg)
        : MinMaxEqReductionBase(std::move(_arg), LibCallKind::RED_MIN) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return MinMaxEqReductionBase::createHelper(std::move(ctx),
                                                   LibCallKind::RED_MIN);
    }
};

class ReduceMaxCall : public MinMaxEqReductionBase {
  public:
    ReduceMaxCall(std::shared_ptr<Expr> _arg)
        : MinMaxEqReductionBase(std::move(_arg), LibCallKind::RED_MAX) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return MinMaxEqReductionBase::createHelper(std::move(ctx),
                                                   LibCallKind::RED_MAX);
    }
};

class ReduceEqCall : public MinMaxEqReductionBase {
  public:
    ReduceEqCall(std::shared_ptr<Expr> _arg)
        : MinMaxEqReductionBase(std::move(_arg), LibCallKind::RED_EQ) {}
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx) {
        return MinMaxEqReductionBase::createHelper(std::move(ctx),
                                                   LibCallKind::RED_EQ);
    }
};

class ExtractCall : public LibCallExpr {
    // TODO: it is not a real extract call. We will always use zero as an index
  public:
    ExtractCall(std::shared_ptr<Expr> _arg);
    bool propagateType() final;
    EvalResType evaluate(EvalCtx &ctx) final;
    EvalResType rebuild(EvalCtx &ctx) final {
        arg->rebuild(ctx);
        return evaluate(ctx);
    };
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<LibCallExpr>
    create(std::shared_ptr<PopulateCtx> ctx);

  protected:
    std::shared_ptr<Expr> arg;
    std::shared_ptr<Expr> idx;
};
} // namespace yarpgen
