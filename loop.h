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
#include <map>
#include "type.h"
#include "array.h"
#include "logic.h"

extern unsigned int MAX_ARRAY_NUM;
extern unsigned int MIN_ARRAY_NUM;
extern unsigned int MAX_STMNT_NUM;
extern unsigned int MIN_STMNT_NUM;
extern unsigned int MAX_DEPTH;
extern unsigned int MIN_DEPTH;

class Loop {
    public:
        Loop ();
        void set_loop_type (unsigned int _loop_type);
        unsigned int get_loop_type () const;
        void set_iter_type (std::shared_ptr<Type> _type);
        std::shared_ptr<Type> get_iter_type ();
        unsigned int get_iter_type_id () const;
        std::string get_iter_type_name () const;
        void set_start_value (uint64_t _start_val);
        uint64_t get_start_value () const;
        std::string get_start_value_str () const;
        void set_end_value (uint64_t _end_val);
        uint64_t get_end_value () const;
        std::string get_end_value_str () const;
        void set_step (uint64_t _step);
        uint64_t get_step () const;
        std::string get_step_str () const;
        void set_condition (unsigned int _condition);
        unsigned int get_condition () const;
        std::string get_out_num_str ();
        unsigned int get_min_size ();
        std::string emit_body ();
        std::string emit_array_def ();
        std::string emit_array_usage (std::string start, bool is_out);
        std::string emit_array_decl (std::string start, std::string end);
        void random_fill (unsigned int outside_num, bool need_reuse, Loop loop);
        void dbg_dump ();

    private:
        std::string get_condition_name ();
        void init_array ();
        void init_stmnt ();
        uint64_t step_distr (uint64_t min, uint64_t max);
        bool can_reuse (Loop loop);
        void rand_fill_bound ();

    public:
        enum CondType {
            EQ, //=
            NE, // !=
            GT, // >
            GE, // >=
            LT, // <
            LE, // <=
            MAX_COND
        };
        enum LoopType {
            FOR, WHILE, DO_WHILE, MAX_LOOP_TYPE
        };

    private:
        std::vector<Array> in;
        std::vector<Array> out;
        std::vector<Statement> stmnt;
        unsigned int loop_type;
        std::shared_ptr<Type> iter_type;
        uint64_t step;
        unsigned int condition;
        std::string out_num_str;
        unsigned int min_size;
};
