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

unsigned int MAX_ARRAY_NUM = 10;
unsigned int MIN_ARRAY_NUM = 1;

unsigned int MAX_STMNT_NUM = 3;
unsigned int MIN_STMNT_NUM = 1;

unsigned int MAX_DEPTH = 50;
unsigned int MIN_DEPTH = 4;

Loop::Loop () {
    this->loop_type = LoopType::FOR;
    this->iter_type = Type::init (Type::TypeID::UINT);
    set_step (1);
    this->condition = CondType::LT;
}

void Loop::set_loop_type (unsigned int _loop_type) { this->loop_type = _loop_type; }

unsigned int Loop::get_loop_type () const { return this->loop_type; }

void Loop::set_iter_type (std::shared_ptr<Type> _type) { this->iter_type = _type; }

std::shared_ptr<Type> Loop::get_iter_type () { return this->iter_type; }

unsigned int Loop::get_iter_type_id () const {return this->iter_type->get_id(); }

std::string Loop::get_iter_type_name () const { return this->iter_type->get_name (); }

void Loop::set_start_value (uint64_t _start_val) { this->iter_type->set_min_value(_start_val); }

uint64_t Loop::get_start_value () const { return this->iter_type->get_min_value(); }

std::string Loop::get_start_value_str () const { return this->iter_type->get_min_value_str(); }

void Loop::set_end_value (uint64_t _end_val) { this->iter_type->set_max_value(_end_val); }

uint64_t Loop::get_end_value () const { return this->iter_type->get_max_value(); }

std::string Loop::get_end_value_str () const { return this->iter_type->get_max_value_str(); }

void Loop::set_step (uint64_t _step) { this->iter_type->set_value(_step); }

uint64_t Loop::get_step () const { return this->iter_type->get_value(); }

std::string Loop::get_step_str () const { return this->iter_type->get_value_str (); }

void Loop::set_condition (unsigned int _condition) { this->condition = _condition; }

unsigned int Loop::get_condition () const { return this->condition; }

unsigned int Loop::get_min_size () { return this->min_size; }

std::string Loop::get_out_num_str () { return this->out_num_str; }

void Loop::init_array () {
    std::uniform_int_distribution<unsigned int> dis(MIN_ARRAY_NUM, MAX_ARRAY_NUM);
    int arr_num = dis(rand_gen);

    std::uniform_int_distribution<unsigned int> var_ess_dis(0, 1);
    unsigned int var_ess = var_ess_dis (rand_gen); // Can essence of array be combined
    std::uniform_int_distribution<unsigned int> ess_dis(0, Array::Ess::MAX_ESS - 1);
    unsigned int ess_type = ess_dis (rand_gen);

    min_size = UINT_MAX;
    for (int i = 0; i < arr_num; ++i) {
        if (var_ess == true)
            ess_type = ess_dis (rand_gen);
        this->in.push_back (Array::get_rand_obj ("in_" + out_num_str + "_" + std::to_string(i), "i_" + out_num_str, ess_type));
        min_size = std::min (min_size, this->in.at(i).get_size());
        if (var_ess == true)
            ess_type = ess_dis (rand_gen);
        this->out.push_back (Array::get_rand_obj ("out_" + out_num_str + "_" + std::to_string(i), "i_" + out_num_str, ess_type));

        //TODO: move to get_rand_obj
        this->out.at(i).set_modifier(Array::Mod::NTHNG);

        min_size = std::min (min_size , this->out.at(i).get_size());
    }
}

void Loop::init_stmnt () {
    std::uniform_int_distribution<unsigned int> depth_dis(MIN_DEPTH, MAX_DEPTH);
    for (int i = 0; i < this->out.size(); ++i) {
        this->stmnt.push_back(Statement (i, std::make_shared<std::vector<Array>>(in), std::make_shared<std::vector<Array>>(out)));
        this->stmnt.at(i).set_depth(depth_dis(rand_gen));
        this->stmnt.at(i).random_fill ();
    }

}

uint64_t Loop::step_distr (uint64_t min, uint64_t max) {
    uint64_t ret = 0;
    bool init = true;
    unsigned int perc = 0;
    std::uniform_int_distribution<unsigned int> step_dis(0,100 );
    do {
        init = true;
        perc = step_dis (rand_gen);
        if (perc < 25)
            ret = 1;
        else if (perc < 35)
            ret = 2;
        else if (perc < 45)
            ret = 3;
        else if (perc < 55)
            ret = 4;
        else if (perc < 65)
            ret = 8;
        else
            ret = iter_type->get_rand_value(min, max);

        if (min == max)
            ret = min;
        else if (ret < min || ret > max)
            init = false;
    }
    while (!init);
    return ret;
}

bool Loop::can_reuse (Loop loop) {
    set_loop_type (loop.get_loop_type ());
    set_iter_type (Type::get_copy(loop.get_iter_type ()));
    set_condition (loop.get_condition ());
    set_step (loop.get_step ());

    bool ret = true;
    uint64_t max_val = get_end_value() < min_size ? get_end_value() : min_size;
    if (get_start_value () > max_val - 1)
        ret = false;
    switch (get_condition()) {
        case EQ:
            break;
        case NE:
            if (!this->iter_type->get_is_signed()) {
                if (get_end_value() < get_start_value() || get_end_value() > (max_val - 1))
                    ret = false;
            }
            break;
        case GT:
            if (!this->iter_type->get_is_signed())
                    ret = false;
                    break;
            break;
        case GE:
            if (!this->iter_type->get_is_signed())
                    ret = false;
                    break;
            break;
        case LT:
            if (get_end_value() > max_val)
                ret = false;
            break;
        case LE:
            if (get_end_value() < get_start_value() || get_end_value() >  (max_val - 1))
                ret = false;
            break;
        case MAX_COND:
                ret = false;
            break;
    }
    return ret;
}

void Loop::rand_fill_bound () {
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
                _step = step_distr(iter_type->get_abs_min(), iter_type->get_abs_max());
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
                _step = step_distr(1, end_val - get_start_value());
                break;
            case LE:
                end_val = this->iter_type->get_rand_value(get_start_value(), (max_val - 1));
                if (end_val == get_start_value())
                    _step = step_distr(1, iter_type->get_abs_max());
                else
                    _step = step_distr(1, end_val - get_start_value());
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
}

void Loop::random_fill (unsigned int outside_num, bool need_reuse, Loop loop) {
    this->out_num_str = std::to_string(outside_num);

    init_array();

    if (!need_reuse || !can_reuse (loop))
        rand_fill_bound ();

    init_stmnt();
}

std::string Loop::emit_array_def () {
    std::string ret = "";
    for (auto i = this->in.begin(); i != this->in.end(); ++i)
        ret += i->emit_definition (true);
    for (auto i = this->out.begin(); i != this->out.end(); ++i)
        ret += i->emit_definition (false);
    return ret;
}

std::string Loop::emit_array_decl (std::string start, std::string end, bool is_extern) {
    std::string ret = "";
    for (auto i = this->in.begin(); i != this->in.end(); ++i)
        ret += start + i->emit_declaration (is_extern) + end + ";\n";
    for (auto i = this->out.begin(); i != this->out.end(); ++i)
        ret += start + i->emit_declaration (is_extern) + end + ";\n";
    return ret;
}

std::string Loop::emit_array_usage (std::string start, std::string end, bool is_out) {
    std::string ret = "";
    for (auto i = (is_out ? out.begin(): in.begin()); i != (is_out ? out.end() : in.end()); ++i)
        ret += start + i->emit_usage() + end + ";\n\t";
    return ret;
}

std::string Loop::emit_body () {
    std::string ret = "";

    switch (get_loop_type ()) {
        case LoopType::FOR:
            ret += "for (";
            ret += this->iter_type->emit_usage ();
            ret += " i_" + out_num_str + " = " + get_start_value_str () + ";";
            ret += " i_" + out_num_str + " " +  get_condition_name () + " ";
            ret += get_end_value_str () + ";";
            ret += " i_" + out_num_str + " += " + get_step_str () + ") { \n\t";
            break;
        case LoopType::WHILE:
            ret += this->iter_type->emit_usage ();
            ret += " i_" + out_num_str + " = " + get_start_value_str () + ";\n";
            ret += "\twhile (";
            ret += " i_" + out_num_str + " " + get_condition_name () + " ";
            ret += get_end_value_str () + ") {\n\t";
            break;
        case LoopType::DO_WHILE:
            ret += this->iter_type->emit_usage ();
            ret += " i_" + out_num_str + " = " + get_start_value_str () + ";\n";
            ret += "\tdo {\n\t";
            break;
        default:
            ret += "ERROR IN LOOP TYPE";
            break;
    }

    for (auto i = this->stmnt.begin(); i != this->stmnt.end(); ++i)
        ret += "\t" + i->emit_usage () + ";\n\t";

    if (get_loop_type () == LoopType::DO_WHILE ||
        get_loop_type () == LoopType::WHILE) {
        ret += "\ti_" + out_num_str + " += " + get_step_str () + ";\n\t";
    }

    ret += "}\n";

    if (get_loop_type () == LoopType::DO_WHILE) {
        ret += "\twhile (";
        ret += " i_" + out_num_str + " " + get_condition_name () + " ";
        ret += get_end_value_str () + ");\n";
    }
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
    std::cout << "step " << get_step() << std::endl;

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
