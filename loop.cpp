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
    this->iter_type = Type (Type::TypeID::UCHAR);
    this->start_val = this->iter_type.get_min_value ();
    this->end_val = this->iter_type.get_max_value ();
    this->step = 1;
    this->condition = CondType::LT;
}

void Loop::set_loop_type (unsigned int _loop_type) { this->loop_type = _loop_type; }

unsigned int Loop::get_loop_type () { return this->loop_type; }

void Loop::set_iter_type (unsigned int _iter_type_id) { this->iter_type = Type (_iter_type_id); }

unsigned int Loop::get_iter_type_id () {return this->iter_type.get_id(); }

std::string Loop::get_iter_type_name () { return this->iter_type.get_name (); }

void Loop::set_start_val (int64_t _start_val) { this->start_val = _start_val; }

int64_t Loop::get_start_val () { return this->start_val; }

void Loop::set_end_val (int64_t _end_val) { this->end_val = _end_val; }

int64_t Loop::get_end_val () { return this->end_val; }

void Loop::set_step (int64_t _step) { this->step = _step; }

int64_t Loop::get_step () { return this->step; }

void Loop::set_condition (unsigned int _condition) { this->condition = _condition; }

unsigned int Loop::get_condition () { return this->condition; }

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
    this->iter_type.dbg_dump ();
    std::cout << "start_val " << this->start_val << std::endl;
    std::cout << "end_val " << this->end_val << std::endl;
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
