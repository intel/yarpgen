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

//////////////////////////////////////////////////////////////////////////////

#include "node.h"
void VarUseExpr::set_variable (std::shared_ptr<Variable> _var) {
    var = _var;
    value.set_type(var->get_type());
}

void AssignExpr::set_to (std::shared_ptr<Expr> _to) {
    to = _to;
    // TODO: choose strategy
    value.set_type(Type::init (to->get_type_id()));
}

void AssignExpr::set_from (std::shared_ptr<Expr> _from) {
    // TODO: add type conversion expr
    from = _from;
}

void AssignExpr::propagate_type () {
    value.set_type(Type::init(to->get_type_id()));

    if (from->get_type_id() != to->get_type_id())
        return;
    TypeCastExpr ins;
    ins.set_type(Type::init(to->get_type_id()));
    ins.set_expr(from);
    ins.propagate_value();
    from = std::make_shared<TypeCastExpr>(ins);
}

std::string AssignExpr::emit () {
    std::string ret = to->emit();
    ret += " = ";
    ret += from->emit();
    return ret;
}

void IndexExpr::set_base (std::shared_ptr<Array> _base) {
    base = _base;
    value.set_type(base->get_type());
}

std::string IndexExpr::emit () {
    std::string ret = base->get_name();
    ret += is_subscr ? " [" : ".at(";
    ret += index->emit();
    ret += is_subscr ? "]" : ")";
    return ret;
}

std::shared_ptr<Expr> ArithExpr::integral_prom (std::shared_ptr<Expr> arg) {
    //[conv.prom]
    if (arg->get_type_id() >= Type::TypeID::INT) // can't perform integral promotiom
        return arg;
    TypeCastExpr ret;
    ret.set_type(Type::init(Type::TypeID::INT));
    ret.set_expr(arg);
    ret.propagate_value();
    return std::make_shared<TypeCastExpr>(ret);
}

std::shared_ptr<Expr> ArithExpr::conv_to_bool (std::shared_ptr<Expr> arg) {
    if (arg->get_type_id() == Type::TypeID::BOOL)
        return arg;
    TypeCastExpr ret;
    ret.set_type(Type::init(Type::TypeID::BOOL));
    ret.set_expr(arg);
    ret.propagate_value();
    return std::make_shared<TypeCastExpr>(ret);
}

void BinaryExpr::perform_arith_conv () {
    // integral promotion should be a part of it, but it was moved to base class
    // 10.5.1
    if (arg0->get_type_id() == arg1->get_type_id())
        return;
    // 10.5.2
    if (arg0->get_type_sign() == arg1->get_type_sign()) {
        TypeCastExpr ins;
        ins.set_type(Type::init(std::max(arg0->get_type_id(), arg1->get_type_id())));
        if (arg0->get_type_id() <  arg1->get_type_id()) {
            ins.set_expr(arg0);
            ins.propagate_value();
            arg0 = std::make_shared<TypeCastExpr>(ins);
        }
        else {
            ins.set_expr(arg1);
            ins.propagate_value();
            arg1 = std::make_shared<TypeCastExpr>(ins);
        }
        return;
    }
    if ((!arg0->get_type_sign() && (arg0->get_type_id() >= arg1->get_type_id())) || // 10.5.3
         (arg0->get_type_sign() && Type::can_repr_value (arg1->get_type_id(), arg0->get_type_id()))) { // 10.5.4
        TypeCastExpr ins;
        ins.set_type(Type::init(arg0->get_type_id()));
        ins.set_expr(arg1);
        ins.propagate_value();
        arg1 = std::make_shared<TypeCastExpr>(ins);
        return;
    }
    if ((!arg1->get_type_sign() && (arg1->get_type_id() >= arg0->get_type_id())) || // 10.5.3
         (arg1->get_type_sign() && Type::can_repr_value (arg0->get_type_id(), arg1->get_type_id()))) { // 10.5.4
        TypeCastExpr ins;
        ins.set_type(Type::init(arg1->get_type_id()));
        ins.set_expr(arg0);
        ins.propagate_value();
        arg0 = std::make_shared<TypeCastExpr>(ins);
        return;
    }
    // 10.5.5
    TypeCastExpr ins0;
    TypeCastExpr ins1;
    if (arg0->get_type_sign()) {
        ins0.set_type(Type::init(Type::get_corr_unsig(arg0->get_type_id())));
        ins1.set_type(Type::init(Type::get_corr_unsig(arg0->get_type_id())));
    }
    if (arg1->get_type_sign()) {
        ins0.set_type(Type::init(Type::get_corr_unsig(arg1->get_type_id())));
        ins1.set_type(Type::init(Type::get_corr_unsig(arg1->get_type_id())));
    }
    ins0.set_expr(arg0);
    ins0.propagate_value();
    arg0 = std::make_shared<TypeCastExpr>(ins0);
    ins1.set_expr(arg1);
    ins1.propagate_value();
    arg1 = std::make_shared<TypeCastExpr>(ins1);
}

void BinaryExpr::propagate_type () {
    if (op == MaxOp || arg0 == NULL || arg1 == NULL) {
        std::cerr << "ERROR: BinaryExpr::propagate_type specify args" << std::endl;
        value.set_type(Type::init (Type::TypeID::MAX_TYPE_ID));
        return;
    }
    switch (op) {
        case Add:
        case Sub:
        case Mul:
        case Div:
        case Mod:
        case Lt:
        case Gt:
        case Le:
        case Ge:
        case Eq:
        case Ne:
        case BitAnd:
        case BitXor:
        case BitOr:
            // arithmetic conversions
            arg0 = integral_prom (arg0);
            arg1 = integral_prom (arg1);
            perform_arith_conv();
            break;
        case Shl:
        case Shr:
            arg0 = integral_prom (arg0);
            arg1 = integral_prom (arg1);
            break;
        case LogAnd:
        case LogOr:
            arg0 = conv_to_bool (arg0);
            arg1 = conv_to_bool (arg1);
            break;
        case MaxOp:
            std::cerr << "ERROR: BinaryExpr::propagate_type bad operator" << std::endl;
            break;
    }
    if (op == Lt || op == Gt || op == Le || op == Ge || op == Eq || op == Ne)
        value.set_type(Type::init (Type::TypeID::INT)); // promote bool to int TODO: it isn't right way to do it for cmp operations
    else
        value.set_type(Type::init (arg0->get_type_id()));
}

bool check_int64_mul (int64_t a, int64_t b, int64_t* res) {
    uint64_t ret = 0;

    int8_t sign = (((a > 0) && (b > 0)) || ((a < 0) && (b < 0))) ? 1 : -1;
    uint64_t a_abs = 0;
    uint64_t b_abs = 0;

    if (a == INT64_MIN)
        // Operation "-" is undefined for "INT64_MIN", as it causes overflow.
        // But converting INT64_MIN to unsigned type yields the correct result,
        // i.e. it will be positive value -INT64_MIN.
        // See 6.3.1.3 section in C99 standart for more details (ISPC follows
        // C standard, unless it's specifically different in the language).
        a_abs = (uint64_t) INT64_MIN;
    else
        a_abs = (a > 0) ? a : -a;

    if (b == INT64_MIN)
        b_abs = (uint64_t) INT64_MIN;
    else
        b_abs = (b > 0) ? b : -b;

    uint32_t a0 = a_abs & 0xFFFFFFFF;
    uint32_t b0 = b_abs & 0xFFFFFFFF;
    uint32_t a1 = a_abs >> 32;
    uint32_t b1 = b_abs >> 32;

    if ((a1 != 0) && (b1 != 0))
        return false;

    uint64_t tmp = (((uint64_t) a1) * b0) + (((uint64_t) b1) * a0);
    if (tmp > 0xFFFFFFFF)
        return false;

    ret = (tmp << 32) + (((uint64_t) a0) * b0);
    if (ret < (tmp << 32))
        return false;

    if ((sign < 0) && (ret > (uint64_t) INT64_MIN)) {
        return false;
    } else if ((sign > 0) && (ret > INT64_MAX)) {
        return false;
    } else {
        *res = ret * sign;
    }
    return true;
}

static uint64_t msb(uint64_t x) {
    uint64_t ret = 0;
    while (x != 0) {
        ret++;
        x = x >> 1;
    }
    return ret;
}

Expr::UB BinaryExpr::propagate_value () {
    if (op == MaxOp || arg0 == NULL || arg1 == NULL) {
        std::cerr << "ERROR: BinaryExpr::propagate_value specify args" << std::endl;
        return NullPtr;
    }
    bool int_eq_long = sizeof(int) == sizeof(long int);
    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    uint64_t a = arg0->get_value();
    uint64_t b = arg1->get_value();

    int64_t s_tmp = 0;
    uint64_t u_tmp = 0;

    switch (op) {
        case Add:
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    s_tmp = (long long int) a + (long long int) b;
                    if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                        return SignOvf;
                    value.set_value((int) s_tmp);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) a + (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    if (!long_eq_long_long) {
                        s_tmp = (long long int) a + (long long int) b;
                        if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                            return SignOvf;
                        value.set_value((long int) s_tmp);
                    }
                    else {
                        uint64_t ua = (long int) a;
                        uint64_t ub = (long int) b;
                        u_tmp = ua + ub;
                        ua = (ua >> 63) + LONG_MAX;
                        if ((int64_t) ((ua ^ ub) | ~(ub ^ u_tmp)) >= 0)
                            return SignOvf;
                        value.set_value((long int) u_tmp);
                    }
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a + (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                {
                    uint64_t ua = (long long int) a;
                    uint64_t ub = (long long int) b;
                    u_tmp = ua + ub;
                    ua = (ua >> 63) + LLONG_MAX;
                    if ((int64_t) ((ua ^ ub) | ~(ub ^ u_tmp)) >= 0)
                        return SignOvf;
                    value.set_value((long long int) a + (long long int) b);
                    break;
                }
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a + (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Sub:
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    s_tmp = (long long int) a - (long long int) b;
                    if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                        return SignOvf;
                    value.set_value((int) s_tmp);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) a - (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    if (!long_eq_long_long) {
                        s_tmp = (long long int) a - (long long int) b;
                        if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                            return SignOvf;
                        value.set_value((long int) s_tmp);
                    }
                    else {
                        uint64_t ua = (long int) a;
                        uint64_t ub = (long int) b;
                        u_tmp = ua - ub;
                        ua = (ua >> 63) + LONG_MAX;
                        if ((int64_t) ((ua ^ ub) & (ua ^ u_tmp)) < 0)
                            return SignOvf;
                        value.set_value((long int) u_tmp);
                    }
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a - (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                {
                    uint64_t ua = (long long int) a;
                    uint64_t ub = (long long int) b;
                    u_tmp = ua - ub;
                    ua = (ua >> 63) + LLONG_MAX;
                    if ((int64_t) ((ua ^ ub) & (ua ^ u_tmp)) < 0)
                        return SignOvf;
                    value.set_value((long long int) u_tmp);
                    break;
                }
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a - (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Mul:
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    s_tmp = (long long int) a * (long long int) b;
                    if ((int) a == INT_MIN && (int) b == -1)
                        return SignOvfMin;
                    if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                        return SignOvf;
                    value.set_value((int) s_tmp);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) a * (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    if ((long int) a == LONG_MIN && (long int) b == -1)
                        return SignOvfMin;
                    if (!long_eq_long_long) {
                        s_tmp = (long long int) a * (long long int) b;
                        if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                            return SignOvf;
                        value.set_value((long int) s_tmp);
                    }
                    else {
                        if (!check_int64_mul((long int) a, (long int) b, &s_tmp))
                            return SignOvf;
                        value.set_value((long int) s_tmp);
                    }
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a * (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    if ((long long int) a == LLONG_MIN && (long long int) b == -1)
                        return SignOvfMin;
                    if (!check_int64_mul((long long int) a, (long long int) b, &s_tmp))
                        return SignOvf;
                    value.set_value((long long int) s_tmp);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a * (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Div:
            if (b == 0)
                return ZeroDiv;
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    s_tmp = (long long int) a / (long long int) b;
                    if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                        return SignOvf;
                    value.set_value((int) s_tmp);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) a / (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    if (!long_eq_long_long) {
                        s_tmp = (long long int) a / (long long int) b;
                        if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                            return SignOvf;
                        value.set_value((long int) s_tmp);
                    }
                    else {
                        if (((long int) a == LONG_MIN && (long int) b == -1) || 
                            ((long int) b == LONG_MIN && (long int) a == -1))
                            return SignOvf;
                        value.set_value((long int) a / (long int) b);
                    }
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a / (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    if (((long long int) a == LLONG_MIN && (long long int) b == -1) ||
                        ((long long int) b == LLONG_MIN && (long long int) a == -1))
                        return SignOvf;
                    value.set_value((long long int) a / (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a / (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Mod:
            if (b == 0)
                return ZeroDiv;
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    s_tmp = (long long int) a / (long long int) b;
                    if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                        return SignOvf;
                    value.set_value((int) a % (int) b);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) a % (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    if (!long_eq_long_long) {
                        s_tmp = (long long int) a / (long long int) b;
                        if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                            return SignOvf;
                        value.set_value((long int) a % (long int) b);
                    }
                    else {
                        if (((long int) a == LONG_MIN && (long int) b == -1) ||
                            ((long int) b == LONG_MIN && (long int) a == -1))
                            return SignOvf;
                        value.set_value((long int) a % (long int) b);
                    }
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a % (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    if (((long long int) a == LLONG_MIN && (long long int) b == -1) ||
                        ((long long int) b == LLONG_MIN && (long long int) a == -1))
                        return SignOvf;
                    value.set_value((long long int) a % (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a % (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Lt:
            switch(get_rhs()->get_type_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    value.set_value((int) a < (int) b);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) (unsigned int) a < (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    value.set_value((long int) a < (long int) b);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a < (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    value.set_value((long long int) a < (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a < (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Gt:
            switch(get_rhs()->get_type_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    value.set_value((int) a > (int) b);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) (unsigned int) a > (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    value.set_value((long int) a > (long int) b);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a > (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    value.set_value((long long int) a > (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a > (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Le:
            switch(get_rhs()->get_type_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    value.set_value((int) a <= (int) b);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) (unsigned int) a <= (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    value.set_value((long int) a <= (long int) b);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a <= (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    value.set_value((long long int) a <= (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a <= (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Ge:
            switch(get_rhs()->get_type_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    value.set_value((int) a >= (int) b);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) (unsigned int) a >= (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    value.set_value((long int) a >= (long int) b);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a >= (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    value.set_value((long long int) a >= (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a >= (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Eq:
            switch(get_rhs()->get_type_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    value.set_value((int) a == (int) b);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) (unsigned int) a == (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    value.set_value((long int) a == (long int) b);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a == (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    value.set_value((long long int) a == (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a == (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Ne:
            switch(get_rhs()->get_type_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    value.set_value((int) a != (int) b);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) (unsigned int) a != (unsigned int) b);
                    break;
                case Type::TypeID::LINT:
                    value.set_value((long int) a != (long int) b);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a != (unsigned long int) b);
                    break;
                case Type::TypeID::LLINT:
                    value.set_value((long long int) a != (long long int) b);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a != (unsigned long long int) b);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in BinaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case BitAnd:
            value.set_value(a & b);
            break;
        case BitOr:
            value.set_value(a | b);
            break;
        case BitXor:
            value.set_value(a ^ b);
            break;
        case Shl:
        case Shr:
            if (arg0->get_type_id() == Type::TypeID::BOOL ||
                arg0->get_type_id() == Type::TypeID::CHAR ||
                arg0->get_type_id() == Type::TypeID::UCHAR ||
                arg0->get_type_id() == Type::TypeID::SHRT ||
                arg0->get_type_id() == Type::TypeID::USHRT ||
                arg1->get_type_id() == Type::TypeID::BOOL ||
                arg1->get_type_id() == Type::TypeID::CHAR ||
                arg1->get_type_id() == Type::TypeID::UCHAR ||
                arg1->get_type_id() == Type::TypeID::SHRT ||
                arg1->get_type_id() == Type::TypeID::USHRT) {
                std::cerr << "BinaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                break;
            }
            if (arg0->get_type_is_signed() && (int64_t) a < 0)
                return NegShift;
            if (arg1->get_type_is_signed() && (int64_t) b < 0)
                return ShiftRhsNeg;
//            std::cout << "DEBUG: " << a << " | " << msb(a) << std::endl;
            if (b >= (arg0->get_type_bit_size()))
                return ShiftRhsLarge;
            if (op == Shl && arg0->get_type_is_signed() && 
               (b >= (arg0->get_type_bit_size() - msb((uint64_t)a)))) // msb applied only for unsigned values
                return ShiftRhsLarge;
            // TODO: I hope it will work
            if (op == Shl)
                value.set_value(a << b);
            else
                value.set_value(a >> b);
            break;
        case LogAnd:
            value.set_value((bool) a && (bool) b);
            break;
        case LogOr:
            value.set_value((bool) a || (bool) b);
            break;
        case MaxOp:
            std::cerr << "BinaryExpr::propagate_value : MaxOp"  << std::endl;
            break;
    }
    return NoUB;
}

std::string BinaryExpr::emit () {
    std::string ret = "((" + arg0->emit() + ")";
    switch (op) {
        case Add:
            ret += " + ";
            break;
        case Sub:
            ret += " - ";
            break;
        case Mul:
            ret += " * ";
            break;
        case Div:
            ret += " / ";
            break;
        case Mod:
            ret += " % ";
            break;
        case Shl:
            ret += " << ";
            break;
        case Shr:
            ret += " >> ";
            break;
        case Lt:
            ret += " < ";
            break;
        case Gt:
            ret += " > ";
            break;
        case Le:
            ret += " <= ";
            break;
        case Ge:
            ret += " >= ";
            break;
        case Eq:
            ret += " == ";
            break;
        case Ne:
            ret += " != ";
            break;
        case BitAnd:
            ret += " & ";
            break;
        case BitXor:
            ret += " ^ ";
            break;
        case BitOr:
            ret += " | ";
            break;
        case LogAnd:
            ret += " && ";
            break;
        case LogOr:
            ret += " || ";
            break;
        case MaxOp:
            std::cerr << "BAD OPERATOR" << std::endl;
            break;
        }
        ret += "(" + arg1->emit() + "))";
        return ret;
}

void UnaryExpr::propagate_type () {
    if (op == MaxOp || arg == NULL) {
        std::cerr << "ERROR: UnaryExpr::propagate_type specify args" << std::endl;
        value.set_type(Type::init (Type::TypeID::MAX_TYPE_ID));
        return;
    }
    switch (op) {
        case PreInc:
        case PreDec:
        case PostInc:
        case PostDec:
            break;
        case Plus:
        case Negate:
        case BitNot:
            arg = integral_prom (arg);
            break;
        case LogNot:
            arg = conv_to_bool (arg);
            break;
        case MaxOp:
            std::cerr << "ERROR: UnaryExpr::propagate_type bad operator" << std::endl;
            break;
    }
    value.set_type(Type::init (arg->get_type_id()));
}

Expr::UB UnaryExpr::propagate_value () {
    if (op == MaxOp || arg == NULL) {
        std::cerr << "ERROR: UnaryExpr::propagate_value specify args" << std::endl;
        return NullPtr;
    }
    uint64_t a = arg->get_value();
    switch (op) {
        case PreInc:
        case PostInc:
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "UnaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    if (a == INT_MAX)
                        return SignOvf;
                    value.set_value((int) a + 1);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) a + 1);
                    break;
                case Type::TypeID::LINT:
                    if (a == LONG_MAX)
                        return SignOvf;
                    value.set_value((long int) a + 1);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a + 1);
                    break;
                case Type::TypeID::LLINT:
                    if (a == LLONG_MAX)
                        return SignOvf;
                    value.set_value((long long int) a + 1);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a + 1);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in UnaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case PreDec:
        case PostDec:
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "UnaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    if (a == INT_MIN)
                        return SignOvf;
                    value.set_value((int) a - 1);
                    break;
                case Type::TypeID::UINT:
                    value.set_value((unsigned int) a - 1);
                    break;
                case Type::TypeID::LINT:
                    if (a == LONG_MIN)
                        return SignOvf;
                    value.set_value((long int) a - 1);
                    break;
                case Type::TypeID::ULINT:
                    value.set_value((unsigned long int) a - 1);
                    break;
                case Type::TypeID::LLINT:
                    if (a == LLONG_MIN)
                        return SignOvf;
                    value.set_value((long long int) a - 1);
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value((unsigned long long int) a - 1);
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in UnaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case Plus:
            value.set_value(a);
            break;
        case Negate:
            switch(value.get_type()->get_id()) {
                case Type::TypeID::BOOL:
                case Type::TypeID::CHAR:
                case Type::TypeID::UCHAR:
                case Type::TypeID::SHRT:
                case Type::TypeID::USHRT:
                    std::cerr << "UnaryExpr::propagate_value : perform propagate_type()"  << std::endl;
                    break;
                case Type::TypeID::INT:
                    if (a == INT_MIN)
                        return SignOvf;
                    value.set_value(-((int) a));
                    break;
                case Type::TypeID::UINT:
                    value.set_value(-((unsigned int) a));
                    break;
                case Type::TypeID::LINT:
                    if (a == LONG_MIN)
                        return SignOvf;
                    value.set_value(-((long int) a));
                    break;
                case Type::TypeID::ULINT:
                    value.set_value(-((unsigned long int) a));
                    break;
                case Type::TypeID::LLINT:
                    if (a == LLONG_MIN)
                        return SignOvf;
                    value.set_value(-((long long int) a));
                    break;
                case Type::TypeID::ULLINT:
                    value.set_value(-((unsigned long long int) a));
                    break;
                case Type::TypeID::PTR:
                case Type::TypeID::MAX_INT_ID:
                case Type::TypeID::MAX_TYPE_ID:
                    std::cerr << "ERROR in UnaryExpr::propagate_value" << std::endl;
                    break;
            }
            break;
        case BitNot:
            value.set_value(~a);
            break;
        case LogNot:
            value.set_value((bool)!a);
            break;
        case MaxOp:
            std::cerr << "ERROR in UnaryExpr::propagate_value : MaxOp" << std::endl;
            break;
    }
    return NoUB;
}

std::string UnaryExpr::emit () {
    std::string op_str = "";
    switch (op) {
        case PreInc:
        case PostInc:
            op_str = "++";
            break;
        case PreDec:
        case PostDec:
            op_str = "--";
            break;
        case Plus:
            op_str = "+";
            break;
        case Negate:
            op_str = "-";
            break;
        case LogNot:
            op_str = "!";
            break;
        case BitNot:
            op_str = "~";
            break;
        case MaxOp:
            std::cerr << "BAD OPERATOR" << std::endl;
            break;
    }
    std::string ret = "";
    if (op == PostInc || op == PostDec)
        ret = "((" + arg->emit() + ")" + op_str + ")";
    else
        ret = "(" + op_str + "(" + arg->emit() + "))";
    return ret;
}

std::string ConstExpr::emit () {
    std::string ret = "";
    switch (get_type_id ()) {
        case Type::TypeID::BOOL:
            ret += std::to_string((bool) data);
            break;
        case Type::TypeID::CHAR:
            ret += std::to_string((signed char) data);
            break;
        case Type::TypeID::UCHAR:
            ret += std::to_string((unsigned char) data);
            break;
        case Type::TypeID::SHRT:
            ret += std::to_string((short) data);
            break;
        case Type::TypeID::USHRT:
            ret += std::to_string((unsigned short) data);
            break;
        case Type::TypeID::INT:
            ret += std::to_string((int) data);
            break;
        case Type::TypeID::UINT:
            ret += std::to_string((unsigned int) data);
            break;
        case Type::TypeID::LINT:
            ret += std::to_string((long int) data);
            break;
        case Type::TypeID::ULINT:
            ret += std::to_string((unsigned long int) data);
            break;
        case Type::TypeID::LLINT:
            ret += std::to_string((long long int) data);
            break;
        case Type::TypeID::ULLINT:
            ret += std::to_string((unsigned long long int) data);
            break;
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE in ConstExpr::emit ()" << std::endl;
            break;
    }
    ret += value.get_type()->get_suffix ();
    return ret;
}

std::string TypeCastExpr::emit () {
    std::string ret = "(" + value.get_type()->get_name() + ")";
    ret += "(" + expr->emit() + ")";
    return ret;
}

std::string ExprListExpr::emit () {
    std::string ret = "(";
    for (auto i = expr_list.begin(); i != expr_list.end(); ++i)
        ret += (*i)->emit() + ", ";
    ret = ret.substr(0, ret.size() - 2); // Remove last comma
    ret += ")";
    return ret;
}

std::string FuncCallExpr::emit () {
    return name.c_str() + args->emit();
}

std::string DeclStmt::emit () {
    std::string ret = "";
    ret += data->get_is_static() && !is_extern ? "static " : "";
    ret += is_extern ? "extern " : "";
    switch (data->get_modifier()) {
        case Variable::Mod::VOLAT:
            ret += "volatile ";
            break;
        case Variable::Mod::CONST:
            ret += "const ";
            break;
        case Variable::Mod::CONST_VOLAT:
            ret += "const volatile ";
            break;
        case Variable::Mod::NTHNG:
            break;
        case Variable::Mod::MAX_MOD:
            std::cerr << "ERROR in DeclStmt::emit bad modifier" << std::endl;
            break;
    }
    if (data->get_class_id() == Variable::VarClassID::ARR) {
        std::shared_ptr<Array> arr = std::static_pointer_cast<Array>(data);
        switch (arr->get_essence()) {
            case Array::Ess::STD_ARR:
                ret += "std::array<" + arr->get_base_type()->get_name() + ", " + std::to_string(arr->get_size()) + ">";
                ret += " " + arr->get_name();
                break;
            case Array::Ess::STD_VEC:
                ret += "std::vector<" + arr->get_base_type()->get_name() + ">";
                ret += " " + arr->get_name();
                ret += is_extern ? "" : " (" + std::to_string(arr->get_size()) + ", 0)";
                break;
            case Array::Ess::C_ARR:
                ret +=  arr->get_base_type()->get_name();
                ret += " " + arr->get_name();
                ret += " [" + std::to_string(arr->get_size()) + "]";
                break;
            case Array::Ess::VAL_ARR:
                ret += "std::valarray<" + arr->get_base_type()->get_name() + ">";
                ret += " " + arr->get_name();
                ret += is_extern ? "" : " ((" + arr->get_base_type()->get_name() + ") 0, " + std::to_string(arr->get_size()) + ")";
                break;
            case Array::Ess::MAX_ESS:
                std::cerr << "ERROR in DeclStmt::emit bad array essence" << std::endl;
                break;
        }
    }
    else {
        ret += data->get_type()->get_name() + " " + data->get_name();
    }
    if (data->get_align() != 0 && is_extern) // TODO: Should we set __attribute__ to non-extern variable?
        ret += " __attribute__((aligned(" + std::to_string(data->get_align()) + ")))";
    if (init != NULL) {
        if (data->get_class_id() == Variable::VarClassID::ARR) {
            std::cerr << "ERROR in DeclStmt::emit init of array" << std::endl;
            return ret;
        }
        if (is_extern) {
            std::cerr << "ERROR in DeclStmt::emit init of extern" << std::endl;
            return ret;
        }
        ret += " = " + init->emit();
    }
    ret += ";";
    return ret;
}

std::string ExprStmt::emit() {
    return expr->emit() + ";";
}

std::string CntLoopStmt::emit() {
    std::string ret;
    switch (loop_id) {
        case LoopStmt::LoopID::FOR:
            ret += "for (";
            ret += iter_decl->emit() + "; ";
            ret += cond->emit() + "; ";
            ret += step_expr->emit() + ") {\n";
            break;
        case LoopStmt::LoopID::WHILE:
            ret += iter_decl->emit() + ";\n";
            ret += "while (" + cond->emit() + ") {\n";
            break;
        case LoopStmt::LoopID::DO_WHILE:
            ret += iter_decl->emit() + ";\n";
            ret += "do {\n";
            break;
        case LoopStmt::LoopID::MAX_LOOP_ID:
            std::cerr << "ERROR in CntLoopStmt::emit invalid loop id" << std::endl;
            break;
    }

    for (auto i = this->body.begin(); i != this->body.end(); ++i) {
        if ((*i)->get_id() == NodeID::IF)
            ret += (*i)->emit() + "\n";
        else
            ret += (*i)->emit() + ";\n";
    }
    if (loop_id == LoopStmt::LoopID::WHILE ||
        loop_id == LoopStmt::LoopID::DO_WHILE) {
        ret += step_expr->emit() + ";\n";
    }

    ret += "}\n";

    if (loop_id == LoopStmt::LoopID::DO_WHILE) {
        ret += "while (" + cond->emit() + ");";
    }

    return ret;
}

std::string IfStmt::emit () {
    std::string ret;
    ret += "if (" + cond->emit() + ") {\n";
    for (auto i = if_branch.begin(); i != if_branch.end(); ++i) {
        ret += (*i)->emit() + ";\n";
    }
    if (else_exist) {
        ret += "}\n";
        ret += "else {\n";
        for (auto i = else_branch.begin(); i != else_branch.end(); ++i) {
            ret += (*i)->emit() + ";\n";
        }
    }
    ret += "}";
    return ret;
}
