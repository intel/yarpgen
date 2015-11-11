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
#include <climits>
#include <memory>

class Type {
    public:
        enum TypeID {
            BOOL,
            CHAR,
            UCHAR,
            SHRT,
            USHRT,
            INT,
            UINT,
            LINT,
            ULINT,
            LLINT,
            ULLINT,
            MAX_INT_ID,
            PTR,
            MAX_TYPE_ID
        };

        explicit Type () {};
        static std::shared_ptr<Type> init (Type::TypeID _type_id);
        static bool can_repr_value (Type::TypeID a, Type::TypeID b); // if type a can represent all of the values of the type b
        static Type::TypeID get_corr_unsig (Type::TypeID _type_id);
        Type::TypeID get_id () { return id; }
        std::string get_name () { return name; }
        std::string get_suffix() { return suffix; }
        bool get_is_signed () { return is_signed; }
        uint64_t get_bit_size () { return bit_size; }
        uint64_t get_min () { return min; }
        uint64_t get_max () { return max; }
        void dbg_dump();
        virtual std::string get_max_str () = 0;
        virtual std::string get_min_str () = 0;

    protected:
        TypeID id;
        std::string name;
        std::string suffix;
        uint64_t min;
        uint64_t max;
        unsigned int bit_size;
        bool is_fp;
        bool is_signed;
};

class TypeBOOL : public Type {
    public:
        TypeBOOL ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeCHAR : public Type {
    public:
        TypeCHAR ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeUCHAR : public Type {
    public:
        TypeUCHAR ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeSHRT : public Type {
    public:
        TypeSHRT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeUSHRT : public Type {
    public:
        TypeUSHRT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeINT : public Type {
    public:
        TypeINT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeUINT : public Type {
    public:
        TypeUINT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeLINT : public Type {
    public:
        TypeLINT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeULINT : public Type {
    public:
        TypeULINT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeLLINT : public Type {
    public:
        TypeLLINT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeULLINT : public Type {
    public:
        TypeULLINT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypePTR : public Type {
    public:
        TypePTR ();
        std::string get_max_str ();
        std::string get_min_str ();
};
