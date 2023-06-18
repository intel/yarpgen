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
#include <numeric>
#include <utility>

using namespace yarpgen;

std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<ScalarVarUseExpr>>
    yarpgen::ScalarVarUseExpr::scalar_var_use_set;
std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<ArrayUseExpr>>
    yarpgen::ArrayUseExpr::array_use_set;
std::unordered_map<std::shared_ptr<Data>, std::shared_ptr<IterUseExpr>>
    yarpgen::IterUseExpr::iter_use_set;

static std::shared_ptr<Data>
replaceValueWith(std::shared_ptr<Data> &_value,
                 std::shared_ptr<Data> _new_value) {
    if (_value->isTypedData())
        return std::static_pointer_cast<TypedData>(_value)->replaceWith(
            std::move(_new_value));
    return _new_value;
}

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
    // this will affect the state of the random generator outside the mutated
    // region
    if (!ctx->isInsideMutation() && can_add_to_buf && replace_in_buf) {
        if (used_consts.size() < gen_pol->const_buf_size)
            used_consts.push_back(ret);
        else {
            auto &replaced_const = rand_val_gen->getRandElem(used_consts);
            replaced_const = ret;
        }
    }

    return ret;
}

std::shared_ptr<Expr> ConstantExpr::copy() {
    return std::make_shared<ConstantExpr>(
        std::static_pointer_cast<ScalarVar>(value)->getCurrentValue());
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
    // This variable is defined, and we can just return it.
    auto find_res = ctx.input.find(value->getName(EmitCtx::default_emit_ctx));
    if (find_res != ctx.input.end())
        value = find_res->second;
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

std::shared_ptr<Expr> ScalarVarUseExpr::copy() {
    return std::make_shared<ScalarVarUseExpr>(value);
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

void ArrayUseExpr::setValue(std::shared_ptr<Expr> _expr, bool main_val) {
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
    arr_val->setCurrentValue(expr_scalar_var->getCurrentValue(), main_val);
}

Expr::EvalResType ArrayUseExpr::evaluate(EvalCtx &ctx) {
    // This array is defined, and we can just return it.
    auto find_res = ctx.input.find(value->getName(EmitCtx::default_emit_ctx));
    if (find_res != ctx.input.end())
        value = find_res->second;

    return value;
}

Expr::EvalResType ArrayUseExpr::rebuild(EvalCtx &ctx) { return evaluate(ctx); }

std::shared_ptr<Expr> ArrayUseExpr::copy() {
    return std::make_shared<ArrayUseExpr>(value);
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

    std::static_pointer_cast<Iterator>(value)->setParameters(
        new_iter->getStart(), new_iter->getEnd(), new_iter->getStep());
}

Expr::EvalResType IterUseExpr::evaluate(EvalCtx &ctx) {
    // This iterator is defined, and we can just return it.
    auto find_res = ctx.input.find(value->getName(EmitCtx::default_emit_ctx));
    if (find_res != ctx.input.end())
        value = find_res->second;
    return value;
}

Expr::EvalResType IterUseExpr::rebuild(EvalCtx &ctx) { return evaluate(ctx); }

std::shared_ptr<Expr> IterUseExpr::copy() {
    return std::make_shared<IterUseExpr>(value);
}

TypeCastExpr::TypeCastExpr(std::shared_ptr<Expr> _expr,
                           std::shared_ptr<Type> _to_type, bool _is_implicit)
    : expr(std::move(_expr)), to_type(std::move(_to_type)),
      is_implicit(_is_implicit) {
    assert(to_type->isIntType() && "We can cast only integral types for now");
    auto to_int_type = std::static_pointer_cast<IntegralType>(to_type);
    if (!to_type->isUniform())
        to_int_type->makeVarying();
    value = std::make_shared<TypedData>(to_int_type);
}

bool TypeCastExpr::propagateType() {
    expr->propagateType();
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
        if (options.isISPC() && to_int_type->isUniform() &&
            !base_type->isUniform())
            ERROR("Can't cast varying to uniform");

        value = replaceValueWith(value, scalar_val);
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

std::shared_ptr<Expr> TypeCastExpr::copy() {
    auto new_expr = expr->copy();
    return std::make_shared<TypeCastExpr>(new_expr, to_type, is_implicit);
}

std::shared_ptr<Expr> ArithmeticExpr::integralProm(std::shared_ptr<Expr> arg) {
    if (!arg->getValue()->isScalarVar() && !arg->getValue()->isTypedData()) {
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
    if (!arg->getValue()->isScalarVar() && !arg->getValue()->isTypedData()) {
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
            IntegralType::canRepresentType(b_type->getIntTypeId(),
                                           a_type->getIntTypeId())) {
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

// This is an auxiliary function that creates a special cases of subscriptions
// We need it here, because it is used during stencil and
// subscript expression generation
// The result should map selected iterator to the actual index in context
// dimensions The same applies to the input
static std::vector<std::pair<size_t, std::shared_ptr<Iterator>>>
createSpecialKindSubsDims(
    size_t dims_num, SubscriptOrderKind subs_order_kind,
    std::vector<std::pair<size_t, std::shared_ptr<Iterator>>> &avail_iters) {
    std::vector<std::pair<size_t, std::shared_ptr<Iterator>>> result;
    result.reserve(dims_num);

    // The IN_ORDER and REVERSE cases are handled in the same way. The actual
    // reversing happens at the very end of the subscript generation
    if (subs_order_kind == SubscriptOrderKind::IN_ORDER ||
        subs_order_kind == SubscriptOrderKind::REVERSE) {
        std::vector<size_t> all_ordered_rel_indexes(avail_iters.size(), 0);
        std::iota(all_ordered_rel_indexes.begin(),
                  all_ordered_rel_indexes.end(), 0);
        auto selected_rel_indexes = rand_val_gen->getRandElemsInOrder(
            all_ordered_rel_indexes, dims_num);

        if (selected_rel_indexes.size() < dims_num) {
            size_t dims_to_dup = dims_num - selected_rel_indexes.size();

            // Choose which avail_iters we will duplicate via index
            std::vector<size_t> added_rel_indexes;
            added_rel_indexes.reserve(dims_to_dup);
            for (size_t i = 0; i < dims_to_dup; ++i)
                added_rel_indexes.push_back(rand_val_gen->getRandValue(
                    static_cast<size_t>(0), selected_rel_indexes.size() - 1));

            // Combine existing vectors of iterators and duplicates via index
            auto new_selected_indexes = selected_rel_indexes;
            new_selected_indexes.insert(new_selected_indexes.end(),
                                        added_rel_indexes.begin(),
                                        added_rel_indexes.end());
            std::swap(selected_rel_indexes, new_selected_indexes);
        }

        // Create new vector of iterators
        for (auto &idx : selected_rel_indexes)
            result.emplace_back(avail_iters.at(idx).first,
                                avail_iters.at(idx).second);

        std::sort(
            result.begin(), result.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
    }
    else if (subs_order_kind == SubscriptOrderKind::DIAGONAL) {
        auto selected_iter = rand_val_gen->getRandElem(avail_iters);
        result = std::vector<std::pair<size_t, std::shared_ptr<Iterator>>>(
            dims_num, selected_iter);
    }
    else if (subs_order_kind == SubscriptOrderKind::RANDOM) {
        result = rand_val_gen->getRandElems(avail_iters, dims_num);
    }
    return result;
}

static std::shared_ptr<Expr> createStencil(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    std::shared_ptr<Expr> new_node;
    // Change distribution of leaf exprs
    // TODO: maybe we need to bump up the probability of binary operators to get
    // a proper stencil pattern
    uint64_t scalar_var_prob = 0, const_prob = 0;
    for (auto &node_distr : gen_pol->arith_node_distr) {
        switch (node_distr.getId()) {
            case IRNodeKind::SCALAR_VAR_USE:
                scalar_var_prob = node_distr.getProb();
                break;
            case IRNodeKind::CONST:
                const_prob = node_distr.getProb();
                break;
            default:
                break;
        }
    }

    scalar_var_prob =
        scalar_var_prob * gen_pol->stencil_prob_weight_alternation;
    const_prob = const_prob * gen_pol->stencil_prob_weight_alternation;

    std::vector<Probability<IRNodeKind>> new_node_distr;
    for (auto &item : gen_pol->arith_node_distr) {
        Probability<IRNodeKind> prob = item;
        if (item.getId() == IRNodeKind::SCALAR_VAR_USE)
            prob.setProb(scalar_var_prob);
        else if (item.getId() == IRNodeKind::CONST)
            prob.setProb(const_prob);
        else if (item.getId() == IRNodeKind::STENCIL)
            continue;
        new_node_distr.push_back(prob);
    }

    auto new_gen_pol = std::make_shared<GenPolicy>(*gen_pol);
    new_gen_pol->arith_node_distr = new_node_distr;
    auto new_ctx = std::make_shared<PopulateCtx>(*ctx);
    new_ctx->setGenPolicy(new_gen_pol);

    // Start of the stencil generation
    std::vector<std::pair<size_t, std::shared_ptr<Iterator>>> avail_dims;
    // We check if iterator can be used to create a
    // subscription expr with offset
    // TODO: each iterator can be used only once for now. We need to fix that
    for (size_t i = 0; i < new_ctx->getDimensions().size(); ++i) {
        auto used_iter = new_ctx->getLocalSymTable()->getIters().at(i);
        size_t max_left_offset = used_iter->getMaxLeftOffset();
        size_t max_right_offset = used_iter->getMaxRightOffset();
        bool non_zero_offset_exists =
            (max_left_offset > 0) || (max_right_offset > 0);
        if (non_zero_offset_exists)
            avail_dims.emplace_back(i, used_iter);
    }

    auto subs_order_kind =
        rand_val_gen->getRandId(gen_pol->subs_order_kind_distr);
    // The purpose of this function is to choose stencil dimensions and map them
    // to the dimensions in ctx.
    auto choose_active_dims = [&gen_pol,
                               &avail_dims](SubscriptOrderKind subs_order_kind,
                                            size_t dims_num_limit) {
        // Pick number of dimensions and choose them
        size_t num_of_active_dims =
            rand_val_gen->getRandId(gen_pol->stencil_dim_num_distr);
        if (num_of_active_dims == 0)
            num_of_active_dims = dims_num_limit;
        return createSpecialKindSubsDims(num_of_active_dims, subs_order_kind,
                                         avail_dims);
    };

    auto remap_chosen_dims =
        [](std::vector<std::pair<size_t, std::shared_ptr<Iterator>>>
               &chosen_dims,
           size_t dims_num_limit) {
            // Create vector of indexes to map them to dimensions in ctx
            std::vector<size_t> all_indexes(dims_num_limit);
            std::iota(all_indexes.begin(), all_indexes.end(), 0);
            // assert(chosen_dims.size() <= all_indexes.size() && "We can't have
            // stencil in more dimensions than we have in the array");
            //  This has to be ordered, so we can rely on the sorting of
            //  chosen_dims to achieve selected order
            auto ret = rand_val_gen->getRandElemsInOrder(all_indexes,
                                                         chosen_dims.size());
            return ret;
        };

    std::vector<std::pair<size_t, std::shared_ptr<Iterator>>> chosen_dims;
    size_t chosen_dims_num_limit = 0;
    std::vector<size_t> chosen_dims_idx_remap;
    std::vector<std::shared_ptr<Array>> active_arrs;
    // Check if we need to make a synchronized decisions about arrays
    bool same_dims_all =
        rand_val_gen->getRandId(gen_pol->stencil_same_dims_all_distr);
    if (same_dims_all && !avail_dims.empty()) {
        // First, we need to check which iterators can support stencil
        // We want to save information about iterator and
        // dimension's idx correspondence
        assert(!new_ctx->getDimensions().empty() &&
               "Can't create stencil in scalar context!");

        chosen_dims_num_limit = gen_pol->array_dims_num_limit;
        chosen_dims =
            choose_active_dims(subs_order_kind, chosen_dims_num_limit);

        size_t min_dim_idx =
            std::min_element(
                chosen_dims.begin(), chosen_dims.end(),
                [](const std::pair<size_t, std::shared_ptr<Iterator>> &a,
                   const std::pair<size_t, std::shared_ptr<Iterator>> &b)
                    -> bool { return a.first < b.first; })
                ->first;
        // Second, we need to see which arrays can satisfy these constraints
        std::vector<std::shared_ptr<Array>> avail_arrays;
        for (const auto &array : SubscriptExpr::getSuitableArrays(new_ctx)) {
            auto arr_type = array->getType();
            assert(arr_type->isArrayType() && "Array should have array type");
            auto real_arr_type = std::static_pointer_cast<ArrayType>(arr_type);
            if (real_arr_type->getDimensions().size() > min_dim_idx)
                avail_arrays.push_back(array);
        }

        // Third, we need to pick active arrays
        // TODO: we need a better mechanism to pick arrays
        if (!avail_arrays.empty()) {
            size_t num_of_active_arrs = std::min(
                rand_val_gen->getRandId(gen_pol->arrs_in_stencil_distr),
                avail_arrays.size());
            active_arrs =
                rand_val_gen->getRandElems(avail_arrays, num_of_active_arrs);

            // This can be rewritten with std::max_element, but it looks
            // horrible
            chosen_dims_num_limit = 0;
            for (auto &arr : active_arrs)
                chosen_dims_num_limit =
                    std::max(chosen_dims_num_limit,
                             std::static_pointer_cast<ArrayType>(arr->getType())
                                 ->getDimensions()
                                 .size());

            if (chosen_dims.size() > chosen_dims_num_limit)
                chosen_dims.resize(chosen_dims_num_limit);
            chosen_dims_idx_remap =
                remap_chosen_dims(chosen_dims, chosen_dims_num_limit);
        }
        else {
            same_dims_all = false;
            chosen_dims.clear();
        }
    }

    // After that, we can process other special cases that doesn't involve
    // synchronized decisions
    bool same_dims_each =
        !same_dims_all &&
        rand_val_gen->getRandId(gen_pol->stencil_same_dims_one_arr_distr);
    if (same_dims_each && !avail_dims.empty()) {
        // TODO: this is a possible place to cause a significant slowdown.
        // We need to check the performance and fix it if necessary
        std::vector<std::shared_ptr<Array>> avail_arrs;
        for (const auto &array : SubscriptExpr::getSuitableArrays(new_ctx)) {
            auto array_type = array->getType();
            assert(array_type->isArrayType() &&
                   "Array should have an array type");
            auto true_array_type =
                std::static_pointer_cast<ArrayType>(array_type);
            size_t array_dims = true_array_type->getDimensions().size();
            auto dims_kind =
                rand_val_gen->getRandId(gen_pol->array_dims_use_kind);
            // TODO: do we need it?
            bool suit_array = (dims_kind == ArrayDimsUseKind::FEWER &&
                               array_dims < new_ctx->getDimensions().size()) ||
                              (dims_kind == ArrayDimsUseKind::SAME &&
                               array_dims == new_ctx->getDimensions().size()) ||
                              (dims_kind == ArrayDimsUseKind::MORE &&
                               array_dims > new_ctx->getDimensions().size());
            if (suit_array)
                avail_arrs.push_back(array);
        }

        // TODO: make sure that we don't duplicate fallback strategies
        // This is a fall-back strategy. If we've decided to have a stencil, it
        // is more important than subscription settings
        if (avail_arrs.empty())
            avail_arrs = SubscriptExpr::getSuitableArrays(new_ctx);

        size_t num_of_active_arrs =
            std::min(rand_val_gen->getRandId(gen_pol->arrs_in_stencil_distr),
                     avail_arrs.size());

        active_arrs =
            rand_val_gen->getRandElems(avail_arrs, num_of_active_arrs);
    }

    // TODO: not sure if we need this fallback
    if (active_arrs.empty()) {
        same_dims_all = same_dims_each = false;
        auto avail_arrs = SubscriptExpr::getSuitableArrays(new_ctx);
        size_t num_of_active_arrs =
            std::min(rand_val_gen->getRandId(gen_pol->arrs_in_stencil_distr),
                     avail_arrs.size());
        active_arrs =
            rand_val_gen->getRandElems(avail_arrs, num_of_active_arrs);
    }

    std::vector<ArrayStencilParams> stencils;
    // TODO: we can do it with initialization list
    stencils.reserve(active_arrs.size());
    for (auto &i : active_arrs) {
        auto arr_type = std::static_pointer_cast<ArrayType>(i->getType());
        assert(new_ctx->getDimensions().front() <=
                   arr_type->getDimensions().front() &&
               "Array dimensions can't be larger than context dimensions");
        stencils.emplace_back(i);
    }

    // Generate offsets for special cases
    if (same_dims_each) {
        for (auto &stencil : stencils) {
            assert(chosen_dims.empty() &&
                   "chosen_dims should not be selected before");
            assert(!avail_dims.empty() &&
                   "We can't create a subscript access with offset if none of "
                   "the iterators support it");

            auto array_type = std::static_pointer_cast<ArrayType>(
                stencil.getArray()->getType());
            chosen_dims_num_limit = array_type->getDimensions().size();

            subs_order_kind =
                rand_val_gen->getRandId(gen_pol->subs_order_kind_distr);
            chosen_dims =
                choose_active_dims(subs_order_kind, chosen_dims_num_limit);
            chosen_dims_idx_remap =
                remap_chosen_dims(chosen_dims, chosen_dims_num_limit);
            std::vector<ArrayStencilParams::ArrayStencilDimParams> new_params(
                chosen_dims_num_limit);

            for (size_t i = 0; i < chosen_dims_idx_remap.size(); ++i) {
                size_t idx = chosen_dims_idx_remap.at(i);
                new_params.at(idx).dim_active = true;
                new_params.at(idx).abs_idx = chosen_dims.at(i).first;
                new_params.at(idx).iter = chosen_dims.at(i).second;
            }

            stencil.setParams(new_params, true, false, subs_order_kind);
            chosen_dims.clear();
        }
    }

    bool same_offset_all =
        same_dims_all &&
        rand_val_gen->getRandId(gen_pol->stencil_same_offset_all_distr);
    if (same_dims_all || same_offset_all) {
        std::vector<ArrayStencilParams::ArrayStencilDimParams> new_params(
            chosen_dims_num_limit);
        for (size_t i = 0; i < chosen_dims_idx_remap.size(); ++i) {
            size_t idx = chosen_dims_idx_remap.at(i);
            new_params.at(idx).dim_active = true;
            new_params.at(idx).abs_idx = chosen_dims.at(i).first;
            new_params.at(idx).iter = chosen_dims.at(i).second;
            if (same_offset_all) {
                size_t max_left_offset =
                    chosen_dims.at(i).second->getMaxLeftOffset();
                size_t max_right_offset =
                    chosen_dims.at(i).second->getMaxRightOffset();
                assert((max_left_offset > 0 || max_right_offset > 0) &&
                       "We should have a non-zero offset at this point");
                int64_t new_offset = 0;
                while (new_offset == 0) {
                    new_offset = rand_val_gen->getRandValue(
                        -static_cast<int64_t>(max_left_offset),
                        static_cast<int64_t>(max_right_offset));
                }
                new_params.at(idx).offset = new_offset;
            }
        }
        for (auto &item : stencils)
            item.setParams(new_params, true, same_offset_all, subs_order_kind);
    }

    new_ctx->getLocalSymTable()->setStencilsParams(stencils);

    new_ctx->setInStencil(true);
    new_node = ArithmeticExpr::create(new_ctx);
    new_ctx->setInStencil(false);

    return new_node;

#if 0

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
        rand_val_gen->getRandId(gen_pol->stencil_same_offset_all_distr);
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
#endif
}

std::shared_ptr<Expr> ArithmeticExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    std::shared_ptr<Expr> new_node;
    ctx->incArithDepth();
    auto active_ctx = std::make_shared<PopulateCtx>(ctx);
    // If we are getting close to the maximum depth, we need to make sure that
    // we generate leaves
    if (active_ctx->getArithDepth() == gen_pol->max_arith_depth) {
        // We can have only constants, variables and arrays as leaves
        std::vector<Probability<IRNodeKind>> new_node_distr;
        bool zero_prob = true;
        for (auto &item : gen_pol->arith_node_distr) {
            if (item.getId() == IRNodeKind::CONST ||
                item.getId() == IRNodeKind::SCALAR_VAR_USE ||
                item.getId() == IRNodeKind::SUBSCRIPT) {
                new_node_distr.push_back(item);
                zero_prob &= item.getProb() == 0;
            }
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

    IRNodeKind node_kind = rand_val_gen->getRandId(gen_pol->arith_node_distr);

    bool guaranteed_non_leaf =
        active_ctx->getInStencil() &&
        (!active_ctx || !active_ctx->getParentCtx()->getInStencil()) &&
        active_ctx->getArithDepth() < gen_pol->max_arith_depth;
    assert((!guaranteed_non_leaf ||
            (guaranteed_non_leaf &&
             active_ctx->getArithDepth() < gen_pol->max_arith_depth)) &&
           "We can't reach a threshold and guarantee a non-leaf expression at "
           "the same time");

    while (guaranteed_non_leaf && (node_kind == IRNodeKind::CONST ||
                                   node_kind == IRNodeKind::SCALAR_VAR_USE ||
                                   node_kind == IRNodeKind::SUBSCRIPT)) {
        node_kind = rand_val_gen->getRandId(gen_pol->arith_node_distr);
    }

    bool apply_const_use =
        rand_val_gen->getRandId(gen_pol->apply_const_use_distr) &&
        node_kind != IRNodeKind::STENCIL;
    if (apply_const_use) {
        auto new_gen_policy = std::make_shared<GenPolicy>(*gen_pol);
        gen_pol = new_gen_policy;
        gen_pol->chooseAndApplyConstUse();
        active_ctx->setGenPolicy(gen_pol);
    }

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
    value = std::make_shared<TypedData>(arg->getValue()->getType());
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
    value = replaceValueWith(
        value,
        std::make_shared<ScalarVar>(
            "",
            IntegralType::init(new_val.getIntTypeID(), false, CVQualifier::NONE,
                               arg->getValue()->getType()->isUniform()),
            new_val));
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
    : op(_op), arg(std::move(_expr)) {}

std::shared_ptr<Expr> UnaryExpr::copy() {
    auto new_arg = arg->copy();
    return std::make_shared<UnaryExpr>(op, new_arg);
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

    bool result_is_bool = op == BinaryOp::LT || op == BinaryOp::GT ||
                          op == BinaryOp::LE || op == BinaryOp::GE ||
                          op == BinaryOp::EQ || op == BinaryOp::NE;
    auto bool_type = IntegralType::init(IntTypeID::BOOL);
    if (options.isISPC() && !lhs->getValue()->getType()->isUniform())
        bool_type =
            std::static_pointer_cast<IntegralType>(bool_type->makeVarying());
    value = std::make_shared<TypedData>(
        result_is_bool ? bool_type : lhs->getValue()->getType());

    return true;
}

Expr::EvalResType BinaryExpr::evaluate(EvalCtx &ctx) {
    propagateType();
    EvalResType lhs_eval_res = lhs->evaluate(ctx);
    EvalResType rhs_eval_res = rhs->evaluate(ctx);

    if (lhs_eval_res->getKind() != DataKind::VAR ||
        rhs_eval_res->getKind() != DataKind::VAR) {
        ERROR("Binary operations are supported only for scalar variables");
    }

    auto lhs_scalar_var = std::static_pointer_cast<ScalarVar>(lhs_eval_res);
    auto rhs_scalar_var = std::static_pointer_cast<ScalarVar>(rhs_eval_res);

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

    value = replaceValueWith(
        value,
        std::make_shared<ScalarVar>(
            "",
            IntegralType::init(new_val.getIntTypeID(), false, CVQualifier::NONE,
                               lhs->getValue()->getType()->isUniform()),
            new_val));
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
                auto adjust_val = IRValue(rhs_int_type->getIntTypeId());
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
    : op(_op), lhs(std::move(_lhs)), rhs(std::move(_rhs)) {}

std::shared_ptr<BinaryExpr>
BinaryExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    BinaryOp op = rand_val_gen->getRandId(gen_pol->binary_op_distr);
    auto lhs = ArithmeticExpr::create(ctx);
    auto rhs = ArithmeticExpr::create(ctx);
    return std::make_shared<BinaryExpr>(op, lhs, rhs);
}

std::shared_ptr<Expr> BinaryExpr::copy() {
    auto new_lhs = lhs->copy();
    auto new_rhs = rhs->copy();
    return std::make_shared<BinaryExpr>(op, new_lhs, new_rhs);
}

TernaryExpr::TernaryExpr(std::shared_ptr<Expr> _cond,
                         std::shared_ptr<Expr> _true_br,
                         std::shared_ptr<Expr> _false_br)
    : cond(std::move(_cond)), true_br(std::move(_true_br)),
      false_br(std::move(_false_br)) {}

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

    value = std::make_shared<TypedData>(true_br->getValue()->getType());

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
        value = replaceValueWith(value, true_br->evaluate(ctx));
    else
        value = replaceValueWith(value, false_br->evaluate(ctx));

    if (cond_eval->hasUB()) {
        auto scalar_var = std::static_pointer_cast<ScalarVar>(value);
        auto scalar_val = scalar_var->getCurrentValue();
        scalar_val.setUBCode(cond_eval->getUBCode());
        value = std::make_shared<ScalarVar>(
            "", std::static_pointer_cast<IntegralType>(scalar_var->getType()),
            scalar_val);
    }

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

std::shared_ptr<Expr> TernaryExpr::copy() {
    auto new_cond = cond->copy();
    auto new_true_br = true_br->copy();
    auto new_false_br = false_br->copy();
    return std::make_shared<TernaryExpr>(new_cond, new_true_br, new_false_br);
}

bool SubscriptExpr::propagateType() {
    array->propagateType();
    idx->propagateType();

    auto array_type =
        std::static_pointer_cast<ArrayType>(array->getValue()->getType());
    if (active_dim < array_type->getDimensions().size() - 1)
        value = std::make_shared<TypedData>(array_type);
    else {
        if (!array_type->getBaseType()->isIntType())
            ERROR("Only integral types are supported for now");
        value = std::make_shared<TypedData>(array_type->getBaseType());
    }

    Options &options = Options::getInstance();
    if (options.isISPC() &&
        (!idx->getValue()->getType()->isUniform() ||
         !array->getValue()->getType()->isUniform()) &&
        value->getType()->isUniform())
        value = value->makeVarying();

    return true;
}

bool SubscriptExpr::inBounds(size_t dim, std::shared_ptr<Data> idx_val,
                             EvalCtx &ctx) {
    // TODO: this function is known to cause slowdowns. We need to check it
    // later

    if (idx_val->isScalarVar()) {
        auto scalar_var = std::static_pointer_cast<ScalarVar>(idx_val);
        IRValue idx_scalar_val = scalar_var->getCurrentValue();
        idx_int_type_id = idx_scalar_val.getIntTypeID();
        // The boundary check has to be done in C++. If we try to use the
        // fuzzer IR, this will cause a huge performance penalty (~10x slowdown)
        auto idx_abs_val = idx_scalar_val.getAbsValue();
        int64_t idx_int_val = static_cast<int64_t>(idx_abs_val.value) *
                              (idx_abs_val.isNegative ? -1 : 1);
        int64_t full_idx_val = idx_int_val + stencil_offset;
        bool in_bounds =
            0 <= full_idx_val && full_idx_val <= static_cast<int64_t>(dim);
        return in_bounds;
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

    bool old_use_main_vals = ctx.use_main_vals;
    bool flip_main_vals =
        at_mul_val_axis &&
        std::abs(stencil_offset) % Options::vals_number == Options::alt_val_idx;
    // TODO: check if this escapes the scope
    ctx.use_main_vals = flip_main_vals ? !ctx.use_main_vals : ctx.use_main_vals;

    EvalResType idx_eval_res = idx->evaluate(ctx);

    if (at_mul_val_axis && idx->getKind() == IRNodeKind::CONST) {
        bool use_main_vals = std::static_pointer_cast<ScalarVar>(idx_eval_res)
                                     ->getCurrentValue()
                                     .getAbsValue()
                                     .value %
                                 Options::vals_number ==
                             Options::main_val_idx;
        ctx.use_main_vals = use_main_vals;
    }

    EvalResType array_eval_res = array->evaluate(ctx);

    Options &options = Options::getInstance();
    if (options.isISPC() &&
        (!idx->getValue()->getType()->isUniform() ||
         !array->getValue()->getType()->isUniform()) &&
        array_eval_res->getType()->isUniform())
        array_eval_res = array_eval_res->makeVarying();

    if (!array_eval_res->getType()->isArrayType()) {
        ERROR("Subscription operation is supported only for Array");
    }
    auto array_type =
        std::static_pointer_cast<ArrayType>(array_eval_res->getType());

    active_size = array_type->getDimensions().at(active_dim);
    UBKind ub_code = UBKind::NoUB;

    // TODO: this is a hack. We do not allow multiple values in the
    // expressions that are used to define iteration space.
    auto new_ctx = ctx;
    new_ctx.use_main_vals = true;
    if (!inBounds(active_size, idx_eval_res, new_ctx))
        ub_code = UBKind::OutOfBounds;

    if (active_dim < array_type->getDimensions().size() - 1)
        value = replaceValueWith(value, array_eval_res);
    else {
        auto array_val = std::static_pointer_cast<Array>(array_eval_res);
        if (!array_type->getBaseType()->isIntType())
            ERROR("Only integral types are supported for now");
        auto value_type =
            std::static_pointer_cast<IntegralType>(array_type->getBaseType());
        if (!array_type->isUniform())
            value_type->makeVarying();
        value = replaceValueWith(
            value, std::make_shared<ScalarVar>(
                       "",
                       std::static_pointer_cast<IntegralType>(
                           array_type->getBaseType()),
                       array_val->getCurrentValues(ctx.use_main_vals)));

        // Restore saved value
        ctx.use_main_vals = old_use_main_vals;
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
    auto arrays = ctx->getExtInpSymTable()->getArrays();
    std::vector<std::shared_ptr<Array>> avail_arrs;
    for (auto &arr : arrays) {
        assert(arr->getType()->isArrayType() &&
               "Array should have an array type");
        auto arr_type = std::static_pointer_cast<ArrayType>(arr->getType());
        if (arr_type->getDimensions().front() < ctx->getDimensions().front())
            continue;
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
      stencil_offset(0), at_mul_val_axis(false) {
    // We need this for ISPC, so we can match varying and uniform types
    propagateType();
}

void SubscriptExpr::setValue(std::shared_ptr<Expr> _expr, bool use_main_vals) {
    bool flip_main_vals =
        at_mul_val_axis &&
        std::abs(stencil_offset) % Options::vals_number == Options::alt_val_idx;
    // TODO: check if this escapes the scope
    use_main_vals = flip_main_vals ? !use_main_vals : use_main_vals;

    if (array->getKind() == IRNodeKind::SUBSCRIPT) {
        auto subs = std::static_pointer_cast<SubscriptExpr>(array);
        subs->setValue(_expr, use_main_vals);
    }
    else if (array->getKind() == IRNodeKind::ARRAY_USE) {
        auto array_use = std::static_pointer_cast<ArrayUseExpr>(array);
        array_use->setValue(_expr, use_main_vals);
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
    return initImpl(ArrayStencilParams(std::move(arr)), std::move(ctx));
}

std::shared_ptr<SubscriptExpr>
SubscriptExpr::initImpl(ArrayStencilParams array_params,
                        std::shared_ptr<PopulateCtx> ctx) {
    // TODO: relax assumptions
    auto gen_pol = ctx->getGenPolicy();
    auto array = array_params.getArray();
    assert(!ctx->getDimensions().empty() &&
           "We can create a SubscriptExpr only inside loops");
    assert(array->getType()->isArrayType() &&
           "We can create a SubscriptExpr only for arrays");
    auto array_type = std::static_pointer_cast<ArrayType>(array->getType());

    // clang-format off

    // Generation Rules
    //                  In stencil  Dims defined  Active dim  Offset set  Subs kind  Iter        Offset
    // General               0           x            x           x          CIO     Gen for IO  Gen for O
    // Stencil               1           0            x           x          CIO     Gen for IO  Gen for I, using stencil in dim distr
    // Same dims each        1           1            0           x          CIO     Gen for IO  Gen for O
    // Same dims each        1           1            1           x           I      Load        Gen using stencil in dim distr
    // Same dims all         1           1            0           x          CIO     Gen for IO  Gen for O
    // Same dims all         1           1            1           x           I      Load        Gen using stencil in dim distr
    // Same offsets all      1           1            0           x          CIO     Gen for IO  Gen for O
    // Same offsets all      1           1            1           1           I      Load        Load

    // clang-format on

    // Extract offsets distribution, so we don't have to do it every iteration
    auto find_res =
        gen_pol->stencil_in_dim_prob.find(array_type->getDimensions().size());
    if (find_res == gen_pol->stencil_in_dim_prob.end())
        ERROR("We can't have arrays that have more dimensions than the total "
              "limit");
    auto stencil_in_dim_prob = find_res->second;

    auto dims_order_kind =
        rand_val_gen->getRandId(gen_pol->subs_order_kind_distr);

    std::vector<std::pair<size_t, std::shared_ptr<Iterator>>> sorted_iters;
    if (dims_order_kind == SubscriptOrderKind::IN_ORDER ||
        dims_order_kind == SubscriptOrderKind::REVERSE) {
        std::vector<std::pair<size_t, std::shared_ptr<Iterator>>> all_iters;
        for (size_t i = 0; i < ctx->getLocalSymTable()->getIters().size(); ++i)
            all_iters.emplace_back(i,
                                   ctx->getLocalSymTable()->getIters().at(i));
        sorted_iters = createSpecialKindSubsDims(
            array_type->getDimensions().size(), dims_order_kind, all_iters);
    }

    auto stencil_params = array_params.getParams();
    bool dims_defined = array_params.areDimsDefined();

    // Check if we are required to have multiple values in this dimension,
    // and override the default behavior if necessary
    bool mul_vals_override = ctx->getMulValsIter() != nullptr &&
                             ctx->getAllowMulVals() &&
                             array->getMulValsAxisIdx() != -1;
    // The dimension that has multiple values
    int64_t mul_val_axis_idx = -1;

    // Check if we are required to have a single value in this dimension,
    // and override the default behavior if necessary
    bool single_val_override =
        !ctx->getAllowMulVals() && array->getMulValsAxisIdx() != -1;
    // Iterators that can be used to generate a single value
    std::vector<std::shared_ptr<Iterator>> single_val_iters;
    if (single_val_override) {
        for (auto &iter : ctx->getLocalSymTable()->getIters()) {
            if (!iter->getSupportsMulValues())
                single_val_iters.push_back(iter);
        }
    }

    // It is an auxiliary variable that is used to keep track of the index of
    // last used iterator in case of a stencil with in-order defined dimensions
    // The index refers to the index within the ctx
    size_t prev_used_iter_idx = 0;

    // In case of a DIAGONAL pattern we need to save the iterator
    std::shared_ptr<Iterator> diag_iter = nullptr;
    if (dims_defined &&
        array_params.getDimsOrderKind() == SubscriptOrderKind::DIAGONAL) {
        for (auto &stencil_param : stencil_params)
            if (stencil_param.dim_active)
                diag_iter = stencil_param.iter;
        assert(diag_iter &&
               "We should have an active iterator in DIAGONAL pattern");
    }

    auto get_random_iter = [&gen_pol, &ctx](size_t dim_id) {
        std::shared_ptr<Iterator> ret = nullptr;
        // This is a pseudo-cache, the proper approach require further research
        // The naive implementation suffers from unfair distribution of
        // iterators and messed up order
        // TODO: we should use a proper cache
        auto use_cached =
            rand_val_gen->getRandId(gen_pol->use_iters_cache_prob);
        if (use_cached && dim_id < ctx->getDimensions().size())
            ret = ctx->getLocalSymTable()->getIters().at(dim_id);
        if (!use_cached || !ret)
            ret =
                rand_val_gen->getRandElem(ctx->getLocalSymTable()->getIters());
        return ret;
    };

    // We want to save subscript expressions so that we can reorder them later
    // We also save offset
    std::vector<std::pair<std::shared_ptr<Expr>, int64_t>> subs_exprs;

    for (size_t i = 0; i < array_type->getDimensions().size(); ++i) {
        auto subs_kind = rand_val_gen->getRandId(gen_pol->subs_kind_prob);

        std::shared_ptr<Iterator> iter = nullptr;
        std::shared_ptr<Expr> iter_use_expr = nullptr;

        bool active_dim = dims_defined && (i <= stencil_params.size() - 1) &&
                          stencil_params.at(i).dim_active;
        bool offset_defined = active_dim && array_params.areOffsetsDefined() &&
                              stencil_params.at(i).offset != 0;
        int64_t offset = 0;

        if (ctx->getInStencil()) {
            // In case where stencil offset dimensions are not defined,
            // we pick them at random
            bool offset_prob = false;
            if (!dims_defined)
                offset_prob = rand_val_gen->getRandId(stencil_in_dim_prob);
            // We should have an offset if we rolled to do so, or if the
            // dimension is active
            if (offset_prob || active_dim)
                subs_kind = SubscriptKind::OFFSET;
        }

        if (static_cast<int64_t>(i) == array->getMulValsAxisIdx()) {
            mul_val_axis_idx = static_cast<int64_t>(i);
            // TODO: add offsets here
            if (single_val_override)
                subs_kind = single_val_iters.empty() ? SubscriptKind::CONST
                                                     : SubscriptKind::ITER;
            else if (mul_vals_override)
                subs_kind = SubscriptKind::ITER;
        }

        // Create iterator
        if (subs_kind == SubscriptKind::CONST) {
            auto roll_const = [&array_type, &i]() {
                return rand_val_gen->getRandValue(
                    static_cast<int64_t>(0),
                    static_cast<int64_t>(array_type->getDimensions().at(i) -
                                         1));
            };
            uint64_t init_val = roll_const();
            if (single_val_override) {
                while (init_val % Options::vals_number != Options::main_val_idx)
                    init_val = roll_const();
            }
            IRValue new_val(rand_val_gen->getRandId(gen_pol->int_type_distr));
            new_val.setValue(IRValue::AbsValue{false, init_val});
            iter_use_expr = std::make_shared<ConstantExpr>(new_val);
        }
        else if (subs_kind == SubscriptKind::ITER ||
                 subs_kind == SubscriptKind::OFFSET ||
                 (subs_kind == SubscriptKind::REPEAT && subs_exprs.empty())) {
            if (static_cast<int64_t>(i) == array->getMulValsAxisIdx()) {
                if (single_val_override) {
                    iter = rand_val_gen->getRandElem(single_val_iters);
                }
                else if (mul_vals_override) {
                    iter = ctx->getMulValsIter();
                }
            }
            else if (active_dim) {
                // If we are in a stencil and the dimension is active, we need
                // to load the iterator
                iter = stencil_params.at(i).iter;
                prev_used_iter_idx = stencil_params.at(i).abs_idx;
            }
            else {
                // Generate iter
                if (dims_defined) {
                    if (array_params.getDimsOrderKind() ==
                            SubscriptOrderKind::IN_ORDER ||
                        array_params.getDimsOrderKind() ==
                            SubscriptOrderKind::REVERSE) {
                        size_t next_iter_idx =
                            ctx->getLocalSymTable()->getIters().size() - 1;
                        if (i <= stencil_params.size() - 1) {
                            size_t next_rel_iter_idx = i;
                            for (; next_rel_iter_idx < stencil_params.size();
                                 ++next_rel_iter_idx) {
                                if (stencil_params.at(next_rel_iter_idx)
                                        .dim_active) {
                                    next_iter_idx =
                                        stencil_params.at(next_rel_iter_idx)
                                            .abs_idx;
                                    break;
                                }
                            }
                        }

                        size_t new_iter_idx = rand_val_gen->getRandValue(
                            prev_used_iter_idx, next_iter_idx);
                        iter = ctx->getLocalSymTable()->getIters().at(
                            new_iter_idx);
                        prev_used_iter_idx = new_iter_idx;
                    }
                    else if (array_params.getDimsOrderKind() ==
                             SubscriptOrderKind::DIAGONAL)
                        iter = diag_iter;
                    else if (array_params.getDimsOrderKind() ==
                             SubscriptOrderKind::RANDOM)
                        iter = get_random_iter(i);
                    else
                        ERROR("Unknown dims order kind");
                }
                else if (dims_order_kind == SubscriptOrderKind::IN_ORDER ||
                         dims_order_kind == SubscriptOrderKind::REVERSE)
                    iter = sorted_iters.at(i).second;
                else if (dims_order_kind == SubscriptOrderKind::DIAGONAL &&
                         diag_iter)
                    iter = diag_iter;
                else if (dims_order_kind == SubscriptOrderKind::RANDOM ||
                         (dims_order_kind == SubscriptOrderKind::DIAGONAL &&
                          !diag_iter)) {
                    iter = get_random_iter(i);
                    if (dims_order_kind == SubscriptOrderKind::DIAGONAL &&
                        !diag_iter)
                        diag_iter = iter;
                }
                else
                    ERROR("Unknown dims order kind");
            }
            assert(iter && "Iterator not defined");
            iter_use_expr = std::make_shared<IterUseExpr>(iter);
        }
        else if (subs_kind == SubscriptKind::REPEAT) {
            auto repeated_elem = rand_val_gen->getRandElem(subs_exprs);
            iter_use_expr = repeated_elem.first;
            offset = repeated_elem.second;
        }
        else {
            ERROR("Unknown subscript kind");
        }

        // Create offsets
        if (subs_kind == SubscriptKind::OFFSET) {
            if (offset_defined)
                // Load offset
                offset = stencil_params.at(i).offset;
            else {
                // Generate offset
                size_t max_left_offset = iter->getMaxLeftOffset();
                size_t max_right_offset = iter->getMaxRightOffset();
                bool non_zero_offset_exists =
                    max_left_offset > 0 || max_right_offset > 0;
                if (non_zero_offset_exists) {
                    while (offset == 0)
                        offset = rand_val_gen->getRandValue(
                            -static_cast<int64_t>(max_left_offset),
                            static_cast<int64_t>(max_right_offset));
                }
            }
        }

        subs_exprs.emplace_back(iter_use_expr, offset);
    }

    if (array_params.getDimsOrderKind() == SubscriptOrderKind::REVERSE ||
        (!dims_defined && dims_order_kind == SubscriptOrderKind::REVERSE)) {
        // We want to guarantee that the subscript with multiple values remains
        // in place to preserve the desired property
        // TODO: this should be done in a better way
        if (mul_vals_override || single_val_override) {
            auto saved_subs_expr = subs_exprs.at(mul_val_axis_idx);
            std::reverse(subs_exprs.begin(), subs_exprs.end());
            subs_exprs.at(mul_val_axis_idx) = saved_subs_expr;
        }
        else
            std::reverse(subs_exprs.begin(), subs_exprs.end());
    }

    std::shared_ptr<Expr> res_expr = std::make_shared<ArrayUseExpr>(array);
    for (size_t i = 0; i < subs_exprs.size(); ++i) {
        auto new_expr =
            std::make_shared<SubscriptExpr>(res_expr, subs_exprs.at(i).first);
        new_expr->active_dim = i;
        new_expr->setOffset(subs_exprs.at(i).second);
        new_expr->at_mul_val_axis = mul_val_axis_idx == static_cast<int64_t>(i);
        res_expr = new_expr;
    }

    res_expr = std::static_pointer_cast<SubscriptExpr>(res_expr);

    return std::static_pointer_cast<SubscriptExpr>(res_expr);
}

std::shared_ptr<Expr> SubscriptExpr::copy() {
    auto new_arr = array->copy();
    auto new_idx = idx->copy();
    auto ret = std::make_shared<SubscriptExpr>(new_arr, new_idx);
    ret->active_dim = active_dim;
    ret->active_size = active_size;
    ret->idx_int_type_id = idx_int_type_id;
    ret->stencil_offset = stencil_offset;
    return ret;
}

bool AssignmentExpr::propagateType() {
    to->propagateType();
    from->propagateType();

    auto to_int_type =
        std::static_pointer_cast<IntegralType>(to->getValue()->getType());
    auto from_int_type =
        std::static_pointer_cast<IntegralType>(from->getValue()->getType());
    if (to_int_type != from_int_type) {
        from = std::make_shared<TypeCastExpr>(from, to_int_type,
                                              /*is_implicit*/ true);
        from->propagateType();
    }

    if (second_from != nullptr) {
        second_from->propagateType();
        auto second_from_int_type = std::static_pointer_cast<IntegralType>(
            second_from->getValue()->getType());
        if (to_int_type != second_from_int_type)
            second_from =
                std::make_shared<TypeCastExpr>(second_from, to_int_type, true);
        second_from->propagateType();
    }

    // TODO: what do we do with the second value? For now it doesn't really
    //  matter, because the types match, and it's all we care about here
    value = std::make_shared<TypedData>(from->getValue()->getType());

    return true;
}

Expr::EvalResType AssignmentExpr::evaluate(EvalCtx &ctx) {
    if (!ctx.use_main_vals && second_from == nullptr) {
        second_from = from->copy();
    }

    propagateType();
    if (!to->getValue()->getType()->isIntType() ||
        !from->getValue()->getType()->isIntType())
        ERROR("We support only Integral Type for now");

    bool use_main_vals = ctx.mul_vals_iter == nullptr;
    use_main_vals |= ctx.mul_vals_iter != nullptr && ctx.use_main_vals;

    bool old_use_main_vals = ctx.use_main_vals;
    ctx.use_main_vals = use_main_vals;

    EvalResType to_eval_res = to->evaluate(ctx);
    EvalResType from_eval_res =
        use_main_vals ? from->evaluate(ctx) : second_from->evaluate(ctx);
    if (to_eval_res->getKind() != from_eval_res->getKind())
        ERROR("We can't assign incompatible data types");

    ctx.use_main_vals = old_use_main_vals;
    return from_eval_res;
}

void AssignmentExpr::propagateValue(EvalCtx &ctx) {
    bool use_main_vals = ctx.mul_vals_iter == nullptr;
    use_main_vals |= ctx.mul_vals_iter != nullptr && ctx.use_main_vals;
    if (ctx.mul_vals_iter != nullptr &&
        to->getKind() == IRNodeKind::SCALAR_VAR_USE)
        use_main_vals = ctx.mul_vals_iter->getMainValsOnLastIter();

    bool old_use_main_vals = ctx.use_main_vals;
    ctx.use_main_vals = use_main_vals;

    EvalResType to_eval_res = to->evaluate(ctx);
    EvalResType from_eval_res =
        use_main_vals ? from->evaluate(ctx) : second_from->evaluate(ctx);
    if (to_eval_res->getKind() != from_eval_res->getKind())
        ERROR("We can't assign incompatible data types");

    if (!taken) {
        ctx.use_main_vals = old_use_main_vals;
        return;
    }

    if (to->getKind() == IRNodeKind::SCALAR_VAR_USE) {
        auto to_scalar = std::static_pointer_cast<ScalarVarUseExpr>(to);
        to_scalar->setValue(use_main_vals ? from : second_from);
    }
    else if (to->getKind() == IRNodeKind::SUBSCRIPT) {
        auto to_array = std::static_pointer_cast<SubscriptExpr>(to);
        // TODO: adjust for multiple values
        to_array->setValue(use_main_vals ? from : second_from,
                           ctx.use_main_vals);
    }
    else
        ERROR("Bad IRNodeKind");

    ctx.use_main_vals = old_use_main_vals;
}

Expr::EvalResType AssignmentExpr::rebuild(EvalCtx &ctx) {
    propagateType();
    to->rebuild(ctx);
    auto new_ctx = ctx;
    new_ctx.use_main_vals = true;
    from->rebuild(new_ctx);
    if (second_from != nullptr) {
        new_ctx.use_main_vals = false;
        second_from->rebuild(new_ctx);
        versioning_iter = ctx.mul_vals_iter;
        evaluate(new_ctx);
    }
    return evaluate(ctx);
}

void AssignmentExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::string offset) {
    stream << offset;
    to->emit(ctx, stream);
    stream << " = ";

    // In case of ISPC foreach loop the iterator is varying, but the value
    // we assign to can be uniform. In this case we need to cast the
    // assigned value to uniform.
    bool cast_to_uniform = false;
    if (versioning_iter != nullptr) {
        Options &options = Options::getInstance();
        cast_to_uniform = options.isISPC() &&
                          to->getValue()->getType()->isUniform() &&
                          !versioning_iter->getType()->isUniform();

        GenPolicy gen_pol;
        bool use_zero_as_var =
            rand_val_gen->getRandId(gen_pol.hide_zero_in_versioning_prob);
        if (cast_to_uniform)
            stream << "extract(";
        stream << "("
               << (cast_to_uniform ? "programIndex"
                                   : versioning_iter->getName(ctx))
               << " % " << Options::vals_number << " == ";
        stream << (use_zero_as_var ? "zero"
                                   : std::to_string(Options::main_val_idx))
               << ") ? (";
    }

    from->emit(ctx, stream);

    if (versioning_iter != nullptr) {
        stream << ") : (";
        second_from->emit(ctx, stream);
        stream << ")";
        // TODO: we need to check that this is emitted only for scalar variables
        if (cast_to_uniform)
            stream << ", " << (ISPC_MAX_VECTOR_SIZE % Options::vals_number)
                   << ")";
    }
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

    if (!from_val->getType()->isUniform()) {
        auto find_res = std::find_if(
            gen_pol->out_kind_distr.begin(), gen_pol->out_kind_distr.end(),
            [](Probability<DataKind> &p) {
                return p.getId() == DataKind::ARR && p.getProb() > 0.0;
            });
        if (find_res != gen_pol->out_kind_distr.end())
            out_kind = DataKind::ARR;
    }

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

std::shared_ptr<Expr> AssignmentExpr::copy() {
    auto new_from = from->copy();
    auto new_to = to->copy();
    auto ret = std::make_shared<AssignmentExpr>(new_to, new_from, taken);
    ret->second_from = second_from;
    ret->versioning_iter = versioning_iter;
    return ret;
}

AssignmentExpr::AssignmentExpr(std::shared_ptr<Expr> _to,
                               std::shared_ptr<Expr> _from, bool _taken) {
    from = std::move(_from);
    second_from = nullptr;
    taken = _taken;
    to = std::move(_to);
    versioning_iter = nullptr;
}

bool ReductionExpr::propagateType() { return AssignmentExpr::propagateType(); }

template <class BinOp>
static IRValue reductionHelper(IRValue base, IRValue inc,
                               size_t total_iters_num, BinOp foo) {
    auto tmp_op = std::make_shared<BinaryExpr>(
        BinaryOp::ADD, std::make_shared<ConstantExpr>(base),
        std::make_shared<ConstantExpr>(inc));
    tmp_op->propagateType();
    auto max_int_type =
        std::static_pointer_cast<IntegralType>(tmp_op->getValue()->getType());
    IntTypeID max_type_id = max_int_type->getIntTypeId();

    // IRValue approach is about as fast as approach with C++ types, but it
    // detects UB automatically
    // Ideally, we want to come up with a precise formula for result
    IRValue conv_inc =
        inc.getIntTypeID() == max_type_id ? inc : inc.castToType(max_type_id);
    bool need_to_cast_ret = base.getIntTypeID() != max_type_id;

    IRValue ret = base;
    for (size_t i = 0; i < total_iters_num; i++) {
        ret = foo((need_to_cast_ret ? ret.castToType(max_type_id) : ret),
                  conv_inc)
                  .castToType(base.getIntTypeID());
    }
    return ret;
}

Expr::EvalResType ReductionExpr::evaluate(EvalCtx &ctx) {
    if (is_degenerate)
        return AssignmentExpr::evaluate(ctx);

    propagateType();
    if (!to->getValue()->getType()->isIntType() ||
        !from->getValue()->getType()->isIntType())
        ERROR("We support only Integral Type for now");

    EvalResType to_eval_res = to->evaluate(ctx);
    EvalResType from_eval_res = from->evaluate(ctx);
    if (to_eval_res->getKind() != from_eval_res->getKind())
        ERROR("We can't assign incompatible data types");
    auto to_int_type =
        std::static_pointer_cast<IntegralType>(to->getValue()->getType());

    assert(to_eval_res->getKind() == DataKind::VAR &&
           from_eval_res->getKind() == DataKind::VAR &&
           "We support only scalar vars for now");
    IRValue to_eval_val =
        std::static_pointer_cast<ScalarVar>(to_eval_res)->getCurrentValue();
    IRValue from_eval_val =
        std::static_pointer_cast<ScalarVar>(from_eval_res)->getCurrentValue();

    assert(ctx.total_iter_num >= 0 &&
           "We can't evaluate reduction operation if we don't know the number "
           "of iterations");

    // TODO: we use a very conservative approach here, we should be able to
    // come up with a more precise formula
    std::shared_ptr<Expr> value_result_expr = nullptr;
    if (bin_op != BinaryOp::MAX_BIN_OP) {
        switch (bin_op) {
            case BinaryOp::ADD:
                result_expr = std::make_shared<ConstantExpr>(
                    reductionHelper(to_eval_val, from_eval_val,
                                    ctx.total_iter_num, std::plus()));
                break;
            case BinaryOp::SUB:
                result_expr = std::make_shared<ConstantExpr>(
                    reductionHelper(to_eval_val, from_eval_val,
                                    ctx.total_iter_num, std::minus()));
                break;
            case BinaryOp::MUL:
                result_expr = std::make_shared<ConstantExpr>(
                    reductionHelper(to_eval_val, from_eval_val,
                                    ctx.total_iter_num, std::multiplies()));
                break;
            case BinaryOp::DIV:
                result_expr = std::make_shared<ConstantExpr>(
                    reductionHelper(to_eval_val, from_eval_val,
                                    ctx.total_iter_num, std::divides()));
                break;
            case BinaryOp::MOD:
                result_expr = std::make_shared<ConstantExpr>(
                    reductionHelper(to_eval_val, from_eval_val,
                                    ctx.total_iter_num, std::modulus()));
                break;
            case BinaryOp::BIT_AND:
                result_expr =
                    std::make_shared<BinaryExpr>(BinaryOp::BIT_AND, to, from);
                break;
            case BinaryOp::BIT_OR:
                result_expr =
                    std::make_shared<BinaryExpr>(BinaryOp::BIT_OR, to, from);
                break;
            case BinaryOp::BIT_XOR:
                result_expr = std::make_shared<ConstantExpr>(
                    reductionHelper(to_eval_val, from_eval_val,
                                    ctx.total_iter_num, std::bit_xor()));
                break;
            default:
                ERROR("Unsupported Binary Operation");
        }
    }
    else if (lib_call_kind != LibCallKind::MAX_LIB_CALL_KIND) {
        switch (lib_call_kind) {
            case LibCallKind::MAX:
                result_expr = std::make_shared<MaxCall>(to, from);
                break;
            case LibCallKind::MIN:
                result_expr = std::make_shared<MinCall>(to, from);
                break;
            default:
                ERROR("Unsupported Lib Call");
        }
    }

    result_expr =
        std::make_shared<TypeCastExpr>(result_expr, to_int_type, true);
    result_expr->propagateType();
    auto result_expr_eval_res = result_expr->evaluate(ctx);
    if (result_expr_eval_res->hasUB())
        return result_expr_eval_res;

    return from_eval_res;
}

void ReductionExpr::propagateValue(EvalCtx &ctx) {
    if (is_degenerate) {
        AssignmentExpr::propagateValue(ctx);
        return;
    }
    EvalResType result_expr_eval_res = result_expr->evaluate(ctx);
    assert(!result_expr_eval_res->hasUB() && "We can't have UB here");

    if (!taken)
        return;

    if (to->getKind() == IRNodeKind::SCALAR_VAR_USE) {
        auto to_scalar = std::static_pointer_cast<ScalarVarUseExpr>(to);
        to_scalar->setValue(result_expr);
    }
    else if (to->getKind() == IRNodeKind::ITER_USE) {
        auto to_iter = std::static_pointer_cast<IterUseExpr>(to);
        to_iter->setValue(result_expr);
    }
    else if (to->getKind() == IRNodeKind::SUBSCRIPT) {
        auto to_array = std::static_pointer_cast<SubscriptExpr>(to);
        to_array->setValue(result_expr, true);
    }
    else
        ERROR("Bad IRNodeKind");
}

Expr::EvalResType ReductionExpr::rebuild(EvalCtx &ctx) {
    if (is_degenerate)
        return AssignmentExpr::rebuild(ctx);
    propagateType();
    to->rebuild(ctx);
    from->rebuild(ctx);
    auto ret = evaluate(ctx);
    if (ret->hasUB()) {
        is_degenerate = true;
        ret = rebuild(ctx);
    }
    assert(!ret->hasUB() && "We can't have UB here");
    return ret;
}

void ReductionExpr::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                         std::string offset) {
    if (is_degenerate) {
        AssignmentExpr::emit(ctx, stream, offset);
        return;
    }

    stream << offset;
    to->emit(ctx, stream);

    if (bin_op != BinaryOp::MAX_BIN_OP) {
        stream << " ";
        switch (bin_op) {
            case BinaryOp::ADD:
                stream << "+";
                break;
            case BinaryOp::SUB:
                stream << "-";
                break;
            case BinaryOp::MUL:
                stream << "*";
                break;
            case BinaryOp::DIV:
                stream << "/";
                break;
            case BinaryOp::MOD:
                stream << "%";
                break;
            case BinaryOp::BIT_AND:
                stream << "&";
                break;
            case BinaryOp::BIT_OR:
                stream << "|";
                break;
            case BinaryOp::BIT_XOR:
                stream << "^";
                break;
            default:
                ERROR("Unsupported Binary Operation");
        }
        stream << "= ";
        from->emit(ctx, stream);
    }
    else if (lib_call_kind != LibCallKind::MAX_LIB_CALL_KIND) {
        stream << " = ";
        result_expr->emit(ctx, stream);
    }
}

std::shared_ptr<ReductionExpr>
ReductionExpr::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();

    bool use_bin_op =
        rand_val_gen->getRandId(gen_pol->reduction_as_bin_op_prob);
    BinaryOp bin_op = BinaryOp::MAX_BIN_OP;
    LibCallKind lib_call = LibCallKind::MAX_LIB_CALL_KIND;
    if (use_bin_op)
        bin_op = rand_val_gen->getRandId(gen_pol->reduction_bin_op_distr);
    else
        lib_call =
            rand_val_gen->getRandId(gen_pol->reduction_as_lib_call_distr);

    auto new_gen_pol = std::make_shared<GenPolicy>(*gen_pol);
    // For "|" and "&" we allow to use arrays as a reduction variable
    if (bin_op != BinaryOp::BIT_AND && bin_op != BinaryOp::BIT_OR) {
        bool other_option_exists = false;
        for (auto &kind_prob : new_gen_pol->out_kind_distr) {
            if (kind_prob.getId() == DataKind::ARR)
                kind_prob.setProb(0);
            else if (kind_prob.getProb() > 0)
                other_option_exists = true;
        }
        if (!other_option_exists) {
            auto base_assign_expr = AssignmentExpr::create(ctx);
            return std::make_shared<ReductionExpr>(
                base_assign_expr, BinaryOp::MAX_BIN_OP,
                LibCallKind::MAX_LIB_CALL_KIND, true, ctx->isTaken());
        }
    }

    auto active_ctx = std::make_shared<PopulateCtx>(*ctx);
    active_ctx->setGenPolicy(new_gen_pol);

    auto base_assign_expr = AssignmentExpr::create(active_ctx);

    // ISPC has some problems with bool type in compound assignments, so we
    // will disable them for now
    // TODO: fix me later!
    Options &options = Options::getInstance();
    if (options.isISPC()) {
        base_assign_expr->propagateType();
        assert(base_assign_expr->getValue()->getType()->isIntType() &&
               "We support only int type for now");
        auto base_int_type = std::static_pointer_cast<IntegralType>(
            base_assign_expr->getTo()->getValue()->getType());
        if (base_int_type->getIntTypeId() == IntTypeID::BOOL) {
            new_gen_pol = std::make_shared<GenPolicy>(*gen_pol);
            bool bin_op_red_is_supported = false;
            for (auto &kind_prob : new_gen_pol->reduction_bin_op_distr) {
                if (kind_prob.getId() != BinaryOp::BIT_AND &&
                    kind_prob.getId() != BinaryOp::BIT_OR &&
                    kind_prob.getId() != BinaryOp::BIT_XOR)
                    kind_prob.setProb(0);
                else if (kind_prob.getProb() > 0)
                    bin_op_red_is_supported = true;
            }

            if (!bin_op_red_is_supported)
                return std::make_shared<ReductionExpr>(
                    base_assign_expr, BinaryOp::MAX_BIN_OP,
                    LibCallKind::MAX_LIB_CALL_KIND, true, ctx->isTaken());

            bin_op =
                rand_val_gen->getRandId(new_gen_pol->reduction_bin_op_distr);
        }
    }

    return std::make_shared<ReductionExpr>(base_assign_expr, bin_op, lib_call,
                                           false, ctx->isTaken());
}

std::shared_ptr<Expr> ReductionExpr::copy() {
    auto new_result_expr = result_expr->copy();
    auto new_assign = AssignmentExpr::copy();
    auto new_reduction = std::make_shared<ReductionExpr>(
        std::static_pointer_cast<AssignmentExpr>(new_assign), bin_op,
        lib_call_kind, is_degenerate, taken);
    new_reduction->result_expr = new_result_expr;
    return new_reduction;
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
    return std::any_of(args.begin(), args.end(), [](std::shared_ptr<Expr> arg) {
        return !arg->getValue()->getType()->isUniform();
    });
    return false;
}

void static ispcBoolPromotion(std::shared_ptr<Expr> &expr) {
    auto expr_type = expr->getValue()->getType();
    if (!expr_type->isIntType())
        ERROR("We support only Integral Types for now");
    auto expr_int_type = std::static_pointer_cast<IntegralType>(expr_type);
    if (expr_int_type->getIntTypeId() != IntTypeID::BOOL)
        return;
    // TODO: it looks like promotion rules for bool in ISPC is broken, so we
    // have to do it manually
    auto int_type = IntegralType::init(IntTypeID::SCHAR);
    if (!expr_int_type->isUniform())
        int_type =
            std::static_pointer_cast<IntegralType>(int_type->makeVarying());
    expr = std::make_shared<TypeCastExpr>(expr, int_type, false);
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
    : a(std::move(_a)), b(std::move(_b)), kind(_kind) {}

bool MinMaxCallBase::propagateType() {
    a->propagateType();
    b->propagateType();

    Options &options = Options::getInstance();
    if (options.isISPC()) {
        bool any_varying = isAnyArgVarying({a, b});
        if (any_varying) {
            ispcArgPromotion(a);
            ispcArgPromotion(b);
        }
        ispcBoolPromotion(a);
        ispcBoolPromotion(b);
    }

    IntTypeID top_type_id = getTopIntID({a, b});
    cxxArgPromotion(a, top_type_id);
    cxxArgPromotion(b, top_type_id);

    value = std::make_shared<TypedData>(a->getValue()->getType());
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
    if (a_eval_res->hasUB())
        res_val = a_val;
    else if (b_eval_res->hasUB())
        res_val = b_val;
    else if (kind == LibCallKind::MAX)
        res_val = (a_max_val > b_max_val).getValueRef<bool>() ? a_val : b_val;
    else if (kind == LibCallKind::MIN)
        res_val = (a_max_val < b_max_val).getValueRef<bool>() ? a_val : b_val;
    else
        ERROR("Unsupported LibCallKind");
    value = replaceValueWith(
        value, std::make_shared<ScalarVar>("", a_int_type, res_val));

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
      false_arg(std::move(_false_arg)) {}

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
    value = std::make_shared<TypedData>(true_arg->getValue()->getType());
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
    EvalResType true_eval_res = true_arg->evaluate(ctx);
    EvalResType false_eval_res = false_arg->evaluate(ctx);
    assert(true_eval_res->getKind() == DataKind::VAR &&
           false_eval_res->getKind() == DataKind::VAR &&
           "We support only scalar variables for now");
    IRValue true_eval_val =
        std::static_pointer_cast<ScalarVar>(true_eval_res)->getCurrentValue();
    IRValue false_eval_val =
        std::static_pointer_cast<ScalarVar>(false_eval_res)->getCurrentValue();
    if (true_eval_val.hasUB())
        value = replaceValueWith(value, true_eval_res);
    else if (false_eval_val.hasUB())
        value = replaceValueWith(value, false_eval_res);
    else if (cond_val)
        value = replaceValueWith(value, true_eval_res);
    else
        value = replaceValueWith(value, false_eval_res);
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
    : arg(std::move(_arg)), kind(_kind) {}

bool LogicalReductionBase::propagateType() {
    arg->propagateType();
    IntTypeID type_id = getTopIntID({arg});
    if (type_id != IntTypeID::BOOL)
        cxxArgPromotion(arg, IntTypeID::BOOL);
    if (!isAnyArgVarying({arg}))
        ispcArgPromotion(arg);
    value = std::make_shared<TypedData>(IntegralType::init(IntTypeID::BOOL));
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
    if (arg_val.hasUB())
        init_val.setUBCode(arg_val.getUBCode());
    value = replaceValueWith(value,
                             std::make_shared<ScalarVar>("", type, init_val));

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
    : arg(std::move(_arg)), kind(_kind) {}

bool MinMaxEqReductionBase::propagateType() {
    arg->propagateType();
    IntTypeID type_id = getTopIntID({arg});
    // TODO: we don't have reduce_min/max for small types
    if (type_id < IntTypeID::INT)
        cxxArgPromotion(arg, IntTypeID::INT);
    if (!isAnyArgVarying({arg}))
        ispcArgPromotion(arg);
    assert(arg->getValue()->getType()->isIntType() &&
           "We support only integer types at this time");
    auto arg_int_type_id =
        std::static_pointer_cast<IntegralType>(arg->getValue()->getType())
            ->getIntTypeId();
    arg_int_type_id =
        kind != LibCallKind::RED_EQ ? arg_int_type_id : IntTypeID::BOOL;
    value = std::make_shared<TypedData>(IntegralType::init(arg_int_type_id));
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
    if (kind == LibCallKind::RED_MIN || kind == LibCallKind::RED_MAX) {
        assert(arg_eval_res->getType()->isIntType() &&
               "We support only integer types at this time");
        auto ret_int_type_id =
            std::static_pointer_cast<IntegralType>(arg_eval_res->getType())
                ->getIntTypeId();
        value = replaceValueWith(
            value, std::make_shared<ScalarVar>(
                       "", IntegralType::init(ret_int_type_id), arg_val));
    }
    else if (kind == LibCallKind::RED_EQ) {
        IRValue init_val(IntTypeID::BOOL);
        init_val.setValue(IRValue::AbsValue{false, true});
        init_val.setUBCode(arg_val.getUBCode());
        value = replaceValueWith(
            value, std::make_shared<ScalarVar>(
                       "", IntegralType::init(IntTypeID::BOOL), init_val));
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

ExtractCall::ExtractCall(std::shared_ptr<Expr> _arg)
    : arg(_arg), is_implicit(false) {
    IRValue idx_val(IntTypeID::UINT);
    idx_val.setValue(IRValue::AbsValue{false, 0});
    idx = std::make_shared<ConstantExpr>(idx_val);
}

bool ExtractCall::propagateType() {
    arg->propagateType();
    auto arg_int_type_id =
        std::static_pointer_cast<IntegralType>(arg->getValue()->getType())
            ->getIntTypeId();
    value = std::make_shared<TypedData>(IntegralType::init(arg_int_type_id));
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
    value = replaceValueWith(
        value, std::make_shared<ScalarVar>("", ret_type, arg_val));
    return value;
}

void ExtractCall::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                       std::string offset) {
    stream << offset;
    if (is_implicit)
        stream << "/* implicit */ ";
    stream << "extract";
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
