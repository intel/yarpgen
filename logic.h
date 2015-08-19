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

#pragma once

#include <boost/property_tree/ptree.hpp>
#include "type.h"
#include "array.h"
#include "operator.h"
#include "tree_elem.h"

typedef boost::property_tree::basic_ptree <std::string, TreeElem> ArithTree;

class Statement {
    public:
        Statement ();
        Statement (unsigned int _num_of_out, std::shared_ptr<std::vector<Array>> _inp_arrays, std::shared_ptr<std::vector<Array>> _out_arrays);
        void set_init_oper_type (unsigned int _oper_type_id);
        unsigned int get_init_oper_type ();
        unsigned int get_num_of_out ();
        void set_depth (unsigned int _depth);
        unsigned int get_depth ();
        void random_fill ();
        std::string emit_usage ();
        void dbg_dump ();

    private:
        ArithTree& fill_level (ArithTree &apt, unsigned int level);
        std::string emit_level (ArithTree &apt, unsigned int level, unsigned int info_type);
        std::string emit_type ();
        void push_out_arr_type ();

    private:
        unsigned int num_of_out;
        unsigned int depth;
        std::shared_ptr<std::vector<Array>> out_arrays;
        std::shared_ptr<std::vector<Array>> inp_arrays;
        ArithTree tree;
        enum InfoType {
            USAGE, TYPE, MAX_INFO_TYPE
        };
};
