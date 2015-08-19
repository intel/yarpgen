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

#include "type.h"
#include "array.h"
#include "loop.h"
#include "operator.h"

class TreeElem {
    public:
        TreeElem () {}
        TreeElem (bool _is_op, std::shared_ptr<Array> _array, unsigned int _oper_type_id);
        bool get_is_op ();
        std::string get_arr_name ();
        unsigned int get_arr_size ();
        std::string get_arr_type_name ();
        void set_arr_max_value (int64_t _max_val);
        int64_t get_arr_max_value ();
        void set_arr_min_value (int64_t _min_val);
        int64_t get_arr_min_value ();
        bool get_arr_is_fp ();
        bool get_arr_is_signed ();
        std::shared_ptr<Array> get_array ();
        unsigned int get_oper_id ();
        std::string get_oper_name ();
        unsigned int get_num_of_op ();
        bool can_oper_cause_ub ();
        void set_oper_type (unsigned int side, std::shared_ptr<Type> _type);
        void set_oper_self_type (unsigned int _type_id);
        std::shared_ptr<Type> get_oper_type (unsigned int side);
        unsigned int get_oper_type_id (unsigned int side);
        std::string get_oper_type_name (unsigned int side);
        bool get_oper_type_is_fp (unsigned int side);
        bool get_oper_type_is_signed (unsigned int side);
        void set_oper_max_value (unsigned int side, int64_t _max_val);
        int64_t get_oper_max_value (unsigned int side);
        void set_oper_min_value (unsigned int side, int64_t _min_val);
        int64_t get_oper_min_value (unsigned int side);
        Operator get_oper ();
        std::string get_type_name ();
        static TreeElem get_rand_obj_op (unsigned int _type_id = 0);
        std::string emit_usage ();
        void dbg_dump ();

    private:
        bool is_op;
        std::shared_ptr<Array> array;
        Operator oper;
};
