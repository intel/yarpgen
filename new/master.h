#pragma once

#include <random>
#include <algorithm>

#include "type.h"
#include "node.h"

class ControlStruct {
    public:
        std::string ext_num;

        std::vector<Type::TypeID> allowed_types;

        int min_arr_num;
        int max_arr_num;
        int min_arr_size;
        int max_arr_size;
        Array::Ess primary_ess;
};

class Master {
    public:
        Master ();
        void generate ();
        std::string emit ();

    private:
        ControlStruct ctrl;
        std::vector<std::shared_ptr<Stmnt>> program;
        std::vector<std::shared_ptr<Data>> sym_table;
};

class Gen {
    public:
        Gen (ControlStruct _ctrl) : ctrl (_ctrl) {}
        void set_ctrl (ControlStruct _ctrl) { ctrl = _ctrl; }
        std::vector<std::shared_ptr<Stmnt>>& get_program () { return program; }
        std::vector<std::shared_ptr<Data>>& get_sym_table () { return sym_table; }
        virtual void generate () = 0;

    protected:
        ControlStruct ctrl;
        std::vector<std::shared_ptr<Stmnt>> program;
        std::vector<std::shared_ptr<Data>> sym_table;
};

class ArrayGen : public Gen {
    public:
        ArrayGen (ControlStruct _ctrl) : Gen (_ctrl) {}
        std::shared_ptr<Array> get_inp_arr () { return inp_arr; }
        std::shared_ptr<Array> get_out_arr () { return out_arr; }
        void generate ();

    private:
        Array::Ess primary_ess;
        std::shared_ptr<Array> inp_arr;
        std::shared_ptr<Array> out_arr;
};

class LoopGen : public Gen {
    public:
        LoopGen (ControlStruct _ctrl) : Gen (_ctrl), min_arr_size (UINT_MAX) {}
        void generate ();

    private:
        unsigned int min_arr_size;
        std::vector<std::shared_ptr<Data>> inp_sym_table;
        std::vector<std::shared_ptr<Data>> out_sym_table;
};
