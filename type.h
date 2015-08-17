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
#include <string>
#include <cstdint>
#include <climits>
#include <random>
#include <typeinfo>

int rand_dev ();
extern std::mt19937_64 rand_gen;

class Type {
    public:
        explicit Type () {};
        explicit Type (unsigned int _type_id);
        unsigned int get_id ();
        std::string get_name ();
        bool get_is_fp ();
        bool get_is_signed ();
        void set_max_value (int64_t _max_val);
        int64_t get_max_value ();
        void set_min_value (int64_t _min_val);
        int64_t get_min_value ();
        static Type get_rand_obj ();
        std::string get_rand_value ();
        void dbg_dump();

    public:
        // TODO: change initialization mechanism in ir.cpp for type_table
        // Synchronize with other places
        enum TypeID {
            UCHAR,
            USHRT,
            UINT,
            ULINT,
            ULLINT,
            MAX_TYPE_ID
        };

    private:
        unsigned int id;
        std::string name;
        int64_t max_val;
        int64_t min_val;
        bool is_fp;
        bool is_signed;
};
