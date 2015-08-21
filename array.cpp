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

#include "array.h"

unsigned int MAX_ARRAY_SIZE = 250;
unsigned int MIN_ARRAY_SIZE = 20;

Array::Array (std::string _name, unsigned int _type_id, unsigned int _size) {
    this->name = _name;
    this->type = Type::init (_type_id);
    this->size = _size;
}

Array Array::get_rand_obj (std::string _name) {
    std::uniform_int_distribution<unsigned int> type_dis(0, Type::TypeID::MAX_TYPE_ID - 1);
    std::uniform_int_distribution<unsigned int> size_dis(MIN_ARRAY_SIZE, MAX_ARRAY_SIZE);
    return Array (_name, type_dis(rand_gen), size_dis(rand_gen));
}

std::string Array::emit_usage () {
    std::string ret = this->get_name () + " [i]";
    return ret;
}

std::string Array::emit_definition (bool rand_init) {
    std::string ret = get_type ()->emit_usage ();
    ret += " " + get_name ();
    ret += " [" + std::to_string(get_size ()) + "] ";
    ret += " = {";
    ret += rand_init ? get_type()->get_rand_value_str() : "0";
    for (int i = 1; i < get_size(); i++)
        ret += ", " + (rand_init ? get_type()->get_rand_value_str() : "0");
    ret += "};\n";
    return ret;
}

void Array::dbg_dump () {
    std::cout << "name " << this->name << std::endl;
    std::cout << "size " << this->size << std::endl;
    this->type->dbg_dump();
}

std::string Array::get_name () const { return this->name; }

void Array::set_size (unsigned int _size) { this->size = _size; }

unsigned int Array::get_size () const { return this->size; }

std::shared_ptr<Type> Array::get_type () { return this->type; }

unsigned int Array::get_type_id () const { return this->type->get_id(); }

std::string Array::get_type_name () const { return this->type->get_name(); }

void Array::set_max_value (uint64_t _max_val) { this->type->set_max_value(_max_val); }

uint64_t Array::get_max_value () const { return this->type->get_max_value(); }

void Array::set_min_value (uint64_t _min_val) { this->type->set_min_value(_min_val); }

uint64_t Array::get_min_value () const { return this->type->get_min_value(); }

void Array::set_bound_value (std::vector<uint64_t> bval) { this->type->set_bound_value(bval); }

std::vector<uint64_t> Array::get_bound_value () const { return this->type->get_bound_value (); }

bool Array::get_is_fp () const { return this->type->get_is_fp(); }

bool Array::get_is_signed () const { return this->type->get_is_signed(); }
