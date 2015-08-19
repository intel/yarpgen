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

unsigned int MAX_ARRAY_SIZE = 100;
unsigned int ARR_BASE_NUM = 0;

Array::Array (std::string _name, unsigned int _type_id, unsigned int _size) {
    this->name = _name;
    this->type = Type::init (_type_id);
    this->size = _size;
}

Array Array::get_rand_obj () {
    std::uniform_int_distribution<unsigned int> type_dis(0, Type::TypeID::MAX_TYPE_ID - 1);
    std::uniform_int_distribution<unsigned int> size_dis(1, MAX_ARRAY_SIZE);
    return Array ("in_" + std::to_string(ARR_BASE_NUM++), type_dis(rand_gen), size_dis(rand_gen));
}

std::string Array::emit_usage () {
    std::string ret = this->get_name () + " [i]";
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
