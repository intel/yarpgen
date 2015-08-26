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

unsigned int MAX_ARRAY_NUM = 200;
unsigned int MIN_ARRAY_NUM = 20;

unsigned int MAX_STMNT_NUM = 200;
unsigned int MIN_STMNT_NUM = 20;

unsigned int MAX_DEPTH = 200;
unsigned int MIN_DEPTH = 6;

Loop::Loop () {
    this->loop_type = LoopType::FOR;
    this->iter_type = Type::init (Type::TypeID::UINT);
    this->step = 1;
    this->condition = CondType::LT;
}

void Loop::set_loop_type (unsigned int _loop_type) { this->loop_type = _loop_type; }

unsigned int Loop::get_loop_type () const { return this->loop_type; }

void Loop::set_iter_type (std::shared_ptr<Type> _type) { this->iter_type = _type; }

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

unsigned int Loop::init_array () {
    std::uniform_int_distribution<unsigned int> dis(MIN_ARRAY_NUM, MAX_ARRAY_NUM);
    int arr_num = dis(rand_gen);
    unsigned int min_size = UINT_MAX;
    for (int i = 0; i < arr_num; ++i) {
        this->in.push_back(Array::get_rand_obj ("in_" + std::to_string(i)));
        min_size = min_size < this->in.at(i).get_size() ? min_size : this->in.at(i).get_size();
        this->out.push_back(Array::get_rand_obj ("out_" + std::to_string(i)));
        //TODO: move to get_rand_obj
        std::uniform_int_distribution<unsigned int> modifier_dis(0, Array::Mod::VOLAT);
        this->out.at(i).set_modifier(modifier_dis(rand_gen));
        min_size = min_size < this->out.at(i).get_size() ? min_size : this->out.at(i).get_size();
    }
    return min_size;
}

void Loop::init_stmnt () {
    std::uniform_int_distribution<unsigned int> depth_dis(MIN_DEPTH, MAX_DEPTH);
    for (int i = 0; i < this->out.size(); ++i) {
        this->stmnt.push_back(Statement (i, std::make_shared<std::vector<Array>>(in), std::make_shared<std::vector<Array>>(out)));
        this->stmnt.at(i).set_depth(depth_dis(rand_gen));
        this->stmnt.at(i).random_fill ();
    }

}

void Loop::random_fill () {
    unsigned int min_size = init_array();

    std::uniform_int_distribution<unsigned int> loop_type_dis(0, MAX_LOOP_TYPE - 1);
    set_loop_type (loop_type_dis(rand_gen));

    bool incomp_cond;

    do {
        incomp_cond = false;
        set_iter_type (Type::get_rand_obj());

        std::uniform_int_distribution<unsigned int> cond_type_dis(0, MAX_COND - 1);
        set_condition (cond_type_dis(rand_gen));


        uint64_t max_val = get_end_value() < min_size ? get_end_value() : min_size;
        set_start_value(this->iter_type->get_rand_value(0, max_val - 1));

        uint64_t end_val = 0;
        uint64_t _step = 0;
        switch (get_condition()) {
            case EQ:
                end_val = get_start_value();
                _step = iter_type->get_rand_value(iter_type->get_abs_min(), iter_type->get_abs_max());
                incomp_cond = _step == 0;
                break;
            case NE:
                if (!this->iter_type->get_is_signed()) {
                    end_val = this->iter_type->get_rand_value(get_start_value(), (max_val - 1));
                    uint64_t diff =  end_val - get_start_value();
                    if (diff == 0) {
                        incomp_cond = true;
                        break;
                    }
                    std::uniform_int_distribution<unsigned int> step_dis(1, diff);
                    do {
                        _step = diff / step_dis(rand_gen);
                    }
                    while (diff % _step != 0);
                }
                break;
            case GT:
                if (!this->iter_type->get_is_signed()) {
                    incomp_cond = true;
                    break;
                }
                end_val = this->iter_type->get_rand_value(0, (get_start_value() != 0) ? (get_start_value() - 1) : 0);
                break;
            case GE:
                if (!this->iter_type->get_is_signed()) {
                    incomp_cond = true;
                    break;
                }
                end_val = this->iter_type->get_rand_value(0, get_start_value());
                break;
            case LT:
                end_val = this->iter_type->get_rand_value((get_start_value() != this->iter_type->get_abs_max()) ? 
                                                          (get_start_value() + 1) : this->iter_type->get_abs_max(), max_val);
                _step = this->iter_type->get_rand_value(1, end_val - get_start_value());
                break;
            case LE:
                end_val = this->iter_type->get_rand_value(get_start_value(), (max_val - 1));
                _step = this->iter_type->get_rand_value(1, end_val - get_start_value());
                break;
            case MAX_COND:
                break;
        }
        set_end_value (end_val);
        set_step(_step);

        //TODO: choose strategy
        if (!this->iter_type->get_is_signed()) {
            if ((get_condition() == GT) || // negative step can't be with unsigned iterator
                (get_condition() == GE))
                incomp_cond = true;
        }
    }
    while (incomp_cond);

    init_stmnt();

}

std::string Loop::emit_usage () {
    std::string ret = "#include <cstdint>\n";
    ret += "#include <iostream>\n";

    for (auto i = this->in.begin(); i != this->in.end(); ++i)
        ret += i->emit_definition (true);
    for (auto i = this->out.begin(); i != this->out.end(); ++i)
        ret += i->emit_definition (false);

    ret += "void foo () {\n\t";
    switch (get_loop_type ()) {
        case LoopType::FOR:
            ret += "for (";
            ret += this->iter_type->emit_usage ();
            ret += " i = " + std::to_string(get_start_value ()) + ";";
            ret += " i " + get_condition_name () + " ";
            ret += std::to_string(get_end_value ()) + ";";
            ret += " i += " + std::to_string(get_step ()) + ") { \n\t";
            break;
        case LoopType::WHILE:
            ret += this->iter_type->emit_usage ();
            ret += " i = " + std::to_string(get_start_value ()) + ";\n";
            ret += "\twhile (";
            ret += " i " + get_condition_name () + " ";
            ret += std::to_string(get_end_value ()) + ") {\n\t";
            break;
        case LoopType::DO_WHILE:
            ret += this->iter_type->emit_usage ();
            ret += " i = " + std::to_string(get_start_value ()) + ";\n";
            ret += "\tdo {\n\t";
            break;
        default:
            ret += "ERROR IN LOOP TYPE";
            break;
    }
/*
    ret += this->iter_type->emit_usage ();
    ret += " i = " + std::to_string(get_start_value ()) + ";";
    ret += " i " + get_condition_name () + " ";
    ret += std::to_string(get_end_value ()) + ";";
    ret += " i += " + std::to_string(get_step ()) + ") { \n";
*/

    for (auto i = this->stmnt.begin(); i != this->stmnt.end(); ++i)
        ret += "\t" + i->emit_usage () + ";\n\t";

    if (get_loop_type () == LoopType::DO_WHILE ||
        get_loop_type () == LoopType::WHILE) {
        ret += "\ti += " + std::to_string(get_step ()) + ";\n\t";
    }

    ret += "}\n";

    if (get_loop_type () == LoopType::DO_WHILE) {
        ret += "\twhile (";
        ret += " i " + get_condition_name () + " ";
        ret += std::to_string(get_end_value ()) + ");\n";
    }

    ret += "}\n";

    unsigned int min_size = UINT_MAX;
    for (auto i = this->in.begin(); i != this->in.end(); ++i)
        min_size = std::min(min_size, i->get_size());
    for (auto i = this->out.begin(); i != this->out.end(); ++i)
        min_size = std::min(min_size, i->get_size());

    ret += "uint64_t checksum () {\n\t";
    ret += "foo ();\n\t";
    ret += "uint64_t ret = 0;\n\t";
    ret += "for (uint64_t i = 0; i < " + std::to_string(min_size) + "; ++i) {\n\t";
    for (auto i = this->out.begin(); i != this->out.end(); ++i)
        ret += "\tret ^= " + i->emit_usage() + ";\n\t";
    ret += "}\n\t";
    ret += "return ret;\n";
    ret += "}\n";
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
    for (auto i = this->in.begin(); i != this->in.end(); ++i) {
        std::cout << "=======================================" << std::endl;
        i->dbg_dump();
    }
    for (auto i = this->out.begin(); i != this->out.end(); ++i) {
        std::cout << "=======================================" << std::endl;
        i->dbg_dump();
    }
    for (auto i = this->stmnt.begin(); i != this->stmnt.end(); ++i) {
        std::cout << "=======================================" << std::endl;
        i->dbg_dump();
    }
}
