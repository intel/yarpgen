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

#include <cassert>

#include "sym_table.h"

using namespace yarpgen;


void SymbolTable::add_struct (std::shared_ptr<Struct> _struct) {
    structs.push_back(_struct);
    form_struct_member_expr(nullptr, _struct);
}

void SymbolTable::form_struct_member_expr (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, bool ignore_const) {
    for (int j = 0; j < struct_var->get_member_count(); ++j) {
        GenPolicy gen_policy;
        if (rand_val_gen->get_rand_id(gen_policy.get_member_use_prob())) {
            std::shared_ptr<MemberExpr> member_expr;
            if (parent_memb_expr != nullptr)
                member_expr = std::make_shared<MemberExpr>(parent_memb_expr, j);
            else
                member_expr = std::make_shared<MemberExpr>(struct_var, j);

            bool is_static = struct_var->get_member(j)->get_type()->get_is_static();

            if (struct_var->get_member(j)->get_type()->is_struct_type()) {
                form_struct_member_expr(member_expr, std::static_pointer_cast<Struct>(struct_var->get_member(j)), is_static || ignore_const);
            }
            else {
                avail_members.push_back(member_expr);
                if (!is_static && !ignore_const) {
                    avail_const_members.push_back(member_expr);
                }
            }
        }
    }
}

std::string SymbolTable::emit_variable_extern_decl (std::string offset) {
    std::string ret = "";
    for (auto i : variable) {
        DeclStmt decl (i, nullptr, true);
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

std::string SymbolTable::emit_struct_type_static_memb_def (std::string offset) {
    std::string ret = "";
    for (auto i : struct_type) {
        ret += i->get_static_memb_def() + "\n";
    }
    return ret;
}

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
        DeclStmt decl (i, nullptr, false);
        ret += offset + decl.emit() + "\n";
    }
    return ret;
}

std::string SymbolTable::emit_struct_extern_decl (std::string offset) {
    std::string ret = "";
    for (auto i : structs) {
        DeclStmt decl (i, nullptr, true);
        ret += offset + decl.emit() + "\n";
    }
    return ret;
}

std::string SymbolTable::emit_struct_init (std::string offset) {
    std::string ret = "";
    for (auto i : structs) {
        ret += emit_single_struct_init(nullptr, i, offset);
    }
    return ret;
}

std::string SymbolTable::emit_single_struct_init (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, std::string offset) {
    std::string ret = "";
    for (int j = 0; j < struct_var->get_member_count(); ++j) {
        std::shared_ptr<MemberExpr> member_expr;
        if  (parent_memb_expr != nullptr)
            member_expr = std::make_shared<MemberExpr>(parent_memb_expr, j);
        else
            member_expr = std::make_shared<MemberExpr>(struct_var, j);

        if (struct_var->get_member(j)->get_type()->is_struct_type()) {
            ret += emit_single_struct_init(member_expr, std::static_pointer_cast<Struct>(struct_var->get_member(j)), offset);
        }
        else {
            std::shared_ptr<ConstExpr> const_init = std::make_shared<ConstExpr>(std::static_pointer_cast<ScalarVariable>(struct_var->get_member(j))->get_init_value());
            AssignExpr assign (member_expr, const_init, false);
            ret += offset + assign.emit() + ";\n";
        }
    }
    return ret;
}

std::string SymbolTable::emit_struct_check (std::string offset) {
    std::string ret = "";
    for (auto i : structs) {
        ret += emit_single_struct_check(nullptr, i, offset);
    }
    return ret;
}

std::string SymbolTable::emit_single_struct_check (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, std::string offset) {
    std::string ret = "";
    for (int j = 0; j < struct_var->get_member_count(); ++j) {
        std::shared_ptr<MemberExpr> member_expr;
        if  (parent_memb_expr != nullptr)
            member_expr = std::make_shared<MemberExpr>(parent_memb_expr, j);
        else
            member_expr = std::make_shared<MemberExpr>(struct_var, j);

        if (struct_var->get_member(j)->get_type()->is_struct_type())
            ret += emit_single_struct_check(member_expr, std::static_pointer_cast<Struct>(struct_var->get_member(j)), offset);
        else
            ret += offset + "hash(seed, " + member_expr->emit() + ");\n";
    }
    return ret;
}

std::string SymbolTable::emit_variable_check (std::string offset) {
    std::string ret = "";
    for (auto i = variable.begin(); i != variable.end(); ++i) {
        ret += offset + "hash(seed, " + (*i)->get_name() + ");\n";
    }
    return ret;
}

Context::Context (GenPolicy _gen_policy, std::shared_ptr<Context> _parent_ctx, Node::NodeID _self_stmt_id, bool _taken) {
    gen_policy = std::make_shared<GenPolicy>(_gen_policy);
    parent_ctx = _parent_ctx;
    local_sym_table = std::make_shared<SymbolTable>();
    depth = 0;
    if_depth = 0;
    self_stmt_id = _self_stmt_id;
    taken = _taken;

    if (parent_ctx != nullptr) {
        extern_inp_sym_table = parent_ctx->get_extern_inp_sym_table ();
        extern_out_sym_table = parent_ctx->get_extern_out_sym_table ();
        extern_mix_sym_table = parent_ctx->get_extern_mix_sym_table();
        depth = parent_ctx->get_depth() + 1;
        if_depth = parent_ctx->get_if_depth();
        taken &= parent_ctx->get_taken();
        //TODO: It should be parent of scope statement
        if (parent_ctx->get_self_stmt_id() == Node::NodeID::IF)
            if_depth++;
    }
}
