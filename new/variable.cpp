#include "variable.h"

Variable::Variable (std::string _name, Type::TypeID _type_id, Mod _modifier) {
    type = Type::init (_type_id);
    name = _name;
    modifier = _modifier;
    switch (type->get_id ()) {
        case Type::TypeID::UINT:
            value.uint_val = type->get_min ();
            min.uint_val = type->get_min ();
            max.uint_val = type->get_max ();
            break;
        case Type::TypeID::ULINT:
            value.ulint_val = type->get_min ();
            min.ulint_val = type->get_min ();
            max.ulint_val = type->get_max ();
            break;
        case Type::TypeID::ULLINT:
            value.ullint_val = type->get_min ();
            min.ullint_val = type->get_min ();
            max.ullint_val = type->get_max ();
            break;
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

void Variable::dbg_dump () {
    std::cout << "type ";
    std::cout << std::endl;
    std::cout << "name: " << name << std::endl;
    std::cout << "modifier: " << modifier << std::endl;
    std::cout << "value: " << value.uint_val << std::endl;
    std::cout << "min: " << min.uint_val << std::endl;
    std::cout << "max: " << max.uint_val << std::endl;
}

void Variable::set_type (Type::TypeID _type_id) {
    // TODO: add value, max an min convertion
    type = Type::init (_type_id);
}

void Variable::set_value (uint64_t _val) {
        switch (type->get_id ()) {
        case Type::TypeID::UINT:
            value.uint_val = (unsigned int) _val;
            break;
        case Type::TypeID::ULINT:
            value.ulint_val = (unsigned long int) _val;
            break;
        case Type::TypeID::ULLINT:
            value.ullint_val = (unsigned long long int) _val;
            break;
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

void Variable::set_max (uint64_t _max) {
    switch (type->get_id ()) {
        case Type::TypeID::UINT:
            max.uint_val = (unsigned int) _max;
            break;
        case Type::TypeID::ULINT:
            max.ulint_val = (unsigned long int) _max;
            break;
        case Type::TypeID::ULLINT:
            max.ullint_val = (unsigned long long int) _max;
            break;
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

void Variable::set_min (uint64_t _min) {
    switch (type->get_id ()) {
        case Type::TypeID::UINT:
            min.uint_val = (unsigned int) _min;
            break;
        case Type::TypeID::ULINT:
            min.ulint_val = (unsigned long int) _min;
            break;
        case Type::TypeID::ULLINT:
            min.ullint_val = (unsigned long long int) _min;
            break;
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

void Array::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump ();
    std::cout << "name: " << name << std::endl;
    std::cout << "size: " << size << std::endl;
    std::cout << "essence: " << essence << std::endl;
    std::cout << "modifier: " << modifier << std::endl;
    std::cout << "value: " << value.uint_val << std::endl;
    std::cout << "min: " << min.uint_val << std::endl;
    std::cout << "max: " << max.uint_val << std::endl;
}
