#pragma once

#include <random>
#include <algorithm>

#include "type.h"
#include "node.h"

struct ControlStruct {
    std::string ext_num;

    std::vector<Type::TypeID> allowed_types;

    int min_arr_num;
    int max_arr_num;
    int min_arr_size;
    int max_arr_size;
    Array::Ess primary_ess;

    uint64_t min_var_val;
    uint64_t max_var_val;
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
        LoopGen (ControlStruct _ctrl) : Gen (_ctrl), min_ex_arr_size (UINT64_MAX) {}
        void generate ();

    private:
        uint64_t min_ex_arr_size;
        std::vector<std::shared_ptr<Data>> inp_sym_table;
        std::vector<std::shared_ptr<Data>> out_sym_table;
};

class VarValGen : public Gen {
    public:
        VarValGen (ControlStruct _ctrl, Type::TypeID _type_id) : Gen (_ctrl), type_id(_type_id), val(0) {}
        void generate();
        void generate_step();
        uint64_t get_value () { return val; }

    private:
        Type::TypeID type_id;
        uint64_t val;
};

class StepExprGen : public Gen {
    public:
        StepExprGen (ControlStruct _ctrl, std::shared_ptr<Variable> _var, int64_t _step) :
                     Gen (_ctrl), var(_var), step(_step), expr(NULL) {}
        void generate ();
        std::shared_ptr<Expr> get_expr () { return expr; }

    private:
        int64_t step;
        std::shared_ptr<Expr> expr;
        std::shared_ptr<Variable> var;
};
