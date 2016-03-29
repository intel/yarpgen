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

#include <iostream>
#include <climits>
#include <memory>
#include <vector>

namespace rl {

class Variable;

class Type {
    public:
        enum TypeID {
            ATOMIC_TYPE,
            STRUCT_TYPE,
            PTR_TYPE,
            MAX_TYPE_ID
        };

        enum Mod {
            NTHG,
            VOLAT,
            CONST,
            CONST_VOLAT,
            MAX_MOD
        };

        explicit Type () {};
        Type (TypeID _id) : name (""), suffix (""), modifier(Mod::NTHG), is_static(false), align(0), id (_id) {}
        Type::TypeID get_type_id () { return id; }
        std::string get_name ();
        std::string get_suffix() { return suffix; }
        void set_modifier (Mod _modifier) { modifier = _modifier; }
        Mod get_modifier () { return modifier; }
        void set_is_static (bool _is_static) { is_static = _is_static; }
        bool get_is_static () { return is_static; }
        void set_align (uint64_t _align) { align = _align; }
        uint64_t get_align () { return align; }
        virtual bool is_atomic_type() { return false; }
        virtual bool is_ptr_type() { return false; }
        virtual bool is_int_type() { return false; }
        virtual bool is_fp_type() { return false; }
        virtual bool is_struct_type() { return false; }
        virtual void dbg_dump() = 0;

    protected:
        std::string name;
        std::string suffix;
        Mod modifier;
        bool is_static;
        uint64_t align;

    private:
        TypeID id;
};

class PtrType : public Type {
    public:
        PtrType () : Type (Type::PTR_TYPE), pointee_t (NULL) {}
        static std::shared_ptr<Type> init (std::shared_ptr<Type> _pointee_t);
        bool is_ptr_type() { return true; }
        void dbg_dump() {};

    private:
        std::shared_ptr<Type> pointee_t;
        //TODO: it is a stub
};

class StructType : public Type {
    public:
        struct StructMember {
            public:
                StructMember (std::shared_ptr<Type> _type, std::string _name) : type(_type), name(_name) {}
                std::string get_name () { return name; }
                std::shared_ptr<Type> get_type() { return type; }
                std::string get_definition (std::string offset = "") { return offset + type->get_name() + " " + name; }

            private:
                std::shared_ptr<Type> type;
                std::string name;
        };

        StructType () : Type (Type::STRUCT_TYPE), nest_depth(0) {}
        static std::shared_ptr<Type> init (std::string _name);
        void add_member (std::shared_ptr<StructMember> new_mem) { members.push_back(new_mem); }
        void add_member (std::shared_ptr<Type> _type, std::string _name);
        uint64_t get_num_of_members () { return members.size(); }
        uint64_t get_nest_depth () { return nest_depth; }
        std::shared_ptr<StructMember> get_member (unsigned int num);
        std::string get_definition (std::string offset = "");
        bool is_struct_type() { return true; }
        void dbg_dump();

    private:
        std::vector<std::shared_ptr<StructMember>> members;
        uint64_t nest_depth;
};

class AtomicType : public Type {
    public:
        enum AtomicTypeID {
            Integer, FP, Max_AtomicTypeID
        };
        AtomicType (AtomicTypeID at_id) : Type (Type::ATOMIC_TYPE),  bit_size (0), atomic_id (at_id) {};
        AtomicTypeID get_atomic_type_id () { return atomic_id; }
        uint64_t get_bit_size () { return bit_size; }
        bool is_atomic_type() { return true; }

    protected:
        unsigned int bit_size;

    private:
        AtomicTypeID atomic_id;
};

class IntegerType : public AtomicType {
    public:
        enum IntegerTypeID {
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
        };
        IntegerType (IntegerTypeID it_id) : AtomicType (AtomicTypeID::Integer), is_signed (false), min (0), max (0), int_type_id (it_id) {}
        static std::shared_ptr<Type> init (IntegerType::IntegerTypeID _type_id);
        IntegerTypeID get_int_type_id () { return int_type_id; }
        static bool can_repr_value (IntegerType::IntegerTypeID A, IntegerType::IntegerTypeID B); // if type a can represent all of the values of the type b
        static IntegerType::IntegerTypeID get_corr_unsig (IntegerType::IntegerTypeID _type_id);
        bool get_is_signed () { return is_signed; }
        uint64_t get_min () { return min; }
        uint64_t get_max () { return max; }
        bool is_int_type() { return true; }

    protected:
        bool is_signed;
        uint64_t min;
        uint64_t max;

    private:
        IntegerTypeID int_type_id;
};

class TypeBOOL : public IntegerType {
    public:
        TypeBOOL () : IntegerType(IntegerType::IntegerTypeID::BOOL) {
            name = "bool";
            suffix = "";
            min = false;
            max = true;
            bit_size = sizeof (bool) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeCHAR : public IntegerType {
    public:
        TypeCHAR () : IntegerType(IntegerType::IntegerTypeID::CHAR) {
            name = "signed char";
            suffix = "";
            min = SCHAR_MIN;
            max = SCHAR_MAX;
            bit_size = sizeof (char) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeUCHAR : public IntegerType {
    public:
        TypeUCHAR () : IntegerType(IntegerType::IntegerTypeID::UCHAR) {
            name = "unsigned char";
            suffix = "";
            min = 0;
            max = UCHAR_MAX;
            bit_size = sizeof (unsigned char) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeSHRT : public IntegerType {
    public:
        TypeSHRT () : IntegerType(IntegerType::IntegerTypeID::SHRT) {
            name = "short";
            suffix = "";
            min = SHRT_MIN;
            max = SHRT_MAX;
            bit_size = sizeof (short) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeUSHRT : public IntegerType {
    public:
        TypeUSHRT () : IntegerType(IntegerType::IntegerTypeID::USHRT) {
            name = "unsigned short";
            suffix = "";
            min = 0;
            max = USHRT_MAX;
            bit_size = sizeof (unsigned short) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeINT : public IntegerType {
    public:
        TypeINT () : IntegerType(IntegerType::IntegerTypeID::INT) {
            name = "int";
            suffix = "";
            min = INT_MIN;
            max = INT_MAX;
            bit_size = sizeof (int) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeUINT : public IntegerType {
    public:
        TypeUINT () : IntegerType(IntegerType::IntegerTypeID::UINT) {
            name = "unsigned int";
            suffix = "U";
            min = 0;
            max = UINT_MAX;
            bit_size = sizeof (unsigned int) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeLINT : public IntegerType {
    public:
        TypeLINT () : IntegerType(IntegerType::IntegerTypeID::LINT) {
            name = "long int";
            suffix = "L";
            min = LONG_MIN;;
            max = LONG_MAX;
            bit_size = sizeof (long int) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeULINT : public IntegerType {
    public:
        TypeULINT () : IntegerType(IntegerType::IntegerTypeID::ULINT) {
            name = "unsigned long int";
            suffix = "UL";
            min = 0;
            max = ULONG_MAX;
            bit_size = sizeof (unsigned long int) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeLLINT : public IntegerType {
    public:
        TypeLLINT () : IntegerType(IntegerType::IntegerTypeID::LLINT) {
            name = "long long int";
            suffix = "LL";
            min = LLONG_MIN;
            max = LLONG_MAX;
            bit_size = sizeof (long long int) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeULLINT : public IntegerType {
    public:
        TypeULLINT () : IntegerType(IntegerType::IntegerTypeID::ULLINT) {
            name = "unsigned long long int";
            suffix = "ULL";
            min = 0;
            max = ULLONG_MAX;
            bit_size = sizeof (unsigned long long int) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};
}
