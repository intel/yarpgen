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

#include <cassert>

#include "sym_table.h"

using namespace rl;

std::shared_ptr<SymbolTable> SymbolTable::merge (std::shared_ptr<SymbolTable> inp_st) {
    SymbolTable new_st;

    assert (this != NULL && "SymbolTable::merge failed");
    new_st.set_variables(this->get_variables());
//    new_st.set_struct_types(this->get_struct_types());
//    new_st.set_structs(this->get_structs());

    if (inp_st != NULL) {
        for (auto i : inp_st->get_variables())
            new_st.add_variable(i);
//        for (auto i : inp_st->get_struct_types())
//            new_st.add_struct_type(i);
//        for (auto i : inp_st->get_structs())
//            new_st.add_struct(i);
    }
    return std::make_shared<SymbolTable> (new_st);
}


std::string SymbolTable::emit_variable_extern_decl (std::string offset) {
    std::string ret = "";
    for (auto i : variable) {
        DeclStmt decl (i, NULL, true);
        ret += offset + decl.emit() + "\n";
    }
    return ret;
}

std::string SymbolTable::emit_variable_def (std::string offset) {
    std::string ret = "";
    for (auto i : variable) {
        std::shared_ptr<ConstExpr> const_init = std::make_shared<ConstExpr>(i->get_init_value());

        std::shared_ptr<DeclStmt> decl = std::make_shared<DeclStmt>(i, const_init);
        ret += offset + decl->emit() + "\n";
    }
    return ret;
}
/*
std::string SymbolTable::emit_struct_type_def (std::string offset) {
    std::string ret = "";
    for (auto i : struct_type) {
        ret += offset + i->get_definition() + "\n";
    }
    return ret;
}

std::string SymbolTable::emit_struct_def (std::string offset) {
    std::string ret = "";
    for (auto i : structs) {
        DeclStmt decl;
        decl.set_data(i);
        decl.set_is_extern(false);
        ret += offset + decl.emit() + "\n";
    }
    return ret;
}

std::string SymbolTable::emit_struct_extern_decl (std::string offset) {
    std::string ret = "";
    for (auto i : structs) {
        DeclStmt decl;
        decl.set_data(i);
        decl.set_is_extern(true);
        ret += offset + decl.emit() + "\n";
    }
    return ret;
}

std::string SymbolTable::emit_struct_init (std::string offset) {
    std::string ret = "";
    for (auto i : structs) {
        ret += emit_single_struct_init(NULL, i, offset);
    }
    return ret;
}

std::string SymbolTable::emit_single_struct_init (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, std::string offset) {
    std::string ret = "";
    for (int j = 0; j < struct_var->get_num_of_members(); ++j) {
        MemberExpr member_expr;
        member_expr.set_struct(struct_var);
        member_expr.set_identifier(j);
        member_expr.set_member_expr(parent_memb_expr);

        if (struct_var->get_member(j)->get_type()->is_struct_type())
            ret += emit_single_struct_init(std::make_shared<MemberExpr> (member_expr), std::static_pointer_cast<Struct>(struct_var->get_member(j)), offset);
        else {
            ConstExpr const_init;
            const_init.set_type (std::static_pointer_cast<IntegerType>(struct_var->get_member(j)->get_type())->get_int_type_id());
            const_init.set_data (struct_var->get_member(j)->get_value());

            AssignExpr assign;
            assign.set_to (std::make_shared<MemberExpr> (member_expr));
            assign.set_from (std::make_shared<ConstExpr> (const_init));

            ret += offset + assign.emit() + ";\n";
        }
    }
    return ret;
}
*/
std::string SymbolTable::emit_variable_check (std::string offset) {
    std::string ret = "";
    for (auto i = variable.begin(); i != variable.end(); ++i) {
        ret += offset + "hash(seed, " + (*i)->get_name() + ");\n";
    }
    return ret;
}

Context::Context (GenPolicy _gen_policy, std::shared_ptr<Context> _parent_ctx) {
    gen_policy = _gen_policy;
    parent_ctx = _parent_ctx;
    local_sym_table = std::make_shared<SymbolTable>();
    depth = 0;
    if_depth = 0;

    if (parent_ctx != NULL) {
        extern_inp_sym_table = parent_ctx->get_extern_inp_sym_table ();
        extern_out_sym_table = parent_ctx->get_extern_out_sym_table ();
        extern_mix_sym_table = parent_ctx->get_extern_mix_sym_table();
        depth = parent_ctx->get_depth() + 1;
        if_depth = parent_ctx->get_if_depth();
        //TODO: It should be parent of scope statement
        if (parent_ctx->get_self_stmt_id() == Node::NodeID::IF)
            if_depth++;
    }
}

