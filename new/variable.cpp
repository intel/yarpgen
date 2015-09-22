#include "variable.h"

Data::Data (std::string _name, Type::TypeID _type_id, Mod _modifier, bool _is_static) {
    type = Type::init (_type_id);
    name = _name;
    modifier = _modifier;
    is_static = _is_static;
    class_id = MAX_CLASS_ID;
    align = 0;
}

Variable::Variable (std::string _name, Type::TypeID _type_id, Mod _modifier, bool _is_static) : Data (_name, _type_id, _modifier, _is_static) {
    class_id = Data::VarClassID::VAR;
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
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE" << std::endl;
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
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
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
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
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
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

Array::Array (std::string _name, Type::TypeID _base_type_id, Mod _modifier, bool _is_static,
              unsigned int _size, Ess _essence) : Data (_name, Type::TypeID::PTR, _modifier, _is_static) {
    class_id = Data::VarClassID::ARR;
    base_type = Type::init (_base_type_id);
    size = _size;
    essence = _essence;
    switch (base_type->get_id ()) {
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
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE" << std::endl;
            break;
    }
}

void Array::set_value (uint64_t _val) {
    switch (base_type->get_id ()) {
        case Type::TypeID::UINT:
            value.uint_val = (unsigned int) _val;
            break;
        case Type::TypeID::ULINT:
            value.ulint_val = (unsigned long int) _val;
            break;
        case Type::TypeID::ULLINT:
            value.ullint_val = (unsigned long long int) _val;
            break;
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

void Array::set_max (uint64_t _max) {
    switch (base_type->get_id ()) {
        case Type::TypeID::UINT:
            max.uint_val = (unsigned int) _max;
            break;
        case Type::TypeID::ULINT:
            max.ulint_val = (unsigned long int) _max;
            break;
        case Type::TypeID::ULLINT:
            max.ullint_val = (unsigned long long int) _max;
            break;
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

void Array::set_min (uint64_t _min) {
    switch (base_type->get_id ()) {
        case Type::TypeID::UINT:
            min.uint_val = (unsigned int) _min;
            break;
        case Type::TypeID::ULINT:
            min.ulint_val = (unsigned long int) _min;
            break;
        case Type::TypeID::ULLINT:
            min.ullint_val = (unsigned long long int) _min;
            break;
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE IN VARIABLE: set_value" << std::endl;
            break;
    }
}

void Array::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump ();
    std::cout << "base_type ";
    base_type->dbg_dump ();
    std::cout << "name: " << name << std::endl;
    std::cout << "size: " << size << std::endl;
    std::cout << "essence: " << essence << std::endl;
    std::cout << "modifier: " << modifier << std::endl;
    std::cout << "value: " << value.uint_val << std::endl;
    std::cout << "min: " << min.uint_val << std::endl;
    std::cout << "max: " << max.uint_val << std::endl;
}
