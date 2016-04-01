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

class Type {
    public:
        enum TypeID {
            ATOMIC_TYPE,
            STRUCT_TYPE,
            MAX_TYPE_ID
        };

        enum Mod {
            NTHG,
            VOLAT,
            CONST,
            CONST_VOLAT,
            MAX_MOD
        };

        Type (TypeID _id) : modifier(Mod::NTHG), is_static(false), align(0), id (_id) {}
        Type (TypeID _id, Mod _modifier, bool _is_static, uint64_t _align) :
              modifier (_modifier), is_static (_is_static), align (_align), id (_id) {}
        Type::TypeID get_type_id () { return id; }
        std::string get_name ();
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
        Mod modifier;
        bool is_static;
        uint64_t align;

    private:
        TypeID id;
};

class StructType : public Type {
    public:
        //TODO: add generator?
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

        StructType (std::string _name) : Type (Type::STRUCT_TYPE), nest_depth(0) { name = "struct " + _name; }
        StructType (std::string _name, Mod _modifier, bool _is_static, uint64_t _align) :
                    Type (Type::STRUCT_TYPE, _modifier, _is_static, _align), nest_depth(0) { name = "struct " + _name; }
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

        union TypedVal {
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

        AtomicType (AtomicTypeID at_id) : Type (Type::ATOMIC_TYPE), bit_size (0), suffix(""), atomic_id (at_id) {}
        AtomicType (AtomicTypeID at_id, Mod _modifier, bool _is_static, uint64_t _align) :
                    Type (Type::ATOMIC_TYPE, _modifier, _is_static, _align), bit_size (0), suffix(""), atomic_id (at_id) {}
        AtomicTypeID get_atomic_type_id () { return atomic_id; }
        uint64_t get_bit_size () { return bit_size; }
        bool is_atomic_type() { return true; }

    protected:
        unsigned int bit_size;
        std::string suffix;

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

        IntegerType (IntegerTypeID it_id) : AtomicType (AtomicTypeID::Integer), is_signed (false), int_type_id (it_id) {}
        IntegerType (IntegerTypeID it_id, Mod _modifier, bool _is_static, uint64_t _align) :
                     AtomicType (AtomicTypeID::Integer, _modifier, _is_static, _align),
                     is_signed (false), int_type_id (it_id) {}
        static std::shared_ptr<Type> init (IntegerType::IntegerTypeID _type_id);
        static std::shared_ptr<Type> init (IntegerType::IntegerTypeID _type_id, Mod _modifier, bool _is_static, uint64_t _align);
        IntegerTypeID get_int_type_id () { return int_type_id; }
        static bool can_repr_value (IntegerType::IntegerTypeID A, IntegerType::IntegerTypeID B); // if type a can represent all of the values of the type b
        static IntegerType::IntegerTypeID get_corr_unsig (IntegerType::IntegerTypeID _type_id);
        bool get_is_signed () { return is_signed; }
        AtomicType::TypedVal get_min () { return min; }
        AtomicType::TypedVal get_max () { return max; }
        bool is_int_type() { return true; }

    protected:
        bool is_signed;
        AtomicType::TypedVal min;
        AtomicType::TypedVal max;

    private:
        IntegerTypeID int_type_id;
};

class TypeBOOL : public IntegerType {
    public:
        TypeBOOL () : IntegerType(IntegerType::IntegerTypeID::BOOL) { init_type (); }
        TypeBOOL (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::BOOL, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "bool";
            suffix = "";
            min.bool_val = false;
            max.bool_val = true;
            bit_size = sizeof (bool) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeCHAR : public IntegerType {
    public:
        TypeCHAR () : IntegerType(IntegerType::IntegerTypeID::CHAR) { init_type (); }
        TypeCHAR (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::CHAR, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "signed char";
            suffix = "";
            min.char_val = SCHAR_MIN;
            max.char_val = SCHAR_MAX;
            bit_size = sizeof (char) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeUCHAR : public IntegerType {
    public:
        TypeUCHAR () : IntegerType(IntegerType::IntegerTypeID::UCHAR) { init_type (); }
        TypeUCHAR (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::UCHAR, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "unsigned char";
            suffix = "";
            min.uchar_val = 0;
            max.uchar_val = UCHAR_MAX;
            bit_size = sizeof (unsigned char) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeSHRT : public IntegerType {
    public:
        TypeSHRT () : IntegerType(IntegerType::IntegerTypeID::SHRT) { init_type (); }
        TypeSHRT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::SHRT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "short";
            suffix = "";
            min.shrt_val = SHRT_MIN;
            max.shrt_val = SHRT_MAX;
            bit_size = sizeof (short) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeUSHRT : public IntegerType {
    public:
        TypeUSHRT () : IntegerType(IntegerType::IntegerTypeID::USHRT) { init_type (); }
        TypeUSHRT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::USHRT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "unsigned short";
            suffix = "";
            min.ushrt_val = 0;
            max.ushrt_val = USHRT_MAX;
            bit_size = sizeof (unsigned short) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeINT : public IntegerType {
    public:
        TypeINT () : IntegerType(IntegerType::IntegerTypeID::INT) { init_type (); }
        TypeINT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::INT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "int";
            suffix = "";
            min.int_val = INT_MIN;
            max.int_val = INT_MAX;
            bit_size = sizeof (int) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeUINT : public IntegerType {
    public:
        TypeUINT () : IntegerType(IntegerType::IntegerTypeID::UINT) { init_type (); }
        TypeUINT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::UINT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "unsigned int";
            suffix = "U";
            min.uint_val = 0;
            max.uint_val = UINT_MAX;
            bit_size = sizeof (unsigned int) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeLINT : public IntegerType {
    public:
        TypeLINT () : IntegerType(IntegerType::IntegerTypeID::LINT) { init_type (); }
        TypeLINT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::LINT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "long int";
            suffix = "L";
            min.lint_val = LONG_MIN;;
            max.lint_val = LONG_MAX;
            bit_size = sizeof (long int) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeULINT : public IntegerType {
    public:
        TypeULINT () : IntegerType(IntegerType::IntegerTypeID::ULINT) { init_type (); }
        TypeULINT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::ULINT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "unsigned long int";
            suffix = "UL";
            min.ulint_val = 0;
            max.ulint_val = ULONG_MAX;
            bit_size = sizeof (unsigned long int) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};

class TypeLLINT : public IntegerType {
    public:
        TypeLLINT () : IntegerType(IntegerType::IntegerTypeID::LLINT) { init_type (); }
        TypeLLINT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::LLINT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "long long int";
            suffix = "LL";
            min.llint_val = LLONG_MIN;
            max.llint_val = LLONG_MAX;
            bit_size = sizeof (long long int) * CHAR_BIT;
            is_signed = true;
        }
        void dbg_dump ();
};

class TypeULLINT : public IntegerType {
    public:
        TypeULLINT () : IntegerType(IntegerType::IntegerTypeID::ULLINT) { init_type (); }
        TypeULLINT (Mod _modifier, bool _is_static, uint64_t _align) :
                  IntegerType(IntegerType::IntegerTypeID::ULLINT, _modifier, _is_static, _align) { init_type (); }
        void init_type () {
            name = "unsigned long long int";
            suffix = "ULL";
            min.ullint_val = 0;
            max.ullint_val = ULLONG_MAX;
            bit_size = sizeof (unsigned long long int) * CHAR_BIT;
            is_signed = false;
        }
        void dbg_dump ();
};
}
