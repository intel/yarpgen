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

std::shared_ptr<Type> PtrType::init (std::shared_ptr<Type> _pointee_t) {
    PtrType ptr_type;
    ptr_type.pointee_t = _pointee_t;
    return std::make_shared<PtrType>(ptr_type);
}

std::shared_ptr<Type> StructType::init (std::string _name) {
    StructType struct_type;
    struct_type.name = "struct " + _name;
    return std::make_shared<StructType>(struct_type);
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

std::shared_ptr<Type> IntegerType::init (IntegerType::IntegerTypeID _type_id) {
    std::shared_ptr<Type> ret (NULL);
    switch (_type_id) {
        case IntegerType::IntegerTypeID::BOOL:
            ret = std::make_shared<TypeBOOL> (TypeBOOL());
            break;
        case IntegerType::IntegerTypeID::CHAR:
            ret = std::make_shared<TypeCHAR> (TypeCHAR());
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            ret = std::make_shared<TypeUCHAR> (TypeUCHAR());
            break;
        case IntegerType::IntegerTypeID::SHRT:
            ret = std::make_shared<TypeSHRT> (TypeSHRT());
            break;
        case IntegerType::IntegerTypeID::USHRT:
            ret = std::make_shared<TypeUSHRT> (TypeUSHRT());
            break;
        case IntegerType::IntegerTypeID::INT:
            ret = std::make_shared<TypeINT> (TypeINT());
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret = std::make_shared<TypeUINT> (TypeUINT());
            break;
        case IntegerType::IntegerTypeID::LINT:
            ret = std::make_shared<TypeLINT> (TypeLINT());
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret = std::make_shared<TypeULINT> (TypeULINT());
            break;
         case IntegerType::IntegerTypeID::LLINT:
            ret = std::make_shared<TypeLLINT> (TypeLLINT());
            break;
         case IntegerType::IntegerTypeID::ULLINT:
            ret = std::make_shared<TypeULLINT> (TypeULLINT());
            break;
        case MAX_INT_ID:
            break;
    }
    return ret;
}

bool IntegerType::can_repr_value (IntegerType::IntegerTypeID a, IntegerType::IntegerTypeID b) {
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

IntegerType::IntegerTypeID IntegerType::get_corr_unsig (IntegerType::IntegerTypeID _type_id) {
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
static std::string dbg_dump_helper (std::string name, int id, uint64_t min, uint64_t max, uint32_t bit_size, bool is_signed) {
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
    std::cout << dbg_dump_helper<bool>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeCHAR::dbg_dump () {
    std::cout << dbg_dump_helper<char>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeUCHAR::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned char>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeSHRT::dbg_dump () {
    std::cout << dbg_dump_helper<short>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeUSHRT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned short>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeINT::dbg_dump () {
    std::cout << dbg_dump_helper<int>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeUINT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned int>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeLINT::dbg_dump () {
    std::cout << dbg_dump_helper<long int>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeULINT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned long int>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeLLINT::dbg_dump () {
    std::cout << dbg_dump_helper<long long int>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}

void TypeULLINT::dbg_dump () {
    std::cout << dbg_dump_helper<unsigned long long int>(get_name(), get_int_type_id(), min, max, bit_size, is_signed) << std::endl;
}
