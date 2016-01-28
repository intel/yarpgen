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
#include "gen_policy.h"
#include "variable.h"
#include "node.h"

///////////////////////////////////////////////////////////////////////////////

class Generator {
    public:
        Generator (std::shared_ptr<GenPolicy> _gen_policy) : gen_policy (_gen_policy) {};
        virtual void generate () = 0;

    protected:
        std::shared_ptr<GenPolicy> gen_policy;
};

class ScalarTypeGen : public Generator {
    public:
        ScalarTypeGen (std::shared_ptr<GenPolicy> _gen_policy) : Generator (_gen_policy), type_id (Type::TypeID::MAX_TYPE_ID) {};
        Type::TypeID get_type() { return type_id; }
        void generate ();

    private:
        Type::TypeID type_id;
};

class ModifierGen : public Generator {
    public:
        ModifierGen (std::shared_ptr<GenPolicy> _gen_policy) : Generator (_gen_policy), modifier (Data::Mod::MAX_MOD) {}
        Data::Mod get_modifier () { return modifier; }
        void generate ();

    private:
        Data::Mod modifier;
};

class StaticSpecifierGen : public Generator {
    public:
        StaticSpecifierGen (std::shared_ptr<GenPolicy> _gen_policy) : Generator (_gen_policy), specifier (false) {}
        bool get_specifier () { return specifier; }
        void generate ();

    private:
        bool specifier;
};

class DataGen : public Generator {
    public:
        DataGen (std::shared_ptr<GenPolicy> _gen_policy) :
                 Generator (_gen_policy), type_id (Type::TypeID::MAX_TYPE_ID), modifier (Data::Mod::MAX_MOD),
                 static_spec (false), rand_init (true), data (NULL) {}
        DataGen (std::shared_ptr<GenPolicy> _gen_policy, Type::TypeID _type_id, Data::Mod _modifier, bool _static_spec) :
                 Generator (_gen_policy), type_id (_type_id), modifier (_modifier), static_spec(_static_spec), rand_init (false), data (NULL) {}
        std::shared_ptr<Data> get_data () { return data; }
        virtual void generate () = 0;

    protected:
        void rand_init_param ();
        bool rand_init;
        Type::TypeID type_id;
        Data::Mod modifier;
        bool static_spec;
        std::shared_ptr<Data> data;
};

class ScalarVariableGen : public DataGen {
    public:
        ScalarVariableGen (std::shared_ptr<GenPolicy> _gen_policy) : DataGen (_gen_policy) {}
        ScalarVariableGen (std::shared_ptr<GenPolicy> _gen_policy, Type::TypeID _type_id, Data::Mod _modifier, bool _static_spec) :
                          DataGen (_gen_policy, _type_id, _modifier, _static_spec) {}
        void generate ();

    private:
        static int variable_num;
};

class ArrayVariableGen : public DataGen {
    public:
        ArrayVariableGen (std::shared_ptr<GenPolicy> _gen_policy) : DataGen (_gen_policy), essence (Array::Ess::MAX_ESS), size (0) {}
        ArrayVariableGen (std::shared_ptr<GenPolicy> _gen_policy, Type::TypeID _type_id, Data::Mod _modifier, bool _static_spec, int _size, Array::Ess _essence) :
                          DataGen (_gen_policy, _type_id, _modifier, _static_spec), size (_size), essence (_essence) {}
        void generate ();

    private:
        Array::Ess essence;
        int size;
        static int array_num;
};

class StmtGen : public Generator {
    public:
        StmtGen (std::shared_ptr<GenPolicy> _gen_policy) : Generator (_gen_policy), stmt (NULL) {}
        std::shared_ptr<Stmt> get_stmt () { return stmt; }
        virtual void generate () = 0;

    protected:
        bool rand_init;
        std::shared_ptr<Stmt> stmt;
};

class DeclStmtGen : public StmtGen {
    public:
        DeclStmtGen (std::shared_ptr<GenPolicy> _gen_policy) : StmtGen (_gen_policy) {
            data = NULL;
            init = NULL;
            is_extern = false;
            rand_init = true;
        }
        DeclStmtGen (std::shared_ptr<GenPolicy> _gen_policy, std::shared_ptr<Data> _data,  std::shared_ptr<Expr> _init) : StmtGen (_gen_policy) {
            data = _data;
            init = _init;
            is_extern = false;
            rand_init = false;
        }
        std::shared_ptr<Data> get_data () { return data; }
        std::shared_ptr<Expr> get_init () { return init; }
        void set_is_extert (bool _is_extern) { is_extern = _is_extern; }
        void generate ();

    private:
        std::shared_ptr<Data> data;
        std::shared_ptr<Expr> init;
        bool is_extern;
};
