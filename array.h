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

#ifndef _ARRAY_H
#define _ARRAY_H

#include "type.h"

extern unsigned int MAX_ARRAY_SIZE;

class Array {
    public:
        explicit Array () {};
        explicit Array (std::string _name, unsigned int _type_id, unsigned int _size);
        std::string get_name ();
        void set_size (unsigned int _size);
        unsigned int get_size ();
        unsigned int get_type_id ();
        std::string get_type_name ();
        void set_max_value (int64_t _max_val);
        int64_t get_max_value ();
        void set_min_value (int64_t _min_val);
        int64_t get_min_value ();
        bool get_is_fp ();
        bool get_is_signed ();
        static Array get_rand_obj ();
        void dbg_dump ();

    private:
        std::string name;
        unsigned int size;
        Type type;
};

#endif
