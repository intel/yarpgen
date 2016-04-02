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
        ret += " __attribute__((aligned(" + std::to_string(align) + ")))";
    return ret;
}

void StructType::add_member (std::shared_ptr<Type> _type, std::string _name) {
    StructType::StructMember new_mem (_type, _name);
    if (_type->is_struct_type()) {
        nest_depth = std::max(std::static_pointer_cast<StructType>(_type)->get_nest_depth(), nest_depth) + 1;
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
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported type of struct member in Struct::allocate_members" << std::endl;
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
