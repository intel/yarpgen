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

#include <memory>

#include "gen_policy.h"
#include "variable.h"
#include "node.h"

///////////////////////////////////////////////////////////////////////////////

class SymbolTable {
    public:
        SymbolTable () {}
        void add_variable (std::shared_ptr<Variable> _var) { variable.push_back (_var); }
        void add_array (std::shared_ptr<Array> _arr) { array.push_back (_arr); }
        void add_struct_type (std::shared_ptr<StructType> _type) { struct_type.push_back (_type); }
        void add_struct (std::shared_ptr<Struct> _struct) { structs.push_back (_struct); }
        std::vector<std::shared_ptr<Variable>>& get_variables () { return variable; }
        std::vector<std::shared_ptr<Array>>& get_arrays () { return array; }
        std::vector<std::shared_ptr<StructType>>& get_struct_types () { return struct_type; }
        std::vector<std::shared_ptr<Struct>>& get_structs () { return structs; }
        // TODO: Add symbol probability
        std::shared_ptr<Variable> get_rand_variable ();
        std::shared_ptr<Array> get_rand_array ();
        std::string emit_variable_extern_decl ();
        std::string emit_variable_def ();
        // TODO: rewrite with IR
        std::string emit_variable_check ();
        std::string emit_struct_type_def ();
        std::string emit_struct_def ();
        std::string emit_struct_extern_decl ();
        std::string emit_struct_init ();

    private:
        std::vector<std::shared_ptr<StructType>> struct_type;
        std::vector<std::shared_ptr<Struct>> structs;
        std::vector<std::shared_ptr<Variable>> variable;
        std::vector<std::shared_ptr<Array>> array;
};

class Context {
    public:
        Context (GenPolicy _gen_policy, std::shared_ptr<Stmt> _glob_stmt, Node::NodeID _glob_stmt_id, 
                 std::shared_ptr<SymbolTable> _global_sym_table, std::shared_ptr<Context> _parent_ctx);

        std::shared_ptr<GenPolicy> get_self_gen_policy () { return std::make_shared<GenPolicy> (self_gen_policy); }
        int get_depth () { return depth; }
        int get_if_depth () { return if_depth; }
        void set_extern_inp_sym_table (std::shared_ptr<SymbolTable> _extern_inp_sym_table) { extern_inp_sym_table = _extern_inp_sym_table; }
        std::shared_ptr<SymbolTable> get_extern_inp_sym_table () { return extern_inp_sym_table; }
        void set_extern_out_sym_table (std::shared_ptr<SymbolTable> _extern_out_sym_table) { extern_out_sym_table = _extern_out_sym_table; }
        std::shared_ptr<SymbolTable> get_extern_out_sym_table () { return extern_out_sym_table; }
        std::shared_ptr<Context> get_parent_ctx () { return parent_ctx; }

    private:
        GenPolicy self_gen_policy;
        std::shared_ptr<Stmt> self_glob_stmt;
        Node::NodeID self_glob_stmt_id;
        std::shared_ptr<SymbolTable> extern_inp_sym_table;
        std::shared_ptr<SymbolTable> extern_out_sym_table;
        std::shared_ptr<SymbolTable> global_sym_table;
        std::shared_ptr<Context> parent_ctx;
        int if_depth;
        int depth;
};
