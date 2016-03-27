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

#include "variable.h"

Data::Data (std::string _name) {
    type = NULL;
    name = _name;
    class_id = MAX_CLASS_ID;
}

Struct::Struct (std::string _name, std::shared_ptr<StructType> _type) : Data (_name) {
    class_id = STRUCT;
    type = _type;
    value.ullint_val = 0;
    min.ullint_val = 0;
    max.ullint_val = 0;
    allocate_members();
}

void Struct::allocate_members() {
    std::shared_ptr<StructType> struct_type = std::static_pointer_cast<StructType>(type);
    // TODO: struct member can be not only integer
    for (int i = 0; i < struct_type->get_num_of_members(); ++i) {
        std::shared_ptr<IntegerType> int_type_member = std::static_pointer_cast<IntegerType>(struct_type->get_member(i)->get_type());
        Variable new_mem (struct_type->get_member(i)->get_name(), int_type_member->get_int_type_id(), int_type_member->get_modifier(), int_type_member->get_is_static());
        members.push_back(std::make_shared<Variable>(new_mem));
    }
}

void Struct::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump();
    std::cout << "name: " << name << std::endl;
    std::cout << "modifier: " << type->get_modifier() << std::endl;
    std::cout << "value: " << value.ullint_val << std::endl;
    std::cout << "min: " << min.ullint_val << std::endl;
    std::cout << "max: " << max.ullint_val << std::endl;
    std::cout << "members ";
    for (auto i : members) {
        i->dbg_dump();
    }
}

Variable::Variable (std::string _name, IntegerType::IntegerTypeID _type_id, Type::Mod _modifier, bool _is_static) : Data (_name) {
    class_id = Data::VarClassID::VAR;
    type = IntegerType::init (_type_id);
    type->set_modifier(_modifier);
    type->set_is_static(_is_static);
    type->set_align(0);
    std::shared_ptr<IntegerType> int_type = std::static_pointer_cast<IntegerType>(type);
    switch (int_type->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            value.bool_val = int_type->get_min ();
            min.bool_val = int_type->get_min ();
            max.bool_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::CHAR:
            value.char_val = int_type->get_min ();
            min.char_val = int_type->get_min ();
            max.char_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            value.uchar_val = int_type->get_min ();
            min.uchar_val = int_type->get_min ();
            max.uchar_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::SHRT:
            value.shrt_val = int_type->get_min ();
            min.shrt_val = int_type->get_min ();
            max.shrt_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::USHRT:
            value.ushrt_val = int_type->get_min ();
            min.ushrt_val = int_type->get_min ();
            max.ushrt_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::INT:
            value.int_val = int_type->get_min ();
            min.int_val = int_type->get_min ();
            max.int_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::UINT:
            value.uint_val = int_type->get_min ();
            min.uint_val = int_type->get_min ();
            max.uint_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::LINT:
            value.lint_val = int_type->get_min ();
            min.lint_val = int_type->get_min ();
            max.lint_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::ULINT:
            value.ulint_val = int_type->get_min ();
            min.ulint_val = int_type->get_min ();
            max.ulint_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::LLINT:
            value.llint_val = int_type->get_min ();
            min.llint_val = int_type->get_min ();
            max.llint_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            value.ullint_val = int_type->get_min ();
            min.ullint_val = int_type->get_min ();
            max.ullint_val = int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN VARIABLE" << std::endl;
            break;
    }
}

void Variable::dbg_dump () {
    std::cout << "type ";
    type->dbg_dump();
    std::cout << "name: " << name << std::endl;
    std::cout << "modifier: " << type->get_modifier() << std::endl;
    std::cout << "value: " << value.ullint_val << std::endl;
    std::cout << "min: " << min.ullint_val << std::endl;
    std::cout << "max: " << max.ullint_val << std::endl;
}

void Variable::set_value (uint64_t _val) {
    switch (std::static_pointer_cast<IntegerType>(type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            value.bool_val = (bool) _val;
            break;
        case IntegerType::IntegerTypeID::CHAR:
            value.char_val = (signed char) _val;
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            value.uchar_val = (unsigned char) _val;
            break;
        case IntegerType::IntegerTypeID::SHRT:
            value.shrt_val = (short) _val;
            break;
        case IntegerType::IntegerTypeID::USHRT:
            value.ushrt_val = (unsigned short) _val;
            break;
        case IntegerType::IntegerTypeID::INT:
            value.int_val = (int) _val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            value.uint_val = (unsigned int) _val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            value.lint_val = (long int) _val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            value.ulint_val = (unsigned long int) _val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            value.llint_val = (long long int) _val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            value.ullint_val = (unsigned long long int) _val;
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: set_value" << std::endl;
            break;
    }
}

uint64_t Variable::get_value () {
    switch (std::static_pointer_cast<IntegerType>(type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            return (bool) value.bool_val;
        case IntegerType::IntegerTypeID::CHAR:
            return (signed char) value.char_val;
        case IntegerType::IntegerTypeID::UCHAR:
            return (unsigned char) value.uchar_val;
        case IntegerType::IntegerTypeID::SHRT:
            return (short) value.shrt_val;
        case IntegerType::IntegerTypeID::USHRT:
            return (unsigned short) value.ushrt_val;
        case IntegerType::IntegerTypeID::INT:
            return (int) value.int_val;
        case IntegerType::IntegerTypeID::UINT:
            return (unsigned int) value.uint_val;
        case IntegerType::IntegerTypeID::LINT:
            return (long int) value.lint_val;
        case IntegerType::IntegerTypeID::ULINT:
            return (unsigned long int) value.ulint_val;
        case IntegerType::IntegerTypeID::LLINT:
            return (long long int) value.llint_val;
        case IntegerType::IntegerTypeID::ULLINT:
            return (unsigned long long int) value.ullint_val;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: get_value" << std::endl;
            return 0;
    }
}

void Variable::set_max (uint64_t _max) {
    switch (std::static_pointer_cast<IntegerType>(type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            max.bool_val = (bool) _max;
            break;
        case IntegerType::IntegerTypeID::CHAR:
            max.char_val = (signed char) _max;
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            max.uchar_val = (unsigned char) _max;
            break;
        case IntegerType::IntegerTypeID::SHRT:
            max.shrt_val = (short) _max;
            break;
        case IntegerType::IntegerTypeID::USHRT:
            max.ushrt_val = (unsigned short) _max;
            break;
        case IntegerType::IntegerTypeID::INT:
            max.int_val = (int) _max;
            break;
        case IntegerType::IntegerTypeID::UINT:
            max.uint_val = (unsigned int) _max;
            break;
        case IntegerType::IntegerTypeID::LINT:
            max.lint_val = (long int) _max;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            max.ulint_val = (unsigned long int) _max;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            max.llint_val = (long long int) _max;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            max.ullint_val = (unsigned long long int) _max;
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN Data: set_max" << std::endl;
            break;
    }
}

uint64_t Variable::get_max () {
    switch (std::static_pointer_cast<IntegerType>(type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            return (bool) max.bool_val;
        case IntegerType::IntegerTypeID::CHAR:
            return (signed char) max.char_val;
        case IntegerType::IntegerTypeID::UCHAR:
            return (unsigned char) max.uchar_val;
        case IntegerType::IntegerTypeID::SHRT:
            return (short) max.shrt_val;
        case IntegerType::IntegerTypeID::USHRT:
            return (unsigned short) max.ushrt_val;
        case IntegerType::IntegerTypeID::INT:
            return (int) max.int_val;
        case IntegerType::IntegerTypeID::UINT:
            return (unsigned int) max.uint_val;
        case IntegerType::IntegerTypeID::LINT:
            return (long int) max.lint_val;
        case IntegerType::IntegerTypeID::ULINT:
            return (unsigned long int) max.ulint_val;
        case IntegerType::IntegerTypeID::LLINT:
            return (long long int) max.llint_val;
        case IntegerType::IntegerTypeID::ULLINT:
            return (unsigned long long int) max.ullint_val;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: get_max" << std::endl;
            return 0;
    }
}

void Variable::set_min (uint64_t _min) {
    switch (std::static_pointer_cast<IntegerType>(type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            min.bool_val = (bool) _min;
            break;
        case IntegerType::IntegerTypeID::CHAR:
            min.char_val = (signed char) _min;
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            min.uchar_val = (unsigned char) _min;
            break;
        case IntegerType::IntegerTypeID::SHRT:
            min.shrt_val = (short) _min;
            break;
        case IntegerType::IntegerTypeID::USHRT:
            min.ushrt_val = (unsigned short) _min;
            break;
        case IntegerType::IntegerTypeID::INT:
            min.int_val = (int) _min;
            break;
        case IntegerType::IntegerTypeID::UINT:
            min.uint_val = (unsigned int) _min;
            break;
        case IntegerType::IntegerTypeID::LINT:
            min.lint_val = (long int) _min;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            min.ulint_val = (unsigned long int) _min;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            min.llint_val = (long long int) _min;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            min.ullint_val = (unsigned long long int) _min;
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN Data: set_min" << std::endl;
            break;
    }
}

uint64_t Variable::get_min () {
    switch (std::static_pointer_cast<IntegerType>(type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            return (bool) min.bool_val;
        case IntegerType::IntegerTypeID::CHAR:
            return (signed char) min.char_val;
        case IntegerType::IntegerTypeID::UCHAR:
            return (unsigned char) min.uchar_val;
        case IntegerType::IntegerTypeID::SHRT:
            return (short) min.shrt_val;
        case IntegerType::IntegerTypeID::USHRT:
            return (unsigned short) min.ushrt_val;
        case IntegerType::IntegerTypeID::INT:
            return (int) min.int_val;
        case IntegerType::IntegerTypeID::UINT:
            return (unsigned int) min.uint_val;
        case IntegerType::IntegerTypeID::LINT:
            return (long int) min.lint_val;
        case IntegerType::IntegerTypeID::ULINT:
            return (unsigned long int) min.ulint_val;
        case IntegerType::IntegerTypeID::LLINT:
            return (long long int) min.llint_val;
        case IntegerType::IntegerTypeID::ULLINT:
            return (unsigned long long int) min.ullint_val;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: get_min" << std::endl;
            return 0;
    }
}

Array::Array (std::string _name, IntegerType::IntegerTypeID _base_type_id, Type::Mod _modifier, bool _is_static,
              uint64_t _size, Ess _essence) : Data (_name) {
    class_id = Data::VarClassID::ARR;
    base_type = IntegerType::init (_base_type_id);
    type = PtrType::init (base_type);
    type->set_modifier(_modifier);
    type->set_is_static(_is_static);
    type->set_align(0);
    size = _size;
    essence = _essence;
    std::shared_ptr<IntegerType> base_int_type = std::static_pointer_cast<IntegerType> (base_type);
    switch (base_int_type->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            value.bool_val = base_int_type->get_min ();
            min.bool_val = base_int_type->get_min ();
            max.bool_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::CHAR:
            value.char_val = base_int_type->get_min ();
            min.char_val = base_int_type->get_min ();
            max.char_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            value.uchar_val = base_int_type->get_min ();
            min.uchar_val = base_int_type->get_min ();
            max.uchar_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::SHRT:
            value.shrt_val = base_int_type->get_min ();
            min.shrt_val = base_int_type->get_min ();
            max.shrt_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::USHRT:
            value.ushrt_val = base_int_type->get_min ();
            min.ushrt_val = base_int_type->get_min ();
            max.ushrt_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::INT:
            value.int_val = base_int_type->get_min ();
            min.int_val = base_int_type->get_min ();
            max.int_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::UINT:
            value.uint_val = base_int_type->get_min ();
            min.uint_val = base_int_type->get_min ();
            max.uint_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::LINT:
            value.lint_val = base_int_type->get_min ();
            min.lint_val = base_int_type->get_min ();
            max.lint_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::ULINT:
            value.ulint_val = base_int_type->get_min ();
            min.ulint_val = base_int_type->get_min ();
            max.ulint_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::LLINT:
            value.llint_val = base_int_type->get_min ();
            min.llint_val = base_int_type->get_min ();
            max.llint_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            value.ullint_val = base_int_type->get_min ();
            min.ullint_val = base_int_type->get_min ();
            max.ullint_val = base_int_type->get_max ();
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN VARIABLE" << std::endl;
            break;
    }
}

void Array::set_value (uint64_t _val) {
    switch (std::static_pointer_cast<IntegerType>(base_type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            value.bool_val = (bool) _val;
            break;
        case IntegerType::IntegerTypeID::CHAR:
            value.char_val = (signed char) _val;
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            value.uchar_val = (unsigned char) _val;
            break;
        case IntegerType::IntegerTypeID::SHRT:
            value.shrt_val = (short) _val;
            break;
        case IntegerType::IntegerTypeID::USHRT:
            value.ushrt_val = (unsigned short) _val;
            break;
        case IntegerType::IntegerTypeID::INT:
            value.int_val = (int) _val;
            break;
        case IntegerType::IntegerTypeID::UINT:
            value.uint_val = (unsigned int) _val;
            break;
        case IntegerType::IntegerTypeID::LINT:
            value.lint_val = (long int) _val;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            value.ulint_val = (unsigned long int) _val;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            value.llint_val = (long long int) _val;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            value.ullint_val = (unsigned long long int) _val;
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: set_value" << std::endl;
            break;
    }
}

uint64_t Array::get_value () {
    switch (std::static_pointer_cast<IntegerType>(base_type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            return (bool) value.bool_val;
        case IntegerType::IntegerTypeID::CHAR:
            return (signed char) value.char_val;
        case IntegerType::IntegerTypeID::UCHAR:
            return (unsigned char) value.uchar_val;
        case IntegerType::IntegerTypeID::SHRT:
            return (short) value.shrt_val;
        case IntegerType::IntegerTypeID::USHRT:
            return (unsigned short) value.ushrt_val;
        case IntegerType::IntegerTypeID::INT:
            return (int) value.int_val;
        case IntegerType::IntegerTypeID::UINT:
            return (unsigned int) value.uint_val;
        case IntegerType::IntegerTypeID::LINT:
            return (long int) value.lint_val;
        case IntegerType::IntegerTypeID::ULINT:
            return (unsigned long int) value.ulint_val;
        case IntegerType::IntegerTypeID::LLINT:
            return (long long int) value.llint_val;
        case IntegerType::IntegerTypeID::ULLINT:
            return (unsigned long long int) value.ullint_val;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: get_value" << std::endl;
            return 0;
    }
}

void Array::set_max (uint64_t _max) {
    switch (std::static_pointer_cast<IntegerType>(base_type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            max.bool_val = (bool) _max;
            break;
        case IntegerType::IntegerTypeID::CHAR:
            max.char_val = (signed char) _max;
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            max.uchar_val = (unsigned char) _max;
            break;
        case IntegerType::IntegerTypeID::SHRT:
            max.shrt_val = (short) _max;
            break;
        case IntegerType::IntegerTypeID::USHRT:
            max.ushrt_val = (unsigned short) _max;
            break;
        case IntegerType::IntegerTypeID::INT:
            max.int_val = (int) _max;
            break;
        case IntegerType::IntegerTypeID::UINT:
            max.uint_val = (unsigned int) _max;
            break;
        case IntegerType::IntegerTypeID::LINT:
            max.lint_val = (long int) _max;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            max.ulint_val = (unsigned long int) _max;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            max.llint_val = (long long int) _max;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            max.ullint_val = (unsigned long long int) _max;
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN Data: set_max" << std::endl;
            break;
    }
}

uint64_t Array::get_max () {
    switch (std::static_pointer_cast<IntegerType>(base_type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            return (bool) max.bool_val;
        case IntegerType::IntegerTypeID::CHAR:
            return (signed char) max.char_val;
        case IntegerType::IntegerTypeID::UCHAR:
            return (unsigned char) max.uchar_val;
        case IntegerType::IntegerTypeID::SHRT:
            return (short) max.shrt_val;
        case IntegerType::IntegerTypeID::USHRT:
            return (unsigned short) max.ushrt_val;
        case IntegerType::IntegerTypeID::INT:
            return (int) max.int_val;
        case IntegerType::IntegerTypeID::UINT:
            return (unsigned int) max.uint_val;
        case IntegerType::IntegerTypeID::LINT:
            return (long int) max.lint_val;
        case IntegerType::IntegerTypeID::ULINT:
            return (unsigned long int) max.ulint_val;
        case IntegerType::IntegerTypeID::LLINT:
            return (long long int) max.llint_val;
        case IntegerType::IntegerTypeID::ULLINT:
            return (unsigned long long int) max.ullint_val;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: get_max" << std::endl;
            return 0;
    }
}

void Array::set_min (uint64_t _min) {
    switch (std::static_pointer_cast<IntegerType>(base_type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            min.bool_val = (bool) _min;
            break;
        case IntegerType::IntegerTypeID::CHAR:
            min.char_val = (signed char) _min;
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            min.uchar_val = (unsigned char) _min;
            break;
        case IntegerType::IntegerTypeID::SHRT:
            min.shrt_val = (short) _min;
            break;
        case IntegerType::IntegerTypeID::USHRT:
            min.ushrt_val = (unsigned short) _min;
            break;
        case IntegerType::IntegerTypeID::INT:
            min.int_val = (int) _min;
            break;
        case IntegerType::IntegerTypeID::UINT:
            min.uint_val = (unsigned int) _min;
            break;
        case IntegerType::IntegerTypeID::LINT:
            min.lint_val = (long int) _min;
            break;
        case IntegerType::IntegerTypeID::ULINT:
            min.ulint_val = (unsigned long int) _min;
            break;
        case IntegerType::IntegerTypeID::LLINT:
            min.llint_val = (long long int) _min;
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            min.ullint_val = (unsigned long long int) _min;
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN Data: set_min" << std::endl;
            break;
    }
}

uint64_t Array::get_min () {
    switch (std::static_pointer_cast<IntegerType>(base_type)->get_int_type_id ()) {
        case IntegerType::IntegerTypeID::BOOL:
            return (bool) min.bool_val;
        case IntegerType::IntegerTypeID::CHAR:
            return (signed char) min.char_val;
        case IntegerType::IntegerTypeID::UCHAR:
            return (unsigned char) min.uchar_val;
        case IntegerType::IntegerTypeID::SHRT:
            return (short) min.shrt_val;
        case IntegerType::IntegerTypeID::USHRT:
            return (unsigned short) min.ushrt_val;
        case IntegerType::IntegerTypeID::INT:
            return (int) min.int_val;
        case IntegerType::IntegerTypeID::UINT:
            return (unsigned int) min.uint_val;
        case IntegerType::IntegerTypeID::LINT:
            return (long int) min.lint_val;
        case IntegerType::IntegerTypeID::ULINT:
            return (unsigned long int) min.ulint_val;
        case IntegerType::IntegerTypeID::LLINT:
            return (long long int) min.llint_val;
        case IntegerType::IntegerTypeID::ULLINT:
            return (unsigned long long int) min.ullint_val;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "BAD TYPE IN DATA: get_min" << std::endl;
            return 0;
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
    std::cout << "modifier: " << type->get_modifier() << std::endl;
    std::cout << "value: " << value.ullint_val << std::endl;
    std::cout << "min: " << min.ullint_val << std::endl;
    std::cout << "max: " << max.ullint_val << std::endl;
}
