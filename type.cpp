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

Type* Type::init (unsigned int _type_id) {
    Type* ret = NULL;
    switch (_type_id) {
        case TypeID::UCHAR:
            ret = new TypeUCHAR ();
            break;
        case TypeID::USHRT:
            ret = new TypeUSHRT ();
            break;
        case TypeID::UINT:
            ret = new TypeUINT ();
            break;
        case TypeID::ULINT:
            ret = new TypeULINT ();
            break;
         case TypeID::ULLINT:
            ret = new TypeULLINT ();
            break;
    }
    return ret;
}

Type* Type::get_rand_obj () {
    std::uniform_int_distribution<unsigned int> dis(0, Type::TypeID::MAX_TYPE_ID - 1);
    Type* ret = init (dis(rand_gen));
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
    std::cout << "name " << this->name << "\nmin_val " << this->min_val << "\nmax_val " << this->max_val << "\nis_fp " << this->is_fp << "\nis_signed " << this->is_signed << std::endl;
}

bool Type::check_val_in_domains (std::string val) {
    if (val == "MAX")
        return check_val_in_domains(get_abs_max());
    if (val == "MIN")
        return check_val_in_domains(get_abs_min());
    return false;
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

std::string TypeULLINT::get_rand_value_str () {
    return std::to_string (get_rand_value ()) + "ULL";
}
