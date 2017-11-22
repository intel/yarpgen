/*
Copyright (c) 2015-2017, Intel Corporation

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
#include "options.h"

namespace yarpgen {

class Context;
class Data;
class ScalarVariable;
class Struct;

// Abstract class, serves as a common ancestor for all types.
class Type {
    public:
        // ID for top-level Type kind
        enum TypeID {
            BUILTIN_TYPE,
            STRUCT_TYPE,
            ARRAY_TYPE,
            POINTER_TYPE,
            MAX_TYPE_ID
        };

        // CV-qualifiers.
        enum CV_Qual {
            NTHG,
            VOLAT,
            CONST,
            CONST_VOLAT,
            MAX_CV_QUAL
        };

        // ID for builtin types (in C terminology they "atomic", in C++ they are "fundamental" types)
        enum BuiltinTypeID {
            Integer, Max_BuiltinTypeID
        };

        enum IntegerTypeID {
            BOOL,
            // Note, char and signed char types are not distinguished,
            // though they are distinguished in C++ standard and char may
            // map to unsigned char in some implementations. By "CHAR" we assume "signed char"
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

        Type (TypeID _id) : cv_qual(CV_Qual::NTHG), is_static(false), align(0), id (_id) {}
        Type (TypeID _id, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
              cv_qual (_cv_qual), is_static (_is_static), align (_align), id (_id) {}

        // Getters and setters for general Type properties
        Type::TypeID get_type_id () { return id; }
        virtual BuiltinTypeID get_builtin_type_id() { return Max_BuiltinTypeID; }
        virtual IntegerTypeID get_int_type_id () { return MAX_INT_ID; }
        virtual bool get_is_signed() { return false; }
        virtual bool get_is_bit_field() { return false; }
        void set_cv_qual (CV_Qual _cv_qual) { cv_qual = _cv_qual; }
        CV_Qual get_cv_qual () { return cv_qual; }
        void set_is_static (bool _is_static) { is_static = _is_static; }
        bool get_is_static () { return is_static; }
        void set_align (uint32_t _align) { align = _align; }
        uint32_t get_align () { return align; }

        // We assume static storage duration, cv-qualifier and alignment as a part of Type's full name
        std::string get_name ();
        std::string get_simple_name () { return name; }
        virtual std::string get_type_suffix() { return ""; }

        // Utility functions, which allows quickly determine Type kind
        virtual bool is_builtin_type() { return false; }
        virtual bool is_int_type() { return false; }
        virtual bool is_struct_type() { return false; }
        virtual bool is_array_type() { return false; }
        virtual bool is_ptr_type() { return false; }

        // Pure virtual function, used for debug purposes
        virtual void dbg_dump() = 0;

        virtual ~Type () {}

    protected:
        std::string name;
        CV_Qual cv_qual;
        bool is_static;
        uint32_t align;

    private:
        TypeID id;
};

// Class which represents structures
class StructType : public Type {
    public:
        // Class which represents member of a structure, including bit-fields
        //TODO: add generator?
        struct StructMember {
            public:
                StructMember (std::shared_ptr<Type> _type, std::string _name);
                std::string get_name () { return name; }
                std::shared_ptr<Type> get_type() { return type; }
                std::shared_ptr<Data> get_data () { return data; }
                std::string get_definition (std::string offset = "");

            private:
                std::shared_ptr<Type> type;
                std::string name;

                std::shared_ptr<Data> data; //TODO: it is a stub for static members
        };

        StructType (std::string _name) : Type (Type::STRUCT_TYPE), nest_depth(0) { name = _name; }
        StructType (std::string _name, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                    Type (Type::STRUCT_TYPE, _cv_qual, _is_static, _align), nest_depth(0) { name = _name; }
        bool is_struct_type() { return true; }

        // Getters and setters for StructType properties
        //TODO: it should handle nest_depth change
        void add_member (std::shared_ptr<StructMember> new_mem) { members.push_back(new_mem); shadow_members.push_back(new_mem); }
        void add_member (std::shared_ptr<Type> _type, std::string _name);
        void add_shadow_member (std::shared_ptr<Type> _type) { shadow_members.push_back(std::make_shared<StructMember>(_type, "")); }
        uint32_t get_member_count () { return members.size(); }
        uint32_t get_shadow_member_count () { return shadow_members.size(); }
        uint32_t get_nest_depth () { return nest_depth; }
        std::shared_ptr<StructMember> get_member (unsigned int num);


        std::string get_definition (std::string offset = "");
        // It returns an out-of-line definition for all static members of the structure
        std::string get_static_memb_def (std::string offset = "");
        std::string get_static_memb_check (std::string offset = "");

        void dbg_dump();

        // Randomly generate StructType
        static std::shared_ptr<StructType> generate (std::shared_ptr<Context> ctx);
        static std::shared_ptr<StructType> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<StructType>> nested_struct_types);

    private:
        //TODO: it is a stub for unnamed bit fields. Nobody should know about them
        std::vector<std::shared_ptr<StructMember>> shadow_members;
        std::vector<std::shared_ptr<StructMember>> members;
        uint32_t nest_depth;
};

// ID for all handled Undefined Behaviour
enum UB {
    NoUB,
    NullPtr, // nullptr ptr dereferencing
    SignOvf, // Signed overflow
    SignOvfMin, // Special case of signed overflow: INT_MIN * (-1)
    ZeroDiv, // FPE
    ShiftRhsNeg, // Shift by negative value
    ShiftRhsLarge, // // Shift by large value
    NegShift, // Shift of negative value
    NoMemeber, // Can't find member of structure
    MaxUB
};

// Common ancestor for all builtin types.
class BuiltinType : public Type {
    public:
        // We need something to link together Type and Value (it should be consistent with Type).
        class ScalarTypedVal {
            public:
                union Val {
                    bool bool_val;
                    signed char char_val;
                    unsigned char uchar_val;
                    short shrt_val;
                    unsigned short ushrt_val;
                    int int_val;
                    unsigned int uint_val;
                    int lint32_val; // for 32-bit mode
                    unsigned int ulint32_val; // for 32-bit mode
                    long long int lint64_val; // for 64-bit mode
                    unsigned long long int ulint64_val; // for 64-bit mode
                    long long int llint_val;
                    unsigned long long int ullint_val;
                };

                ScalarTypedVal (BuiltinType::IntegerTypeID _int_type_id) : int_type_id(_int_type_id), res_of_ub(NoUB) { val.ullint_val = 0; }
                ScalarTypedVal (BuiltinType::IntegerTypeID _int_type_id, UB _res_of_ub) : int_type_id (_int_type_id), res_of_ub(_res_of_ub)  { val.ullint_val = 0; }
                Type::IntegerTypeID get_int_type_id () const { return int_type_id; }

                // Utility functions for UB
                UB get_ub () { return res_of_ub; }
                void set_ub (UB _ub) { res_of_ub = _ub; }
                bool has_ub () { return res_of_ub != NoUB; }

                // Interface to value through uint64_t
                //TODO: it is a stub for shift rebuild. Can we do it better?
                uint64_t get_abs_val ();
                void set_abs_val (uint64_t new_val);

                // Functions which implements UB detection and semantics of all operators
                ScalarTypedVal cast_type (Type::IntegerTypeID to_type_id);
                ScalarTypedVal operator++ (int) { return pre_op(true ); } // Postfix, but used also as prefix
                ScalarTypedVal operator-- (int) { return pre_op(false); }// Postfix, but used also as prefix
                ScalarTypedVal operator- ();
                ScalarTypedVal operator~ ();
                ScalarTypedVal operator! ();

                ScalarTypedVal operator+ (ScalarTypedVal rhs);
                ScalarTypedVal operator- (ScalarTypedVal rhs);
                ScalarTypedVal operator* (ScalarTypedVal rhs);
                ScalarTypedVal operator/ (ScalarTypedVal rhs);
                ScalarTypedVal operator% (ScalarTypedVal rhs);
                ScalarTypedVal operator< (ScalarTypedVal rhs);
                ScalarTypedVal operator> (ScalarTypedVal rhs);
                ScalarTypedVal operator<= (ScalarTypedVal rhs);
                ScalarTypedVal operator>= (ScalarTypedVal rhs);
                ScalarTypedVal operator== (ScalarTypedVal rhs);
                ScalarTypedVal operator!= (ScalarTypedVal rhs);
                ScalarTypedVal operator&& (ScalarTypedVal rhs);
                ScalarTypedVal operator|| (ScalarTypedVal rhs);
                ScalarTypedVal operator& (ScalarTypedVal rhs);
                ScalarTypedVal operator| (ScalarTypedVal rhs);
                ScalarTypedVal operator^ (ScalarTypedVal rhs);
                ScalarTypedVal operator<< (ScalarTypedVal rhs);
                ScalarTypedVal operator>> (ScalarTypedVal rhs);

                // Randomly generate ScalarTypedVal
                static ScalarTypedVal generate (std::shared_ptr<Context> ctx, BuiltinType::IntegerTypeID _int_type_id);
                static ScalarTypedVal generate (std::shared_ptr<Context> ctx, ScalarTypedVal min, ScalarTypedVal max);

                // The value itself
                Val val;

            private:
                // Common fuction for all pre-increment and post-increment operators
                ScalarTypedVal pre_op (bool inc);

                BuiltinType::IntegerTypeID int_type_id;
                // If we can use the value or it was obtained from operation with UB
                UB res_of_ub;
        };

        BuiltinType (BuiltinTypeID _builtin_id) : Type (Type::BUILTIN_TYPE), bit_size (0), suffix(""), builtin_id (_builtin_id) {}
        BuiltinType (BuiltinTypeID _builtin_id, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                    Type (Type::BUILTIN_TYPE, _cv_qual, _is_static, _align), bit_size (0), suffix(""), builtin_id (_builtin_id) {}
        bool is_builtin_type() { return true; }

        // Getters for BuiltinType properties
        BuiltinTypeID get_builtin_type_id() { return builtin_id; }
        uint32_t get_bit_size () { return bit_size; }
        std::string get_int_literal_suffix() { return suffix; }

    protected:
        unsigned int bit_size;
        // Suffix for integer literals
        std::string suffix;

    private:
        BuiltinTypeID builtin_id;
};

std::ostream& operator<< (std::ostream &out, const BuiltinType::ScalarTypedVal &scalar_typed_val);

// Class which serves as common ancestor for all standard integer types, bool and bit-fields
class IntegerType : public BuiltinType {
    public:
        IntegerType (IntegerTypeID it_id) : BuiltinType (BuiltinTypeID::Integer), is_signed (false), min(it_id), max(it_id), int_type_id (it_id) {}
        IntegerType (IntegerTypeID it_id, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                     BuiltinType (BuiltinTypeID::Integer, _cv_qual, _is_static, _align),
                     is_signed (false), min(it_id), max(it_id), int_type_id (it_id) {}
        bool is_int_type() { return true; }

        // Getters for IntegerType properties
        IntegerTypeID get_int_type_id () { return int_type_id; }
        bool get_is_signed () { return is_signed; }
        BuiltinType::ScalarTypedVal get_min () { return min; }
        BuiltinType::ScalarTypedVal get_max () { return max; }

        // This utility functions take IntegerTypeID and return shared pointer to corresponding type
        static std::shared_ptr<IntegerType> init (BuiltinType::IntegerTypeID _type_id);
        static std::shared_ptr<IntegerType> init (BuiltinType::IntegerTypeID _type_id, CV_Qual _cv_qual, bool _is_static, uint32_t _align);

        // If type A can represent all the values of type B
        static bool can_repr_value (BuiltinType::IntegerTypeID A, BuiltinType::IntegerTypeID B); // if type B can represent all of the values of the type A
        // Returns corresponding unsigned type
        static BuiltinType::IntegerTypeID get_corr_unsig (BuiltinType::IntegerTypeID _type_id);

        // Randomly generate IntegerType (except bit-fields)
        static std::shared_ptr<IntegerType> generate (std::shared_ptr<Context> ctx);

    protected:
        bool is_signed;
        // Minimum and maximum value, which can fit in type
        BuiltinType::ScalarTypedVal min;
        BuiltinType::ScalarTypedVal max;

    private:
        IntegerTypeID int_type_id;
};

// Class which represents bit-field
class BitField : public IntegerType {
    public:
        BitField (IntegerTypeID it_id, uint32_t _bit_size) : IntegerType(it_id) { init_type(it_id, _bit_size); }
        BitField (IntegerTypeID it_id, uint32_t _bit_size, CV_Qual _cv_qual) : IntegerType(it_id, _cv_qual, false, 0) { init_type(it_id, _bit_size); }

        // Getters of BitField properties
        bool get_is_bit_field() { return true; }
        uint32_t get_bit_field_width() { return bit_field_width; }

        // If all values of the bit-field can fit in signed/unsigned int
        static bool can_fit_in_int (BuiltinType::ScalarTypedVal val, bool is_unsigned);

        // Randomly generate BitField
        static std::shared_ptr<BitField> generate (std::shared_ptr<Context> ctx, bool is_unnamed = false);

        void dbg_dump ();

    private:
        // Common initializer functions, used in constructors
        void init_type(IntegerTypeID it_id, uint32_t _bit_size);
        uint32_t bit_field_width;
};

// Following classes represents standard integer types and bool
// TODO: maybe all this classes should be singletons?
class TypeBOOL : public IntegerType {
    public:
        TypeBOOL () : IntegerType(BuiltinType::IntegerTypeID::BOOL) { init_type (); }
        TypeBOOL (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                  IntegerType(BuiltinType::IntegerTypeID::BOOL, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "bool";
            suffix = "";
            min.val.bool_val = false;
            max.val.bool_val = true;
            bit_size = sizeof (bool) * CHAR_BIT;
            is_signed = false;
        }
};

class TypeCHAR : public IntegerType {
    public:
        TypeCHAR () : IntegerType(BuiltinType::IntegerTypeID::CHAR) { init_type (); }
        TypeCHAR (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                  IntegerType(BuiltinType::IntegerTypeID::CHAR, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "signed char";
            suffix = "";
            min.val.char_val = SCHAR_MIN;
            max.val.char_val = SCHAR_MAX;
            bit_size = sizeof (char) * CHAR_BIT;
            is_signed = true;
        }
};

class TypeUCHAR : public IntegerType {
    public:
        TypeUCHAR () : IntegerType(BuiltinType::IntegerTypeID::UCHAR) { init_type (); }
        TypeUCHAR (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                   IntegerType(BuiltinType::IntegerTypeID::UCHAR, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "unsigned char";
            suffix = "";
            min.val.uchar_val = 0;
            max.val.uchar_val = UCHAR_MAX;
            bit_size = sizeof (unsigned char) * CHAR_BIT;
            is_signed = false;
        }
};

class TypeSHRT : public IntegerType {
    public:
        TypeSHRT () : IntegerType(BuiltinType::IntegerTypeID::SHRT) { init_type (); }
        TypeSHRT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                  IntegerType(BuiltinType::IntegerTypeID::SHRT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "short";
            suffix = "";
            min.val.shrt_val = SHRT_MIN;
            max.val.shrt_val = SHRT_MAX;
            bit_size = sizeof (short) * CHAR_BIT;
            is_signed = true;
        }
};

class TypeUSHRT : public IntegerType {
    public:
        TypeUSHRT () : IntegerType(BuiltinType::IntegerTypeID::USHRT) { init_type (); }
        TypeUSHRT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                   IntegerType(BuiltinType::IntegerTypeID::USHRT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "unsigned short";
            suffix = "";
            min.val.ushrt_val = 0;
            max.val.ushrt_val = USHRT_MAX;
            bit_size = sizeof (unsigned short) * CHAR_BIT;
            is_signed = false;
        }
};

class TypeINT : public IntegerType {
    public:
        TypeINT () : IntegerType(BuiltinType::IntegerTypeID::INT) { init_type (); }
        TypeINT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                 IntegerType(BuiltinType::IntegerTypeID::INT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "int";
            suffix = "";
            min.val.int_val = INT_MIN;
            max.val.int_val = INT_MAX;
            bit_size = sizeof (int) * CHAR_BIT;
            is_signed = true;
        }
};

class TypeUINT : public IntegerType {
    public:
        TypeUINT () : IntegerType(BuiltinType::IntegerTypeID::UINT) { init_type (); }
        TypeUINT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                  IntegerType(BuiltinType::IntegerTypeID::UINT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "unsigned int";
            suffix = "U";
            min.val.uint_val = 0;
            max.val.uint_val = UINT_MAX;
            bit_size = sizeof (unsigned int) * CHAR_BIT;
            is_signed = false;
        }
};

class TypeLINT : public IntegerType {
    public:
        TypeLINT () : IntegerType(BuiltinType::IntegerTypeID::LINT) { init_type (); }
        TypeLINT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                  IntegerType(BuiltinType::IntegerTypeID::LINT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "long int";
            suffix = "L";
            if (options->mode_64bit) {
                bit_size = sizeof(long long int) * CHAR_BIT;
                min.val.lint64_val = LLONG_MIN;
                max.val.lint64_val = LLONG_MAX;
            }
            else {
                bit_size = sizeof(int) * CHAR_BIT;
                min.val.lint32_val = INT_MIN;
                max.val.lint32_val = INT_MAX;
            }
            is_signed = true;
        }
};

class TypeULINT : public IntegerType {
    public:
        TypeULINT () : IntegerType(BuiltinType::IntegerTypeID::ULINT) { init_type (); }
        TypeULINT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                   IntegerType(BuiltinType::IntegerTypeID::ULINT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "unsigned long int";
            suffix = "UL";
            if (options->mode_64bit) {
                bit_size = sizeof (unsigned long long int) * CHAR_BIT;
                min.val.ulint64_val = 0;
                max.val.ulint64_val = ULLONG_MAX;
            }
            else {
                bit_size = sizeof(unsigned int) * CHAR_BIT;
                min.val.ulint32_val = 0;
                max.val.ulint32_val = UINT_MAX;
            }
            is_signed = false;
        }
};

class TypeLLINT : public IntegerType {
    public:
        TypeLLINT () : IntegerType(BuiltinType::IntegerTypeID::LLINT) { init_type (); }
        TypeLLINT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                   IntegerType(BuiltinType::IntegerTypeID::LLINT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "long long int";
            suffix = "LL";
            min.val.llint_val = LLONG_MIN;
            max.val.llint_val = LLONG_MAX;
            bit_size = sizeof (long long int) * CHAR_BIT;
            is_signed = true;
        }
};

class TypeULLINT : public IntegerType {
    public:
        TypeULLINT () : IntegerType(BuiltinType::IntegerTypeID::ULLINT) { init_type (); }
        TypeULLINT (CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                    IntegerType(BuiltinType::IntegerTypeID::ULLINT, _cv_qual, _is_static, _align) { init_type (); }
        void dbg_dump ();

    private:
        void init_type () {
            name = "unsigned long long int";
            suffix = "ULL";
            min.val.ullint_val = 0;
            max.val.ullint_val = ULLONG_MAX;
            bit_size = sizeof (unsigned long long int) * CHAR_BIT;
            is_signed = false;
        }
};

// ArrayType represents arrays of all kinds.
//TODO: 1) maybe we should split it to several classes?
//      2) nowadays it can represent only one-dimensional array
class ArrayType : public Type {
    public:
        enum ElementSubscript {
            Brackets, At, MaxElementSubscript
        };

        enum Kind {
            C_ARR,
            VAL_ARR,
            STD_ARR,
            STD_VEC,
            MAX_KIND
        };

        ArrayType(std::shared_ptr<Type> _base_type, uint32_t _size, Kind _kind);

        bool is_array_type () { return true; }
        std::shared_ptr<Type> get_base_type () { return base_type; }
        uint32_t get_size () { return size; }
        Kind get_kind () { return kind; }

        std::string get_type_suffix();
        void dbg_dump();

        static std::shared_ptr<ArrayType> generate(std::shared_ptr<Context> ctx);

    private:
        std::shared_ptr<Type> base_type;
        uint32_t size;
        Kind kind;
};

// Class which represents pointers
class PointerType : public Type {
    public:
        PointerType (std::shared_ptr<Type> base_type) :
                Type(TypeID::POINTER_TYPE), pointee_type(base_type), depth(1) { init(); }
        PointerType (std::shared_ptr<Type> base_type, CV_Qual _cv_qual, bool _is_static, uint32_t _align) :
                Type(TypeID::POINTER_TYPE, _cv_qual, _is_static, _align), pointee_type(base_type), depth(1) { init(); }
        bool is_ptr_type() { return true; }
        std::shared_ptr<Type> get_pointee_type() { return pointee_type; }
        uint32_t get_depth() { return depth; }
        void dbg_dump();

    private:
        void init();

        std::shared_ptr<Type> pointee_type;
        uint32_t depth;
};

// This function checks if we can assign one pointer type to another
bool is_pointers_compatible (std::shared_ptr<PointerType> type_a, std::shared_ptr<PointerType> type_b);
}
