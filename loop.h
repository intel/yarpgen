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

#ifndef _LOOP_H
#define _LOOP_H

#include <iostream>
#include <map>
#include "type.h"
#include "array.h"

class Loop {
    public:
        Loop (); 
        void set_loop_type (unsigned int _loop_type);
        unsigned int get_loop_type ();
        void set_iter_type (unsigned int _iter_type_id);
        unsigned int get_iter_type_id ();
        std::string get_iter_type_name ();
        void set_start_val (int64_t _start_val);
        int64_t get_start_val ();
        void set_end_val (int64_t _end_val);
        int64_t get_end_val ();
        void set_step (int64_t _step);
        int64_t get_step ();
        void set_condition (unsigned int _condition);
        unsigned int get_condition ();
        Loop rand_init ();
        void dbg_dump ();

    public:
        enum CondType {
            EQ, NE, GT, GE, LT, LE, MAX_COND
        };
        enum LoopType {
            FOR, WHILE, DO_WHILE, MAX_LOOP_TYPE
        };

    private:
        unsigned int loop_type;
        Type iter_type;
        int64_t start_val;
        int64_t end_val;
        int64_t step;
        unsigned int condition; 
};

#endif
