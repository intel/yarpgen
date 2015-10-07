#include "type.h"

std::shared_ptr<Type> Type::init (Type::TypeID _type_id) {
    std::shared_ptr<Type> ret (NULL);
    switch (_type_id) {
        case Type::TypeID::BOOL:
            ret = std::make_shared<TypeBOOL> (TypeBOOL());
            break;
        case Type::TypeID::CHAR:
            ret = std::make_shared<TypeCHAR> (TypeCHAR());
            break;
        case Type::TypeID::UCHAR:
            ret = std::make_shared<TypeUCHAR> (TypeUCHAR());
            break;
        case Type::TypeID::SHRT:
            ret = std::make_shared<TypeSHRT> (TypeSHRT());
            break;
        case Type::TypeID::USHRT:
            ret = std::make_shared<TypeUSHRT> (TypeUSHRT());
            break;
        case Type::TypeID::INT:
            ret = std::make_shared<TypeINT> (TypeINT());
            break;
        case Type::TypeID::UINT:
            ret = std::make_shared<TypeUINT> (TypeUINT());
            break;
        case Type::TypeID::LINT:
            ret = std::make_shared<TypeLINT> (TypeLINT());
            break;
        case Type::TypeID::ULINT:
            ret = std::make_shared<TypeULINT> (TypeULINT());
            break;
         case Type::TypeID::LLINT:
            ret = std::make_shared<TypeLLINT> (TypeLLINT());
            break;
         case Type::TypeID::ULLINT:
            ret = std::make_shared<TypeULLINT> (TypeULLINT());
            break;
        case Type::TypeID::PTR:
            ret = std::make_shared<TypePTR> (TypePTR());
            break;
        case MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            break;
    }
    return ret;
}

bool Type::can_repr_value (Type::TypeID a, Type::TypeID b) {
    // This function is used for different conversion rules, so it can be called only after integral promotion
    std::shared_ptr<Type> B = init(b);
    bool int_eq_long = sizeof(int) == sizeof(long int);
    bool long_eq_long_long =  sizeof(long int) == sizeof(long long int);
    switch (a) {
        case INT:
            return B->get_is_signed();
        case UINT:
            if (B->get_id() == INT)
                return false;
            if (B->get_id() == LINT)
                return !int_eq_long;
            return true;
        case LINT:
            if (!B->get_is_signed())
                return false;
            if (B->get_id() == INT)
                return int_eq_long;
            return true;
        case ULINT:
            switch (B->get_id()) {
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
            }
        case LLINT:
            switch (B->get_id()) {
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
            }
        case ULLINT:
            switch (B->get_id()) {
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
            }
        default:
            std::cerr << "ERROR: Type::can_repr_value" << std::cerr;
            return false;
    }
}

Type::TypeID Type::get_corr_unsig (Type::TypeID _type_id) {
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
            std::cerr << "ERROR: Type::get_corr_unsig" << std::cerr;
            return MAX_INT_ID;
    }
}

void Type::dbg_dump () {
    std::cout << "name: " << name << std::endl;
    std::cout << "id: " << id << std::endl;
    std::cout << "min: " << min <<  suffix << std::endl;
    std::cout << "max: " << max <<  suffix << std::endl;
    std::cout << "bit_size: " << bit_size << std::endl;
    std::cout << "is_fp: " << is_fp << std::endl;
    std::cout << "is_signed: " << is_signed << std::endl;
}

TypeBOOL::TypeBOOL () {
    id = Type::TypeID::BOOL;
    name = "bool";
    suffix = "";
    min = false;
    max = true;
    bit_size = sizeof (bool) * CHAR_BIT;
    is_fp = false;
    is_signed = false;
}

std::string TypeBOOL::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeBOOL::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeCHAR::TypeCHAR () {
    id = Type::TypeID::CHAR;
    name = "signed char";
    suffix = "";
    min = SCHAR_MIN;
    max = SCHAR_MAX;
    bit_size = sizeof (char) * CHAR_BIT;
    is_fp = false;
    is_signed = true;
}

std::string TypeCHAR::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeCHAR::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeUCHAR::TypeUCHAR () {
    id = Type::TypeID::UCHAR;
    name = "unsigned char";
    suffix = "";
    min = 0;
    max = UCHAR_MAX;
    bit_size = sizeof (unsigned char) * CHAR_BIT;
    is_fp = false;
    is_signed = false;
}

std::string TypeUCHAR::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeUCHAR::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeSHRT::TypeSHRT () {
    id = Type::TypeID::SHRT;
    name = "short";
    suffix = "";
    min = SHRT_MIN;
    max = SHRT_MAX;
    bit_size = sizeof (short) * CHAR_BIT;
    is_fp = false;
    is_signed = true;
}

std::string TypeSHRT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeSHRT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeUSHRT::TypeUSHRT () {
    id = Type::TypeID::USHRT;
    name = "unsigned short";
    suffix = "";
    min = 0;
    max = USHRT_MAX;
    bit_size = sizeof (unsigned short) * CHAR_BIT;
    is_fp = false;
    is_signed = false;
}

std::string TypeUSHRT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeUSHRT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeINT::TypeINT () {
    id = Type::TypeID::INT;
    name = "int";
    suffix = "";
    min = INT_MIN;
    max = INT_MAX;
    bit_size = sizeof (int) * CHAR_BIT;
    is_fp = false;
    is_signed = true;
}

std::string TypeINT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeINT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeUINT::TypeUINT () {
    id = Type::TypeID::UINT;
    name = "unsigned int";
    suffix = "U";
    min = 0;
    max = UINT_MAX;
    bit_size = sizeof (unsigned int) * CHAR_BIT;
    is_fp = false;
    is_signed = false;
}

std::string TypeUINT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeUINT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeLINT::TypeLINT () {
    id = Type::TypeID::LINT;
    name = "long int";
    suffix = "L";
    min = LONG_MIN;;
    max = LONG_MAX;
    bit_size = sizeof (long int) * CHAR_BIT;
    is_fp = false;
    is_signed = true;
}

std::string TypeLINT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeLINT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeULINT::TypeULINT () {
    id = Type::TypeID::ULINT;
    name = "unsigned long int";
    suffix = "UL";
    min = 0;
    max = ULONG_MAX;
    bit_size = sizeof (unsigned long int) * CHAR_BIT;
    is_fp = false;
    is_signed = false;
}

std::string TypeULINT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeULINT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeLLINT::TypeLLINT () {
    id = Type::TypeID::LLINT;
    name = "long long int";
    suffix = "LL";
    min = LLONG_MIN;
    max = LLONG_MAX;
    bit_size = sizeof (long long int) * CHAR_BIT;
    is_fp = false;
    is_signed = true;
}

std::string TypeLLINT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeLLINT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypeULLINT::TypeULLINT () {
    id = Type::TypeID::ULLINT;
    name = "unsigned long long int";
    suffix = "ULL";
    min = 0;
    max = ULLONG_MAX;
    bit_size = sizeof (unsigned long long int) * CHAR_BIT;
    is_fp = false;
    is_signed = false;
}

std::string TypeULLINT::get_max_str () {
    return std::to_string (get_max ()) + get_suffix ();
}

std::string TypeULLINT::get_min_str () {
    return std::to_string (get_min ()) + get_suffix ();
}

TypePTR::TypePTR () {
    id = Type::TypeID::PTR;
    name = "*";
    suffix = "";
    min = 0;
    max = 0;
    bit_size = sizeof (unsigned int*) * CHAR_BIT;
    is_fp = false;
    is_signed = false;
}

std::string TypePTR::get_max_str () {
    return "";
}

std::string TypePTR::get_min_str () {
    return "";
}
