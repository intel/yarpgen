#pragma once

#include "type.h"

class Variable {
    public:
        enum Mod {
            NTHNG,
            CONST,
            VOLAT,
            CONST_VOLAT,
            MAX_MOD
        };

        explicit Variable (std::string _name, Type::TypeID _type_id, Mod _modifier);
        void set_type (Type::TypeID _type_id);
        void set_modifier (Mod _modifier) { modifier = _modifier; }
        void set_value (uint64_t _val);
        void set_max (uint64_t _max);
        void set_min (uint64_t _min);
        std::string get_name () { return name; }
        std::shared_ptr<Type> get_type () { return type; }
        void dbg_dump ();

    protected:
        union TypeVal {
            unsigned int uint_val;
            unsigned long int ulint_val;
            unsigned long long int ullint_val;
        };

        std::shared_ptr<Type> type;
        std::string name;
        Mod modifier;
        TypeVal value;
        TypeVal min;
        TypeVal max;
};

class Array : public Variable {
    public:
        enum Ess {
            C_ARR,
            STD_ARR,
            STD_VEC,
            MAX_ESS
        };

    explicit Array (std::string _name, Type::TypeID _type_id,  Mod _modifier,
                    unsigned int _size, Ess _essence) :
                    Variable (_name, _type_id, _modifier),
                    size (_size), essence (_essence) {};
    Ess get_essence () { return essence; }
    void dbg_dump ();

    private:
        unsigned int size;
        Ess essence;
};
