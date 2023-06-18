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

#include "data.h"
#include "context.h"
#include "expr.h"

#include <utility>

using namespace yarpgen;

void ScalarVar::dbgDump() {
    std::cout << "Scalar var: " << name << std::endl;
    std::cout << "Type info:" << std::endl;
    type->dbgDump();
    std::cout << "Init val: " << init_val << std::endl;
    std::cout << "Current val: " << cur_val << std::endl;
    std::cout << "Is dead: " << is_dead << std::endl;
}

std::shared_ptr<ScalarVar> ScalarVar::create(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    IntTypeID type_id = rand_val_gen->getRandId(gen_pol->int_type_distr);
    IRValue init_val = rand_val_gen->getRandValue(type_id);
    auto int_type = IntegralType::init(type_id);
    NameHandler &nh = NameHandler::getInstance();
    return std::make_shared<ScalarVar>(nh.getVarName(), int_type, init_val);
}

std::string ScalarVar::getName(std::shared_ptr<EmitCtx> ctx) {
    if (!ctx)
        ERROR("Can't give a name without a context");

    std::string ret;
    if (!ctx->getSYCLPrefix().empty())
        ret = ctx->getSYCLPrefix();
    ret += Data::getName(ctx);
    if (ctx->useSYCLAccess())
        ret += "[0]";
    return ret;
}

void Array::dbgDump() {
    std::cout << "Array: " << name << std::endl;
    std::cout << "Type info:" << std::endl;
    type->dbgDump();
    std::cout << "Init val: " << init_vals[Options::main_val_idx] << std::endl;
    std::cout << "Cur val: " << cur_vals[Options::main_val_idx] << std::endl;
    std::cout << "Is dead: " << is_dead << std::endl;
    if (mul_vals_axis_idx != -1) {
        std::cout << "Multiple vals axis idx: " << mul_vals_axis_idx
                  << std::endl;
        std::cout << "Second init: " << init_vals.back() << std::endl;
        std::cout << "Second cur: " << cur_vals.back() << std::endl;
    }
}

Array::Array(std::string _name, const std::shared_ptr<ArrayType> &_type,
             IRValue _val)
    : Data(std::move(_name), _type), mul_vals_axis_idx(-1) {
    if (!type->isArrayType())
        ERROR("Array variable should have an ArrayType");

    auto array_type = std::static_pointer_cast<ArrayType>(type);
    init_vals[Options::main_val_idx] = _val;
    cur_vals[Options::main_val_idx] = _val;
    if (!array_type->getBaseType()->isIntType())
        ERROR("Only integer types are supported by now");
    if (std::static_pointer_cast<IntegralType>(array_type->getBaseType())
            ->getIntTypeId() != _val.getIntTypeID())
        ERROR("Array initialization value should have the same type as array");

    ub_code = init_vals[Options::main_val_idx].getUBCode();
}

void Array::setInitValue(IRValue _val, bool use_main_vals,
                         int64_t _mul_vals_axis_idx) {
    assert(type->isArrayType() && "Array should have array type");
    auto arr_type = std::static_pointer_cast<ArrayType>(type);
    mul_vals_axis_idx = _mul_vals_axis_idx;
    init_vals[use_main_vals ? Options::main_val_idx : Options::alt_val_idx] =
        _val;
    ub_code =
        init_vals[use_main_vals ? Options::main_val_idx : Options::alt_val_idx]
            .getUBCode();
}

void Array::setCurrentValue(IRValue _val, bool use_main_vals) {
    assert(type->isArrayType() && "Array should have array type");
    auto arr_type = std::static_pointer_cast<ArrayType>(type);
    if (mul_vals_axis_idx != -1) {
        cur_vals[use_main_vals ? Options::main_val_idx : Options::alt_val_idx] =
            _val;
        ub_code = cur_vals[use_main_vals ? Options::main_val_idx
                                         : Options::alt_val_idx]
                      .getUBCode();
    }
    else {
        cur_vals[Options::main_val_idx] = _val;
        ub_code = cur_vals[Options::main_val_idx].getUBCode();
    }
}

std::shared_ptr<Array> Array::create(std::shared_ptr<PopulateCtx> ctx,
                                     bool inp) {
    auto array_type = ArrayType::create(ctx);
    auto base_type = array_type->getBaseType();
    if (!base_type->isIntType())
        ERROR("We support only array of integers for now");
    auto int_type = std::static_pointer_cast<IntegralType>(base_type);
    IRValue init_val = rand_val_gen->getRandValue(int_type->getIntTypeId());
    NameHandler &nh = NameHandler::getInstance();
    auto new_array =
        std::make_shared<Array>(nh.getArrayName(), array_type, init_val);

    auto mul_vals =
        ctx->getMulValsIter() != nullptr &&
        rand_val_gen->getRandId(ctx->getGenPolicy()->array_with_mul_vals_prob);
    mul_vals = ctx->getAllowMulVals() && mul_vals;
    // We need to have multiple values in the output array, so we don't need to
    // worry about assigning multiple values to the array that does not support
    // it
    mul_vals |= !inp && ctx->getMulValsIter() != nullptr;

    if (mul_vals) {
        init_val = rand_val_gen->getRandValue(int_type->getIntTypeId());
        auto mul_val_idx = static_cast<int64_t>(rand_val_gen->getRandValue(
            static_cast<size_t>(0), array_type->getDimensions().size() - 1));
        new_array->setInitValue(init_val, false, mul_val_idx);
        new_array->setCurrentValue(init_val, false);
    }
    return new_array;
}

void Iterator::setParameters(std::shared_ptr<Expr> _start,
                             std::shared_ptr<Expr> _end,
                             std::shared_ptr<Expr> _step) {
    // TODO: if start equals to end, then the iterator should be degenerate
    start = std::move(_start);
    end = std::move(_end);
    step = std::move(_step);
}

std::shared_ptr<Iterator> Iterator::create(std::shared_ptr<PopulateCtx> ctx,
                                           size_t _end_val, bool is_uniform) {
    // TODO: this function is full of magic constants and weird hacks to cut
    //  some corners for ISPC and overflows
    auto gen_pol = ctx->getGenPolicy();

    IntTypeID type_id = rand_val_gen->getRandId(gen_pol->int_type_distr);

    // TODO: It looks like integral promotion rules for bool in ISPC are
    // broken, so we have to do them manually
    Options &options = Options::getInstance();
    if (options.isISPC() && type_id == IntTypeID::BOOL)
        type_id = IntTypeID::INT;

    std::shared_ptr<Type> type = IntegralType::init(type_id);
    if (!is_uniform)
        type = type->makeVarying();
    auto int_type = std::static_pointer_cast<IntegralType>(type);

    // Stencil left span logic
    bool allow_stencil = rand_val_gen->getRandId(gen_pol->allow_stencil_prob);
    uint64_t type_max_val = int_type->getMax().getAbsValue().value;
    auto roll_stencil_span = [&allow_stencil, &gen_pol]() -> size_t {
        if (!allow_stencil)
            return 0;
        return rand_val_gen->getRandId(gen_pol->stencil_span_distr);
    };
    size_t left_span = roll_stencil_span();
    // The start of the iteration space will be at left_span, so it can't be
    // larger than the target end value
    left_span = left_span > _end_val ? 0 : left_span;
    left_span = left_span > type_max_val ? 0 : left_span;

    // ISPC has problems with division in foreach loops that are not aligned
    // to the vector size, so we have to make sure that the start value is
    // zero
    if (options.isISPC() && !is_uniform)
        left_span = 0;

    auto start =
        std::make_shared<ConstantExpr>(IRValue{type_id, {false, left_span}});

    size_t end_val = _end_val;
    // We can't go pass the maximal value of the type
    end_val = static_cast<size_t>(std::min((uint64_t)end_val, type_max_val));
    size_t right_span = roll_stencil_span();
    right_span = end_val < right_span ? 0 : right_span;
    // end_val - right_span is the smallest number that we can start to iterate
    // from, so it has to be larger than the start value
    right_span = end_val - right_span < left_span ? 0 : right_span;
    end_val = end_val - right_span;

    // Again, ISPC has problems with division in foreach loops that are not
    // aligned to the vector size, so we have to make sure that the end value
    // is aligned
    if (options.isISPC() && !is_uniform) {
        right_span = gen_pol->max_stencil_span;
        // _end_val was set with the appropriate right span outside
        end_val = _end_val - right_span;
    }

    auto end =
        std::make_shared<ConstantExpr>(IRValue(type_id, {false, end_val}));

    size_t step_val = rand_val_gen->getRandId(gen_pol->iters_step_distr);
    if (!is_uniform)
        step_val = 1;
    // We can't overflow uncontrollably
    // TODO: we need to support controlled overflow and better strategy
    if ((end_val / step_val + 1) * step_val >=
        int_type->getMax().getAbsValue().value)
        step_val = 1;
    auto step =
        std::make_shared<ConstantExpr>(IRValue{type_id, {false, step_val}});

    size_t total_iters_num = (end_val - left_span + step_val - 1) / step_val;

    NameHandler &nh = NameHandler::getInstance();
    auto iter = std::make_shared<Iterator>(
        nh.getIterName(), type, start, left_span, end, right_span, step,
        end_val == left_span, total_iters_num);

    bool supports_mul_vals = step_val % 2 != Options::main_val_idx ||
                             left_span % 2 != Options::main_val_idx;
    if (supports_mul_vals) {
        iter->setSupportsMulValues(supports_mul_vals);
        size_t last_val = (total_iters_num - 1) * step_val + left_span;
        iter->setMainValsOnLastIter(last_val % 2 == Options::main_val_idx);
    }

    return iter;
}

void Iterator::dbgDump() {
    std::cout << name << std::endl;
    type->dbgDump();
    auto emit_ctx = std::make_shared<EmitCtx>();
    start->emit(emit_ctx, std::cout);
    end->emit(emit_ctx, std::cout);
    end->emit(emit_ctx, std::cout);
}

// This function bring the value of an expression that used in iterator
// specification to the desired value, using additions
static std::shared_ptr<Expr> adjustIterExprValue(std::shared_ptr<Expr> expr,
                                                 IRValue value) {
    EvalCtx eval_ctx;
    auto eval_res = expr->evaluate(eval_ctx);

    if (!eval_res->getType()->isIntType())
        ERROR("We support only integer types for now");

    auto int_type = std::static_pointer_cast<IntegralType>(eval_res->getType());
    if (int_type->getIntTypeId() != value.getIntTypeID())
        ERROR("You are supposed to handle cast first");

    if (eval_res->getKind() != DataKind::VAR)
        ERROR("We support only scalar variables for now");

    // ISPC cast to uniform. Despite the fact that iterator can have varying
    // type, its initialization expression should always have uniform type
    Options &options = Options::getInstance();
    if (options.isISPC())
        if (!eval_res->getType()->isUniform()) {
            auto tmp = std::make_shared<ExtractCall>(expr);
            tmp->setIsImplicit(true);
            expr = tmp;
        }

    // Every binary operation applies integral promotion first, so we need to
    // guarantee that expression can be processed
    if (int_type->getIntTypeId() < IntTypeID::INT) {
        expr = std::make_shared<TypeCastExpr>(
            expr, IntegralType::init(IntTypeID::INT), true);
        value = value.castToType(IntTypeID::INT);
    }

    std::shared_ptr<Expr> ret = expr;
    IRValue expr_val;
    IRValue zero = value;
    zero.setValue(IRValue::AbsValue{false, 0});
    // TODO: is it enough to perform these checks only once?
    do {
        eval_res = ret->evaluate(eval_ctx);
        expr_val =
            std::static_pointer_cast<ScalarVar>(eval_res)->getCurrentValue();

        IRValue diff;
        if ((value > expr_val).getValueRef<bool>())
            diff = value - expr_val;
        else
            diff = expr_val - value;

        if (diff.hasUB())
            diff = int_type->getMax();

        if ((diff == zero).getValueRef<bool>())
            break;

        if ((value > expr_val).getValueRef<bool>())
            ret = std::make_shared<BinaryExpr>(
                BinaryOp::ADD, ret, std::make_shared<ConstantExpr>(diff));
        else
            ret = std::make_shared<BinaryExpr>(
                BinaryOp::SUB, ret, std::make_shared<ConstantExpr>(diff));
    } while (true);

    return ret;
}

void Iterator::populate(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();

    Options &options = Options::getInstance();

    auto new_ctx = std::make_shared<PopulateCtx>(*ctx);
    new_ctx->setAllowMulVals(false);

    auto populate_impl =
        [&new_ctx, &gen_pol,
         &options](std::shared_ptr<Type> type,
                   std::shared_ptr<Expr> expr) -> std::shared_ptr<Expr> {
        LoopEndKind loop_end_kind =
            rand_val_gen->getRandId(gen_pol->loop_end_kind_distr);

        // TODO: add fall-back safety mechanism
        std::shared_ptr<Expr> ret = expr;
        if (loop_end_kind == LoopEndKind::CONST)
            return ret;
        else if (loop_end_kind == LoopEndKind::VAR) {
            // TODO: add appropriate option
            DataKind data_kind =
                rand_val_gen->getRandId(gen_pol->out_kind_distr);
            if (data_kind == DataKind::VAR || type->isUniform() ||
                (data_kind == DataKind::ARR &&
                 (new_ctx->getExtInpSymTable()->getArrays().empty() ||
                  new_ctx->getLocalSymTable()->getIters().empty()))) {
                auto new_scalar_var_expr = ScalarVarUseExpr::create(new_ctx);
                new_scalar_var_expr->setIsDead(false);
                ret = new_scalar_var_expr;
            }
            else if (data_kind == DataKind::ARR) {
                auto new_subs_expr = SubscriptExpr::create(new_ctx);
                new_subs_expr->setIsDead(false);
                ret = new_subs_expr;
            }
            else
                ERROR("Bad data kind");
        }
        else if (loop_end_kind == LoopEndKind::EXPR) {
            ret = ArithmeticExpr::create(new_ctx);
        }
        else
            ERROR("Bad Loop End Kind");

        EvalCtx eval_ctx;
        auto ret_eval_res = ret->rebuild(eval_ctx);

        if (!type->isIntType() || !ret_eval_res->getType()->isIntType())
            ERROR("We support only integer types for now");

        auto int_type = std::static_pointer_cast<IntegralType>(type);
        auto int_eval_res_type =
            std::static_pointer_cast<IntegralType>(ret_eval_res->getType());

        if (options.isISPC())
            if (!ret_eval_res->getType()->isUniform()) {
                ret = std::make_shared<ExtractCall>(ret);
                ret_eval_res = ret->rebuild(eval_ctx);
                int_eval_res_type = std::static_pointer_cast<IntegralType>(
                    ret_eval_res->getType());
            }

        if (int_type->getIntTypeId() != int_eval_res_type->getIntTypeId()) {
            ret = std::make_shared<TypeCastExpr>(
                ret, IntegralType::init(int_type->getIntTypeId()), true);
            ret_eval_res = ret->rebuild(eval_ctx);
        }

        auto expr_eval_res = expr->evaluate(eval_ctx);
        if (expr_eval_res->getKind() != DataKind::VAR)
            ERROR("We support only integer variables for now");

        ret = adjustIterExprValue(
            ret, std::static_pointer_cast<ScalarVar>(expr_eval_res)
                     ->getCurrentValue());

        return ret;
    };

    start = populate_impl(type, start);
    end = populate_impl(type, end);
    step = populate_impl(type, step);
}

DataType TypedData::replaceWith(DataType _new_data) {
    auto new_type = _new_data->getType();
    bool types_match = true;
    if (type->isIntType() != new_type->isIntType() ||
        type->isArrayType() != new_type->isArrayType())
        types_match = false;
    if (types_match && type->isIntType()) {
        auto int_type = std::static_pointer_cast<IntegralType>(type);
        auto new_int_type = std::static_pointer_cast<IntegralType>(new_type);
        types_match = IntegralType::isSame(int_type, new_int_type);
    }
    else if (types_match && type->isArrayType()) {
        auto array_type = std::static_pointer_cast<ArrayType>(type);
        auto new_array_type = std::static_pointer_cast<ArrayType>(new_type);
        types_match = ArrayType::isSame(array_type, new_array_type);
    }
    else if (types_match)
        ERROR("Unsupported type");
    if (!types_match)
        ERROR("We can't replace typed data with a one that doesn't have the "
              "same type!");
    return _new_data;
}

void TypedData::dbgDump() {
    std::cout << "Typed Data" << std::endl;
    type->dbgDump();
}
