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

Statement::Statement (unsigned int _num_of_out, std::shared_ptr<std::vector<Array>> _inp_arrays, std::shared_ptr<std::vector<Array>> _out_arrays) {
    this->out_arrays = _out_arrays;
    this->inp_arrays = _inp_arrays;
    this->num_of_out = _num_of_out;
    this->depth = 0;
    this->tree.put_value(TreeElem(true, NULL, Operator::OperType::MAX_OPER_TYPE));
    push_out_arr_type ();
}

void Statement::push_out_arr_type () {
    TreeElem subtree = this->tree.get_value<TreeElem>();
    if (this->out_arrays != NULL)
        subtree.set_oper_type(Operator::Side::SELF, this->out_arrays->at(this->num_of_out).get_type ());
    this->tree.put_value(subtree);
}

ArithTree& Statement::determ_and_prop_on_level (ArithTree &apt) {
    TreeElem subtree = apt.get_value<TreeElem>();
    subtree.determine_range();
    apt.put_value(subtree);
    return apt;
}

unsigned int Statement::get_num_of_out () { return this->num_of_out; }

void Statement::set_depth (unsigned int _depth) { this->depth = _depth; }

unsigned int Statement::get_depth () { return this->depth; }

void Statement::set_init_oper_type (unsigned int _oper_type_id) { this->tree.put_value(TreeElem(true, NULL, _oper_type_id)); }

unsigned int Statement::get_init_oper_type () { return tree.get_value<TreeElem>().get_oper_id(); }

void Statement::random_fill () {
    if (tree.get_value<TreeElem>().get_oper_id () == Operator::OperType::MAX_OPER_TYPE)
        this->tree.put_value(TreeElem::get_rand_obj_op ());
    push_out_arr_type();
    fill_level(this->tree, 1);
}

bool Statement::need_arr (ArithTree &apt) {
//    return (apt.get_value<TreeElem>().get_oper_id() == Operator::OperType::BIT_SHL) ||
//           (apt.get_value<TreeElem>().get_oper_id() == Operator::OperType::BIT_SHR) ||
    return (apt.get_value<TreeElem>().get_oper_id() == Operator::OperType::MOD) ||
           (apt.get_value<TreeElem>().get_oper_id() == Operator::OperType::DIV);
}

ArithTree& Statement::fill_level (ArithTree &apt, unsigned int level) {
    determ_and_prop_on_level (apt);
    for (int i = apt.get_value<TreeElem>().get_num_of_op(); i > 0; --i) { // i = 2, 1 or 1
        std::uniform_int_distribution<unsigned int> dis(0, 1);
        bool variate = (need_arr(apt) && i == Operator::Side::RGHT) ? true : dis(rand_gen);
        variate |= (level == this->get_depth()); // Leaves are always arrays
        TreeElem insert_val;
        if (variate) { // Array
            std::uniform_int_distribution<unsigned int> dis(0, this->inp_arrays->size() - 1);
            unsigned int arr_num = dis(rand_gen);
            this->inp_arrays->at(arr_num).get_type()->combine_range(apt.get_value<TreeElem>().get_oper_type(2 - i));
            insert_val = TreeElem(false, std::make_shared<Array>(this->inp_arrays->at(arr_num)), Operator::OperType::MAX_OPER_TYPE);
            apt.put(std::to_string(2 - i), insert_val);
        }
        else { // Operator
            insert_val = TreeElem::get_rand_obj_op (apt.get_value<TreeElem>().get_oper_type(2 - i));
            apt.put(std::to_string(2 - i), insert_val);
            if (level < this->get_depth())
                fill_level(apt.get_child(std::to_string(2 - i)), level + 1);
        }
    }
    return apt;
}

std::string Statement::emit_usage () {
    std::string ret = (this->out_arrays != NULL) ? this->out_arrays->at(this->num_of_out).emit_usage() : "";
    ret += " = ";
    ret += emit_level (this->tree, 0, InfoType::USAGE);
    return ret;
}

std::string Statement::emit_type () {
    std::string ret = emit_level (this->tree, 0, InfoType::TYPE);
    return ret;
}

std::string Statement::emit_domain (bool is_max) {
    std::string ret = emit_level (this->tree, 0, is_max ? InfoType::DOMAIN_MAX : InfoType::DOMAIN_MIN);
    return ret;
}

std::string Statement::emit_level (ArithTree &apt, unsigned int level, unsigned int info_type) {
    std::string ret = "(";
    if (apt.get_value<TreeElem>().get_num_of_op() == 2) {
        if (apt.get_value<TreeElem>().get_is_op()) {
            ret += emit_level(apt.get_child("0"), level + 1, info_type);
        }
    }
    switch (info_type) {
        case InfoType::USAGE:
            ret += apt.get_value<TreeElem>().emit_usage();
            break;
        case InfoType::TYPE:
            ret += apt.get_value<TreeElem>().get_type_name();
            break;
        case InfoType::DOMAIN_MAX:
            ret += std::to_string(apt.get_value<TreeElem>().get_max_value());
            break;
        case InfoType::DOMAIN_MIN:
            ret += std::to_string(apt.get_value<TreeElem>().get_min_value());
            break;
    };
    if (apt.get_value<TreeElem>().get_is_op()) {
        ret += emit_level(apt.get_child("1"), level + 1, info_type);
    }
    ret += ")";
    return ret;
}

void Statement::dbg_dump () {
    if (this->out_arrays != NULL) this->out_arrays->at(this->num_of_out).dbg_dump();
    std::cout << "num of out array " << this->num_of_out << std::endl;
    std::cout << emit_usage () << std::endl;
    std::cout << emit_type () << std::endl;
    std::cout << emit_domain (true) << std::endl;
    std::cout << emit_domain (false) << std::endl;
}
