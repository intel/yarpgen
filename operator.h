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

#include <iostream>
#include "type.h"
#include "array.h"
#include "loop.h"

class Operator {
    public:
        explicit Operator ();
        explicit Operator (unsigned int _id);
        Operator (const Operator& _op);
        Operator& operator=(const Operator& _op);
        ~Operator ();
        unsigned int get_id ();
        std::string get_name ();
        unsigned int get_num_of_op ();
        bool can_cause_ub ();
        void set_type (unsigned int side, Type* _type);
        Type* get_type (unsigned int side);
        unsigned int get_type_id (unsigned int side);
        std::string get_type_name (unsigned int side);
        bool get_is_fp (unsigned int side);
        bool get_is_signed (unsigned int side);
        void set_max_value (unsigned int side, int64_t _max_val);
        int64_t get_max_value (unsigned int side);
        void set_min_value (unsigned int side, int64_t _min_val);
        int64_t get_min_value (unsigned int side);
        static Operator get_rand_obj ();
        void dbg_dump ();


    public:
        enum OperType {
            UN_INC, // +
            UN_DEC, // -
            LOG_NOT, // !
            MUL, // *
            DIV, // /
            MOD, // %
            ADD, // +
            SUB, // -
            BIT_SHL, // <<
            BIT_SHR, // >>
            MAX_OPER_TYPE 
        };
        enum Side {
            SELF, LEFT, RGHT, MAX_SIDE
        };


    private:
        unsigned int id;
        std::string name;
        unsigned int num_of_op;
        Type* type [MAX_SIDE];
        bool cause_ub;
};
