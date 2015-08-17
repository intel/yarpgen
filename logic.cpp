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
    this->tree.put_value(TreeElem(true, NULL, 0));
}

unsigned int Statement::get_num_of_out () { return this->num_of_out; }

void Statement::set_depth (unsigned int _depth) { this->depth = _depth; }

unsigned int Statement::get_depth () { return this->depth; }

void Statement::set_init_oper_type (unsigned int _oper_type_id) { this->tree.put_value(TreeElem(true, NULL, _oper_type_id)); }

unsigned int Statement::get_init_oper_type () { return tree.get_value<TreeElem>().get_oper_id(); }

void Statement::random_fill () {
/*    std::uniform_int_distribution<unsigned int> dis(0, Operator::OperType::MAX_OPER_TYPE);
    this->tree.put_value(TreeElem(true, NULL, dis(rand_gen)));
    for (int i = 0; i < tree.get_value<TreeElem>().get_num_of_op(); i++) {
        std::uniform_int_distribution<unsigned int> dis(0, 1);
        if (dis(rand_gen)) { // Array usage
            std::uniform_int_distribution<unsigned int> dis(0, this->inp_arrays->size());
            Array* chld_arr
        }
        std::uniform_int_distribution<unsigned int> dis(0, this->inp_arrays->size());
    }
*/
}

void Statement::dbg_dump () {
    if (this->out_arrays != NULL) this->out_arrays->at(this->num_of_out).dbg_dump();
    std::cout << "num of out array " << this->num_of_out << std::endl;
    
    tree.get_value<TreeElem>().dbg_dump();
}
