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

        auto& get_variables () { return variable; }
        auto& get_struct_types () { return struct_type; }
        auto& get_structs () { return structs; }
        auto& get_avail_members() { return avail_members; }
        auto& get_avail_const_members() { return avail_const_members; }
        void del_avail_member(int idx) { avail_members.erase(avail_members.begin() + idx); }

        void emit_variable_extern_decl (std::ostream& stream, std::string offset = "");
        void emit_variable_def (std::ostream& stream, std::string offset = "");
        // TODO: rewrite with IR
        void emit_variable_check (std::ostream& stream, std::string offset = "");
        void emit_struct_type_static_memb_def (std::ostream& stream, std::string offset = "");
        void emit_struct_type_def (std::ostream& stream, std::string offset = "");
        void emit_struct_def (std::ostream& stream, std::string offset = "");
        void emit_struct_extern_decl (std::ostream& stream, std::string offset = "");
        void emit_struct_init (std::ostream& stream, std::string offset = "");
        void emit_struct_check (std::ostream& stream, std::string offset = "");

    private:
        void form_struct_member_expr (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, bool ignore_const = false);
        void emit_single_struct_init (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var,
                                      std::ostream& stream, std::string offset = "");
        void emit_single_struct_check (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var,
                                       std::ostream& stream, std::string offset = "");

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
        auto get_gen_policy () { return gen_policy; }
        uint32_t get_depth () { return depth; }
        uint32_t get_if_depth () { return if_depth; }
        auto get_self_stmt_id () { return self_stmt_id; }
        bool get_taken () { return taken; }
        void set_extern_inp_sym_table (std::shared_ptr<SymbolTable> st) { extern_inp_sym_table = st; }
        auto get_extern_inp_sym_table () { return extern_inp_sym_table; }
        void set_extern_out_sym_table (std::shared_ptr<SymbolTable> st) { extern_out_sym_table = st; }
        auto get_extern_out_sym_table () { return extern_out_sym_table; }
        void set_extern_mix_sym_table (std::shared_ptr<SymbolTable> st) { extern_mix_sym_table = st; }
        auto get_extern_mix_sym_table () { return extern_mix_sym_table; }

        auto get_local_sym_table () { return local_sym_table; }
        void set_local_sym_table (std::shared_ptr<SymbolTable> _lst) { local_sym_table = _lst; }
        auto get_parent_ctx () { return parent_ctx; }

    private:
        std::shared_ptr<GenPolicy> gen_policy;

        std::shared_ptr<SymbolTable> extern_inp_sym_table;
        std::shared_ptr<SymbolTable> extern_out_sym_table;
        std::shared_ptr<SymbolTable> extern_mix_sym_table;
        //TODO: what about static variables?

        std::shared_ptr<Context> parent_ctx;
        std::shared_ptr<SymbolTable> local_sym_table;
        Node::NodeID self_stmt_id;
        uint32_t if_depth;
        uint32_t depth;
        bool taken;
        //TODO: maybe we should add taken member?
};
}
