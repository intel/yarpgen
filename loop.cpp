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

#include "loop.h"

Loop::Loop () {
    this->loop_type = LoopType::FOR;
    this->iter_type = Type::init (Type::TypeID::UCHAR);
    this->step = 1;
    this->condition = CondType::LT;
}

void Loop::set_loop_type (unsigned int _loop_type) { this->loop_type = _loop_type; }

unsigned int Loop::get_loop_type () const { return this->loop_type; }

void Loop::set_iter_type (unsigned int _iter_type_id) { this->iter_type = Type::init (_iter_type_id); }

unsigned int Loop::get_iter_type_id () const {return this->iter_type->get_id(); }

std::string Loop::get_iter_type_name () const { return this->iter_type->get_name (); }

void Loop::set_start_value (uint64_t _start_val) { this->iter_type->set_min_value(_start_val); }

uint64_t Loop::get_start_value () const { return this->iter_type->get_min_value(); }

void Loop::set_end_value (uint64_t _end_val) { this->iter_type->set_max_value(_end_val); }

uint64_t Loop::get_end_value () const { return this->iter_type->get_max_value(); }

void Loop::set_step (uint64_t _step) { this->step = _step; }

uint64_t Loop::get_step () const { return this->step; }

void Loop::set_condition (unsigned int _condition) { this->condition = _condition; }

unsigned int Loop::get_condition () const { return this->condition; }

void Loop::random_fill () {

}

std::string Loop::emit_usage () {
    std::string ret;
    switch (get_loop_type ()) {
        case LoopType::FOR:
            ret += "for (";
            break;
        case LoopType::WHILE:
            ret += "while (";
            break;
        case LoopType::DO_WHILE:
            ret += "do {\n";
            break;
        default:
            ret += "ERROR IN LOOP TYPE";
            break;
    }
    ret += this->iter_type->emit_usage ();
    ret += " i = " + std::to_string(get_start_value ()) + ";";
    ret += " i " + get_condition_name () + " ";
    ret += std::to_string(get_end_value ()) + ";";
    ret += " i += " + std::to_string(get_step ()) + ") { \n";
    ret += "\n}";
    return ret;
}

std::string Loop::get_condition_name () {
    std::string ret = "";
    switch (this->condition) {
        case CondType::EQ:
            ret += "==";
            break;
        case CondType::NE:
            ret += "!=";
            break;
        case CondType::GT:
            ret += ">";
            break;
        case CondType::GE:
            ret += ">=";
            break;
        case CondType::LT:
            ret += "<";
            break;
        case CondType::LE:
            ret += "<=";
            break;
        default:
            ret += "ERROR IN CONDITION";
            break;
    }
    return ret;
}

void Loop::dbg_dump () {
    std::string loop_type_str;
    switch (this->loop_type) {
        case LoopType::FOR:
            loop_type_str = "for";
            break;
        case LoopType::WHILE:
            loop_type_str = "while";
            break;
        case LoopType::DO_WHILE:
            loop_type_str = "do while";
            break;
        default:
            loop_type_str = "ERROR IN LOOP TYPE";
            break;
    }
    std::cout << "loop type " << loop_type_str << std::endl;
    this->iter_type->dbg_dump ();
    std::cout << "start_val " << get_start_value() << std::endl;
    std::cout << "end_val " << get_end_value() << std::endl;
    std::cout << "step " << this->step << std::endl;

    std::string cond_str;
    switch (this->condition) {
        case CondType::EQ:
            cond_str = "==";
            break;
        case CondType::NE:
            cond_str = "!=";
            break;
        case CondType::GT:
            cond_str = ">";
            break;
        case CondType::GE:
            cond_str = ">=";
            break;
        case CondType::LT:
            cond_str = "<";
            break;
        case CondType::LE:
            cond_str = "<=";
            break;
        default:
            cond_str = "ERROR IN CONDITION";
            break;
    }
    std::cout << "condition " << cond_str << std::endl;
}
