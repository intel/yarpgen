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
    std::random_device rd;
    return rd();
//    return 4096; // TODO: enable random
}

std::mt19937_64 rand_gen(rand_dev());

std::shared_ptr<Type> Type::init (unsigned int _type_id) {
    std::shared_ptr<Type> ret (NULL);
    switch (_type_id) {
        case TypeID::UCHAR:
            ret = std::make_shared<TypeUCHAR> (TypeUCHAR());
            break;
        case TypeID::USHRT:
            ret = std::make_shared<TypeUSHRT> (TypeUSHRT());
            break;
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

void Type::set_bound_value (std::vector<uint64_t> bval) { this->bound_val = bval; }

std::vector<uint64_t> Type::get_bound_value () { return this->bound_val; }

void Type::add_bound_value (uint64_t bval) { this->bound_val.push_back(bval); }

bool Type::check_val_in_domains (uint64_t val) {
    bool ret = (get_min_value() <= val);
    ret &= (val <= get_max_value());
    ret |= (std::find(bound_val.begin(), bound_val.end(), val) != bound_val.end());
    return ret;
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

void Type::combine_bound_value (std::shared_ptr<Type> _type) {
    // TODO: WTF???
/*
    std::cout << "DEBUG bound values ";
    for (std::vector<uint64_t>::iterator i = _type->get_bound_value().begin(); i != _type->get_bound_value().end(); ++i)
        std::cout << *i << ' ';
    std::cout << std::endl;
    std::cout << "DEBUG bound values ";
    for (std::vector<uint64_t>::iterator i = this->bound_val.begin(); i != this->bound_val.end(); ++i)
        std::cout << *i << ' ';
    std::cout << std::endl;

    //std::cout << "DEBUG bound values ";
    //for (std::vector<uint64_t>::iterator i = _type->get_bound_value().begin(); i != _type->get_bound_value().end(); ++i) {
    //    this->bound_val.push_back(*i);
    //    std::cout << *i << ' ';
    //}
    //std::cout << std::endl;
*/
    for (int i = 0; i < _type->get_bound_value().size(); ++i)
        this->bound_val.push_back(_type->get_bound_value().at(i));
/*
    //this->bound_val.insert(this->bound_val.end(), _type->get_bound_value().begin(), _type->get_bound_value().end());

    std::cout << "DEBUG bound values ";
    for (std::vector<uint64_t>::iterator i = _type->get_bound_value().begin(); i != _type->get_bound_value().end(); ++i)
        std::cout << *i << ' ';
    std::cout << std::endl;
    std::cout << "DEBUG bound values ";
    for (std::vector<uint64_t>::iterator i = this->bound_val.begin(); i != this->bound_val.end(); ++i)
        std::cout << *i << ' ';
    std::cout << std::endl;
*/
}

TypeUCHAR::TypeUCHAR () {
    this->id = Type::TypeID::UCHAR;
    this->name = "unsigned char";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = UCHAR_MAX;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeUCHAR::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned char> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen); 
    return ret;
}

void TypeUCHAR::combine_range (std::shared_ptr<Type> _type) {
    this->combine_bound_value (_type);
    uint64_t val_1 = get_min_value ();
    uint64_t val_2 =  _type-> get_min_value ();
    uint64_t comb_min = ((unsigned char) val_1 > (unsigned char) val_2) ? val_1 : val_2;
    val_1 = get_max_value ();
    val_2 =  _type-> get_max_value ();
    uint64_t comb_max = ((unsigned char) val_1 < (unsigned char) val_2) ? val_1 : val_2;
    this->set_min_value(((unsigned char) comb_min < (unsigned char) comb_max) ? comb_min : comb_max);
    this->set_max_value(((unsigned char) comb_max > (unsigned char) comb_min) ? comb_max : comb_min);
}

std::string TypeUCHAR::get_rand_value_str () {
    return std::to_string (get_rand_value ());
}

TypeUSHRT::TypeUSHRT () {
    this->id = Type::TypeID::USHRT;
    this->name = "unsigned short";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = USHRT_MAX;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeUSHRT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned short> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

void TypeUSHRT::combine_range (std::shared_ptr<Type> _type) {
    this->combine_bound_value (_type);
    uint64_t val_1 = get_min_value ();
    uint64_t val_2 =  _type-> get_min_value ();
    uint64_t comb_min = ((unsigned short) val_1 > (unsigned short) val_2) ? val_1 : val_2;
    val_1 = get_max_value ();
    val_2 =  _type-> get_max_value ();
    uint64_t comb_max = ((unsigned short) val_1 < (unsigned short) val_2) ? val_1 : val_2;
    this->set_min_value(((unsigned short) comb_min < (unsigned short) comb_max) ? comb_min : comb_max);
    this->set_max_value(((unsigned short) comb_max > (unsigned short) comb_min) ? comb_max : comb_min);
}

std::string TypeUSHRT::get_rand_value_str () {
    return std::to_string (get_rand_value ());
}

TypeUINT::TypeUINT () {
    this->id = Type::TypeID::UINT;
    this->name = "unsigned int";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = UINT_MAX;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeUINT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned int> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

void TypeUINT::combine_range (std::shared_ptr<Type> _type) {
    this->combine_bound_value (_type);
    uint64_t val_1 = get_min_value ();
    uint64_t val_2 =  _type-> get_min_value ();
    uint64_t comb_min = ((unsigned int) val_1 > (unsigned int) val_2) ? val_1 : val_2;
    val_1 = get_max_value ();
    val_2 =  _type-> get_max_value ();
    uint64_t comb_max = ((unsigned int) val_1 < (unsigned int) val_2) ? val_1 : val_2;
    this->set_min_value(((unsigned int) comb_min < (unsigned int) comb_max) ? comb_min : comb_max);
    this->set_max_value(((unsigned int) comb_max > (unsigned int) comb_min) ? comb_max : comb_min);
}

std::string TypeUINT::get_rand_value_str () {
    return std::to_string (get_rand_value ()) + "U";
}

TypeULINT::TypeULINT () {
    this->id = Type::TypeID::ULINT;
    this->name = "unsigned long int";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = ULONG_MAX;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeULINT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned long int> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

void TypeULINT::combine_range (std::shared_ptr<Type> _type) {
    this->combine_bound_value (_type);
    uint64_t val_1 = get_min_value ();
    uint64_t val_2 =  _type-> get_min_value ();
    uint64_t comb_min = ((unsigned long int) val_1 > (unsigned long int) val_2) ? val_1 : val_2;
    val_1 = get_max_value ();
    val_2 =  _type-> get_max_value ();
    uint64_t comb_max = ((unsigned long int) val_1 < (unsigned long int) val_2) ? val_1 : val_2;
    this->set_min_value(((unsigned long int) comb_min < (unsigned long int) comb_max) ? comb_min : comb_max);
    this->set_max_value(((unsigned long int) comb_max > (unsigned long int) comb_min) ? comb_max : comb_min);
}

std::string TypeULINT::get_rand_value_str () {
    return std::to_string (get_rand_value ()) + "UL";
}

TypeULLINT::TypeULLINT () {
    this->id = Type::TypeID::ULLINT;
    this->name = "unsigned long long int";
    this->min_val = this->abs_min = 0;
    this->max_val = this->abs_max = ULLONG_MAX;
    this->is_fp = false;
    this->is_signed = false;
}

uint64_t TypeULLINT::get_rand_value () {
//    TODO: add boundary values
    std::uniform_int_distribution<unsigned long long int> dis(this->min_val, this->max_val);
    uint64_t ret = dis(rand_gen);
    return ret;
}

void TypeULLINT::combine_range (std::shared_ptr<Type> _type) {
    this->combine_bound_value (_type);
    uint64_t val_1 = get_min_value ();
    uint64_t val_2 =  _type-> get_min_value ();
    uint64_t comb_min = ((unsigned long long int) val_1 > (unsigned long long int) val_2) ? val_1 : val_2;
    val_1 = get_max_value ();
    val_2 =  _type-> get_max_value ();
    uint64_t comb_max = ((unsigned long long int) val_1 < (unsigned long long int) val_2) ? val_1 : val_2;
    this->set_min_value(((unsigned long long int) comb_min < (unsigned long long int) comb_max) ? comb_min : comb_max);
    this->set_max_value(((unsigned long long int) comb_max > (unsigned long long int) comb_min) ? comb_max : comb_min);
}


std::string TypeULLINT::get_rand_value_str () {
    return std::to_string (get_rand_value ()) + "ULL";
}
