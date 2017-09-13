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
    for (uint32_t j = 0; j < struct_var->get_member_count(); ++j) {
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

void SymbolTable::emit_variable_extern_decl (std::ostream& stream, std::string offset) {
    for (const auto &i : variable) {
        DeclStmt decl (i, nullptr, true);
        stream << offset;
        decl.emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_variable_def (std::ostream& stream, std::string offset) {
    for (const auto &i : variable) {
        std::shared_ptr<ConstExpr> const_init = std::make_shared<ConstExpr>(i->get_init_value());

        std::shared_ptr<DeclStmt> decl = std::make_shared<DeclStmt>(i, const_init);
        stream << offset;
        decl->emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_struct_type_static_memb_def (std::ostream& stream, std::string offset) {
    for (const auto &i : struct_type) {
        stream << i->get_static_memb_def() + "\n";
    }
}

void SymbolTable::emit_struct_type_def (std::ostream& stream, std::string offset) {
    for (const auto &i : struct_type) {
        stream << offset + i->get_definition() + "\n";
    }
}

void SymbolTable::emit_struct_def (std::ostream& stream, std::string offset) {
    for (const auto &i : structs) {
        DeclStmt decl (i, nullptr, false);
        stream << offset;
        decl.emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_struct_extern_decl (std::ostream& stream, std::string offset) {
    for (const auto &i : structs) {
        DeclStmt decl (i, nullptr, true);
        stream << offset;
        decl.emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_struct_init (std::ostream& stream, std::string offset) {
    for (const auto &i : structs)
        emit_single_struct_init(nullptr, i, stream, offset);
}

void SymbolTable::emit_single_struct_init (std::shared_ptr<MemberExpr> parent_memb_expr,
                                                  std::shared_ptr<Struct> struct_var,
                                                  std::ostream& stream, std::string offset) {
    for (uint64_t j = 0; j < struct_var->get_member_count(); ++j) {
        std::shared_ptr<MemberExpr> member_expr;
        if  (parent_memb_expr != nullptr)
            member_expr = std::make_shared<MemberExpr>(parent_memb_expr, j);
        else
            member_expr = std::make_shared<MemberExpr>(struct_var, j);

        if (struct_var->get_member(j)->get_type()->is_struct_type()) {
            emit_single_struct_init(member_expr, std::static_pointer_cast<Struct>(struct_var->get_member(j)),
                                    stream, offset);
        }
        else {
            std::shared_ptr<ConstExpr> const_init = std::make_shared<ConstExpr>(std::static_pointer_cast<ScalarVariable>(struct_var->get_member(j))->get_init_value());
            AssignExpr assign (member_expr, const_init, false);
            stream << offset;
            assign.emit(stream);
            stream << ";\n";
        }
    }
}

void SymbolTable::emit_struct_check (std::ostream& stream, std::string offset) {
    for (const auto &i : structs)
        emit_single_struct_check(nullptr, i, stream, offset);
}

void SymbolTable::emit_single_struct_check (std::shared_ptr<MemberExpr> parent_memb_expr,
                                            std::shared_ptr<Struct> struct_var,
                                            std::ostream& stream,
                                            std::string offset) {
    for (uint64_t j = 0; j < struct_var->get_member_count(); ++j) {
        std::shared_ptr<MemberExpr> member_expr;
        if  (parent_memb_expr != nullptr)
            member_expr = std::make_shared<MemberExpr>(parent_memb_expr, j);
        else
            member_expr = std::make_shared<MemberExpr>(struct_var, j);

        if (struct_var->get_member(j)->get_type()->is_struct_type())
            emit_single_struct_check(member_expr, std::static_pointer_cast<Struct>(struct_var->get_member(j)),
                                     stream, offset);
        else {
            stream << offset + "hash(&seed, ";
            member_expr->emit(stream);
            stream << ");\n";
        }
    }
}

void SymbolTable::emit_variable_check (std::ostream& stream, std::string offset) {
    for (const auto &i : variable) {
        stream << offset + "hash(&seed, " + i->get_name() + ");\n";
    }
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
