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

TypeCastExpr::TypeCastExpr(std::shared_ptr<Expr> _expr, std::shared_ptr<Type> _to_type, bool _is_implicit) :
        expr(std::move(_expr)), to_type(std::move(_to_type)), is_implicit(_is_implicit) {
    std::shared_ptr<Type> base_type = expr->getValue()->getType();
    // Check that we try to convert between compatible types.
    if (!((base_type->isIntType() && to_type->isIntType()) ||
          (base_type->isArrayType() && to_type->isArrayType()))) {
        ERROR("Can't create TypeCastExpr for types that can't be casted");
    }

    if (base_type->isIntType() && expr->getValue()->isScalarVar()) {
        std::shared_ptr<IntegralType> to_int_type = std::static_pointer_cast<IntegralType>(to_type);
        value = std::make_shared<ScalarVar>("", to_int_type, IRValue(to_int_type->getIntTypeId()));
        std::shared_ptr<ScalarVar> scalar_val = std::static_pointer_cast<ScalarVar>(value);
        std::shared_ptr<ScalarVar> base_scalar_var = std::static_pointer_cast<ScalarVar>(expr->getValue());
        scalar_val->setCurrentValue(base_scalar_var->getCurrentValue().castToType(to_int_type->getIntTypeId()));
    }
    else {
        //TODO: extend it
        ERROR("We can cast only integer scalar variables for now");
    }

}

void TypeCastExpr::emit(std::ostream &stream, std::string offset) {
    //TODO: add switch for C++ style conversions and switch for implicit casts
    stream << "((" << to_type->getName() << ") ";
    expr->emit(stream);
    stream << ")";
}

std::shared_ptr<Expr> ArithmeticExpr::integralProm(std::shared_ptr<Expr> arg) {
    if (!arg->getValue()->isScalarVar()) {
        ERROR("Can perform integral promotion only on scalar variables");
    }

    //[conv.prom]
    assert(arg->getValue()->getType()->isIntType() && "Scalar variable can have only Integral Type");
    std::shared_ptr<IntegralType> int_type = std::static_pointer_cast<IntegralType>(arg->getValue()->getType());
    if (int_type->getIntTypeId() >= IntTypeID::INT) // can't perform integral promotion
        return arg;
    //TODO: we need to check if type fits in int or unsigned int
    return std::make_shared<TypeCastExpr>(arg, IntegralType::init(IntTypeID::INT), true);
}

std::shared_ptr<Expr> ArithmeticExpr::convToBool(std::shared_ptr<Expr> arg) {
    if (!arg->getValue()->isScalarVar()) {
        ERROR("Can perform conversion to bool only on scalar variables");
    }

    std::shared_ptr<IntegralType> int_type = std::static_pointer_cast<IntegralType>(arg->getValue()->getType());
    if (int_type->getIntTypeId() == IntTypeID::BOOL)
        return arg;
    return std::make_shared<TypeCastExpr>(arg, IntegralType::init(IntTypeID::BOOL), true);
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
    assert(arg->getValue()->getKind() == DataKind::VAR && "Unary operations are supported for Scalar Variables only");
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
    assert(scalar_arg->getType()->isIntType() && "Unary operations are supported for Scalar Variables of Integral Types only");
    auto int_type = std::static_pointer_cast<IntegralType>(scalar_arg->getType());
    value = std::make_shared<ScalarVar>("", int_type, new_val);
    return value;
}

Expr::EvalResType UnaryExpr::rebuild(EvalCtx &ctx) {
    EvalResType eval_res = evaluate(ctx);
    assert(eval_res->getKind() == DataKind::VAR && "Unary operations are supported for Scalar Variables of Integral Types only");
    auto eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
    if (!eval_scalar_res->getCurrentValue().isUndefined()) {
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
        if (!eval_scalar_res->getCurrentValue().isUndefined())
            break;
        rebuild(ctx);
    }
    while (eval_scalar_res->getCurrentValue().isUndefined());

    value = eval_res;
    return value;
}

void UnaryExpr::emit(std::ostream &stream, std::string offset) {
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
    arg->emit(stream);
    stream << "))";
}

bool BinaryExpr::propagateType() {
    lhs->propagateType();
    rhs->propagateType();

    switch(op) {
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
            arithConv();
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

void BinaryExpr::arithConv() {
    if (!lhs->getValue()->getType()->isIntType() || !rhs->getValue()->getType()->isIntType()) {
        ERROR("We assume that we can perform binary operations only in Scalar Variables with integral type");
    }

    std::shared_ptr<IntegralType> lhs_type = std::static_pointer_cast<IntegralType>(lhs->getValue()->getType());
    std::shared_ptr<IntegralType> rhs_type = std::static_pointer_cast<IntegralType>(rhs->getValue()->getType());

    //[expr.arith.conv]
    // 1.5.1
    if (lhs_type->getIntTypeId() == rhs_type->getIntTypeId())
        return;

    // 1.5.2
    if (lhs_type->getIsSigned() == rhs_type->getIsSigned()) {
        std::shared_ptr<IntegralType> max_type = IntegralType::init(std::max(lhs_type->getIntTypeId(), rhs_type->getIntTypeId()));
        if (lhs_type->getIntTypeId() > rhs_type->getIntTypeId())
            rhs = std::make_shared<TypeCastExpr>(rhs, max_type, /*is_implicit*/ true);
        else
            lhs = std::make_shared<TypeCastExpr>(lhs, max_type, /*is_implicit*/ true);
        return;
    }

    // 1.5.3
    // Helper function that converts signed type to "bigger" unsigned type
    auto signed_to_unsigned_conv = [] (std::shared_ptr<IntegralType> &a_type, std::shared_ptr<IntegralType> &b_type,
                                       std::shared_ptr<Expr> &b_expr) -> bool {
         if (!a_type->getIsSigned() && (a_type->getIntTypeId() >= b_type->getIntTypeId())) {
             b_expr = std::make_shared<TypeCastExpr>(b_expr, a_type, /*is_implicit*/ true);
             return true;
         }
         return false;
    };
    if (signed_to_unsigned_conv(lhs_type, rhs_type, rhs) ||
        signed_to_unsigned_conv(rhs_type, lhs_type, lhs))
        return;

    // 1.5.4
    // Same idea, but for unsigned to signed conversions
    auto unsigned_to_signed_conv = [] (std::shared_ptr<IntegralType> &a_type, std::shared_ptr<IntegralType> &b_type,
                                       std::shared_ptr<Expr> &b_expr) -> bool {
        if (a_type->getIsSigned() && IntegralType::canRepresentType(a_type->getIntTypeId(), b_type->getIntTypeId())) {
            b_expr = std::make_shared<TypeCastExpr>(b_expr, a_type, /*is_implicit*/ true);
            return true;
        }
        return false;
    };
    if (unsigned_to_signed_conv(lhs_type, rhs_type, rhs) ||
        unsigned_to_signed_conv(rhs_type, lhs_type, lhs))
        return;

    // 1.5.5
    auto final_conversion = [] (std::shared_ptr<IntegralType> &a_type,
                                std::shared_ptr<Expr> &a_expr, std::shared_ptr<Expr> &b_expr) -> bool {
        if (a_type->getIsSigned()) {
            std::shared_ptr<IntegralType> new_type = IntegralType::init(IntegralType::getCorrUnsigned(a_type->getIntTypeId()));
            a_expr = std::make_shared<TypeCastExpr>(a_expr, new_type, /*is_implicit*/ true);
            b_expr = std::make_shared<TypeCastExpr>(b_expr, new_type, /*is_implicit*/ true);
            return true;
        }
        return false;
    };
    if (final_conversion(lhs_type, lhs, rhs) || final_conversion(rhs_type, lhs, rhs))
        return;

    ERROR("Unreachable: conversions went wrong");
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

    IRValue new_val = IRValue(lhs_val.getIntTypeID());

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

    assert(lhs->getValue()->getType()->isIntType() && "Binary operations are supported only for Scalar Variables of Integral Type");
    auto res_int_type = std::static_pointer_cast<IntegralType>(lhs->getValue()->getType());
    value = std::make_shared<ScalarVar>("", res_int_type, new_val);
    return value;
}

Expr::EvalResType BinaryExpr::rebuild(EvalCtx &ctx) {
    std::shared_ptr<Data> eval_res = evaluate(ctx);
    assert(eval_res->getKind() == DataKind::VAR && "Binary operations are supported only for Scalar Variables");

    auto eval_scalar_res = std::static_pointer_cast<ScalarVar>(eval_res);
    if (!eval_scalar_res->getCurrentValue().isUndefined()) {
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
                // Auxiliary function
                auto abs_value = [] (IRValue val) -> uint64_t {
                    switch (val.getIntTypeID()) {
                        case IntTypeID::BOOL:
                            return val.getValueRef<bool>();
                        case IntTypeID::SCHAR:
                            return std::abs(val.getValueRef<signed char>());
                        case IntTypeID::UCHAR:
                            return val.getValueRef<unsigned char>();
                        case IntTypeID::SHORT:
                            return std::abs(val.getValueRef<short>());
                        case IntTypeID::USHORT:
                            return val.getValueRef<unsigned short>();
                        case IntTypeID::INT:
                            return std::abs(val.getValueRef<int>());
                        case IntTypeID::UINT:
                            return val.getValueRef<unsigned int>();
                        case IntTypeID::LLONG:
                            return std::abs(val.getValueRef<long long int>());
                        case IntTypeID::ULLONG:
                            return val.getValueRef<unsigned long long int>();
                        case IntTypeID::MAX_INT_TYPE_ID:
                            ERROR("Bad Integral Type code");
                            break;
                    }
                };

                // First of all, we need to find the maximal valid shift value
                assert(lhs->getValue()->getType()->isIntType() && "Binary operations are supported only for Scalar Variables of Integral Types");
                auto lhs_int_type = std::static_pointer_cast<IntegralType>(lhs->getValue()->getType());
                assert(lhs->getValue()->getKind() == DataKind::VAR && "Binary operations are supported only for Scalar Variables");
                auto lhs_scalar_var = std::static_pointer_cast<ScalarVar>(lhs->getValue());
                // We can't shift pass the type size
                uint64_t max_sht_val = lhs_int_type->getBitSize();
                // And we can't shift MSB pass the type size
                if (op == BinaryOp::SHL && lhs_int_type->getIsSigned() && ub == UBKind::ShiftRhsLarge)
                    max_sht_val -= findMSB(abs_value(lhs_scalar_var->getCurrentValue()));

                // Secondly, we choose a new shift value in a valid range
                uint64_t new_val = rand_val_gen->getRandValue(0UL, max_sht_val);

                // Thirdly, we need to combine the chosen value with the existing one
                assert(rhs->getValue()->getType()->isIntType() && "Binary operations are supported only for Scalar Variables of Integral Types");
                auto rhs_int_type = std::static_pointer_cast<IntegralType>(rhs->getValue()->getType());
                assert(rhs->getValue()->getKind() == DataKind::VAR && "Binary operations are supported only for Scalar Variables");
                auto rhs_scalar_var = std::static_pointer_cast<ScalarVar>(rhs->getValue());
                if (ub == UBKind::ShiftRhsNeg)
                    //TODO: it won't work for INT_MIN
                    new_val = std::min(new_val + abs_value(rhs_scalar_var->getCurrentValue()),
                                       static_cast<uint64_t>(rhs_int_type->getBitSize()));
                // UBKind::ShiftRhsLarge
                else
                    new_val = abs_value(rhs_scalar_var->getCurrentValue()) - new_val;

                // Finally, we need to make changes to the program
                IRValue adjust_val = IRValue(rhs_int_type->getIntTypeId());
                //TODO: We need to check that it actually works as intended
                adjust_val.getValueRef<uint64_t>() = new_val;
                auto const_val = std::make_shared<ConstantExpr>(adjust_val);
                if (ub == UBKind::ShiftRhsNeg)
                    rhs = std::make_shared<BinaryExpr>(BinaryOp::ADD, rhs, const_val);
                // UBKind::ShiftRhsLarge
                else
                    rhs = std::make_shared<BinaryExpr>(BinaryOp::SUB, rhs, const_val);
            }
            //UBKind::NegShift
            else {
                // We can just add maximal value of the type
                assert(lhs->getValue()->getType()->isIntType() && "Binary operations are supported only for Scalar Variables of Integral Types");
                auto lhs_int_type = std::static_pointer_cast<IntegralType>(lhs->getValue()->getType());
                auto const_val = std::make_shared<ConstantExpr>(lhs_int_type->getMax());
                lhs = std::make_shared<BinaryExpr>(BinaryOp::ADD, lhs, const_val);
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
        if (!eval_scalar_res->getCurrentValue().isUndefined())
            break;
        rebuild(ctx);
    }
    while (eval_scalar_res->getCurrentValue().isUndefined());

    value = eval_res;
    return eval_res;
}

void BinaryExpr::emit(std::ostream &stream, std::string offset) {
    stream << offset << "(";
    lhs->emit(stream);
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
    rhs->emit(stream);
    stream << ")";
}

