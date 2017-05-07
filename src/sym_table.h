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

#include <memory>

#include "gen_policy.h"
#include "variable.h"
#include "ir_node.h"

///////////////////////////////////////////////////////////////////////////////

namespace yarpgen {

class SymbolTable {
    public:
        SymbolTable () {}

        void add_variable (std::shared_ptr<ScalarVariable> _var) { variable.push_back (_var); }
        void add_struct_type (std::shared_ptr<StructType> _type) { struct_type.push_back (_type); }
        void add_struct (std::shared_ptr<Struct> _struct);

        void set_variables (std::vector<std::shared_ptr<ScalarVariable>> _variable) { variable = _variable; }
        void set_struct_types (std::vector<std::shared_ptr<StructType>> _struct_type) { struct_type = _struct_type; }
        void set_structs (std::vector<std::shared_ptr<Struct>> _structs) { structs = _structs; }

        std::vector<std::shared_ptr<ScalarVariable>>& get_variables () { return variable; }
        std::vector<std::shared_ptr<StructType>>& get_struct_types () { return struct_type; }
        std::vector<std::shared_ptr<Struct>>& get_structs () { return structs; }
        std::vector<std::shared_ptr<MemberExpr>>& get_avail_members() { return avail_members; }
        std::vector<std::shared_ptr<MemberExpr>>& get_avail_const_members() { return avail_const_members; }
        void del_avail_member(int idx) { avail_members.erase(avail_members.begin() + idx); }

        std::string emit_variable_extern_decl (std::string offset = "");
        std::string emit_variable_def (std::string offset = "");
        // TODO: rewrite with IR
        std::string emit_variable_check (std::string offset = "");
        std::string emit_struct_type_static_memb_def (std::string offset = "");
        std::string emit_struct_type_def (std::string offset = "");
        std::string emit_struct_def (std::string offset = "");
        std::string emit_struct_extern_decl (std::string offset = "");
        std::string emit_struct_init (std::string offset = "");
        std::string emit_struct_check (std::string offset = "");

    private:
        void form_struct_member_expr (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, bool ignore_const = false);
        std::string emit_single_struct_init (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, std::string offset = "");
        std::string emit_single_struct_check (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, std::string offset = "");

        std::vector<std::shared_ptr<StructType>> struct_type;
        std::vector<std::shared_ptr<Struct>> structs;
        std::vector<std::shared_ptr<MemberExpr>> avail_members;
        std::vector<std::shared_ptr<MemberExpr>> avail_const_members; // TODO: it is a stub, because now static members can't be const input in CSE gen
        std::vector<std::shared_ptr<ScalarVariable>> variable;
};

class Context {
    public:
        Context (GenPolicy _gen_policy, std::shared_ptr<Context> _parent_ctx, Node::NodeID _self_stmt_id, bool _taken);

        void set_gen_policy (GenPolicy _gen_policy) { gen_policy = std::make_shared<GenPolicy>(_gen_policy); }
        std::shared_ptr<GenPolicy> get_gen_policy () { return gen_policy; }
        int get_depth () { return depth; }
        int get_if_depth () { return if_depth; }
        Node::NodeID get_self_stmt_id () { return self_stmt_id; }
        bool get_taken () { return taken; }
        void set_extern_inp_sym_table (std::shared_ptr<SymbolTable> _extern_inp_sym_table) { extern_inp_sym_table = _extern_inp_sym_table; }
        std::shared_ptr<SymbolTable> get_extern_inp_sym_table () { return extern_inp_sym_table; }
        void set_extern_out_sym_table (std::shared_ptr<SymbolTable> _extern_out_sym_table) { extern_out_sym_table = _extern_out_sym_table; }
        std::shared_ptr<SymbolTable> get_extern_out_sym_table () { return extern_out_sym_table; }
        void set_extern_mix_sym_table (std::shared_ptr<SymbolTable> _extern_mix_sym_table) { extern_mix_sym_table = _extern_mix_sym_table; }
        std::shared_ptr<SymbolTable> get_extern_mix_sym_table () { return extern_mix_sym_table; }

        std::shared_ptr<SymbolTable> get_local_sym_table () { return local_sym_table; }
        void set_local_sym_table (std::shared_ptr<SymbolTable> _lst) { local_sym_table = _lst; }
        std::shared_ptr<Context> get_parent_ctx () { return parent_ctx; }

    private:
        std::shared_ptr<GenPolicy> gen_policy;

        std::shared_ptr<SymbolTable> extern_inp_sym_table;
        std::shared_ptr<SymbolTable> extern_out_sym_table;
        std::shared_ptr<SymbolTable> extern_mix_sym_table;
        //TODO: what about static variables?

        std::shared_ptr<Context> parent_ctx;
        std::shared_ptr<SymbolTable> local_sym_table;
        Node::NodeID self_stmt_id;
        int if_depth;
        int depth;
        bool taken;
        //TODO: maybe we should add taken member?
};
}
