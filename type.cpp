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

#include "type.h"

int rand_dev () {
//    return 2107662808; // TODO: enable random
    std::random_device rd;
    int ret = rd ();
    std::cout << "/*SEED " << ret << "*/\n";
    std::cerr << "/*SEED " << ret << "*/\n";
    return ret;
}

std::mt19937_64 rand_gen(rand_dev());

std::shared_ptr<Type> Type::init (unsigned int _type_id) {
    std::shared_ptr<Type> ret (NULL);
    switch (_type_id) {
/*
        case TypeID::UCHAR:
            ret = std::make_shared<TypeUCHAR> (TypeUCHAR());
            break;
        case TypeID::USHRT:
            ret = std::make_shared<TypeUSHRT> (TypeUSHRT());
            break;
*/
        case TypeID::UINT:
            ret = std::make_shared<TypeUINT> (TypeUINT());
            break;
        case TypeID::ULINT:
            ret = std::make_shared<TypeULINT> (TypeULINT());
            break;
         case TypeID::ULLINT:
            ret = std::make_shared<TypeULLINT> (TypeULLINT());
            break;
    }
    return ret;
}

std::shared_ptr<Type> Type::get_rand_obj () {
    std::uniform_int_distribution<unsigned int> dis(0, Type::TypeID::MAX_TYPE_ID - 1);
    std::shared_ptr<Type> ret = init (dis(rand_gen));
    return ret;
}

std::shared_ptr<Type> Type::get_copy (std::shared_ptr<Type> type) {
    std::shared_ptr<Type> ret = Type::init(type->get_id ());
    ret->set_max_value (type->get_max_value ());
    ret->set_min_value (type->get_min_value ());
    ret->set_value (type->get_value ());
    return ret;
}

unsigned int Type::get_id () { return this->id; }

std::string Type::get_name () { return this->name; }

bool Type::get_is_fp () { return this->is_fp; }

bool Type::get_is_signed () { return this->is_signed; }

void Type::set_max_value (uint64_t _max_val) { this->max_val = _max_val; }

uint64_t Type::get_max_value () { return this->max_val; }

void Type::set_min_value (uint64_t _min_val) { this->min_val = _min_val; }

uint64_t Type::get_min_value () { return this->min_val; }

uint64_t Type::get_abs_max () { return this->abs_max; }

uint64_t Type::get_abs_min () { return this->abs_min; }

uint64_t Type::get_bit_size () { return this->bit_size; }

 void Type::set_value (uint64_t _val) { this->value = _val; }

uint64_t Type::get_value () {  return this->value; }

void Type::set_bound_value (std::vector<uint64_t> bval) { this->bound_val = bval; }

std::vector<uint64_t> Type::get_bound_value () { return this->bound_val; }

void Type::add_bound_value (uint64_t bval) { this->bound_val.push_back(bval); }

bool Type::check_val_in_domains (uint64_t val) {
    bool ret = (get_min_value() <= val);
    ret &= (val <= get_max_value());
    ret |= (std::find(bound_val.begin(), bound_val.end(), val) != bound_val.end());
    return ret;
}

std::string Type::emit_usage () {
    return get_name ();
}

void Type::combine_range (std::shared_ptr<Type> _type) {
    //TODO: need something for signed values and fp

    //TODO: bound values
//    this->combine_bound_value (_type);
    uint64_t val_1 = get_min_value ();
    uint64_t val_2 = _type->get_min_value ();
    uint64_t comb_min = std::max (val_1, val_2);
    val_1 = get_max_value ();
    val_2 = _type->get_max_value ();
    uint64_t comb_max = std::min(val_1, val_2);
    this->set_min_value(std::min(comb_min, comb_max));
    this->set_max_value(std::max(comb_min, comb_max));
}


void Type::dbg_dump () {
    std::cout << "name " << this->name << std::endl;
    std::cout << "min_val " << this->min_val << std::endl;
    std::cout << "max_val " << this->max_val << std::endl;
    std::cout << "bound values ";
    for (std::vector<uint64_t>::iterator i = this->bound_val.begin(); i != bound_val.end(); ++i)
        std::cout << *i << ' ';
    std::cout << std::endl;
    std::cout << "is_fp " << this->is_fp << std::endl;
    std::cout << "is_signed " << this->is_signed << std::endl;
}

bool Type::check_val_in_domains (std::string val) {
    if (val == "MAX")
        return check_val_in_domains(get_abs_max());
    if (val == "MIN")
        return check_val_in_domains(get_abs_min());
    return false;
}

/*
void Type::combine_bound_value (std::shared_ptr<Type> _type) {
    // TODO: ret is const ref
    for (int i = 0; i < _type->get_bound_value().size(); ++i)
        this->bound_val.push_back(_type->get_bound_value().at(i));
}
*/
/*
TypeUCHAR::TypeUCHAR () {
    this->id = Type::TypeID::UCHAR;
    this->name = "unsigned char";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = UCHAR_MAX;
    this->bit_size = sizeof (unsigned char) * CHAR_BIT;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeUCHAR::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned char> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen); 
    return ret;
}

uint64_t TypeUCHAR::get_rand_value (uint64_t a, uint64_t b) {
    std::shared_ptr<Type> _type = Type::init(get_id());
    _type->set_min_value (a);
    _type->set_max_value (b);
    return _type->get_rand_value ();
}

std::string TypeUCHAR::get_rand_value_str () {
    return std::to_string ((unsigned char) get_rand_value ());
}

TypeUSHRT::TypeUSHRT () {
    this->id = Type::TypeID::USHRT;
    this->name = "unsigned short";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = USHRT_MAX;
    this->bit_size = sizeof (unsigned short) * CHAR_BIT;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeUSHRT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned short> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

uint64_t TypeUSHRT::get_rand_value (uint64_t a, uint64_t b) {
    std::shared_ptr<Type> _type = Type::init(get_id());
    _type->set_min_value (a);
    _type->set_max_value (b);
    return _type->get_rand_value ();
}

std::string TypeUSHRT::get_rand_value_str () {
    return std::to_string ((unsigned short) get_rand_value ());
}
*/
TypeUINT::TypeUINT () {
    this->id = Type::TypeID::UINT;
    this->name = "unsigned int";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = UINT_MAX;
    this->bit_size = sizeof (unsigned int) * CHAR_BIT;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeUINT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned int> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

uint64_t TypeUINT::get_rand_value (uint64_t a, uint64_t b) {
    std::shared_ptr<Type> _type = Type::init(get_id());
    _type->set_min_value (a);
    _type->set_max_value (b);
    return _type->get_rand_value ();
}

std::string TypeUINT::get_rand_value_str () {
    return std::to_string ((unsigned int) get_rand_value ()) + "U";
}

std::string TypeUINT::get_value_str () {
    return std::to_string ((unsigned int) get_value ()) + "U";
}

std::string TypeUINT::get_max_value_str () {
    return std::to_string ((unsigned int) get_max_value ()) + "U";
}

std::string TypeUINT::get_min_value_str () {
    return std::to_string ((unsigned int) get_min_value ()) + "U";
}

TypeULINT::TypeULINT () {
    this->id = Type::TypeID::ULINT;
    this->name = "unsigned long int";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = ULONG_MAX;
    this->bit_size = sizeof (unsigned long int) * CHAR_BIT;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeULINT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned long int> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

uint64_t TypeULINT::get_rand_value (uint64_t a, uint64_t b) {
    std::shared_ptr<Type> _type = Type::init(get_id());
    _type->set_min_value (a);
    _type->set_max_value (b);
    return _type->get_rand_value ();
}

std::string TypeULINT::get_rand_value_str () {
    return std::to_string ((unsigned long int) get_rand_value ()) + "UL";
}

std::string TypeULINT::get_value_str () {
    return std::to_string ((unsigned long int) get_value ()) + "UL";
}

std::string TypeULINT::get_max_value_str () {
    return std::to_string ((unsigned long int) get_max_value ()) + "UL";
}

std::string TypeULINT::get_min_value_str () {
    return std::to_string ((unsigned long int) get_min_value ()) + "UL";
}

TypeULLINT::TypeULLINT () {
    this->id = Type::TypeID::ULLINT;
    this->name = "unsigned long long int";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = ULLONG_MAX;
    this->bit_size = sizeof (unsigned long long int) * CHAR_BIT;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeULLINT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned long long int> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

uint64_t TypeULLINT::get_rand_value (uint64_t a, uint64_t b) {
    std::shared_ptr<Type> _type = Type::init(get_id());
    _type->set_min_value (a);
    _type->set_max_value (b);
    return _type->get_rand_value ();
}

std::string TypeULLINT::get_rand_value_str () {
    return std::to_string ((unsigned long long int) get_rand_value ()) + "ULL";
}

std::string TypeULLINT::get_value_str () {
    return std::to_string ((unsigned long long int) get_value ()) + "ULL";
}

std::string TypeULLINT::get_max_value_str () {
    return std::to_string ((unsigned long long int) get_max_value ()) + "ULL";
}

std::string TypeULLINT::get_min_value_str () {
    return std::to_string ((unsigned long long int) get_min_value ()) + "ULL";
}
