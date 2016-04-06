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

#include "type.h"
#include "sym_table.h"

using namespace rl;

std::string Type::get_name () {
    std::string ret = "";
    ret += is_static ? "static " : "";
    switch (modifier) {
        case Mod::VOLAT:
            ret += "volatile ";
            break;
        case Mod::CONST:
            ret += "const ";
            break;
        case Mod::CONST_VOLAT:
            ret += "const volatile ";
            break;
        case Mod::NTHG:
            break;
        case Mod::MAX_MOD:
            std::cerr << "ERROR in Type::get_name: bad modifier" << std::endl;
            exit(-1);
            break;
    }
    ret += name;
    if (align != 0)
        ret += " __attribute__(aligned(" + std::to_string(align) + "))";
    return ret;
}

void StructType::add_member (std::shared_ptr<Type> _type, std::string _name) {
    StructType::StructMember new_mem (_type, _name);
    if (_type->is_struct_type()) {
        nest_depth = std::static_pointer_cast<StructType>(_type)->get_nest_depth() >= nest_depth ?
                     std::static_pointer_cast<StructType>(_type)->get_nest_depth() + 1 : nest_depth;
    }
    members.push_back(std::make_shared<StructMember>(new_mem));
}

std::shared_ptr<StructType::StructMember> StructType::get_member (unsigned int num) {
    if (num >= members.size())
        return NULL;
    else
        return members.at(num);
}

std::string StructType::get_definition (std::string offset) {
    std::string ret = "";
    ret+= name + " {\n";
    for (auto i : members) {
        ret += i->get_definition(offset + "    ") + ";\n";
    }
    ret += "};\n";
    return ret;
}

void StructType::dbg_dump() {
    std::cout << get_definition () << std::endl;
    std::cout << "depth: " << nest_depth << std::endl;
}

std::shared_ptr<StructType> StructType::generate (Context ctx) {
    std::vector<std::shared_ptr<StructType>> empty_vec;
    return generate(ctx, empty_vec);
}

std::shared_ptr<StructType> StructType::generate (Context ctx, std::vector<std::shared_ptr<StructType>> nested_struct_types) {
    Type::Mod primary_mod = ctx.get_gen_policy()->get_allowed_modifiers().at(rand_val_gen->get_rand_value<int>(0, ctx.get_gen_policy()->get_allowed_modifiers().size() - 1));

    bool primary_static_spec = false;
    //TODO: allow static members in struct
/*
    if (ctx.get_gen_policy()->get_allow_static_var())
        primary_static_spec = rand_val_gen->get_rand_value<int>(0, 1);
    else
        primary_static_spec = false;
*/
    IntegerType::IntegerTypeID int_type_id = (IntegerType::IntegerTypeID) rand_val_gen->get_rand_id(ctx.get_gen_policy()->get_allowed_int_types());
    //TODO: what about align?
    std::shared_ptr<Type> primary_type = IntegerType::init(int_type_id, primary_mod, primary_static_spec, 0);

    std::shared_ptr<StructType> struct_type = std::make_shared<StructType>(rand_val_gen->get_struct_type_name());
    int struct_member_num = rand_val_gen->get_rand_value<int>(ctx.get_gen_policy()->get_min_struct_members_num(), ctx.get_gen_policy()->get_max_struct_members_num());
    int member_num = 0;
    for (int i = 0; i < struct_member_num; ++i) {
        if (ctx.get_gen_policy()->get_allow_mix_types_in_struct()) {
            Data::VarClassID member_class = rand_val_gen->get_rand_id(ctx.get_gen_policy()->get_member_class_prob());
            bool add_substruct = false;
            int substruct_type_idx = 0;
            std::shared_ptr<StructType> substruct_type = NULL;
            if (member_class == Data::VarClassID::STRUCT && ctx.get_gen_policy()->get_max_struct_depth() > 0 && nested_struct_types.size() > 0) {
                substruct_type_idx = rand_val_gen->get_rand_value<int>(0, nested_struct_types.size() - 1);
                substruct_type = nested_struct_types.at(substruct_type_idx);
                if (substruct_type->get_nest_depth() + 1 == ctx.get_gen_policy()->get_max_struct_depth()) {
                    add_substruct = false;
                }
                else {
                    add_substruct = true;
                }
            }
            if (add_substruct) {
                primary_type = substruct_type;
            }
            else {
                primary_type = IntegerType::generate(ctx);
            }
        }
        if (ctx.get_gen_policy()->get_allow_mix_mod_in_struct()) {
            primary_mod = ctx.get_gen_policy()->get_allowed_modifiers().at(rand_val_gen->get_rand_value<int>(0, ctx.get_gen_policy()->get_allowed_modifiers().size() - 1));;
//            static_spec_gen.generate ();
//            primary_static_spec = static_spec_gen.get_specifier ();
        }
        primary_type->set_modifier(primary_mod);
        primary_type->set_is_static(primary_static_spec);
        struct_type->add_member(primary_type, "member_" + std::to_string(rand_val_gen->get_struct_type_num()) + "_" + std::to_string(member_num++));
    }
    return struct_type;
}

//TODO: maybe we can use template instead of it?
#define CAST_CASE(new_val_memb)                                     \
switch (int_type_id) {                                              \
    case Type::IntegerTypeID::BOOL:                                 \
        new_val_memb = val.bool_val;                                \
        break;                                                      \
    case Type::IntegerTypeID::CHAR:                                 \
        new_val_memb = val.char_val;                                \
        break;                                                      \
    case Type::IntegerTypeID::UCHAR:                                \
        new_val_memb = val.uchar_val;                               \
        break;                                                      \
    case Type::IntegerTypeID::SHRT:                                 \
        new_val_memb = val.shrt_val;                                \
        break;                                                      \
    case Type::IntegerTypeID::USHRT:                                \
        new_val_memb = val.ushrt_val;                               \
        break;                                                      \
    case Type::IntegerTypeID::INT:                                  \
        new_val_memb = val.int_val;                                 \
        break;                                                      \
    case Type::IntegerTypeID::UINT:                                 \
        new_val_memb = val.uint_val;                                \
        break;                                                      \
    case Type::IntegerTypeID::LINT:                                 \
        new_val_memb = val.lint_val;                                \
        break;                                                      \
    case Type::IntegerTypeID::ULINT:                                \
        new_val_memb = val.ulint_val;                               \
        break;                                                      \
    case Type::IntegerTypeID::LLINT:                                \
        new_val_memb = val.llint_val;                               \
        break;                                                      \
    case Type::IntegerTypeID::ULLINT:                               \
        new_val_memb = val.ullint_val;                              \
        break;                                                      \
    case Type::IntegerTypeID::MAX_INT_ID:                           \
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported int type in AtomicType::ScalarTypedVal::cast_type" << std::endl;\
        exit(-1);                                                   \
} 

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::cast_type (Type::IntegerTypeID to_type_id) {
    ScalarTypedVal new_val = ScalarTypedVal (to_type_id);
    switch (to_type_id) {
        case Type::IntegerTypeID::BOOL:
            CAST_CASE(new_val.val.bool_val)
            break;
        case Type::IntegerTypeID::CHAR:
            CAST_CASE(new_val.val.char_val)
            break;
        case Type::IntegerTypeID::UCHAR:
            CAST_CASE(new_val.val.uchar_val)
            break;
        case Type::IntegerTypeID::SHRT:
            CAST_CASE(new_val.val.shrt_val)
            break;
        case Type::IntegerTypeID::USHRT:
            CAST_CASE(new_val.val.ushrt_val)
            break;
        case Type::IntegerTypeID::INT:
            CAST_CASE(new_val.val.int_val)
            break;
        case Type::IntegerTypeID::UINT:
            CAST_CASE(new_val.val.uint_val)
            break;
        case Type::IntegerTypeID::LINT:
            CAST_CASE(new_val.val.lint_val)
            break;
        case Type::IntegerTypeID::ULINT:
            CAST_CASE(new_val.val.ulint_val)
            break;
        case Type::IntegerTypeID::LLINT:
            CAST_CASE(new_val.val.llint_val)
            break;
        case Type::IntegerTypeID::ULLINT:
            CAST_CASE(new_val.val.ullint_val)
            break;
        case Type::IntegerTypeID::MAX_INT_ID:                           \
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported int type in AtomicType::ScalarTypedVal::cast_type" << std::endl;
            exit(-1);
    }
    return new_val;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::pre_op (bool inc) { // Prefix
    AtomicType::ScalarTypedVal ret = *this;
    int add = inc ? 1 : -1;
    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator+" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            if (val.int_val == INT_MAX)
                ret.set_ub(UB::SignOvf);
            else
                ret.val.int_val = val.int_val + add;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = val.uint_val + add;
            break;
        case IntegerType::IntegerTypeID::LINT:
            if (val.lint_val == LONG_MAX)
                ret.set_ub(UB::SignOvf);
            else
                ret.val.lint_val = val.lint_val + add;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = val.ulint_val + add;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            if (val.llint_val == LLONG_MAX)
                ret.set_ub(UB::SignOvf);
            else
                ret.val.llint_val = val.llint_val + add;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = val.ullint_val + add;
            break;
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator- () {
    AtomicType::ScalarTypedVal ret = *this;
    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator-" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            if (val.int_val == INT_MIN)
                ret.set_ub(UB::SignOvf);
            else
                ret.val.int_val = -val.int_val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = -val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            if (val.lint_val == LONG_MIN)
                ret.set_ub(UB::SignOvf);
            else
                ret.val.lint_val = -val.lint_val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = -val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            if (val.llint_val == LLONG_MIN)
                ret.set_ub(UB::SignOvf);
            else
                ret.val.llint_val = -val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = -val.ullint_val;
            break;
    }
    return ret;

}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator~ () {
    AtomicType::ScalarTypedVal ret = *this;
    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator-" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            ret.val.int_val = ~val.int_val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = ~val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            ret.val.lint_val = ~val.lint_val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = ~val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            ret.val.llint_val = ~val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = ~val.ullint_val;
            break;
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator! () {
    AtomicType::ScalarTypedVal ret = AtomicType::ScalarTypedVal(Type::IntegerTypeID::BOOL);
    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
            ret.val.bool_val = !val.bool_val;
            break;
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::INT:
        case IntegerType::IntegerTypeID::UINT:
        case IntegerType::IntegerTypeID::LINT:
        case IntegerType::IntegerTypeID::ULINT:
        case IntegerType::IntegerTypeID::LLINT:
        case IntegerType::IntegerTypeID::ULLINT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator-" << std::endl;
            exit(-1);
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator+ (ScalarTypedVal rhs) {
    AtomicType::ScalarTypedVal ret = *this;

    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    int64_t s_tmp = 0;
    uint64_t u_tmp = 0;

    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator+" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            s_tmp = (long long int) val.int_val + (long long int) rhs.val.int_val;
            if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                ret.set_ub(SignOvf);
            else
                ret.val.int_val = (int) s_tmp;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = val.uint_val + rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            if (!long_eq_long_long) {
                s_tmp = (long long int) val.lint_val + (long long int) rhs.val.lint_val;
                if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) s_tmp;
            }
            else {
                uint64_t ua = val.lint_val;
                uint64_t ub = rhs.val.lint_val;
                u_tmp = ua + ub;
                ua = (ua >> 63) + LONG_MAX;
                if ((int64_t) ((ua ^ ub) | ~(ub ^ u_tmp)) >= 0)
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) u_tmp;
            }
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = val.ulint_val + rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
        {
            uint64_t ua = val.llint_val;
            uint64_t ub = rhs.val.llint_val;
            u_tmp = ua + ub;
            ua = (ua >> 63) + LLONG_MAX;
            if ((int64_t) ((ua ^ ub) | ~(ub ^ u_tmp)) >= 0)
                ret.set_ub(SignOvf);
            else
                ret.val.llint_val =  val.llint_val + rhs.val.llint_val;
            break;
        }
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = val.ullint_val + rhs.val.ullint_val;
            break;
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator- (ScalarTypedVal rhs) {
    AtomicType::ScalarTypedVal ret = *this;

    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    int64_t s_tmp = 0;
    uint64_t u_tmp = 0;

    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator-" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            s_tmp = (long long int) val.int_val - (long long int) ret.val.int_val;
            if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                ret.set_ub(SignOvf);
            else
                ret.val.int_val = (int) s_tmp;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = val.uint_val - rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            if (!long_eq_long_long) {
                s_tmp = (long long int) val.lint_val - (long long int) rhs.val.lint_val;
                if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) s_tmp;
            }
            else {
                uint64_t ua = val.lint_val;
                uint64_t ub = rhs.val.lint_val;
                u_tmp = ua - ub;
                ua = (ua >> 63) + LONG_MAX;
                if ((int64_t) ((ua ^ ub) & (ua ^ u_tmp)) < 0)
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) u_tmp;
            }
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = val.ulint_val - rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
        {
            uint64_t ua = val.llint_val;
            uint64_t ub = rhs.val.llint_val;
            u_tmp = ua - ub;
            ua = (ua >> 63) + LLONG_MAX;
            if ((int64_t) ((ua ^ ub) & (ua ^ u_tmp)) < 0)
                ret.set_ub(SignOvf);
            else
                ret.val.llint_val = (long long int) u_tmp;
            break;
        }
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = val.ullint_val - rhs.val.ullint_val;
            break;
    }
    return ret;
}

static bool check_int64_mul (int64_t a, int64_t b, int64_t* res) {
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

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator* (ScalarTypedVal rhs) {
    AtomicType::ScalarTypedVal ret = *this;

    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    int64_t s_tmp = 0;

    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator*" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            s_tmp = (long long int) val.int_val * (long long int) rhs.val.int_val;
            if ((int) val.int_val == INT_MIN && (int) rhs.val.int_val == -1)
                ret.set_ub(SignOvfMin);
            else if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                ret.set_ub(SignOvf);
            else
                ret.val.int_val = (int) s_tmp;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = val.uint_val * rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            if (val.lint_val == LONG_MIN && (long int) rhs.val.lint_val == -1)
                ret.set_ub(SignOvfMin);
            else if (!long_eq_long_long) {
                s_tmp = (long long int) val.lint_val * (long long int) rhs.val.lint_val;
                if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) s_tmp;
            }
            else {
                if (!check_int64_mul(val.lint_val, rhs.val.lint_val, &s_tmp))
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) s_tmp;
            }
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = val.ulint_val * rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            if ((long long int) val.llint_val == LLONG_MIN && (long long int) rhs.val.llint_val == -1)
                ret.set_ub(SignOvfMin);
            else if (!check_int64_mul(val.llint_val, rhs.val.llint_val, &s_tmp))
                ret.set_ub(SignOvfMin);
            else
                ret.val.llint_val = (long long int) s_tmp;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = val.ullint_val * rhs.val.ullint_val;
            break;
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator/ (ScalarTypedVal rhs) {
    AtomicType::ScalarTypedVal ret = *this;

    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    int64_t s_tmp = 0;

    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator/" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            if (rhs.val.int_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            s_tmp = (long long int) val.int_val / (long long int) rhs.val.int_val;
            if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                ret.set_ub(SignOvf);
            else
                ret.val.int_val = (int) s_tmp;
            break;
        case IntegerType::IntegerTypeID::UINT:
            if (rhs.val.uint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            ret.val.uint_val = val.uint_val / rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            if (rhs.val.lint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            if (!long_eq_long_long) {
                s_tmp = (long long int) val.lint_val / (long long int) rhs.val.lint_val;
                if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) s_tmp;
            }
            else {
                if ((val.lint_val == LONG_MIN && rhs.val.lint_val == -1) ||
                    (rhs.val.lint_val == LONG_MIN && val.lint_val == -1))
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = val.lint_val / rhs.val.lint_val;
            }
            break;
        case IntegerType::IntegerTypeID::ULINT:
            if (rhs.val.ulint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            ret.val.ulint_val = val.ulint_val / rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            if (rhs.val.llint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            if ((val.llint_val == LLONG_MIN && rhs.val.llint_val == -1) ||
                (rhs.val.llint_val == LLONG_MIN && val.llint_val == -1))
                ret.set_ub(SignOvf);
            else
                ret.val.llint_val = val.llint_val / rhs.val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            if (rhs.val.ullint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            ret.val.ullint_val = val.ullint_val / rhs.val.ullint_val;
            break;
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator% (ScalarTypedVal rhs) {
    AtomicType::ScalarTypedVal ret = *this;

    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    int64_t s_tmp = 0;

    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator%" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            if (rhs.val.int_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            s_tmp = (long long int) val.int_val % (long long int) rhs.val.int_val;
            if (s_tmp < INT_MIN || s_tmp > INT_MAX)
                ret.set_ub(SignOvf);
            else
                ret.val.int_val = (int) s_tmp;
            break;
        case IntegerType::IntegerTypeID::UINT:
            if (rhs.val.uint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            ret.val.uint_val = val.uint_val % rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            if (rhs.val.lint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            if (!long_eq_long_long) {
                s_tmp = (long long int) val.lint_val % (long long int) rhs.val.lint_val;
                if (s_tmp < LONG_MIN || s_tmp > LONG_MAX)
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = (long int) s_tmp;
            }
            else {
                if ((val.lint_val == LONG_MIN && rhs.val.lint_val == -1) ||
                    (rhs.val.lint_val == LONG_MIN && val.lint_val == -1))
                    ret.set_ub(SignOvf);
                else
                    ret.val.lint_val = val.lint_val % rhs.val.lint_val;
            }
            break;
        case IntegerType::IntegerTypeID::ULINT:
            if (rhs.val.ulint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            ret.val.ulint_val = val.ulint_val % rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            if (rhs.val.llint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            if ((val.llint_val == LLONG_MIN && rhs.val.llint_val == -1) ||
                (rhs.val.llint_val == LLONG_MIN && val.llint_val == -1))
                ret.set_ub(SignOvf);
            else
                ret.val.llint_val = val.llint_val % rhs.val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            if (rhs.val.ullint_val == 0) {
                ret.set_ub(ZeroDiv);
                return ret;
            }
            ret.val.ullint_val = val.ullint_val % rhs.val.ullint_val;
            break;
    }
    return ret;
}

#define ScalarTypedValLogOp(__op__)                                                                 \
AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator __op__ (ScalarTypedVal rhs) {       \
    AtomicType::ScalarTypedVal ret = AtomicType::ScalarTypedVal(Type::IntegerTypeID::BOOL);         \
                                                                                                    \
    switch (int_type_id) {                                                                          \
        case IntegerType::IntegerTypeID::BOOL:                                                      \
        case IntegerType::IntegerTypeID::CHAR:                                                      \
        case IntegerType::IntegerTypeID::UCHAR:                                                     \
        case IntegerType::IntegerTypeID::SHRT:                                                      \
        case IntegerType::IntegerTypeID::USHRT:                                                     \
        case IntegerType::IntegerTypeID::MAX_INT_ID:                                                \
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator"#__op__ << std::endl;\
            exit(-1);                                                                               \
        case IntegerType::IntegerTypeID::INT:                                                       \
            ret.val.bool_val = val.int_val __op__ rhs.val.int_val;                                  \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::UINT:                                                      \
            ret.val.bool_val = val.uint_val __op__ rhs.val.uint_val;                                \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::LINT:                                                      \
            ret.val.bool_val = val.lint_val __op__ rhs.val.lint_val;                                \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::ULINT:                                                     \
            ret.val.bool_val = val.ulint_val __op__ rhs.val.ulint_val;                              \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::LLINT:                                                     \
            ret.val.bool_val = val.llint_val __op__ rhs.val.llint_val;                              \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::ULLINT:                                                    \
            ret.val.bool_val = val.ullint_val __op__ rhs.val.ullint_val;                            \
            break;                                                                                  \
    }                                                                                               \
    return ret;                                                                                     \
}

ScalarTypedValLogOp(<)
ScalarTypedValLogOp(>)
ScalarTypedValLogOp(<=)
ScalarTypedValLogOp(>=)
ScalarTypedValLogOp(==)
ScalarTypedValLogOp(!=)
ScalarTypedValLogOp(&&)
ScalarTypedValLogOp(||)

#define ScalarTypedValBitOp(__op__)                                                                 \
AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator __op__ (ScalarTypedVal rhs) {       \
    AtomicType::ScalarTypedVal ret = *this;                                                         \
                                                                                                    \
    switch (int_type_id) {                                                                          \
        case IntegerType::IntegerTypeID::BOOL:                                                      \
        case IntegerType::IntegerTypeID::CHAR:                                                      \
        case IntegerType::IntegerTypeID::UCHAR:                                                     \
        case IntegerType::IntegerTypeID::SHRT:                                                      \
        case IntegerType::IntegerTypeID::USHRT:                                                     \
        case IntegerType::IntegerTypeID::MAX_INT_ID:                                                \
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator"#__op__ << std::endl;\
            exit(-1);                                                                               \
        case IntegerType::IntegerTypeID::INT:                                                       \
            ret.val.int_val = val.int_val __op__ rhs.val.int_val;                                   \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::UINT:                                                      \
            ret.val.uint_val = val.uint_val __op__ rhs.val.uint_val;                                \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::LINT:                                                      \
            ret.val.lint_val = val.lint_val __op__ rhs.val.lint_val;                                \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::ULINT:                                                     \
            ret.val.ulint_val = val.ulint_val __op__ rhs.val.ulint_val;                             \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::LLINT:                                                     \
            ret.val.llint_val = val.llint_val __op__ rhs.val.llint_val;                             \
            break;                                                                                  \
        case IntegerType::IntegerTypeID::ULLINT:                                                    \
            ret.val.ullint_val = val.ullint_val __op__ rhs.val.ullint_val;                          \
            break;                                                                                  \
    }                                                                                               \
    return ret;                                                                                     \
}

ScalarTypedValBitOp(&)
ScalarTypedValBitOp(|)
ScalarTypedValBitOp(^)

static uint64_t msb(uint64_t x) {
    uint64_t ret = 0;
    while (x != 0) {
        ret++;
        x = x >> 1;
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator<< (ScalarTypedVal rhs) {
    AtomicType::ScalarTypedVal ret = *this;

    int64_t s_lhs = 0;
    int64_t u_lhs = 0;
    int64_t s_rhs = 0;
    int64_t u_rhs = 0;
    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator<<" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            s_lhs = val.int_val;
            s_rhs = rhs.val.int_val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            u_lhs = val.uint_val;
            u_rhs = rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            s_lhs = val.lint_val;
            s_rhs = rhs.val.lint_val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            u_lhs = val.ulint_val;
            u_rhs = rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            s_lhs = val.llint_val;
            s_rhs = rhs.val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            u_lhs = val.ullint_val;
            u_rhs = rhs.val.ullint_val;
            break;
    }

    bool lhs_is_signed = IntegerType::init(int_type_id)->get_is_signed();
    bool rhs_is_signed = IntegerType::init(rhs.get_int_type_id())->get_is_signed();
    if (lhs_is_signed && (s_lhs < 0))
        ret.set_ub(NegShift);
    if (rhs_is_signed && (s_rhs < 0))
        ret.set_ub(ShiftRhsNeg);

    uint64_t lhs_bit_size = IntegerType::init(int_type_id)->get_bit_size();
    if (rhs_is_signed) {
        if (s_rhs >= lhs_bit_size)
            ret.set_ub(ShiftRhsLarge);
    }
    else {
        if(u_rhs >= lhs_bit_size)
            ret.set_ub(ShiftRhsLarge);
    }

    if (lhs_is_signed && 
       (s_rhs >= (lhs_bit_size - msb(s_lhs))))
        ret.set_ub(ShiftRhsLarge);

    if (ret.has_ub())
        return ret;

    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator<<" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            ret.val.int_val = val.int_val << rhs.val.int_val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = val.uint_val << rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            ret.val.lint_val = val.lint_val << rhs.val.lint_val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = val.ulint_val << rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            ret.val.llint_val = val.llint_val << rhs.val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = val.ullint_val << rhs.val.ullint_val;
            break;
    }
    return ret;
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::operator>> (ScalarTypedVal rhs) {
    AtomicType::ScalarTypedVal ret = *this;

    int64_t s_lhs = 0;
    int64_t u_lhs = 0;
    int64_t s_rhs = 0;
    int64_t u_rhs = 0;
    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator>>" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            s_lhs = val.int_val;
            s_rhs = rhs.val.int_val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            u_lhs = val.uint_val;
            u_rhs = rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            s_lhs = val.lint_val;
            s_rhs = rhs.val.lint_val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            u_lhs = val.ulint_val;
            u_rhs = rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            s_lhs = val.llint_val;
            s_rhs = rhs.val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            u_lhs = val.ullint_val;
            u_rhs = rhs.val.ullint_val;
            break;
    }

    bool lhs_is_signed = IntegerType::init(int_type_id)->get_is_signed();
    bool rhs_is_signed = IntegerType::init(rhs.get_int_type_id())->get_is_signed();
    if (lhs_is_signed && (s_lhs < 0))
        ret.set_ub(NegShift);
    if (rhs_is_signed && (s_rhs < 0))
        ret.set_ub(ShiftRhsNeg);

    uint64_t lhs_bit_size = IntegerType::init(int_type_id)->get_bit_size();
    if (rhs_is_signed) {
        if (s_rhs >= lhs_bit_size)
            ret.set_ub(ShiftRhsLarge);
    }
    else {
        if(u_rhs >= lhs_bit_size)
            ret.set_ub(ShiftRhsLarge);
    }

    if (ret.has_ub())
        return ret;

    switch (int_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
        case IntegerType::IntegerTypeID::CHAR:
        case IntegerType::IntegerTypeID::UCHAR:
        case IntegerType::IntegerTypeID::SHRT:
        case IntegerType::IntegerTypeID::USHRT:
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": perform propagate_type in AtomicType::ScalarTypedVal::operator>>" << std::endl;
            exit(-1);
        case IntegerType::IntegerTypeID::INT:
            ret.val.int_val = val.int_val >> rhs.val.int_val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret.val.uint_val = val.uint_val >> rhs.val.uint_val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            ret.val.lint_val = val.lint_val >> rhs.val.lint_val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret.val.ulint_val = val.ulint_val >> rhs.val.ulint_val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            ret.val.llint_val = val.llint_val >> rhs.val.llint_val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            ret.val.ullint_val = val.ullint_val >> rhs.val.ullint_val;
            break;
    }
    return ret;
}

template <typename T>
static void gen_rand_typed_val (T& ret, T& min, T& max) {
    ret = (T) rand_val_gen->get_rand_value<int>(min, max);
}

AtomicType::ScalarTypedVal AtomicType::ScalarTypedVal::generate (Context ctx, AtomicType::IntegerTypeID _int_type_id) {
    AtomicType::ScalarTypedVal ret(_int_type_id);
    std::shared_ptr<IntegerType> tmp_type = IntegerType::init (_int_type_id);
    AtomicType::ScalarTypedVal min = tmp_type->get_min();
    AtomicType::ScalarTypedVal max = tmp_type->get_max();
    switch(_int_type_id) {
        case AtomicType::BOOL:
            gen_rand_typed_val(ret.val.bool_val, min.val.bool_val, max.val.bool_val);
            break;
        case AtomicType::CHAR:
            gen_rand_typed_val(ret.val.char_val, min.val.char_val, max.val.char_val);
            break;
        case AtomicType::UCHAR:
            gen_rand_typed_val(ret.val.uchar_val, min.val.uchar_val, max.val.uchar_val);
            break;
        case AtomicType::SHRT:
            gen_rand_typed_val(ret.val.shrt_val, min.val.shrt_val, max.val.shrt_val);
            break;
        case AtomicType::USHRT:
            gen_rand_typed_val(ret.val.ushrt_val, min.val.ushrt_val, max.val.ushrt_val);
            break;
        case AtomicType::INT:
            gen_rand_typed_val(ret.val.int_val, min.val.int_val, max.val.int_val);
            break;
        case AtomicType::UINT:
            gen_rand_typed_val(ret.val.uint_val, min.val.uint_val, max.val.uint_val);
            break;
        case AtomicType::LINT:
            gen_rand_typed_val(ret.val.lint_val, min.val.lint_val, max.val.lint_val);
            break;
        case AtomicType::ULINT:
            gen_rand_typed_val(ret.val.ulint_val, min.val.ulint_val, max.val.ulint_val);
            break;
        case AtomicType::LLINT:
            gen_rand_typed_val(ret.val.llint_val, min.val.llint_val, max.val.llint_val);
            break;
        case AtomicType::ULLINT:
            gen_rand_typed_val(ret.val.ullint_val, min.val.ullint_val, max.val.ullint_val);
            break;
        case AtomicType::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported type of struct member in AtomicType::ScalarTypedVal::generate" << std::endl;
            exit(-1);
            break;
    }
    return ret;
}

std::ostream& rl::operator<< (std::ostream &out, const AtomicType::ScalarTypedVal &scalar_typed_val) {
    switch(scalar_typed_val.get_int_type_id()) {
        case AtomicType::BOOL:
            out << scalar_typed_val.val.bool_val;
            break;
        case AtomicType::CHAR:
            out << scalar_typed_val.val.char_val;
            break;
        case AtomicType::UCHAR:
            out << scalar_typed_val.val.uchar_val;
            break;
        case AtomicType::SHRT:
            out << scalar_typed_val.val.shrt_val;
            break;
        case AtomicType::USHRT:
            out << scalar_typed_val.val.ushrt_val;
            break;
        case AtomicType::INT:
            out << scalar_typed_val.val.int_val;
            break;
        case AtomicType::UINT:
            out << scalar_typed_val.val.uint_val;
            break;
        case AtomicType::LINT:
            out << scalar_typed_val.val.lint_val;
            break;
        case AtomicType::ULINT:
            out << scalar_typed_val.val.ulint_val;
            break;
        case AtomicType::LLINT:
            out << scalar_typed_val.val.llint_val;
            break;
        case AtomicType::ULLINT:
            out << scalar_typed_val.val.ullint_val;
            break;
        case AtomicType::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported type of struct member in operator<<" << std::endl;
            exit(-1);
            break;
    }
    return out;
}

std::shared_ptr<IntegerType> IntegerType::init (AtomicType::IntegerTypeID _type_id) {
    std::shared_ptr<IntegerType> ret (NULL);
    switch (_type_id) {
        case AtomicType::IntegerTypeID::BOOL:
            ret = std::make_shared<TypeBOOL> (TypeBOOL());
            break;
        case AtomicType::IntegerTypeID::CHAR:
            ret = std::make_shared<TypeCHAR> (TypeCHAR());
            break;
        case AtomicType::IntegerTypeID::UCHAR:
            ret = std::make_shared<TypeUCHAR> (TypeUCHAR());
            break;
        case AtomicType::IntegerTypeID::SHRT:
            ret = std::make_shared<TypeSHRT> (TypeSHRT());
            break;
        case AtomicType::IntegerTypeID::USHRT:
            ret = std::make_shared<TypeUSHRT> (TypeUSHRT());
            break;
        case AtomicType::IntegerTypeID::INT:
            ret = std::make_shared<TypeINT> (TypeINT());
            break;
        case AtomicType::IntegerTypeID::UINT:
            ret = std::make_shared<TypeUINT> (TypeUINT());
            break;
        case AtomicType::IntegerTypeID::LINT:
            ret = std::make_shared<TypeLINT> (TypeLINT());
            break;
        case AtomicType::IntegerTypeID::ULINT:
            ret = std::make_shared<TypeULINT> (TypeULINT());
            break;
         case AtomicType::IntegerTypeID::LLINT:
            ret = std::make_shared<TypeLLINT> (TypeLLINT());
            break;
         case AtomicType::IntegerTypeID::ULLINT:
            ret = std::make_shared<TypeULLINT> (TypeULLINT());
            break;
        case MAX_INT_ID:
            break;
    }
    return ret;
}

std::shared_ptr<IntegerType> IntegerType::init (AtomicType::IntegerTypeID _type_id, Type::Mod _modifier, bool _is_static, uint64_t _align) {
    std::shared_ptr<IntegerType> ret = IntegerType::init (_type_id);
    ret->set_modifier(_modifier);
    ret->set_is_static(_is_static);
    ret->set_align(_align);
    return ret;
}

std::shared_ptr<IntegerType> IntegerType::generate (Context ctx) {
    Type::Mod modifier = ctx.get_gen_policy()->get_allowed_modifiers().at(rand_val_gen->get_rand_value<int>(0, ctx.get_gen_policy()->get_allowed_modifiers().size() - 1));

    bool specifier = false;
    if (ctx.get_gen_policy()->get_allow_static_var())
        specifier = rand_val_gen->get_rand_value<int>(0, 1);
    else
        specifier = false;
    //TODO: what about align?

    IntegerType::IntegerTypeID int_type_id = (IntegerType::IntegerTypeID) rand_val_gen->get_rand_id(ctx.get_gen_policy()->get_allowed_int_types());
    return IntegerType::init(int_type_id, modifier, specifier, 0);
}


bool IntegerType::can_repr_value (AtomicType::IntegerTypeID a, AtomicType::IntegerTypeID b) {
    // This function is used for different conversion rules, so it can be called only after integral promotion
    std::shared_ptr<IntegerType> B = std::static_pointer_cast<IntegerType>(init(b));
    bool int_eq_long = sizeof(int) == sizeof(long int);
    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    switch (a) {
        case INT:
            return B->get_is_signed();
        case UINT:
            if (B->get_int_type_id() == INT)
                return false;
            if (B->get_int_type_id() == LINT)
                return !int_eq_long;
            return true;
        case LINT:
            if (!B->get_is_signed())
                return false;
            if (B->get_int_type_id() == INT)
                return int_eq_long;
            return true;
        case ULINT:
            switch (B->get_int_type_id()) {
                case INT:
                    return false;
                case UINT:
                    return int_eq_long;
                case LINT:
                    return false;
                case ULINT:
                    return true;
                case LLINT:
                    return !long_eq_long_long;
                case ULLINT:
                    return true;
                default:
                    std::cerr << "ERROR: Type::can_repr_value in case ULINT" << std::endl;
            }
        case LLINT:
            switch (B->get_int_type_id()) {
                case INT:
                case UINT:
                   return false;
                case LINT:
                    return long_eq_long_long;
                case ULINT:
                   return false;
                case LLINT:
                    return true;
                case ULLINT:
                   return false;
                default:
                    std::cerr << "ERROR: Type::can_repr_value in case ULINT" << std::endl;
            }
        case ULLINT:
            switch (B->get_int_type_id()) {
                case INT:
                case UINT:
                case LINT:
                   return false;
                case ULINT:
                   return long_eq_long_long;
                case LLINT:
                   return false;
                case ULLINT:
                    return true;
                default:
                    std::cerr << "ERROR: Type::can_repr_value in case ULINT" << std::endl;
            }
        default:
            std::cerr << "ERROR: Type::can_repr_value" << std::endl;
            return false;
    }
}

AtomicType::IntegerTypeID IntegerType::get_corr_unsig (AtomicType::IntegerTypeID _type_id) {
    // This function is used for different conversion rules, so it can be called only after integral promotion
    switch (_type_id) {
        case INT:
        case UINT:
            return UINT;
        case LINT:
        case ULINT:
            return ULINT;
        case LLINT:
        case ULLINT:
            return ULLINT;
        default:
            std::cerr << "ERROR: Type::get_corr_unsig" << std::endl;
            return MAX_INT_ID;
    }
}

template <class T>
static std::string dbg_dump_helper (std::string name, int id, T min, T max, uint32_t bit_size, bool is_signed) {
    std::string ret = "";
    ret += "name: " + name + "\n";
    ret += "int_type_id: " + std::to_string(id) + "\n";
    ret += "min: " + std::to_string((T)min) + "\n";
    ret += "max: " + std::to_string((T)max) + "\n";
    ret += "bit_size: " + std::to_string(bit_size) + "\n";
    ret += "is_signed: " + std::to_string(is_signed) + "\n";
    return ret;
}

void TypeBOOL::dbg_dump () {
    std::cout << dbg_dump_helper<bool>(get_name(), get_int_type_id(), min.val.bool_val, max.val.bool_val, bit_size, is_signed) << std::endl;
}

void TypeCHAR::dbg_dump () {
    std::cout << dbg_dump_helper<char>(get_name(), get_int_type_id(), min.val.char_val, max.val.char_val, bit_size, is_signed) << std::endl;
}

void TypeUCHAR::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned char>(get_name(), get_int_type_id(), min.val.uchar_val, max.val.uchar_val, bit_size, is_signed) << std::endl;
}

void TypeSHRT::dbg_dump () {
    std::cout << dbg_dump_helper<short>(get_name(), get_int_type_id(), min.val.shrt_val, max.val.shrt_val, bit_size, is_signed) << std::endl;
}

void TypeUSHRT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned short>(get_name(), get_int_type_id(), min.val.ushrt_val, max.val.ushrt_val, bit_size, is_signed) << std::endl;
}

void TypeINT::dbg_dump () {
    std::cout << dbg_dump_helper<int>(get_name(), get_int_type_id(), min.val.int_val, max.val.int_val, bit_size, is_signed) << std::endl;
}

void TypeUINT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned int>(get_name(), get_int_type_id(), min.val.uint_val, max.val.uint_val, bit_size, is_signed) << std::endl;
}

void TypeLINT::dbg_dump () {
    std::cout << dbg_dump_helper<long int>(get_name(), get_int_type_id(), min.val.lint_val, max.val.lint_val, bit_size, is_signed) << std::endl;
}

void TypeULINT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned long int>(get_name(), get_int_type_id(), min.val.ulint_val, max.val.ulint_val, bit_size, is_signed) << std::endl;
}

void TypeLLINT::dbg_dump () {
    std::cout << dbg_dump_helper<long long int>(get_name(), get_int_type_id(), min.val.llint_val, max.val.llint_val, bit_size, is_signed) << std::endl;
}

void TypeULLINT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned long long int>(get_name(), get_int_type_id(), min.val.ullint_val, max.val.ullint_val, bit_size, is_signed) << std::endl;
}
