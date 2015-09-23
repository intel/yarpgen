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
        case Type::TypeID::BOOL:
            value.bool_val = type->get_min ();
            min.bool_val = type->get_min ();
            max.bool_val = type->get_max ();
            break;
        case Type::TypeID::CHAR:
            value.char_val = type->get_min ();
            min.char_val = type->get_min ();
            max.char_val = type->get_max ();
            break;
        case Type::TypeID::UCHAR:
            value.uchar_val = type->get_min ();
            min.uchar_val = type->get_min ();
            max.uchar_val = type->get_max ();
            break;
        case Type::TypeID::SHRT:
            value.shrt_val = type->get_min ();
            min.shrt_val = type->get_min ();
            max.shrt_val = type->get_max ();
            break;
        case Type::TypeID::USHRT:
            value.ushrt_val = type->get_min ();
            min.ushrt_val = type->get_min ();
            max.ushrt_val = type->get_max ();
            break;
        case Type::TypeID::INT:
            value.int_val = type->get_min ();
            min.int_val = type->get_min ();
            max.int_val = type->get_max ();
            break;
        case Type::TypeID::UINT:
            value.uint_val = type->get_min ();
            min.uint_val = type->get_min ();
            max.uint_val = type->get_max ();
            break;
        case Type::TypeID::LINT:
            value.lint_val = type->get_min ();
            min.lint_val = type->get_min ();
            max.lint_val = type->get_max ();
            break;
        case Type::TypeID::ULINT:
            value.ulint_val = type->get_min ();
            min.ulint_val = type->get_min ();
            max.ulint_val = type->get_max ();
            break;
        case Type::TypeID::LLINT:
            value.llint_val = type->get_min ();
            min.llint_val = type->get_min ();
            max.llint_val = type->get_max ();
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
        case Type::TypeID::BOOL:
            value.bool_val = (bool) _val;
            break;
        case Type::TypeID::CHAR:
            value.char_val = (signed char) _val;
            break;
        case Type::TypeID::UCHAR:
            value.uchar_val = (unsigned char) _val;
            break;
        case Type::TypeID::SHRT:
            value.shrt_val = (short) _val;
            break;
        case Type::TypeID::USHRT:
            value.ushrt_val = (unsigned short) _val;
            break;
        case Type::TypeID::INT:
            value.int_val = (int) _val;
            break;
        case Type::TypeID::UINT:
            value.uint_val = (unsigned int) _val;
            break;
        case Type::TypeID::LINT:
            value.lint_val = (long int) _val;
            break;
        case Type::TypeID::ULINT:
            value.ulint_val = (unsigned long int) _val;
            break;
        case Type::TypeID::LLINT:
            value.llint_val = (long long int) _val;
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
        case Type::TypeID::BOOL:
            max.bool_val = (bool) _max;
            break;
        case Type::TypeID::CHAR:
            max.char_val = (signed char) _max;
            break;
        case Type::TypeID::UCHAR:
            max.uchar_val = (unsigned char) _max;
            break;
        case Type::TypeID::SHRT:
            max.shrt_val = (short) _max;
            break;
        case Type::TypeID::USHRT:
            max.ushrt_val = (unsigned short) _max;
            break;
        case Type::TypeID::INT:
            max.int_val = (int) _max;
            break;
        case Type::TypeID::UINT:
            max.uint_val = (unsigned int) _max;
            break;
        case Type::TypeID::LINT:
            max.lint_val = (long int) _max;
            break;
        case Type::TypeID::ULINT:
            max.ulint_val = (unsigned long int) _max;
            break;
        case Type::TypeID::LLINT:
            max.llint_val = (long long int) _max;
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
        case Type::TypeID::BOOL:
            min.bool_val = (bool) _min;
            break;
        case Type::TypeID::CHAR:
            min.char_val = (signed char) _min;
            break;
        case Type::TypeID::UCHAR:
            min.uchar_val = (unsigned char) _min;
            break;
        case Type::TypeID::SHRT:
            min.shrt_val = (short) _min;
            break;
        case Type::TypeID::USHRT:
            min.ushrt_val = (unsigned short) _min;
            break;
        case Type::TypeID::INT:
            min.int_val = (int) _min;
            break;
        case Type::TypeID::UINT:
            min.uint_val = (unsigned int) _min;
            break;
        case Type::TypeID::LINT:
            min.lint_val = (long int) _min;
            break;
        case Type::TypeID::ULINT:
            min.ulint_val = (unsigned long int) _min;
            break;
        case Type::TypeID::LLINT:
            min.llint_val = (long long int) _min;
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
        case Type::TypeID::BOOL:
            value.bool_val = type->get_min ();
            min.bool_val = type->get_min ();
            max.bool_val = type->get_max ();
            break;
        case Type::TypeID::CHAR:
            value.char_val = type->get_min ();
            min.char_val = type->get_min ();
            max.char_val = type->get_max ();
            break;
        case Type::TypeID::UCHAR:
            value.uchar_val = type->get_min ();
            min.uchar_val = type->get_min ();
            max.uchar_val = type->get_max ();
            break;
        case Type::TypeID::SHRT:
            value.shrt_val = type->get_min ();
            min.shrt_val = type->get_min ();
            max.shrt_val = type->get_max ();
            break;
        case Type::TypeID::USHRT:
            value.ushrt_val = type->get_min ();
            min.ushrt_val = type->get_min ();
            max.ushrt_val = type->get_max ();
            break;
        case Type::TypeID::INT:
            value.int_val = type->get_min ();
            min.int_val = type->get_min ();
            max.int_val = type->get_max ();
            break;
        case Type::TypeID::UINT:
            value.uint_val = type->get_min ();
            min.uint_val = type->get_min ();
            max.uint_val = type->get_max ();
            break;
        case Type::TypeID::LINT:
            value.lint_val = type->get_min ();
            min.lint_val = type->get_min ();
            max.lint_val = type->get_max ();
            break;
        case Type::TypeID::ULINT:
            value.ulint_val = type->get_min ();
            min.ulint_val = type->get_min ();
            max.ulint_val = type->get_max ();
            break;
        case Type::TypeID::LLINT:
            value.llint_val = type->get_min ();
            min.llint_val = type->get_min ();
            max.llint_val = type->get_max ();
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
        case Type::TypeID::BOOL:
            value.bool_val = (bool) _val;
            break;
        case Type::TypeID::CHAR:
            value.char_val = (signed char) _val;
            break;
        case Type::TypeID::UCHAR:
            value.uchar_val = (unsigned char) _val;
            break;
        case Type::TypeID::SHRT:
            value.shrt_val = (short) _val;
            break;
        case Type::TypeID::USHRT:
            value.ushrt_val = (unsigned short) _val;
            break;
        case Type::TypeID::INT:
            value.int_val = (int) _val;
            break;
        case Type::TypeID::UINT:
            value.uint_val = (unsigned int) _val;
            break;
        case Type::TypeID::LINT:
            value.lint_val = (long int) _val;
            break;
        case Type::TypeID::ULINT:
            value.ulint_val = (unsigned long int) _val;
            break;
        case Type::TypeID::LLINT:
            value.llint_val = (long long int) _val;
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
        case Type::TypeID::BOOL:
            max.bool_val = (bool) _max;
            break;
        case Type::TypeID::CHAR:
            max.char_val = (signed char) _max;
            break;
        case Type::TypeID::UCHAR:
            max.uchar_val = (unsigned char) _max;
            break;
        case Type::TypeID::SHRT:
            max.shrt_val = (short) _max;
            break;
        case Type::TypeID::USHRT:
            max.ushrt_val = (unsigned short) _max;
            break;
        case Type::TypeID::INT:
            max.int_val = (int) _max;
            break;
        case Type::TypeID::UINT:
            max.uint_val = (unsigned int) _max;
            break;
        case Type::TypeID::LINT:
            max.lint_val = (long int) _max;
            break;
        case Type::TypeID::ULINT:
            max.ulint_val = (unsigned long int) _max;
            break;
        case Type::TypeID::LLINT:
            max.llint_val = (long long int) _max;
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
        case Type::TypeID::BOOL:
            min.bool_val = (bool) _min;
            break;
        case Type::TypeID::CHAR:
            min.char_val = (signed char) _min;
            break;
        case Type::TypeID::UCHAR:
            min.uchar_val = (unsigned char) _min;
            break;
        case Type::TypeID::SHRT:
            min.shrt_val = (short) _min;
            break;
        case Type::TypeID::USHRT:
            min.ushrt_val = (unsigned short) _min;
            break;
        case Type::TypeID::INT:
            min.int_val = (int) _min;
            break;
        case Type::TypeID::UINT:
            min.uint_val = (unsigned int) _min;
            break;
        case Type::TypeID::LINT:
            min.lint_val = (long int) _min;
            break;
        case Type::TypeID::ULINT:
            min.ulint_val = (unsigned long int) _min;
            break;
        case Type::TypeID::LLINT:
            min.llint_val = (long long int) _min;
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
