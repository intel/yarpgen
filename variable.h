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

//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "type.h"

namespace rl {

class Data {
    public:
        enum VarClassID {
            VAR, ARR, STRUCT, MAX_CLASS_ID
        };

        union TypeVal {
            bool bool_val;
            signed char char_val;
            unsigned char uchar_val;
            short shrt_val;
            unsigned short ushrt_val;
            int int_val;
            unsigned int uint_val;
            long int lint_val;
            unsigned long int ulint_val;
            long long int llint_val;
            unsigned long long int ullint_val;
        };

        //TODO: Data can be not only integer, but who cares
        explicit Data (std::string _name);
        void set_modifier (Type::Mod _modifier) { type->set_modifier(_modifier); }
        Type::Mod get_modifier () { return type->get_modifier(); }
        bool get_is_static () { return type->get_is_static(); }
        void set_align (uint64_t _align) { type->set_align(_align); }
        uint64_t get_align () { return type->get_align(); }
        VarClassID get_class_id () { return class_id; }
        std::string get_name () { return name; }
        void set_type(std::shared_ptr<Type> _type) { type = _type; }
        std::shared_ptr<Type> get_type () { return type; }
        virtual void set_value (uint64_t _val) = 0;
        virtual void set_max (uint64_t _max) = 0;
        virtual void set_min (uint64_t _min) = 0;
        virtual uint64_t get_value () = 0;
        virtual uint64_t get_max () = 0;
        virtual uint64_t get_min () = 0;
        virtual void dbg_dump () = 0;

    protected:
        std::shared_ptr<Type> type;
        std::string name;
        TypeVal value;
        TypeVal min;
        TypeVal max;
        VarClassID class_id;
};

class Struct : public Data {
    public:
        explicit Struct (std::string _name, std::shared_ptr<StructType> _type);
        uint64_t get_num_of_members () { return members.size(); }
        std::shared_ptr<Data> get_member (unsigned int num);
        void set_value (uint64_t _val) {}
        void set_max (uint64_t _max) {}
        void set_min (uint64_t _min) {}
        uint64_t get_value () { return 0; }
        uint64_t get_max () { return 0; }
        uint64_t get_min () { return 0; }
        void dbg_dump ();

    private:
        void allocate_members();
        std::vector<std::shared_ptr<Data>> members;
};

class Variable : public Data {
    public:
        explicit Variable (std::string _name, IntegerType::IntegerTypeID _type_id, Type::Mod _modifier, bool _is_static);
        void set_value (uint64_t _val);
        void set_max (uint64_t _max);
        void set_min (uint64_t _min);
        uint64_t get_value ();
        uint64_t get_max ();
        uint64_t get_min ();
        void dbg_dump ();
};

class Array : public Data {
    public:
        enum Ess {
            C_ARR,
            STD_ARR,
            STD_VEC,
            VAL_ARR,
            MAX_ESS
        };

        explicit Array (std::string _name, IntegerType::IntegerTypeID _base_type_id,  Type::Mod _modifier, bool _is_static,
                        uint64_t _size, Ess _essence);
        std::shared_ptr<Type> get_base_type () { return base_type; }
        uint64_t get_size () { return size; }
        Ess get_essence () { return essence; }
        void set_value (uint64_t _val);
        void set_max (uint64_t _max);
        void set_min (uint64_t _min);
        uint64_t get_value ();
        uint64_t get_max ();
        uint64_t get_min ();
        void dbg_dump ();

    private:
        std::shared_ptr<Type> base_type;
        uint64_t size;
        Ess essence;
};
}
