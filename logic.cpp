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

#include "logic.h"

Statement::Statement () {
    this->out_arrays = NULL;
    this->num_of_out = 0;
    this->depth = 0;
    this->tree.put_value(TreeElem(true, NULL, 0));
}

Statement::Statement (unsigned int _num_of_out, std::vector<Array>* _inp_arrays, std::vector<Array>* _out_arrays) {
    this->out_arrays = _out_arrays;
    this->inp_arrays = _inp_arrays;
    this->num_of_out = _num_of_out;
    this->depth = 0;
    this->tree.put_value(TreeElem(true, NULL, Operator::OperType::MAX_OPER_TYPE));
}

unsigned int Statement::get_num_of_out () { return this->num_of_out; }

void Statement::set_depth (unsigned int _depth) { this->depth = _depth; }

unsigned int Statement::get_depth () { return this->depth; }

void Statement::set_init_oper_type (unsigned int _oper_type_id) { this->tree.put_value(TreeElem(true, NULL, _oper_type_id)); }

unsigned int Statement::get_init_oper_type () { return tree.get_value<TreeElem>().get_oper_id(); }

void Statement::random_fill () {
    if (tree.get_value<TreeElem>().get_oper_id () == Operator::OperType::MAX_OPER_TYPE)
        this->tree.put_value(TreeElem::get_rand_obj_op ());
    fill_level(this->tree, 1);
}

ArithTree Statement::fill_level (ArithTree &apt, unsigned int level) {
    for (int i = apt.get_value<TreeElem>().get_num_of_op(); i > 0; --i) {
        std::uniform_int_distribution<unsigned int> dis(0, 1);
        unsigned int variate = (level == this->get_depth()) ? true : dis(rand_gen);
        if (variate) { // Array
            std::uniform_int_distribution<unsigned int> dis(0, this->inp_arrays->size() - 1);
            // 2 - i because I want to reduce if statements in emit phase
            apt.put(std::to_string(2 - i), TreeElem(false, &this->inp_arrays->at(dis(rand_gen)), Operator::OperType::MAX_OPER_TYPE));
        }
        else { // Operator
            apt.put(std::to_string(2 - i), TreeElem::get_rand_obj_op ());
        }
        if (level < this->get_depth())
            fill_level(apt.get_child(std::to_string(2 - i)), level + 1);
    }
    return apt;
}

std::string Statement::emit_usage () {
    std::string ret = emit_level (this->tree, 0);
    return ret;
}

std::string Statement::emit_level (ArithTree &apt, unsigned int level) {
    std::string ret = "(";
    if (apt.get_value<TreeElem>().get_num_of_op() == 2) {
        if (level < this->get_depth()&& apt.get_value<TreeElem>().get_is_op()) {
            ret += emit_level(apt.get_child("0"), level + 1);
        }
    }
    ret += apt.get_value<TreeElem>().emit_usage();
    if (level < this->get_depth() && apt.get_value<TreeElem>().get_is_op()) {
        ret += emit_level(apt.get_child("1"), level + 1);
    }
    ret += ")";
    return ret;
}

void Statement::dbg_dump () {
//    if (this->out_arrays != NULL) this->out_arrays->at(this->num_of_out).dbg_dump();
//    std::cout << "num of out array " << this->num_of_out << std::endl;

//    std::cout << this->tree.get_value<TreeElem>().emit_usage() << std::endl;

//    for (auto iter: this->tree) {
//        std::cout << iter.first << std::endl;
//        std::cout << iter.second.data().emit_usage() << std::endl;
//    }
    std::cout << emit_usage () << std::endl;
       

//    tree.get_value<TreeElem>().dbg_dump();
}
