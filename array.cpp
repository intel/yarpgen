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

unsigned int MAX_ARRAY_SIZE = 10000;
unsigned int MIN_ARRAY_SIZE = 1000;

Array::Array (std::string _name, unsigned int _type_id, unsigned int _size) {
    this->name = _name;
    this->type = Type::init (_type_id);
    this->size = _size;
}

Array Array::get_rand_obj (std::string _name, std::string _iter_name) {
    std::uniform_int_distribution<unsigned int> type_dis(0, Type::TypeID::MAX_TYPE_ID - 1);
    std::uniform_int_distribution<unsigned int> size_dis(MIN_ARRAY_SIZE, MAX_ARRAY_SIZE);
    Array ret = Array (_name, type_dis(rand_gen), size_dis(rand_gen));
    ret.set_iter_name (_iter_name);
    // TODO: volatile modifier prevents optimization, so disable it
    // TODO: can't dymanically init global array, so disable it
//    std::uniform_int_distribution<unsigned int> modifier_dis(0, Array::Mod::VOLAT - 1);
    ret.set_modifier( Array::Mod::NTHNG);
    std::uniform_int_distribution<unsigned int> static_dis(0, 1);
    // TODO: enable static and extern
//    ret.set_is_static(static_dis(rand_gen));
    ret.set_is_static(false);
    return ret;
}

std::string Array::emit_usage () {
    std::string ret = this->get_name () + " [" + this->get_iter_name () + "]";
    return ret;
}

std::string Array::emit_definition (bool rand_init) {
    std::string ret = "\tfor (int i = 0; i < " + std::to_string(get_size ()) + "; ++i)\n\t\t";
    ret += get_name () + " [i] = " + (rand_init ? get_type()->get_rand_value_str() : "0");
    ret += ";\n";
    return ret;
}

std::string Array::emit_declaration () {
    std::string ret = get_is_static() ? "static " : "";
    switch (get_modifier()) {
        case VOLAT:
            ret += "volatile ";
            break;
        case CONST:
            ret += "const ";
            break;
        case CONST_VOLAT:
            ret += "const volatile ";
            break;
        case NTHNG:
            break;
    }
    ret += get_type ()->emit_usage ();
    ret += " " + get_name ();
    ret += " [" + std::to_string(get_size ()) + "]";
    return ret;
}

void Array::dbg_dump () {
    std::cout << "name " << this->name << std::endl;
    std::cout << "size " << this->size << std::endl;
    this->type->dbg_dump();
}

std::string Array::get_name () const { return this->name; }

void  Array::set_iter_name (std::string _iter_name) { this->iter_name = _iter_name; }

std::string Array::get_iter_name () { return this->iter_name; }

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

void Array::set_modifier (unsigned int _mod) { this->modifier = _mod; }

unsigned int Array::get_modifier () { return this->modifier; }

void Array::set_is_static (bool _stat) { this->is_static = _stat; }
bool Array::get_is_static () { return this->is_static; }
