#pragma once

#include "type.h"

class Data {
    public:
        enum Mod {
            NTHNG,
            CONST,
            VOLAT,
            CONST_VOLAT,
            MAX_MOD
        };

        enum VarClassID {
            VAR, ARR, MAX_CLASS_ID
        };

        explicit Data (std::string _name, Type::TypeID _type_id, Mod _modifier, bool _is_static);
        void set_modifier (Mod _modifier) { modifier = _modifier; }
        Mod get_modifier () { return modifier; }
        bool get_is_static () { return is_static; }
        VarClassID get_class_id () { return class_id; }
        void set_align (uint64_t _align) { align = _align; }
        uint64_t get_align () { return align; }
        std::string get_name () { return name; }
        std::shared_ptr<Type> get_type () { return type; }
        virtual void set_value (uint64_t _val) = 0;
        virtual void set_max (uint64_t _max) = 0;
        virtual void set_min (uint64_t _min) = 0;
        virtual void dbg_dump () = 0;

    protected:
        std::shared_ptr<Type> type;
        std::string name;
        Mod modifier;
        bool is_static;
        uint64_t align;
        VarClassID class_id;
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
};

class Variable : public Data{
    public:
        explicit Variable (std::string _name, Type::TypeID _type_id, Mod _modifier, bool _is_static);
        void set_value (uint64_t _val);
        void set_max (uint64_t _max);
        void set_min (uint64_t _min);
        void dbg_dump ();

    private:
        TypeVal value;
        TypeVal min;
        TypeVal max;
};

class Array : public Data {
    public:
        enum Ess {
            C_ARR,
            STD_ARR,
            STD_VEC,
            MAX_ESS
        };

    explicit Array (std::string _name, Type::TypeID _base_type_id,  Mod _modifier, bool _is_static,
                    unsigned int _size, Ess _essence);
    std::shared_ptr<Type> get_base_type () { return base_type; }
    unsigned int get_size () { return size; }
    Ess get_essence () { return essence; }
    void set_value (uint64_t _val);
    void set_max (uint64_t _max);
    void set_min (uint64_t _min);
    void dbg_dump ();

    private:
        std::shared_ptr<Type> base_type;
        TypeVal value;
        TypeVal min;
        TypeVal max;
        unsigned int size;
        Ess essence;
};
