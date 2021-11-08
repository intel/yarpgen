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

#include "expr.h"
#include "context.h"
#include "options.h"
#include <algorithm>
#include <deque>
#include <utility>

using namespace yarpgen;

std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<ScalarVarUseExpr>>
    yarpgen::ScalarVarUseExpr::scalar_var_use_set;
std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<ArrayUseExpr>>
    yarpgen::ArrayUseExpr::array_use_set;
std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<IterUseExpr>>
    yarpgen::IterUseExpr::iter_use_set;

std::shared_ptr<Data> Expr::getValue() {
    // TODO: it might cause some problems in the future, but it is good for now
    return value;
}

std::vector<std::shared_ptr<ConstantExpr>> yarpgen::ConstantExpr::used_consts;

ConstantExpr::ConstantExpr(IRValue _value) {
    // TODO: maybe we need a constant data type rather than an anonymous scalar
    // variable
    value = std::make_shared<ScalarVar>(
        "", IntegralType::init(_value.getIntTypeID()), _value);
}

Expr::EvalResType ConstantExpr::evaluate(EvalCtx &ctx) { return value; }

Expr::EvalResType ConstantExpr::rebuild(EvalCtx &ctx) { return evaluate(ctx); }

void ConstantExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                        std::string offset) {
    assert(value->isScalarVar() &&
           "ConstExpr can represent only scalar constant");
    auto scalar_var = std::static_pointer_cast<ScalarVar>(value);

    assert(scalar_var->getType()->isIntType() &&
           "ConstExpr can represent only scalar integral constant");
    auto int_type =
        std::static_pointer_cast<IntegralType>(scalar_var->getType());

    auto emit_helper = [&stream, &int_type, &ctx]() {
        if (int_type->getIntTypeId() < IntTypeID::INT)
            stream << "(" << int_type->getName(ctx) << ")";
    };

    IRValue val = scalar_var->getCurrentValue();
    IRValue min_val = int_type->getMin();
    IntTypeID max_type_id =
        int_type->getIsSigned() ? IntTypeID::LLONG : IntTypeID::ULLONG;
    val = val.castToType(max_type_id);
    min_val = min_val.castToType(max_type_id);
    if (!int_type->getIsSigned() || (val != min_val).getValueRef<bool>()) {
        emit_helper();
        stream << val << int_type->getLiteralSuffix();
        return;
    }

    // INT_MIN is defined as -INT_MAX - 1, so we have to do the same
    IRValue one(max_type_id);
    // TODO: this is not an appropriate way to do it
    one.setValue(IRValue::AbsValue{false, 1});
    IRValue min_one_val = min_val + one;
    emit_helper();
    stream << "(" << min_one_val << int_type->getLiteralSuffix() << " - " << one
           << int_type->getLiteralSuffix() << ")";
}

std::shared_ptr<ConstantExpr>
ConstantExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    bool reuse_const = rand_val_gen->getRandId(gen_pol->reuse_const_prob);
    std::shared_ptr<ConstantExpr> ret;
    bool can_add_to_buf = true;
    bool can_use_offset = true;
    IntTypeID type_id;
    std::shared_ptr<IntegralType> int_type;
    if (reuse_const && !used_consts.empty()) {
        bool use_transformation =
            rand_val_gen->getRandId(gen_pol->use_const_transform_distr);
        ret = rand_val_gen->getRandElem(used_consts);
        can_add_to_buf = use_transformation;
        assert(ret->getKind() == IRNodeKind::CONST &&
               "Buffer of used constants should contain only constants");
        auto scalar_val = std::static_pointer_cast<ScalarVar>(ret->getValue());
        int_type =
            std::static_pointer_cast<IntegralType>(scalar_val->getType());
        type_id = int_type->getIntTypeId();
        if (use_transformation) {
            UnaryOp transformation =
                rand_val_gen->getRandId(gen_pol->const_transform_distr);
            IRValue ir_val = scalar_val->getCurrentValue();

            IntTypeID active_type_id = type_id;
            // TODO: do we need to cast to unsigned also?
            if (type_id < IntTypeID::INT) {
                if (int_type->getIsSigned())
                    active_type_id = IntTypeID::INT;
                else
                    active_type_id = IntTypeID::UINT;
                ir_val = ir_val.castToType(active_type_id);
            }

            auto active_int_type = IntegralType::init(active_type_id);
            if (transformation == UnaryOp::BIT_NOT ||
                (transformation == UnaryOp::NEGATE &&
                 (ir_val == active_int_type->getMin()).getValueRef<bool>()))
                ir_val = ~ir_val;
            else if (transformation == UnaryOp::NEGATE)
                ir_val = -ir_val;
            else
                ERROR("Unsupported constant transformation");

            if (type_id < IntTypeID::INT)
                ir_val = ir_val.castToType(type_id);

            ret = std::make_shared<ConstantExpr>(ir_val);
        }
    }
    else {
        bool use_special_const =
            rand_val_gen->getRandId(gen_pol->use_special_const_distr);
        type_id = rand_val_gen->getRandId(gen_pol->int_type_distr);
        int_type = IntegralType::init(type_id);
        IRValue init_val(type_id);
        if (use_special_const) {
            SpecialConst special_const_kind =
                rand_val_gen->getRandId(gen_pol->special_const_distr);

            // Utility function for EndBits and BitBlock
            auto fill_bits = [](size_t start, size_t end) -> uint64_t {
                assert(end >= start &&
                       "Ends should be sorted in increasing order");
                return (end - start) == 63
                           ? UINT64_MAX
                           : ((1ULL << (end - start + 1)) - 1ULL) << start;
            };

            if (special_const_kind == SpecialConst::ZERO)
                init_val.setValue(IRValue::AbsValue{false, 0});
            else if (special_const_kind == SpecialConst::MIN)
                init_val = int_type->getMin();
            else if (special_const_kind == SpecialConst::MAX)
                init_val = int_type->getMax();
            else if (special_const_kind == SpecialConst::BIT_BLOCK) {
                size_t start = rand_val_gen->getRandValue(
                    static_cast<size_t>(0), int_type->getBitSize() - 1);
                size_t end = rand_val_gen->getRandValue(
                    start, int_type->getBitSize() - 1);
                init_val.setValue(
                    IRValue::AbsValue{false, fill_bits(start, end)});
                // TODO: does it make sense?
                can_use_offset = false;
            }
            else if (special_const_kind == SpecialConst::END_BITS) {
                size_t bit_idx = rand_val_gen->getRandValue(
                    static_cast<size_t>(0), int_type->getBitSize() - 1);
                bool use_lsb_end =
                    rand_val_gen->getRandId(gen_pol->use_lsb_bit_end_distr);
                if (use_lsb_end)
                    init_val.setValue(
                        IRValue::AbsValue{false, fill_bits(0, bit_idx)});
                else
                    init_val.setValue(IRValue::AbsValue{
                        false, fill_bits(0, int_type->getBitSize() - 1)});
                // TODO: does it make sense?
                can_use_offset = false;
            }
            else
                ERROR("Bad special const kind");
        }
        else
            init_val = rand_val_gen->getRandValue(type_id);

        ret = std::make_shared<ConstantExpr>(init_val);
    }

    bool use_offset = rand_val_gen->getRandId(gen_pol->use_const_offset_distr);
    if (can_use_offset && use_offset) {
        IRValue ir_val = std::static_pointer_cast<ScalarVar>(ret->getValue())
                             ->getCurrentValue();
        IRValue offset(type_id);
        if (type_id < IntTypeID::INT) {
            // TODO: do we need to cast to unsigned also?
            if (int_type->getIsSigned()) {
                offset = offset.castToType(IntTypeID::INT);
                ir_val = ir_val.castToType(IntTypeID::INT);
            }
            else {
                offset = offset.castToType(IntTypeID::UINT);
                ir_val = ir_val.castToType(IntTypeID::UINT);
            }
        }

        size_t offset_size =
            rand_val_gen->getRandId(gen_pol->const_offset_distr);
        offset.setValue(IRValue::AbsValue{false, offset_size});

        bool pos_offset =
            rand_val_gen->getRandId(gen_pol->pos_const_offset_distr);
        if (pos_offset)
            ir_val = ir_val + offset;
        else
            ir_val = ir_val - offset;

        if (type_id < IntTypeID::INT)
            ir_val = ir_val.castToType(type_id);

        if (!ir_val.hasUB()) {
            ret = std::make_shared<ConstantExpr>(ir_val);
            can_add_to_buf = true;
        }
    }

    bool replace_in_buf =
        rand_val_gen->getRandId(gen_pol->replace_in_buf_distr);
    // If we are inside mutation, we can't change the buffer. Otherwise,
    // this will affect the state of the random generator outside of the mutated
    // region
    if (!ctx->isInsideMutation() && can_add_to_buf && replace_in_buf) {
        if (used_consts.size() < gen_pol->const_buf_size)
            used_consts.push_back(ret);
        else {
            auto replaced_const = rand_val_gen->getRandElem(used_consts);
            replaced_const = ret;
        }
    }

    return ret;
}

std::shared_ptr<ScalarVarUseExpr>
ScalarVarUseExpr::init(std::shared_ptr<Data> _val) {
    assert(_val->isScalarVar() &&
           "ScalarVarUseExpr accepts only scalar variables!");
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

    std::static_pointer_cast<ScalarVar>(value)->setCurrentValue(
        std::static_pointer_cast<ScalarVar>(new_val)->getCurrentValue());
}

Expr::EvalResType ScalarVarUseExpr::evaluate(EvalCtx &ctx) {
    // This variable is defined and we can just return it.
    auto find_res = ctx.input.find(value->getName(EmitCtx::default_emit_ctx));
    if (find_res != ctx.input.end()) {
        return find_res->second;
    }
    return value;
}

Expr::EvalResType ScalarVarUseExpr::rebuild(EvalCtx &ctx) {
    return evaluate(ctx);
}

std::shared_ptr<ScalarVarUseExpr>
ScalarVarUseExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto avail_vars = ctx->getExtInpSymTable()->getAvailVars();
    return rand_val_gen->getRandElem(avail_vars);
}

std::shared_ptr<ArrayUseExpr> ArrayUseExpr::init(std::shared_ptr<Data> _val) {
    assert(_val->isArray() &&
           "ArrayUseExpr can be initialized only with Arrays");
    auto find_res = array_use_set.find(_val);
    if (find_res != array_use_set.end())
        return find_res->second;

    auto ret = std::make_shared<ArrayUseExpr>(_val);
    array_use_set[_val] = ret;
    return ret;
}

void ArrayUseExpr::setValue(std::shared_ptr<Expr> _expr,
                            std::deque<size_t> &span,
                            std::deque<size_t> &steps) {
    /*
    std::shared_ptr<Data> new_val = _expr->getValue();
    assert(new_val->isArray() && "ArrayUseExpr can store only Arrays");
    auto new_array = std::static_pointer_cast<Array>(new_val);
    if (value->getType() != new_array->getType()) {
        ERROR("Can't assign incompatible types");
    }
    */
    auto arr_val = std::static_pointer_cast<Array>(value);
    if (!_expr->getValue()->isScalarVar())
        ERROR("Only scalar variables are supported for now");
    auto expr_scalar_var =
        std::static_pointer_cast<ScalarVar>(_expr->getValue());
    arr_val->setValue(expr_scalar_var->getCurrentValue(), span, steps);
}

Expr::EvalResType ArrayUseExpr::evaluate(EvalCtx &ctx) {
    // This Array is defined and we can just return it.
    auto find_res = ctx.input.find(value->getName(EmitCtx::default_emit_ctx));
    if (find_res != ctx.input.end()) {
        return find_res->second;
    }

    return value;
}

Expr::EvalResType ArrayUseExpr::rebuild(EvalCtx &ctx) { return evaluate(ctx); }

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

    std::static_pointer_cast<Iterator>(value)->setParameters(
        new_iter->getStart(), new_iter->getEnd(), new_iter->getStep());
}

Expr::EvalResType IterUseExpr::evaluate(EvalCtx &ctx) {
    // This iterator is defined and we can just return it.
    auto find_res = ctx.input.find(value->getName(EmitCtx::default_emit_ctx));
    if (find_res != ctx.input.end()) {
        return find_res->second;
    }

    return value;
}

Expr::EvalResType IterUseExpr::rebuild(EvalCtx &ctx) { return evaluate(ctx); }

TypeCastExpr::TypeCastExpr(std::shared_ptr<Expr> _expr,
                           std::shared_ptr<Type> _to_type, bool _is_implicit)
    : expr(std::move(_expr)), to_type(std::move(_to_type)),
      is_implicit(_is_implicit) {
    propagateType();
}

bool TypeCastExpr::propagateType() {
    assert(to_type->isIntType() && "We can cast only integral types for now");
    auto to_int_type = std::static_pointer_cast<IntegralType>(to_type);
    value = std::make_shared<ScalarVar>("", to_int_type,
                                        IRValue(to_int_type->getIntTypeId()));
    return true;
}

void TypeCastExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                        std::string offset) {
    // TODO: add switch for C++ style conversions and switch for implicit casts
    stream << "((" << (is_implicit ? "/* implicit */" : "")
           << to_type->getName(ctx) << ") ";
    expr->emit(ctx, stream);
    stream << ")";
}

std::shared_ptr<TypeCastExpr>
TypeCastExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    // TODO: we might want to create TypeCastExpr not only to integer types
    IntTypeID to_type = rand_val_gen->getRandId(gen_pol->int_type_distr);
    auto expr = ArithmeticExpr::create(ctx);

    Options &options = Options::getInstance();
    bool is_uniform = true;
    if (options.isISPC()) {
        EvalCtx eval_ctx;
        EvalResType expr_val = expr->evaluate(eval_ctx);
        is_uniform = expr_val->getType()->isUniform();
    }

    return std::make_shared<TypeCastExpr>(
        expr, IntegralType::init(to_type, false, CVQualifier::NONE, is_uniform),
        /*is_implicit*/ false);
}

Expr::EvalResType TypeCastExpr::evaluate(EvalCtx &ctx) {
    EvalResType expr_eval_res = expr->evaluate(ctx);
    std::shared_ptr<Type> base_type = expr_eval_res->getType();
    // Check that we try to convert between compatible types.
    if (!((base_type->isIntType() && to_type->isIntType()) ||
          (base_type->isArrayType() && to_type->isArrayType()))) {
        ERROR("Can't create TypeCastExpr for types that can't be casted");
    }

    if (base_type->isIntType() && expr_eval_res->isScalarVar()) {
        std::shared_ptr<IntegralType> to_int_type =
            std::static_pointer_cast<IntegralType>(to_type);
        auto scalar_val = std::make_shared<ScalarVar>(
            "", to_int_type, IRValue(to_int_type->getIntTypeId()));
        std::shared_ptr<ScalarVar> base_scalar_var =
            std::static_pointer_cast<ScalarVar>(expr_eval_res);
        scalar_val->setCurrentValue(
            base_scalar_var->getCurrentValue().castToType(
                to_int_type->getIntTypeId()));

        Options &options = Options::getInstance();
        if (options.isISPC()) {
            if (to_int_type->isUniform() && !base_type->isUniform())
                ERROR("Can't cast varying to uniform");
        }

        value = scalar_val;
    }
    else {
        // TODO: extend it
        ERROR("We can cast only integer scalar variables for now");
    }
    return value;
}

Expr::EvalResType TypeCastExpr::rebuild(EvalCtx &ctx) {
    propagateType();
    expr->rebuild(ctx);
    std::shared_ptr<Data> eval_res = evaluate(ctx);
    assert(eval_res->getKind() == DataKind::VAR &&
           "Type Cast operations are only supported for Scalar Variables");

    auto eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
    if (!eval_scalar_res->getCurrentValue().hasUB()) {
        value = eval_res;
        return eval_res;
    }

    do {
        eval_res = evaluate(ctx);
        eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
        if (!eval_scalar_res->getCurrentValue().hasUB())
            break;
        rebuild(ctx);
    } while (eval_scalar_res->getCurrentValue().hasUB());

    value = eval_res;
    return value;
}

std::shared_ptr<Expr> ArithmeticExpr::integralProm(std::shared_ptr<Expr> arg) {
    if (!arg->getValue()->isScalarVar()) {
        ERROR("Can perform integral promotion only on scalar variables");
    }

    // C++ draft N4713: 7.6 Integral promotions [conv.prom]
    assert(arg->getValue()->getType()->isIntType() &&
           "Scalar variable can have only Integral Type");
    std::shared_ptr<IntegralType> int_type =
        std::static_pointer_cast<IntegralType>(arg->getValue()->getType());
    if (int_type->getIntTypeId() >=
        IntTypeID::INT) // can't perform integral promotion
        return arg;
    // TODO: we need to check if type fits in int or unsigned int
    return std::make_shared<TypeCastExpr>(
        arg,
        IntegralType::init(IntTypeID::INT, false, CVQualifier::NONE,
                           arg->getValue()->getType()->isUniform()),
        true);
}

std::shared_ptr<Expr> ArithmeticExpr::convToBool(std::shared_ptr<Expr> arg) {
    if (!arg->getValue()->isScalarVar()) {
        ERROR("Can perform conversion to bool only on scalar variables");
    }

    std::shared_ptr<IntegralType> int_type =
        std::static_pointer_cast<IntegralType>(arg->getValue()->getType());
    if (int_type->getIntTypeId() == IntTypeID::BOOL)
        return arg;
    return std::make_shared<TypeCastExpr>(
        arg,
        IntegralType::init(IntTypeID::BOOL, false, CVQualifier::NONE,
                           arg->getValue()->getType()->isUniform()),
        true);
}

void ArithmeticExpr::arithConv(std::shared_ptr<Expr> &lhs,
                               std::shared_ptr<Expr> &rhs) {
    if (!lhs->getValue()->getType()->isIntType() ||
        !rhs->getValue()->getType()->isIntType()) {
        ERROR("We assume that we can perform binary operations only in Scalar "
              "Variables with integral type");
    }

    auto lhs_type =
        std::static_pointer_cast<IntegralType>(lhs->getValue()->getType());
    auto rhs_type =
        std::static_pointer_cast<IntegralType>(rhs->getValue()->getType());

    // C++ draft N4713: 8.3 Usual arithmetic conversions [expr.arith.conv]
    // 1.5.1
    if (lhs_type->getIntTypeId() == rhs_type->getIntTypeId())
        return;

    // 1.5.2
    if (lhs_type->getIsSigned() == rhs_type->getIsSigned()) {
        std::shared_ptr<IntegralType> max_type =
            lhs_type->getIntTypeId() > rhs_type->getIntTypeId() ? lhs_type
                                                                : rhs_type;
        if (lhs_type->getIntTypeId() > rhs_type->getIntTypeId())
            rhs = std::make_shared<TypeCastExpr>(rhs, max_type,
                                                 /*is_implicit*/ true);
        else
            lhs = std::make_shared<TypeCastExpr>(lhs, max_type,
                                                 /*is_implicit*/ true);
        return;
    }

    // 1.5.3
    // Helper function that converts signed type to "bigger" unsigned type
    auto signed_to_unsigned_conv = [](std::shared_ptr<IntegralType> &a_type,
                                      std::shared_ptr<IntegralType> &b_type,
                                      std::shared_ptr<Expr> &b_expr) -> bool {
        if (!a_type->getIsSigned() &&
            (a_type->getIntTypeId() >= b_type->getIntTypeId())) {
            b_expr = std::make_shared<TypeCastExpr>(b_expr, a_type,
                                                    /*is_implicit*/ true);
            return true;
        }
        return false;
    };
    if (signed_to_unsigned_conv(lhs_type, rhs_type, rhs) ||
        signed_to_unsigned_conv(rhs_type, lhs_type, lhs))
        return;

    // 1.5.4
    // Same idea, but for unsigned to signed conversions
    auto unsigned_to_signed_conv = [](std::shared_ptr<IntegralType> &a_type,
                                      std::shared_ptr<IntegralType> &b_type,
                                      std::shared_ptr<Expr> &b_expr) -> bool {
        if (a_type->getIsSigned() &&
            IntegralType::canRepresentType(a_type->getIntTypeId(),
                                           b_type->getIntTypeId())) {
            b_expr = std::make_shared<TypeCastExpr>(b_expr, a_type,
                                                    /*is_implicit*/ true);
            return true;
        }
        return false;
    };
    if (unsigned_to_signed_conv(lhs_type, rhs_type, rhs) ||
        unsigned_to_signed_conv(rhs_type, lhs_type, lhs))
        return;

    // 1.5.5
    auto final_conversion = [](std::shared_ptr<IntegralType> &a_type,
                               std::shared_ptr<Expr> &a_expr,
                               std::shared_ptr<Expr> &b_expr) -> bool {
        if (a_type->getIsSigned()) {
            std::shared_ptr<IntegralType> new_type = IntegralType::init(
                IntegralType::getCorrUnsigned(a_type->getIntTypeId()));
            if (!a_type->isUniform())
                new_type = std::static_pointer_cast<IntegralType>(
                    new_type->makeVarying());
            a_expr = std::make_shared<TypeCastExpr>(a_expr, new_type,
                                                    /*is_implicit*/ true);
            b_expr = std::make_shared<TypeCastExpr>(b_expr, new_type,
                                                    /*is_implicit*/ true);
            return true;
        }
        return false;
    };
    if (final_conversion(lhs_type, lhs, rhs) ||
        final_conversion(rhs_type, lhs, rhs))
        return;

    ERROR("Unreachable: conversions went wrong");
}

void ArithmeticExpr::varyingPromotion(std::shared_ptr<Expr> &lhs,
                                      std::shared_ptr<Expr> &rhs) {
    auto lhs_type = lhs->getValue()->getType();
    auto rhs_type = rhs->getValue()->getType();

    auto varying_conversion = [](std::shared_ptr<Type> &a_type,
                                 std::shared_ptr<Type> &b_type,
                                 std::shared_ptr<Expr> &b_expr) -> bool {
        if (!a_type->isUniform() && b_type->isUniform()) {
            auto new_type = b_type->makeVarying();
            b_expr = std::make_shared<TypeCastExpr>(b_expr, new_type,
                                                    /*is_implicit*/ true);
            return true;
        }
        return false;
    };

    if (varying_conversion(lhs_type, rhs_type, rhs) ||
        varying_conversion(rhs_type, lhs_type, lhs))
        return;
}

static std::shared_ptr<Expr> createStencil(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    std::shared_ptr<Expr> new_node;
    // Disable other kind of leaf exprs
    // TODO: This is not the best option
    std::vector<Probability<IRNodeKind>> new_node_distr;
    auto find_prob = [&gen_pol](IRNodeKind kind) -> uint64_t {
        auto find_res = std::find_if(
            gen_pol->arith_node_distr.begin(), gen_pol->arith_node_distr.end(),
            [kind](Probability<IRNodeKind> prob) -> bool {
                return prob.getId() == kind;
            });
        return (find_res != gen_pol->arith_node_distr.end())
                   ? find_res->getProb()
                   : 0;
    };
    uint64_t stencil_prob = find_prob(IRNodeKind::STENCIL);
    uint64_t scalar_var_prob = find_prob(IRNodeKind::SCALAR_VAR_USE);
    uint64_t subscript_prob = find_prob(IRNodeKind::SUBSCRIPT);
    uint64_t const_prob = find_prob(IRNodeKind::CONST);

    uint64_t total_prob =
        stencil_prob + scalar_var_prob + subscript_prob + const_prob;

    subscript_prob = static_cast<uint64_t>(
        total_prob * gen_pol->stencil_prob_weight_alternation);
    const_prob = scalar_var_prob = static_cast<uint64_t>(
        total_prob * (1 - gen_pol->stencil_prob_weight_alternation) * 0.5);

    for (auto &item : gen_pol->arith_node_distr) {
        Probability<IRNodeKind> prob = item;
        if (item.getId() == IRNodeKind::SCALAR_VAR_USE)
            prob.setProb(scalar_var_prob);
        else if (item.getId() == IRNodeKind::CONST)
            prob.setProb(const_prob);
        else if (item.getId() == IRNodeKind::SUBSCRIPT)
            prob.setProb(subscript_prob);
        else if (item.getId() == IRNodeKind::STENCIL)
            continue;
        new_node_distr.push_back(prob);
    }
    auto new_gen_pol = std::make_shared<GenPolicy>(*gen_pol);
    new_gen_pol->arith_node_distr = new_node_distr;
    auto new_ctx = std::make_shared<PopulateCtx>(*ctx);
    new_ctx->setGenPolicy(new_gen_pol);

    // Pick arrays for stencil
    auto avail_arrays = SubscriptExpr::getSuitableArrays(new_ctx);
    size_t arrs_in_stencil_num =
        rand_val_gen->getRandId(gen_pol->arrs_in_stencil_distr);
    auto used_arrays =
        rand_val_gen->getRandElems(avail_arrays, arrs_in_stencil_num);

    std::vector<ArrayStencilParams> stencils;
    stencils.reserve(arrs_in_stencil_num);
    for (auto &i : used_arrays) {
        stencils.emplace_back(i);
    }

    // Pick dimensions
    // We need to process only special cases here. If each array use have its
    // own set of offsets, then they will be generated later
    bool same_dims_each =
        rand_val_gen->getRandId(gen_pol->stencil_same_dims_one_arr_distr);
    bool same_dims_all =
        rand_val_gen->getRandId(gen_pol->stencil_same_dims_all_distr);
    if (same_dims_each || same_dims_all) {
        for (auto &stencil : stencils) {
            assert(new_ctx->getDimensions().size() > 0 &&
                   "Can't create stencil in scalar context!");
            size_t num_of_dims_used = rand_val_gen->getRandValue(
                static_cast<size_t>(1), new_ctx->getDimensions().size());
            // We want to save information about iterator and
            // dimension's idx correspondence
            std::vector<std::pair<size_t, std::shared_ptr<Iterator>>> dims_idx;
            // We check if iterator can be used to create a
            // subscription expr with offset
            for (size_t i = 0; i < new_ctx->getDimensions().size(); ++i) {
                auto used_iter = new_ctx->getLocalSymTable()->getIters().at(i);
                size_t max_left_offset = used_iter->getMaxLeftOffset();
                size_t max_right_offset = used_iter->getMaxRightOffset();
                bool non_zero_offset_exists =
                    (max_left_offset > 0) || (max_right_offset > 0);
                if (non_zero_offset_exists)
                    dims_idx.emplace_back(i, used_iter);
            }
            auto chosen_dims =
                rand_val_gen->getRandElems(dims_idx, num_of_dims_used);

            // Then we sort iterators and suitable dimensions in order
            auto cmp_func =
                [](std::pair<size_t, std::shared_ptr<Iterator>> a,
                   std::pair<size_t, std::shared_ptr<Iterator>> b) -> bool {
                return a.first < b.first;
            };
            std::sort(chosen_dims.begin(), chosen_dims.end(), cmp_func);

            // We use binary vector to indicate if dimension can be used for
            // subscript expr with offset. Now we convert chosen active
            // dimensions indexes to such vector
            std::vector<std::pair<size_t, std::shared_ptr<Iterator>>>
                new_dims_params(new_ctx->getDimensions().size(),
                                std::make_pair(false, nullptr));
            for (const auto &chosen_dim : chosen_dims)
                new_dims_params.at(chosen_dim.first) =
                    std::make_pair(true, chosen_dim.second);

            // After that we split vector of pairs into binary vector and
            // iterators vector to add that info into stencil parameters
            std::vector<size_t> new_dims;
            std::vector<std::shared_ptr<Iterator>> new_iters;

            for (auto it = std::make_move_iterator(new_dims_params.begin()),
                      end = std::make_move_iterator(new_dims_params.end());
                 it != end; ++it) {
                new_dims.push_back(it->first);
                new_iters.push_back(std::move(it->second));
            }

            if (same_dims_all) {
                for (auto &item : stencils) {
                    item.setActiveDims(new_dims);
                    item.setIters(new_iters);
                }
                break;
            }
            // If same_dims_each is set is the only possible case
            stencil.setActiveDims(new_dims);
            stencil.setIters(new_iters);
        }
    }

    // Generate offsets for special cases
    bool reuse_offset =
        rand_val_gen->getRandId(gen_pol->stencil_reuse_offset_distr);
    // Here we process only special cases. General case is handled by each
    // subscription expr itself
    if (same_dims_all && reuse_offset) {
        std::vector<int64_t> used_offsets(new_ctx->getDimensions().size(), 0);
        auto &active_dims = stencils.front().getActiveDims();
        for (size_t i = 0; i < new_ctx->getDimensions().size(); ++i) {
            // All offsets are supposed to be the same, so we can pick the first
            // one
            if (!active_dims.at(i))
                continue;
            auto used_iter = new_ctx->getLocalSymTable()->getIters().at(i);
            size_t max_left_offset = used_iter->getMaxLeftOffset();
            size_t max_right_offset = used_iter->getMaxRightOffset();
            bool non_zero_offset_exists =
                (max_left_offset > 0) || (max_right_offset > 0);
            int64_t offset = 0;
            if (non_zero_offset_exists) {
                while (offset == 0)
                    offset = rand_val_gen->getRandValue(
                        -static_cast<int64_t>(max_left_offset),
                        static_cast<int64_t>(max_right_offset));
            }
            used_offsets.at(i) = offset;
        }
        for (auto &stencil : stencils)
            stencil.setOffsets(used_offsets);
    }

    new_ctx->getLocalSymTable()->setStencilsParams(stencils);

    new_ctx->setInStencil(true);
    new_node = ArithmeticExpr::create(new_ctx);
    new_ctx->setInStencil(false);
    return new_node;
}

std::shared_ptr<Expr> ArithmeticExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    std::shared_ptr<Expr> new_node;
    ctx->incArithDepth();
    auto active_ctx = std::make_shared<PopulateCtx>(*ctx);
    if (active_ctx->getArithDepth() == gen_pol->max_arith_depth) {
        // We can have only constants, variables and arrays as leaves
        std::vector<Probability<IRNodeKind>> new_node_distr;
        for (auto &item : gen_pol->arith_node_distr) {
            if (item.getId() == IRNodeKind::CONST ||
                item.getId() == IRNodeKind::SCALAR_VAR_USE ||
                item.getId() == IRNodeKind::SUBSCRIPT)
                new_node_distr.push_back(item);
        }

        bool zero_prob = true;
        for (auto &item : new_node_distr) {
            if (item.getProb())
                zero_prob = false;
        }

        // If after the option shuffling probability of all appropriate leaves
        // was set to zero, we need a backup-plan. We just bump it to some
        // value.
        if (zero_prob) {
            for (auto &item : new_node_distr) {
                item.increaseProb(GenPolicy::leaves_prob_bump);
            }
        }

        auto new_gen_policy = std::make_shared<GenPolicy>(*gen_pol);
        new_gen_policy->arith_node_distr = new_node_distr;
        active_ctx->setGenPolicy(new_gen_policy);
    }
    gen_pol = active_ctx->getGenPolicy();

    bool apply_similar_op =
        rand_val_gen->getRandId(gen_pol->apply_similar_op_distr);
    if (apply_similar_op) {
        auto new_gen_policy = std::make_shared<GenPolicy>(*gen_pol);
        gen_pol = new_gen_policy;
        gen_pol->chooseAndApplySimilarOp();
        active_ctx->setGenPolicy(gen_pol);
    }

    bool apply_const_use =
        rand_val_gen->getRandId(gen_pol->apply_const_use_distr);
    if (apply_const_use) {
        auto new_gen_policy = std::make_shared<GenPolicy>(*gen_pol);
        gen_pol = new_gen_policy;
        gen_pol->chooseAndApplyConstUse();
        active_ctx->setGenPolicy(gen_pol);
    }

    IRNodeKind node_kind = rand_val_gen->getRandId(gen_pol->arith_node_distr);

    if (node_kind == IRNodeKind::CONST) {
        new_node = ConstantExpr::create(active_ctx);
    }
    else if (node_kind == IRNodeKind::SCALAR_VAR_USE ||
             ((active_ctx->getExtInpSymTable()->getArrays().empty() ||
               active_ctx->getLocalSymTable()->getIters().empty()) &&
              (node_kind == IRNodeKind::SUBSCRIPT ||
               node_kind == IRNodeKind::STENCIL))) {
        auto new_scalar_var_use_expr = ScalarVarUseExpr::create(active_ctx);
        new_scalar_var_use_expr->setIsDead(false);
        new_node = new_scalar_var_use_expr;
    }
    else if (node_kind == IRNodeKind::SUBSCRIPT) {
        auto new_subs_expr = SubscriptExpr::create(active_ctx);
        new_subs_expr->setIsDead(false);
        new_node = new_subs_expr;
    }
    else if (node_kind == IRNodeKind::TYPE_CAST) {
        new_node = TypeCastExpr::create(active_ctx);
    }
    else if (node_kind == IRNodeKind::UNARY) {
        new_node = UnaryExpr::create(active_ctx);
    }
    else if (node_kind == IRNodeKind::BINARY) {
        new_node = BinaryExpr::create(active_ctx);
    }
    else if (node_kind == IRNodeKind::CALL) {
        new_node = LibCallExpr::create(active_ctx);
    }
    else if (node_kind == IRNodeKind::TERNARY) {
        new_node = TernaryExpr::create(active_ctx);
    }
    else if (node_kind == IRNodeKind::STENCIL) {
        new_node = createStencil(active_ctx);
    }
    else
        ERROR("Bad node kind");

    ctx->decArithDepth();

    if (ctx->getArithDepth() == 0) {
        Options &options = Options::getInstance();
        bool allow_ub = !ctx->isTaken() &&
                        (options.getAllowUBInDC() == OptionLevel::ALL ||
                         (options.getAllowUBInDC() == OptionLevel::SOME &&
                          rand_val_gen->getRandId(gen_pol->ub_in_dc_prob)));
        // We normally generate UB in the first place and eliminate it later.
        // If we don't do anything to avoid it, we will get it for free
        if (!allow_ub) {
            new_node->propagateType();
            EvalCtx eval_ctx;
            new_node->rebuild(eval_ctx);
        }
    }

    return new_node;
}

bool UnaryExpr::propagateType() {
    arg->propagateType();
    switch (op) {
        case UnaryOp::PLUS:
        case UnaryOp::NEGATE:
        case UnaryOp::BIT_NOT:
            arg = integralProm(arg);
            break;
        case UnaryOp::LOG_NOT:
            arg = convToBool(arg);
            break;
        case UnaryOp::MAX_UN_OP:
            ERROR("Bad unary operator");
            break;
    }
    return true;
}

Expr::EvalResType UnaryExpr::evaluate(EvalCtx &ctx) {
    propagateType();
    EvalResType eval_res = arg->evaluate(ctx);
    assert(eval_res->getKind() == DataKind::VAR &&
           "Unary operations are supported for Scalar Variables only");
    auto scalar_arg = std::static_pointer_cast<ScalarVar>(arg->getValue());
    IRValue new_val;
    switch (op) {
        case UnaryOp::PLUS:
            new_val = +scalar_arg->getCurrentValue();
            break;
        case UnaryOp::NEGATE:
            new_val = -scalar_arg->getCurrentValue();
            break;
        case UnaryOp::LOG_NOT:
            new_val = !scalar_arg->getCurrentValue();
            break;
        case UnaryOp::BIT_NOT:
            new_val = ~scalar_arg->getCurrentValue();
            break;
        case UnaryOp::MAX_UN_OP:
            ERROR("Bad unary operator");
            break;
    }
    assert(scalar_arg->getType()->isIntType() &&
           "Unary operations are supported for Scalar Variables of Integral "
           "Types only");
    value = std::make_shared<ScalarVar>(
        "",
        IntegralType::init(new_val.getIntTypeID(), false, CVQualifier::NONE,
                           arg->getValue()->getType()->isUniform()),
        new_val);
    return value;
}

Expr::EvalResType UnaryExpr::rebuild(EvalCtx &ctx) {
    propagateType();
    arg->rebuild(ctx);
    EvalResType eval_res = evaluate(ctx);
    assert(eval_res->getKind() == DataKind::VAR &&
           "Unary operations are supported for Scalar Variables of Integral "
           "Types only");
    auto eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
    if (!eval_scalar_res->getCurrentValue().hasUB()) {
        value = eval_res;
        return value;
    }

    if (op == UnaryOp::NEGATE) {
        op = UnaryOp::PLUS;
    }
    else {
        ERROR("Something went wrong, this should be unreachable");
    }

    do {
        eval_res = evaluate(ctx);
        eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
        if (!eval_scalar_res->getCurrentValue().hasUB())
            break;
        rebuild(ctx);
    } while (eval_scalar_res->getCurrentValue().hasUB());

    value = eval_res;
    return value;
}

void UnaryExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                     std::string offset) {
    stream << offset << "(";
    switch (op) {
        case UnaryOp::PLUS:
            stream << "+";
            break;
        case UnaryOp::NEGATE:
            stream << "-";
            break;
        case UnaryOp::LOG_NOT:
            stream << "!";
            break;
        case UnaryOp::BIT_NOT:
            stream << "~";
            break;
        case UnaryOp::MAX_UN_OP:
            ERROR("Bad unary operator");
            break;
    }
    stream << "(";
    arg->emit(ctx, stream);
    stream << "))";
}
std::shared_ptr<UnaryExpr> UnaryExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    UnaryOp op = rand_val_gen->getRandId(gen_pol->unary_op_distr);
    auto expr = ArithmeticExpr::create(ctx);
    return std::make_shared<UnaryExpr>(op, expr);
}

UnaryExpr::UnaryExpr(UnaryOp _op, std::shared_ptr<Expr> _expr)
    : op(_op), arg(std::move(_expr)) {
    propagateType();
    EvalCtx ctx;
    evaluate(ctx);
}

bool BinaryExpr::propagateType() {
    lhs->propagateType();
    rhs->propagateType();

    Options &options = Options::getInstance();
    if (options.isISPC())
        varyingPromotion(lhs, rhs);

    switch (op) {
        case BinaryOp::ADD:
        case BinaryOp::SUB:
        case BinaryOp::MUL:
        case BinaryOp::DIV:
        case BinaryOp::MOD:
        case BinaryOp::LT:
        case BinaryOp::GT:
        case BinaryOp::LE:
        case BinaryOp::GE:
        case BinaryOp::EQ:
        case BinaryOp::NE:
        case BinaryOp::BIT_AND:
        case BinaryOp::BIT_OR:
        case BinaryOp::BIT_XOR:
            // Arithmetic conversions
            lhs = integralProm(lhs);
            rhs = integralProm(rhs);
            arithConv(lhs, rhs);
            break;
        case BinaryOp::SHL:
        case BinaryOp::SHR:
            lhs = integralProm(lhs);
            rhs = integralProm(rhs);
            break;
        case BinaryOp::LOG_AND:
        case BinaryOp::LOG_OR:
            lhs = convToBool(lhs);
            rhs = convToBool(rhs);
            break;
        case BinaryOp::MAX_BIN_OP:
            ERROR("Bad operation code");
            break;
    }
    return true;
}

Expr::EvalResType BinaryExpr::evaluate(EvalCtx &ctx) {
    propagateType();
    if (lhs->getValue()->getKind() != DataKind::VAR ||
        rhs->getValue()->getKind() != DataKind::VAR) {
        ERROR("Binary operations are supported only for scalar variables");
    }

    lhs->evaluate(ctx);
    rhs->evaluate(ctx);

    auto lhs_scalar_var = std::static_pointer_cast<ScalarVar>(lhs->getValue());
    auto rhs_scalar_var = std::static_pointer_cast<ScalarVar>(rhs->getValue());

    IRValue lhs_val = lhs_scalar_var->getCurrentValue();
    IRValue rhs_val = rhs_scalar_var->getCurrentValue();

    IRValue new_val(lhs_val.getIntTypeID());

    switch (op) {
        case BinaryOp::ADD:
            new_val = lhs_val + rhs_val;
            break;
        case BinaryOp::SUB:
            new_val = lhs_val - rhs_val;
            break;
        case BinaryOp::MUL:
            new_val = lhs_val * rhs_val;
            break;
        case BinaryOp::DIV:
            new_val = lhs_val / rhs_val;
            break;
        case BinaryOp::MOD:
            new_val = lhs_val % rhs_val;
            break;
        case BinaryOp::LT:
            new_val = lhs_val < rhs_val;
            break;
        case BinaryOp::GT:
            new_val = lhs_val > rhs_val;
            break;
        case BinaryOp::LE:
            new_val = lhs_val <= rhs_val;
            break;
        case BinaryOp::GE:
            new_val = lhs_val >= rhs_val;
            break;
        case BinaryOp::EQ:
            new_val = lhs_val == rhs_val;
            break;
        case BinaryOp::NE:
            new_val = lhs_val != rhs_val;
            break;
        case BinaryOp::LOG_AND:
            new_val = lhs_val && rhs_val;
            break;
        case BinaryOp::LOG_OR:
            new_val = lhs_val || rhs_val;
            break;
        case BinaryOp::BIT_AND:
            new_val = lhs_val & rhs_val;
            break;
        case BinaryOp::BIT_OR:
            new_val = lhs_val | rhs_val;
            break;
        case BinaryOp::BIT_XOR:
            new_val = lhs_val ^ rhs_val;
            break;
        case BinaryOp::SHL:
            new_val = lhs_val << rhs_val;
            break;
        case BinaryOp::SHR:
            new_val = lhs_val >> rhs_val;
            break;
        case BinaryOp::MAX_BIN_OP:
            ERROR("Bad operator code");
            break;
    }

    value = std::make_shared<ScalarVar>(
        "",
        IntegralType::init(new_val.getIntTypeID(), false, CVQualifier::NONE,
                           lhs->getValue()->getType()->isUniform()),
        new_val);
    return value;
}

Expr::EvalResType BinaryExpr::rebuild(EvalCtx &ctx) {
    propagateType();
    lhs->rebuild(ctx);
    rhs->rebuild(ctx);
    std::shared_ptr<Data> eval_res = evaluate(ctx);
    assert(eval_res->getKind() == DataKind::VAR &&
           "Binary operations are supported only for Scalar Variables");

    auto eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
    if (!eval_scalar_res->getCurrentValue().hasUB()) {
        value = eval_res;
        return eval_res;
    }

    UBKind ub = eval_scalar_res->getCurrentValue().getUBCode();

    switch (op) {
        case BinaryOp::ADD:
            op = BinaryOp::SUB;
            break;
        case BinaryOp::SUB:
            op = BinaryOp::ADD;
            break;
        case BinaryOp::MUL:
            op = ub == UBKind::SignOvfMin ? BinaryOp::SUB : BinaryOp::DIV;
            break;
        case BinaryOp::DIV:
        case BinaryOp::MOD:
            op = ub == UBKind::ZeroDiv ? BinaryOp::MUL : BinaryOp::SUB;
            break;
        case BinaryOp::SHR:
        case BinaryOp::SHL:
            if (ub == UBKind::ShiftRhsLarge || ub == UBKind::ShiftRhsNeg) {
                // First of all, we need to find the maximal valid shift value
                assert(lhs->getValue()->getType()->isIntType() &&
                       "Binary operations are supported only for Scalar "
                       "Variables of Integral Types");
                auto lhs_int_type = std::static_pointer_cast<IntegralType>(
                    lhs->getValue()->getType());
                assert(lhs->getValue()->getKind() == DataKind::VAR &&
                       "Binary operations are supported only for Scalar "
                       "Variables");
                auto lhs_scalar_var =
                    std::static_pointer_cast<ScalarVar>(lhs->getValue());
                // We can't shift pass the type size
                Options &options = Options::getInstance();
                size_t max_sht_val = lhs_int_type->getBitSize() - 1;
                // And we can't shift MSB pass the type size
                if (op == BinaryOp::SHL && lhs_int_type->getIsSigned() &&
                    ub == UBKind::ShiftRhsLarge) {
                    size_t msb = lhs_scalar_var->getCurrentValue().getMSB();
                    if (msb > 0) {
                        max_sht_val -= (msb - 1);
                        // For C the resulting value has to fit in type itself
                        if (options.isC() && max_sht_val > 0)
                            max_sht_val -= 1;
                    }
                }

                // Secondly, we choose a new shift value in a valid range
                size_t new_val = rand_val_gen->getRandValue(
                    static_cast<size_t>(0), max_sht_val);

                // Thirdly, we need to combine the chosen value with the
                // existing one
                assert(rhs->getValue()->getType()->isIntType() &&
                       "Binary operations are supported only for Scalar "
                       "Variables of Integral Types");
                auto rhs_int_type = std::static_pointer_cast<IntegralType>(
                    rhs->getValue()->getType());
                assert(rhs->getValue()->getKind() == DataKind::VAR &&
                       "Binary operations are supported only for Scalar "
                       "Variables");
                auto rhs_scalar_var =
                    std::static_pointer_cast<ScalarVar>(rhs->getValue());
                IRValue::AbsValue rhs_abs_val =
                    rhs_scalar_var->getCurrentValue().getAbsValue();
                uint64_t rhs_abs_int_val =
                    rhs_abs_val.isNegative
                        ? std::abs(static_cast<int64_t>(rhs_abs_val.value))
                        : rhs_abs_val.value;
                if (ub == UBKind::ShiftRhsNeg)
                    // TODO: it won't work for INT_MIN
                    new_val = static_cast<size_t>(new_val + rhs_abs_int_val);
                // UBKind::ShiftRhsLarge
                else
                    new_val = static_cast<size_t>(rhs_abs_int_val - new_val);

                // Finally, we need to make changes to the program
                IRValue adjust_val = IRValue(rhs_int_type->getIntTypeId());
                assert(new_val > 0 && "Correction values can't be negative");
                adjust_val.setValue(IRValue::AbsValue{false, new_val});
                auto const_val = std::make_shared<ConstantExpr>(adjust_val);
                if (ub == UBKind::ShiftRhsNeg)
                    rhs = std::make_shared<BinaryExpr>(BinaryOp::ADD, rhs,
                                                       const_val);
                // UBKind::ShiftRhsLarge
                else
                    rhs = std::make_shared<BinaryExpr>(BinaryOp::SUB, rhs,
                                                       const_val);
            }
            // UBKind::NegShift
            else {
                // We can just add maximal value of the type
                assert(lhs->getValue()->getType()->isIntType() &&
                       "Binary operations are supported only for Scalar "
                       "Variables of Integral Types");
                auto lhs_int_type = std::static_pointer_cast<IntegralType>(
                    lhs->getValue()->getType());
                auto const_val =
                    std::make_shared<ConstantExpr>(lhs_int_type->getMax());
                lhs =
                    std::make_shared<BinaryExpr>(BinaryOp::ADD, lhs, const_val);
            }
            break;
        case BinaryOp::LT:
        case BinaryOp::GT:
        case BinaryOp::LE:
        case BinaryOp::GE:
        case BinaryOp::EQ:
        case BinaryOp::NE:
        case BinaryOp::BIT_AND:
        case BinaryOp::BIT_OR:
        case BinaryOp::BIT_XOR:
        case BinaryOp::LOG_AND:
        case BinaryOp::LOG_OR:
            break;
        case BinaryOp::MAX_BIN_OP:
            ERROR("Bad binary operator");
            break;
    }

    do {
        eval_res = evaluate(ctx);
        eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
        if (!eval_scalar_res->getCurrentValue().hasUB())
            break;
        rebuild(ctx);
    } while (eval_scalar_res->getCurrentValue().hasUB());

    value = eval_res;
    return eval_res;
}

void BinaryExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                      std::string offset) {
    stream << offset << "((";
    lhs->emit(ctx, stream);
    stream << ")";
    switch (op) {
        case BinaryOp::ADD:
            stream << " + ";
            break;
        case BinaryOp::SUB:
            stream << " - ";
            break;
        case BinaryOp::MUL:
            stream << " * ";
            break;
        case BinaryOp::DIV:
            stream << " / ";
            break;
        case BinaryOp::MOD:
            stream << " % ";
            break;
        case BinaryOp::LT:
            stream << " < ";
            break;
        case BinaryOp::GT:
            stream << " > ";
            break;
        case BinaryOp::LE:
            stream << " <= ";
            break;
        case BinaryOp::GE:
            stream << " >= ";
            break;
        case BinaryOp::EQ:
            stream << " == ";
            break;
        case BinaryOp::NE:
            stream << " != ";
            break;
        case BinaryOp::LOG_AND:
            stream << " && ";
            break;
        case BinaryOp::LOG_OR:
            stream << " || ";
            break;
        case BinaryOp::BIT_AND:
            stream << " & ";
            break;
        case BinaryOp::BIT_OR:
            stream << " | ";
            break;
        case BinaryOp::BIT_XOR:
            stream << " ^ ";
            break;
        case BinaryOp::SHL:
            stream << " << ";
            break;
        case BinaryOp::SHR:
            stream << " >> ";
            break;
        case BinaryOp::MAX_BIN_OP:
            ERROR("Bad binary operator");
            break;
    }
    stream << "(";
    rhs->emit(ctx, stream);
    stream << "))";
}

BinaryExpr::BinaryExpr(BinaryOp _op, std::shared_ptr<Expr> _lhs,
                       std::shared_ptr<Expr> _rhs)
    : op(_op), lhs(std::move(_lhs)), rhs(std::move(_rhs)) {
    EvalCtx ctx;
    evaluate(ctx);
}

std::shared_ptr<BinaryExpr>
BinaryExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    BinaryOp op = rand_val_gen->getRandId(gen_pol->binary_op_distr);
    auto lhs = ArithmeticExpr::create(ctx);
    auto rhs = ArithmeticExpr::create(ctx);
    return std::make_shared<BinaryExpr>(op, lhs, rhs);
}

TernaryExpr::TernaryExpr(std::shared_ptr<Expr> _cond,
                         std::shared_ptr<Expr> _true_br,
                         std::shared_ptr<Expr> _false_br)
    : cond(std::move(_cond)), true_br(std::move(_true_br)),
      false_br(std::move(_false_br)) {
    EvalCtx ctx;
    evaluate(ctx);
}

bool TernaryExpr::propagateType() {
    cond->propagateType();
    true_br->propagateType();
    false_br->propagateType();

    cond = convToBool(cond);

    Options &options = Options::getInstance();
    if (options.isISPC()) {
        if (LibCallExpr::isAnyArgVarying({cond})) {
            LibCallExpr::ispcArgPromotion(true_br);
            LibCallExpr::ispcArgPromotion(false_br);
        }
        varyingPromotion(true_br, false_br);
    }

    // Arithmetic conversions
    true_br = integralProm(true_br);
    false_br = integralProm(false_br);
    arithConv(true_br, false_br);
    return true;
}

Expr::EvalResType TernaryExpr::evaluate(EvalCtx &ctx) {
    propagateType();
    EvalResType cond_eval = cond->evaluate(ctx);
    if (cond_eval->getKind() != DataKind::VAR)
        ERROR("We support only scalar variables for now");

    IRValue cond_val =
        std::static_pointer_cast<ScalarVar>(cond_eval)->getCurrentValue();
    if (cond_val.getValueRef<bool>())
        value = true_br->evaluate(ctx);
    else
        value = false_br->evaluate(ctx);
    return value;
}

Expr::EvalResType TernaryExpr::rebuild(EvalCtx &ctx) {
    cond->rebuild(ctx);
    true_br->rebuild(ctx);
    false_br->rebuild(ctx);
    return evaluate(ctx);
}

void TernaryExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                       std::string offset) {
    stream << offset << "((";
    cond->emit(ctx, stream);
    stream << ") ? (";
    true_br->emit(ctx, stream);
    stream << ") : (";
    false_br->emit(ctx, stream);
    stream << "))";
}

std::shared_ptr<TernaryExpr>
TernaryExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto cond = ArithmeticExpr::create(ctx);
    auto true_br = ArithmeticExpr::create(ctx);
    auto false_br = ArithmeticExpr::create(ctx);

    return std::make_shared<TernaryExpr>(cond, true_br, false_br);
}

bool SubscriptExpr::propagateType() {
    array->propagateType();
    idx->propagateType();
    return true;
}

bool SubscriptExpr::inBounds(size_t dim, std::shared_ptr<Data> idx_val,
                             EvalCtx &ctx) {
    if (idx_val->isScalarVar()) {
        auto scalar_var = std::static_pointer_cast<ScalarVar>(idx_val);
        IRValue idx_scalar_val = scalar_var->getCurrentValue();
        idx_int_type_id = idx_scalar_val.getIntTypeID();
        IRValue zero(idx_scalar_val.getIntTypeID());
        zero.setValue({false, 0});
        IRValue size(idx_scalar_val.getIntTypeID());
        size.setValue({false, dim});

        assert(scalar_var->getType()->isIntType() &&
               "Scalar variable can have only integral type for now");
        auto int_type =
            std::static_pointer_cast<IntegralType>(scalar_var->getType());
        IntTypeID max_type_id =
            int_type->getIsSigned() ? IntTypeID::LLONG : IntTypeID::ULLONG;
        idx_scalar_val = idx_scalar_val.castToType(max_type_id);
        zero = zero.castToType(max_type_id);
        size = size.castToType(max_type_id);
        return (zero <= idx_scalar_val).getValueRef<bool>() &&
               (idx_scalar_val <= size).getValueRef<bool>();
    }
    else if (idx_val->isIterator()) {
        auto iter_var = std::static_pointer_cast<Iterator>(idx_val);
        return inBounds(dim, iter_var->getStart()->evaluate(ctx), ctx) &&
               inBounds(dim, iter_var->getEnd()->evaluate(ctx), ctx);
    }
    else {
        ERROR("We can use only Scalar Variables or Iterator as index");
    }
    return false;
}

Expr::EvalResType SubscriptExpr::evaluate(EvalCtx &ctx) {
    propagateType();

    EvalResType array_eval_res = array->evaluate(ctx);
    if (!array_eval_res->getType()->isArrayType()) {
        ERROR("Subscription operation is supported only for Array");
    }
    auto array_type =
        std::static_pointer_cast<ArrayType>(array_eval_res->getType());

    IRNodeKind base_expr_kind = array->getKind();
    if (base_expr_kind == IRNodeKind::ARRAY_USE)
        active_dim = 0;
    else if (base_expr_kind == IRNodeKind::SUBSCRIPT) {
        auto base_expr = std::static_pointer_cast<SubscriptExpr>(array);
        active_dim = base_expr->getActiveDim() + 1;
    }
    else
        ERROR("Bad base expression for Subscription operation");

    active_size = array_type->getDimensions().at(active_dim);
    UBKind ub_code = UBKind::NoUB;

    EvalResType idx_eval_res = idx->evaluate(ctx);
    if (!inBounds(active_size, idx_eval_res, ctx))
        ub_code = UBKind::OutOfBounds;

    if (active_dim < array_type->getDimensions().size() - 1)
        value = array_eval_res;
    else {
        auto array_val = std::static_pointer_cast<Array>(array_eval_res);
        if (!array_type->getBaseType()->isIntType())
            ERROR("Only integral types are supported for now");
        value = std::make_shared<ScalarVar>(
            "",
            std::static_pointer_cast<IntegralType>(array_type->getBaseType()),
            std::get<0>(array_val->getCurrentValues()));
    }

    Options &options = Options::getInstance();
    if (options.isISPC()) {
        if ((!idx->getValue()->getType()->isUniform() ||
             !array->getValue()->getType()->isUniform()) &&
            value->getType()->isUniform())
            value = value->makeVarying();
    }

    value->setUBCode(ub_code);

    return value;
}

Expr::EvalResType SubscriptExpr::rebuild(EvalCtx &ctx) {
    propagateType();
    idx->rebuild(ctx);
    array->rebuild(ctx);
    EvalResType eval_res = evaluate(ctx);
    if (!eval_res->hasUB())
        return eval_res;

    assert(eval_res->getUBCode() == UBKind::OutOfBounds &&
           "Every other UB should be handled before");

    IRValue active_size_val(idx_int_type_id);
    active_size_val.setValue({false, active_size});
    auto size_constant = std::make_shared<ConstantExpr>(active_size_val);
    idx = std::make_shared<BinaryExpr>(BinaryOp::MOD, idx, size_constant);

    eval_res = evaluate(ctx);
    assert(eval_res->hasUB() && "All of the UB should be fixed by now");
    value = eval_res;
    return eval_res;
}

void SubscriptExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                         std::string offset) {
    stream << offset;
    // TODO: it may cause some problems in the future
    array->emit(ctx, stream);
    stream << " [";
    idx->emit(ctx, stream);
    if (stencil_offset != 0) {
        stream << (stencil_offset > 0 ? " + " : " - ")
               << std::to_string(std::abs(stencil_offset));
    }
    stream << "]";
}

std::vector<std::shared_ptr<Array>>
SubscriptExpr::getSuitableArrays(std::shared_ptr<PopulateCtx> ctx) {
    auto arrs_with_dim =
        ctx->getExtInpSymTable()->getArraysWithDimNum(ctx->getLoopDepth());
    std::vector<std::shared_ptr<Array>> avail_arrs;
    for (auto &arr : arrs_with_dim) {
        assert(arr->getType()->isArrayType() &&
               "Array should have an array type");
        auto arr_type = std::static_pointer_cast<ArrayType>(arr->getType());
        bool suit_arr = true;
        assert(arr_type->getDimensions().size() <= ctx->getLoopDepth() &&
               "Array and context should have same number of dimensions to "
               "create a SubscriptionExpr");
        assert(ctx->getLoopDepth() <= ctx->getDimensions().size());
        for (size_t i = 0; i < arr_type->getDimensions().size(); ++i) {
            if (arr_type->getDimensions().at(i) < ctx->getDimensions().at(i)) {
                suit_arr = false;
                break;
            }
        }
        if (suit_arr)
            avail_arrs.push_back(arr);
    }
    assert(!avail_arrs.empty());
    return avail_arrs;
}

std::shared_ptr<SubscriptExpr>
SubscriptExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    if (ctx->getInStencil()) {
        auto array_params = rand_val_gen->getRandElem(
            ctx->getLocalSymTable()->getStencilsParams());
        return initImpl(array_params, ctx);
    }
    else {
        auto avail_arrs = getSuitableArrays(ctx);
        auto inp_arr = rand_val_gen->getRandElem(avail_arrs);
        return init(inp_arr, ctx);
    }
    ERROR("Unreachable!");
}

SubscriptExpr::SubscriptExpr(std::shared_ptr<Expr> _arr,
                             std::shared_ptr<Expr> _idx)
    : array(std::move(_arr)), idx(std::move(_idx)), active_dim(0),
      active_size(-1), idx_int_type_id(IntTypeID::MAX_INT_TYPE_ID),
      stencil_offset(0) {
    EvalCtx ctx;
    evaluate(ctx);
}

void SubscriptExpr::setValue(std::shared_ptr<Expr> _expr,
                             std::deque<size_t> &span,
                             std::deque<size_t> &steps) {
    EvalCtx ctx;
    auto eval_res = idx->evaluate(ctx);
    // TODO: step can be negative
    // We can't use size_t, because MacOS maps it to unsigned long, and
    // getValueRef doesn't support it
    uint64_t step = 0;
    bool is_idx_iter = eval_res->isIterator();
    if (is_idx_iter) {
        auto iter_eval_res = std::static_pointer_cast<Iterator>(eval_res);
        eval_res = iter_eval_res->getEnd()->evaluate(ctx);
        auto step_eval = iter_eval_res->getStep()->evaluate(ctx);
        if (!step_eval->isScalarVar())
            ERROR("only scalar variables are supported for now");
        auto step_scalar_val = std::static_pointer_cast<ScalarVar>(step_eval);
        step = step_scalar_val->getCurrentValue()
                   .castToType(IntTypeID::ULLONG)
                   .getValueRef<uint64_t>();
    }
    if (!eval_res->isScalarVar())
        ERROR("only scalar variables are supported for now");
    auto eval_scalar_val = std::static_pointer_cast<ScalarVar>(eval_res);

    IRValue cur_idx =
        eval_scalar_val->getCurrentValue().castToType(IntTypeID::ULLONG);
    if (!is_idx_iter)
        step = cur_idx.getValueRef<uint64_t>();
    span.push_front(static_cast<size_t>(cur_idx.getValueRef<uint64_t>()));
    steps.push_front(static_cast<size_t>(step));

    if (array->getKind() == IRNodeKind::SUBSCRIPT) {
        auto subs = std::static_pointer_cast<SubscriptExpr>(array);
        subs->setValue(_expr, span, steps);
    }
    else if (array->getKind() == IRNodeKind::ARRAY_USE) {
        auto array_use = std::static_pointer_cast<ArrayUseExpr>(array);
        array_use->setValue(_expr, span, steps);
    }
    else
        ERROR("Bad IRNodeKind");
}

void SubscriptExpr::setIsDead(bool val) {
    if (array->getKind() == IRNodeKind::SUBSCRIPT) {
        auto subs = std::static_pointer_cast<SubscriptExpr>(array);
        subs->setIsDead(val);
    }
    else if (array->getKind() == IRNodeKind::ARRAY_USE) {
        auto array_use = std::static_pointer_cast<ArrayUseExpr>(array);
        array_use->setIsDead(val);
    }
    else
        ERROR("Bad IRNodeKind");
}

std::shared_ptr<SubscriptExpr>
SubscriptExpr::init(std::shared_ptr<Array> arr,
                    std::shared_ptr<PopulateCtx> ctx) {
    return initImpl(ArrayStencilParams(arr), ctx);
}

std::shared_ptr<SubscriptExpr>
SubscriptExpr::initImpl(ArrayStencilParams array_params,
                        std::shared_ptr<PopulateCtx> ctx) {
    // TODO: relax assumptions
    auto gen_pol = ctx->getGenPolicy();
    auto array = array_params.getArray();
    std::shared_ptr<Expr> res_expr = std::make_shared<ArrayUseExpr>(array);
    assert(!ctx->getDimensions().empty() &&
           "We can create a SubscriptExpr only inside loops");
    assert(array->getType()->isArrayType() &&
           "We can create a SubscriptExpr only for arrays");
    auto array_type = std::static_pointer_cast<ArrayType>(array->getType());
    assert(array_type->getDimensions().size() <=
               ctx->getLocalSymTable()->getIters().size() &&
           "We can create a SubscriptExpr only if we have enough iterators");

    // Generation Rules
    // Stencil | Dims defined | Active dim | Offset set || Iter  | Offset
    //    0    |    X         |    X       |      X     ||  Gen  |  0
    //    1    |    0       --|--> 0     --|-->   0     ||  Gen  |  Gen w/ prob
    //    1    |    1         |    0     --|-->   0     ||  Gen  |  0
    //    1    |    1         |    1       |      0     ||  Gen  |  Gen
    //    1    |    1         |    1       |      1     ||  Load |  Load

    auto stencil_dims = array_params.getActiveDims();
    for (size_t i = 0; i < array_type->getDimensions().size(); ++i) {
        // Create iterator
        // There is only one case when we need to load it
        std::shared_ptr<Iterator> iter = nullptr;
        std::shared_ptr<Expr> iter_use_expr = nullptr;
        bool offsets_defined = ctx->getInStencil() &&
                               !array_params.getOffsets().empty() &&
                               array_params.getOffsets().at(i) != 0;
        if (offsets_defined) {
            // Load iter
            iter_use_expr =
                std::make_shared<IterUseExpr>(array_params.getIters().at(i));
        }
        else {
            // Generate iter
            iter = ctx->getLocalSymTable()->getIters().at(i);
            iter_use_expr = std::make_shared<IterUseExpr>(iter);
        }

        // Create offsets
        int64_t offset = 0;
        if (offsets_defined) {
            // Load offset
            offset = array_params.getOffsets().at(i);
        }
        else if (ctx->getInStencil()) {
            // Generate offset
            bool offset_prob = false;
            // In case where stencil offset dimensions are not defined,
            // we pick them at random
            if (stencil_dims.empty())
                offset_prob =
                    rand_val_gen->getRandId(gen_pol->stencil_in_dim_prob);
            // Otherwise, we check if it is a dimension that should have offset
            bool offset_in_dim = !stencil_dims.empty() && stencil_dims.at(i);
            bool generate_offset = offset_prob || offset_in_dim;

            size_t max_left_offset = iter->getMaxLeftOffset();
            size_t max_right_offset = iter->getMaxRightOffset();
            bool non_zero_offset_exists =
                max_left_offset > 0 || max_right_offset > 0;
            if (generate_offset && non_zero_offset_exists) {
                while (offset == 0)
                    offset = rand_val_gen->getRandValue(
                        -static_cast<int64_t>(max_left_offset),
                        static_cast<int64_t>(max_right_offset));
            }
        }
        auto new_expr =
            std::make_shared<SubscriptExpr>(res_expr, iter_use_expr);
        new_expr->setOffset(offset);
        res_expr = new_expr;
    }

    return std::static_pointer_cast<SubscriptExpr>(res_expr);
}

bool AssignmentExpr::propagateType() {
    to->propagateType();
    from->propagateType();
    from =
        std::make_shared<TypeCastExpr>(from, to->getValue()->getType(), true);
    return true;
}

Expr::EvalResType AssignmentExpr::evaluate(EvalCtx &ctx) {
    propagateType();
    if (!to->getValue()->getType()->isIntType() ||
        !from->getValue()->getType()->isIntType())
        ERROR("We support only Integral Type for now");

    auto to_int_type =
        std::static_pointer_cast<IntegralType>(to->getValue()->getType());
    auto from_int_type =
        std::static_pointer_cast<IntegralType>(from->getValue()->getType());
    if (to_int_type != from_int_type)
        from = std::make_shared<TypeCastExpr>(from, to_int_type,
                                              /*is_implicit*/ true);

    EvalResType to_eval_res = to->evaluate(ctx);
    EvalResType from_eval_res = from->evaluate(ctx);
    if (to_eval_res->getKind() != from_eval_res->getKind())
        ERROR("We can't assign incompatible data types");

    if (!taken)
        return from_eval_res;

    if (to->getKind() == IRNodeKind::SCALAR_VAR_USE) {
        auto to_scalar = std::static_pointer_cast<ScalarVarUseExpr>(to);
        to_scalar->setValue(from);
    }
    else if (to->getKind() == IRNodeKind::ITER_USE) {
        auto to_iter = std::static_pointer_cast<IterUseExpr>(to);
        to_iter->setValue(from);
    }
    else if (to->getKind() == IRNodeKind::SUBSCRIPT) {
        auto to_array = std::static_pointer_cast<SubscriptExpr>(to);
        std::deque<size_t> span, steps;
        to_array->setValue(from, span, steps);
    }
    else
        ERROR("Bad IRNodeKind");

    return from_eval_res;
}

Expr::EvalResType AssignmentExpr::rebuild(EvalCtx &ctx) {
    propagateType();
    to->rebuild(ctx);
    from->rebuild(ctx);
    return evaluate(ctx);
}

void AssignmentExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::string offset) {
    stream << offset;
    to->emit(ctx, stream);
    stream << " = ";
    from->emit(ctx, stream);
}

std::shared_ptr<AssignmentExpr>
AssignmentExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();

    auto from = ArithmeticExpr::create(ctx);
    Options &options = Options::getInstance();
    if (options.getMutationKind() == MutationKind::EXPRS ||
        options.getMutationKind() == MutationKind::ALL) {
        rand_val_gen->switchMutationStates();
        bool mutate = rand_val_gen->getRandId(gen_pol->mutation_probability);
        if (mutate) {
            bool old_state = ctx->isInsideMutation();
            ctx->setIsInsideMutation(true);
            from = ArithmeticExpr::create(ctx);
            ctx->setIsInsideMutation(old_state);
        }
        rand_val_gen->switchMutationStates();
    }

    EvalCtx eval_ctx;
    EvalResType from_val = from->evaluate(eval_ctx);

    DataKind out_kind = rand_val_gen->getRandId(gen_pol->out_kind_distr);
    std::shared_ptr<Expr> to;

    if (!from_val->getType()->isUniform())
        out_kind = DataKind::ARR;

    if ((out_kind == DataKind::VAR || ctx->getLoopDepth() == 0)) {
        auto new_var = ScalarVar::create(ctx);
        ctx->getExtOutSymTable()->addVar(new_var);
        auto new_scalar_use_expr = std::make_shared<ScalarVarUseExpr>(new_var);
        new_scalar_use_expr->setIsDead(false);
        to = new_scalar_use_expr;
    }
    else if (out_kind == DataKind::ARR) {
        auto new_array = Array::create(ctx, false);
        ctx->getExtOutSymTable()->addArray(new_array);
        auto new_subs_expr = SubscriptExpr::init(new_array, ctx);
        new_subs_expr->setIsDead(false);
        to = new_subs_expr;
    }
    else
        ERROR("Bad data kind for assignment");

    if (!from_val->getType()->isUniform() &&
        to->getValue()->getType()->isUniform())
        from = std::make_shared<ExtractCall>(from);

    return std::make_shared<AssignmentExpr>(to, from, ctx->isTaken());
}

std::shared_ptr<LibCallExpr>
LibCallExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    LibCallKind call_kind = LibCallKind::MAX_LIB_CALL_KIND;
    Options &options = Options::getInstance();
    if (options.isC())
        call_kind = rand_val_gen->getRandId(gen_pol->c_lib_call_distr);
    else if (options.isCXX())
        call_kind = rand_val_gen->getRandId(gen_pol->cxx_lib_call_distr);
    else if (options.isISPC())
        call_kind = rand_val_gen->getRandId(gen_pol->ispc_lib_call_distr);
    else
        ERROR("Not supported");

    if (call_kind == LibCallKind::MIN)
        return MinCall::create(ctx);
    else if (call_kind == LibCallKind::MAX)
        return MaxCall::create(ctx);
    else if (call_kind == LibCallKind::SELECT)
        return SelectCall::create(ctx);
    else if (call_kind == LibCallKind::ANY)
        return AnyCall::create(ctx);
    else if (call_kind == LibCallKind::ALL)
        return AllCall::create(ctx);
    else if (call_kind == LibCallKind::NONE)
        return NoneCall::create(ctx);
    else if (call_kind == LibCallKind::RED_MIN)
        return ReduceMinCall::create(ctx);
    else if (call_kind == LibCallKind::RED_MAX)
        return ReduceMaxCall::create(ctx);
    else if (call_kind == LibCallKind::RED_EQ)
        return ReduceEqCall::create(ctx);
    else if (call_kind == LibCallKind::EXTRACT)
        return ExtractCall::create(ctx);
    else
        ERROR("Unsupported call");
}

bool LibCallExpr::isAnyArgVarying(std::vector<std::shared_ptr<Expr>> args) {
    for (const auto &arg : args) {
        auto arg_type = arg->getValue()->getType();
        if (!arg_type->isUniform())
            return true;
    }
    return false;
}

void LibCallExpr::ispcArgPromotion(std::shared_ptr<Expr> &arg) {
    auto arg_type = arg->getValue()->getType();
    if (!arg_type->isUniform())
        return;
    arg_type = arg_type->makeVarying();
    arg = std::make_shared<TypeCastExpr>(arg, arg_type, true);
}

IntTypeID LibCallExpr::getTopIntID(std::vector<std::shared_ptr<Expr>> args) {
    if (args.empty())
        return IntTypeID::MAX_INT_TYPE_ID;

    IntTypeID top_id = IntTypeID::BOOL;
    for (const auto &arg : args) {
        auto arg_type = arg->getValue()->getType();
        if (!arg_type->isIntType())
            ERROR("We support only Integral Types for now");
        auto arg_int_type = std::static_pointer_cast<IntegralType>(arg_type);
        top_id = std::max(arg_int_type->getIntTypeId(), top_id);
    }
    return top_id;
}

void LibCallExpr::cxxArgPromotion(std::shared_ptr<Expr> &arg,
                                  IntTypeID type_id) {
    auto arg_type = arg->getValue()->getType();
    if (!arg_type->isIntType())
        ERROR("We support only Integral Types for now");
    auto arg_int_type = std::static_pointer_cast<IntegralType>(arg_type);
    if (arg_int_type->getIntTypeId() == type_id)
        return;
    arg = std::make_shared<TypeCastExpr>(
        arg,
        IntegralType::init(type_id, arg_type->getIsStatic(),
                           arg_type->getCVQualifier(), arg_type->isUniform()),
        true);
}

MinMaxCallBase::MinMaxCallBase(std::shared_ptr<Expr> _a,
                               std::shared_ptr<Expr> _b, LibCallKind _kind)
    : a(std::move(_a)), b(std::move(_b)), kind(_kind) {
    EvalCtx ctx;
    evaluate(ctx);
}

bool MinMaxCallBase::propagateType() {
    a->propagateType();
    b->propagateType();

    IntTypeID top_type_id = getTopIntID({a, b});
    cxxArgPromotion(a, top_type_id);
    cxxArgPromotion(b, top_type_id);

    Options &options = Options::getInstance();
    if (options.isISPC()) {
        bool any_varying = isAnyArgVarying({a, b});
        if (any_varying) {
            ispcArgPromotion(a);
            ispcArgPromotion(b);
        }
    }

    return true;
}

Expr::EvalResType MinMaxCallBase::evaluate(yarpgen::EvalCtx &ctx) {
    propagateType();

    EvalResType a_eval_res = a->evaluate(ctx);
    EvalResType b_eval_res = b->evaluate(ctx);

    if (!a_eval_res->isScalarVar() || !b_eval_res->isScalarVar())
        ERROR("Arguments should be scalar variables");
    IRValue a_val =
        std::static_pointer_cast<ScalarVar>(a_eval_res)->getCurrentValue();
    IRValue b_val =
        std::static_pointer_cast<ScalarVar>(b_eval_res)->getCurrentValue();

    assert(a_eval_res->getType() == b_eval_res->getType() &&
           "Both of the arguments should have the same type");
    auto a_int_type =
        std::static_pointer_cast<IntegralType>(a_eval_res->getType());
    IntTypeID max_int_type_id =
        a_int_type->getIsSigned() ? IntTypeID::LLONG : IntTypeID ::ULLONG;

    IRValue a_max_val = a_val.castToType(max_int_type_id);
    IRValue b_max_val = b_val.castToType(max_int_type_id);

    IRValue res_val;
    if (kind == LibCallKind::MAX)
        res_val = (a_max_val > b_max_val).getValueRef<bool>() ? a_val : b_val;
    else if (kind == LibCallKind::MIN)
        res_val = (a_max_val < b_max_val).getValueRef<bool>() ? a_val : b_val;
    else
        ERROR("Unsupported LibCallKind");
    value = std::make_shared<ScalarVar>("", a_int_type, res_val);

    return value;
}

void MinMaxCallBase::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::string offset) {
    Options &options = Options::getInstance();
    stream << offset;
    if (options.isCXX())
        stream << "std::";
    if (kind == LibCallKind::MAX)
        stream << "max";
    else if (kind == LibCallKind::MIN)
        stream << "min";
    else
        ERROR("Unsupported LibCallKind");
    stream << "((";
    a->emit(ctx, stream);
    stream << "), (";
    b->emit(ctx, stream);
    stream << "))";
}

std::shared_ptr<LibCallExpr>
MinMaxCallBase::createHelper(std::shared_ptr<PopulateCtx> ctx,
                             LibCallKind kind) {
    auto gen_pol = ctx->getGenPolicy();

    auto a = ArithmeticExpr::create(ctx);
    auto b = ArithmeticExpr::create(ctx);
    // TODO: we don't have overloaded functions for bool
    auto cast_helper = [&gen_pol](std::shared_ptr<Expr> &expr) {
        expr->propagateType();
        EvalCtx eval_ctx;
        EvalResType expr_res = expr->evaluate(eval_ctx);
        assert(expr_res->getKind() == DataKind::VAR &&
               "We support only scalar vars for now");
        auto expr_type =
            std::static_pointer_cast<ScalarVar>(expr_res)->getType();
        assert(expr_type->isIntType() &&
               "We support only scalar variables with integral type for now");
        auto expr_int_type = std::static_pointer_cast<IntegralType>(expr_type);
        if (expr_int_type->getIntTypeId() == IntTypeID::BOOL) {
            IntTypeID new_type_id =
                rand_val_gen->getRandId(gen_pol->int_type_distr);
            // TODO: not the best way to exclude bad cases
            if (new_type_id == IntTypeID::BOOL)
                new_type_id = IntTypeID::INT;
            auto new_type = IntegralType::init(
                new_type_id, expr_int_type->getIsStatic(),
                expr_int_type->getCVQualifier(), expr_int_type->isUniform());
            expr = std::make_shared<TypeCastExpr>(expr, new_type, false);
        }
    };

    Options &options = Options::getInstance();
    if (options.isISPC()) {
        cast_helper(a);
        cast_helper(b);
    }

    if (kind == LibCallKind::MAX)
        return std::make_shared<MaxCall>(a, b);
    else if (kind == LibCallKind::MIN)
        return std::make_shared<MinCall>(a, b);
    else
        ERROR("Unsupported LibCallKind");
}

void MinMaxCallBase::emitCDefinitionImpl(std::shared_ptr<EmitCtx> ctx,
                                         std::ostream &stream,
                                         std::string offset, LibCallKind kind) {
    std::string func_name, func_sign;
    if (kind == LibCallKind::MAX) {
        func_name = "max";
        func_sign = ">";
    }
    else {
        func_name = "min";
        func_sign = "<";
    }
    stream << "#define " << func_name << "(a,b) \\\n";
    stream << "    ({ __typeof__ (a) _a = (a); \\\n";
    stream << "       __typeof__ (b) _b = (b); \\\n";
    stream << "       _a " << func_sign << " _b ? _a : _b; })\n";
}

SelectCall::SelectCall(std::shared_ptr<Expr> _cond,
                       std::shared_ptr<Expr> _true_arg,
                       std::shared_ptr<Expr> _false_arg)
    : cond(std::move(_cond)), true_arg(std::move(_true_arg)),
      false_arg(std::move(_false_arg)) {
    EvalCtx ctx;
    evaluate(ctx);
}

bool SelectCall::propagateType() {
    cond->propagateType();
    true_arg->propagateType();
    false_arg->propagateType();

    auto cond_type = cond->getValue()->getType();
    assert(cond_type->isIntType() && "We support only integral types for now");
    auto cond_int_type = std::static_pointer_cast<IntegralType>(cond_type);
    if (cond_int_type->getIntTypeId() != IntTypeID::BOOL)
        cond = std::make_shared<TypeCastExpr>(
            cond,
            IntegralType::init(IntTypeID::BOOL, cond_type->getIsStatic(),
                               cond_type->getCVQualifier(),
                               cond_type->isUniform()),
            true);
    IntTypeID top_type_id = getTopIntID({true_arg, false_arg});
    // TODO: we don't have a variant for bool
    if (top_type_id == IntTypeID::BOOL)
        top_type_id = IntTypeID::INT;
    // TODO: we don't have functions for unsigned types
    if (!IntegralType::init(top_type_id)->getIsSigned())
        top_type_id = IntTypeID::LLONG;

    cxxArgPromotion(true_arg, top_type_id);
    cxxArgPromotion(false_arg, top_type_id);

    Options &options = Options::getInstance();
    if (options.isISPC()) {
        bool any_varying = isAnyArgVarying({cond, true_arg, false_arg});
        if (any_varying) {
            ispcArgPromotion(true_arg);
            ispcArgPromotion(false_arg);
        }
    }
    return true;
}

Expr::EvalResType SelectCall::evaluate(EvalCtx &ctx) {
    propagateType();
    EvalResType cond_eval = cond->evaluate(ctx);
    if (cond_eval->getKind() != DataKind::VAR)
        ERROR("We support only scalar variables");
    auto cond_var = std::static_pointer_cast<ScalarVar>(cond->getValue());
    bool cond_val = (cond_var->getCurrentValue().castToType(IntTypeID::BOOL))
                        .getValueRef<bool>();
    // TODO: check if select is generated with short-circuit logic
    if (cond_val)
        value = true_arg->evaluate(ctx);
    else
        value = false_arg->evaluate(ctx);
    return value;
}

Expr::EvalResType SelectCall::rebuild(EvalCtx &ctx) {
    cond->rebuild(ctx);
    true_arg->rebuild(ctx);
    false_arg->rebuild(ctx);
    return evaluate(ctx);
}

void SelectCall::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                      std::string offset) {
    stream << offset << "select((";
    cond->emit(ctx, stream);
    stream << "), (";
    true_arg->emit(ctx, stream);
    stream << "), (";
    false_arg->emit(ctx, stream);
    stream << "))";
}

std::shared_ptr<LibCallExpr>
SelectCall::create(std::shared_ptr<PopulateCtx> ctx) {
    auto cond = ArithmeticExpr::create(ctx);
    auto true_arg = ArithmeticExpr::create(ctx);
    auto false_arg = ArithmeticExpr::create(ctx);
    return std::make_shared<SelectCall>(cond, true_arg, false_arg);
}

LogicalReductionBase::LogicalReductionBase(std::shared_ptr<Expr> _arg,
                                           LibCallKind _kind)
    : arg(std::move(_arg)), kind(_kind) {
    EvalCtx ctx;
    evaluate(ctx);
}

bool LogicalReductionBase::propagateType() {
    arg->propagateType();
    IntTypeID type_id = getTopIntID({arg});
    if (type_id != IntTypeID::BOOL)
        cxxArgPromotion(arg, IntTypeID::BOOL);
    if (!isAnyArgVarying({arg}))
        ispcArgPromotion(arg);
    return true;
}

Expr::EvalResType LogicalReductionBase::evaluate(EvalCtx &ctx) {
    propagateType();
    EvalResType arg_eval_res = arg->evaluate(ctx);
    assert(arg_eval_res->isScalarVar() &&
           "We support only scalar variables at this time");
    IRValue arg_val =
        std::static_pointer_cast<ScalarVar>(arg_eval_res)->getCurrentValue();
    auto type = IntegralType::init(IntTypeID::BOOL);
    IRValue init_val(IntTypeID::BOOL);
    if (kind == LibCallKind::ANY || kind == LibCallKind::ALL)
        init_val.setValue(
            IRValue::AbsValue{false, arg_val.getValueRef<bool>()});
    else if (kind == LibCallKind::NONE)
        init_val.setValue(
            IRValue::AbsValue{false, !arg_val.getValueRef<bool>()});
    else
        ERROR("Unsupported LibCallKind");
    value = std::make_shared<ScalarVar>("", type, init_val);
    return value;
}

void LogicalReductionBase::emit(std::shared_ptr<EmitCtx> ctx,
                                std::ostream &stream, std::string offset) {
    stream << offset;
    if (kind == LibCallKind::ANY)
        stream << "any";
    else if (kind == LibCallKind::ALL)
        stream << "all";
    else if (kind == LibCallKind::NONE)
        stream << "none";
    else
        ERROR("Unsupported LibCallKind");
    stream << "((";
    arg->emit(ctx, stream);
    stream << "))";
}

std::shared_ptr<LibCallExpr>
LogicalReductionBase::createHelper(std::shared_ptr<PopulateCtx> ctx,
                                   LibCallKind kind) {
    auto arg = ArithmeticExpr::create(std::move(ctx));
    if (kind == LibCallKind::ANY)
        return std::make_shared<AnyCall>(arg);
    else if (kind == LibCallKind::ALL)
        return std::make_shared<AllCall>(arg);
    else if (kind == LibCallKind::NONE)
        return std::make_shared<NoneCall>(arg);
    else
        ERROR("Unsupported LibCallKind");
}

MinMaxEqReductionBase::MinMaxEqReductionBase(std::shared_ptr<Expr> _arg,
                                             LibCallKind _kind)
    : arg(std::move(_arg)), kind(_kind) {
    EvalCtx ctx;
    evaluate(ctx);
}

bool MinMaxEqReductionBase::propagateType() {
    arg->propagateType();
    IntTypeID type_id = getTopIntID({arg});
    // TODO: we don't have reduce_min/max for small types
    if (type_id < IntTypeID::INT)
        cxxArgPromotion(arg, IntTypeID::INT);
    if (!isAnyArgVarying({arg}))
        ispcArgPromotion(arg);
    return true;
}

Expr::EvalResType MinMaxEqReductionBase::evaluate(EvalCtx &ctx) {
    propagateType();
    EvalResType arg_eval_res = arg->evaluate(ctx);
    assert(arg_eval_res->isScalarVar() &&
           "We support only scalar variables for now");
    IRValue arg_val =
        std::static_pointer_cast<ScalarVar>(arg_eval_res)->getCurrentValue();
    if (!arg_eval_res->getType()->isIntType())
        ERROR("Reduce_min/max accept only integral types");
    if (kind == LibCallKind::RED_MIN || kind == LibCallKind::RED_MAX)
        value = std::make_shared<ScalarVar>(
            "", std::static_pointer_cast<IntegralType>(arg_eval_res->getType()),
            arg_val);
    else if (kind == LibCallKind::RED_EQ) {
        IRValue init_val(IntTypeID::BOOL);
        init_val.setValue(IRValue::AbsValue{false, true});
        value = std::make_shared<ScalarVar>(
            "", IntegralType::init(IntTypeID::BOOL), init_val);
    }
    else
        ERROR("Unsupported LibCallKind");
    return value;
}

void MinMaxEqReductionBase::emit(std::shared_ptr<EmitCtx> ctx,
                                 std::ostream &stream, std::string offset) {
    stream << offset;
    if (kind == LibCallKind::RED_MIN)
        stream << "reduce_min";
    else if (kind == LibCallKind::RED_MAX)
        stream << "reduce_max";
    else if (kind == LibCallKind::RED_EQ)
        stream << "reduce_equal";
    else
        ERROR("Unsupported LibCallKind");
    stream << "((";
    arg->emit(ctx, stream);
    stream << "))";
}

std::shared_ptr<LibCallExpr>
MinMaxEqReductionBase::createHelper(std::shared_ptr<PopulateCtx> ctx,
                                    LibCallKind kind) {
    auto arg = ArithmeticExpr::create(std::move(ctx));
    if (kind == LibCallKind::RED_MIN)
        return std::make_shared<ReduceMinCall>(arg);
    else if (kind == LibCallKind::RED_MAX)
        return std::make_shared<ReduceMaxCall>(arg);
    else if (kind == LibCallKind::RED_EQ)
        return std::make_shared<ReduceEqCall>(arg);
    else
        ERROR("Unsupported LibCallKind");
}

ExtractCall::ExtractCall(std::shared_ptr<Expr> _arg) : arg(_arg) {
    IRValue idx_val(IntTypeID::UINT);
    idx_val.setValue(IRValue::AbsValue{false, 0});
    idx = std::make_shared<ConstantExpr>(idx_val);
    EvalCtx ctx;
    evaluate(ctx);
}

bool ExtractCall::propagateType() {
    arg->propagateType();
    return true;
}

Expr::EvalResType ExtractCall::evaluate(EvalCtx &ctx) {
    propagateType();
    EvalResType arg_eval_res = arg->evaluate(ctx);
    assert(arg_eval_res->isScalarVar() &&
           "We support only scalar variables for now");
    IRValue arg_val =
        std::static_pointer_cast<ScalarVar>(arg_eval_res)->getCurrentValue();
    assert(arg_eval_res->getType()->isIntType() &&
           "We support only integral types for now");
    auto arg_type =
        std::static_pointer_cast<IntegralType>(arg_eval_res->getType());
    auto ret_type = IntegralType::init(arg_type->getIntTypeId());
    value = std::make_shared<ScalarVar>("", ret_type, arg_val);
    return value;
}

void ExtractCall::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                       std::string offset) {
    stream << offset << "extract";
    stream << "((";
    arg->emit(ctx, stream);
    stream << "), (";
    idx->emit(ctx, stream);
    stream << "))";
}

std::shared_ptr<LibCallExpr>
ExtractCall::create(std::shared_ptr<PopulateCtx> ctx) {
    auto arg = ArithmeticExpr::create(std::move(ctx));
    return std::make_shared<ExtractCall>(arg);
}
