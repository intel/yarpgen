#pragma once

#include <iostream>
#include <climits>
#include <memory>

class Type {
    public:
        enum TypeID {
            UINT,
            ULINT,
            ULLINT,
            MAX_INT_ID,
            PTR,
            MAX_TYPE_ID
        };

        explicit Type () {};
        static std::shared_ptr<Type> init (Type::TypeID _type_id);
        Type::TypeID get_id () { return id; }
        std::string get_name () { return name; }
        std::string get_suffix() { return suffix; }
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

class TypeUINT : public Type {
    public:
        TypeUINT ();
        std::string get_max_str ();
        std::string get_min_str ();
};

class TypeULINT : public Type {
    public:
        TypeULINT ();
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
