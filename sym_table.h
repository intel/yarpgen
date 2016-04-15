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

namespace rl {

class SymbolTable {
    public:
        SymbolTable () {}
        std::shared_ptr<SymbolTable> merge (std::shared_ptr<SymbolTable> inp_st);
        std::shared_ptr<SymbolTable> copy();

        void add_variable (std::shared_ptr<Variable> _var) { variable.push_back (_var); }
        void add_array_decl (std::shared_ptr<crosschain::VectorDeclStmt> _arr) { array.push_back (_arr); }
        void add_struct_type (std::shared_ptr<StructType> _type) { struct_type.push_back (_type); }
        void add_struct (std::shared_ptr<Struct> _struct) { structs.push_back (_struct); }

        void set_variables (std::vector<std::shared_ptr<Variable>> _variable) { variable = _variable; }
        void set_array_decls (std::vector<std::shared_ptr<crosschain::VectorDeclStmt>> _array) { array = _array; }
        void set_struct_types (std::vector<std::shared_ptr<StructType>> _struct_type) { struct_type = _struct_type; }
        void set_structs (std::vector<std::shared_ptr<Struct>> _structs) { structs = _structs; }

        std::vector<std::shared_ptr<Variable>>& get_variables () { return variable; }
        std::vector<std::shared_ptr<crosschain::Vector>> get_arrays ();
        std::vector<std::shared_ptr<crosschain::VectorDeclStmt>> get_array_decls () {return this->array; }
        std::vector<std::shared_ptr<StructType>>& get_struct_types () { return struct_type; }
        std::vector<std::shared_ptr<Struct>>& get_structs () { return structs; }

        // TODO: Add symbol probability
        std::shared_ptr<Variable> get_rand_variable ();
        std::shared_ptr<crosschain::Vector> get_rand_array ();

        std::string emit_variable_extern_decl (std::string offset = "");
        std::string emit_variable_def (std::string offset = "");
        // TODO: rewrite with IR
        std::string emit_variable_check (std::string offset = "");
        std::string emit_struct_type_def (std::string offset = "");
        std::string emit_struct_def (std::string offset = "");
        std::string emit_struct_extern_decl (std::string offset = "");
        std::string emit_struct_init (std::string offset = "");

        // Arrays
        std::string emit_array_def (std::string offset = "");
        std::string emit_array_extern_decl (std::string offset = "");
        std::string emit_array_init (std::string offset = "");
        std::string emit_array_check (std::string offset = "");

    private:
        std::string emit_single_struct_init (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, std::string offset = "");

        std::string emit_single_struct_init ();
        std::vector<std::shared_ptr<StructType>> struct_type;
        std::vector<std::shared_ptr<Struct>> structs;
        std::vector<std::shared_ptr<Variable>> variable;
        std::vector<std::shared_ptr<crosschain::VectorDeclStmt>> array;
};

class Context {
    public:
        Context (GenPolicy _gen_policy, std::shared_ptr<Stmt> _glob_stmt, Node::NodeID _glob_stmt_id, 
                 std::shared_ptr<SymbolTable> _global_sym_table, std::shared_ptr<Context> _parent_ctx);
        Context (GenPolicy _gen_policy, std::shared_ptr<Stmt> _glob_stmt, std::shared_ptr<Context> _parent_ctx);
        std::shared_ptr<GenPolicy> get_self_gen_policy () { return std::make_shared<GenPolicy> (self_gen_policy); }
        int get_depth () { return depth; }
        int get_if_depth () { return if_depth; }
        int get_loop_depth () { return loop_depth; }
        void inc_loop_depth () {this->loop_depth += 1;}
        unsigned long long get_complexity () {return this->complexity;}
        void add_to_complexity (unsigned long long diff);
        void set_verbose_level(unsigned long long level) {this->verbose_level = level;}
        unsigned long long get_verbose_level() {return this->verbose_level;}

        void set_extern_inp_sym_table (std::shared_ptr<SymbolTable> _extern_inp_sym_table) { extern_inp_sym_table = _extern_inp_sym_table; }
        std::shared_ptr<SymbolTable> get_extern_inp_sym_table () { return extern_inp_sym_table; }
        void set_extern_out_sym_table (std::shared_ptr<SymbolTable> _extern_out_sym_table) { extern_out_sym_table = _extern_out_sym_table; }
        std::shared_ptr<SymbolTable> get_extern_out_sym_table () { return extern_out_sym_table; }
        std::shared_ptr<SymbolTable> get_global_sym_table () { return global_sym_table; }
        std::shared_ptr<Context> get_parent_ctx () { return parent_ctx; }

        void setScopeId(unsigned long long id_) {this->scope_id = id_;}
        unsigned long long getScopeId() {return this->scope_id;}

    private:
        GenPolicy self_gen_policy;
        std::shared_ptr<Stmt> self_glob_stmt;
        Node::NodeID self_glob_stmt_id;
        std::shared_ptr<SymbolTable> extern_inp_sym_table;
        std::shared_ptr<SymbolTable> extern_out_sym_table;
        std::shared_ptr<SymbolTable> global_sym_table;
        std::shared_ptr<Context> parent_ctx;
        unsigned long long scope_id;
        unsigned long long complexity;
        unsigned long long verbose_level;
        int if_depth;
        int loop_depth;
        int depth;
};
}
