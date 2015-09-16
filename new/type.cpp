#include "type.h"

std::shared_ptr<Type> Type::init (Type::TypeID _type_id) {
    std::shared_ptr<Type> ret (NULL);
    switch (_type_id) {
        case Type::TypeID::UINT:
            ret = std::make_shared<TypeUINT> (TypeUINT());
            break;
        case Type::TypeID::ULINT:
            ret = std::make_shared<TypeULINT> (TypeULINT());
            break;
         case Type::TypeID::ULLINT:
            ret = std::make_shared<TypeULLINT> (TypeULLINT());
            break;
        case Type::TypeID::MAX_TYPE_ID:
            break;
    }
    return ret;
}

void Type::dbg_dump () {
    std::cout << "name: " << name << std::endl;
    std::cout << "id: " << id << std::endl;
    std::cout << "min: " << min <<  suffix << std::endl;
    std::cout << "max: " << max <<  suffix << std::endl;
    std::cout << "bit_size: " << bit_size << std::endl;
    std::cout << "is_fp: " << is_fp << std::endl;
    std::cout << "is_signe: " << is_signed << std::endl;
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
