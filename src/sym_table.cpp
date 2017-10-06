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
#include <sstream>

#include "sym_table.h"
#include "util.h"

using namespace yarpgen;


void SymbolTable::add_variable (std::shared_ptr<ScalarVariable> _var) {
    variable.push_back (_var);
    // We also need to store ReferenceExpr to this variable
    std::shared_ptr<VarUseExpr> var_use_expr = std::make_shared<VarUseExpr>(_var);
    std::shared_ptr<ReferenceExpr> var_ref_expr = std::make_shared<ReferenceExpr>(var_use_expr);
    std::shared_ptr<PointerType> ptr_type = std::static_pointer_cast<PointerType>(var_ref_expr->get_value()->get_type());
    std::string ptr_key = ptr_type->get_name() + ptr_type->get_type_suffix();
    all_expr_with_ptr_type[ptr_key].push_back(var_ref_expr);
}

void SymbolTable::add_struct (std::shared_ptr<Struct> _struct) {
    structs.push_back(_struct);
    form_struct_member_expr(members_in_structs, nullptr, _struct);
}

void SymbolTable::form_struct_member_expr (std::tuple<MemberVector, MemberVector>& ret,
                                           std::shared_ptr<MemberExpr> parent_memb_expr,
                                           std::shared_ptr<Struct> struct_var,
                                           bool ignore_const) {
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
                form_struct_member_expr(ret, member_expr, std::static_pointer_cast<Struct>(struct_var->get_member(j)),
                                        is_static || ignore_const);
            }
            else {
                std::get<ALL>(ret).push_back(member_expr);
                if (!is_static && !ignore_const) {
                    std::get<CONST>(ret).push_back(member_expr);
                }

                // Can't take address of bit-field
                if (member_expr->get_value()->get_type()->get_is_bit_field())
                    continue;
                // We also need to store ReferenceExpr to this MemberExpr
                std::shared_ptr<ReferenceExpr> memb_ref_expr = std::make_shared<ReferenceExpr>(member_expr);
                std::shared_ptr<PointerType> ptr_type = std::static_pointer_cast<PointerType>(memb_ref_expr->get_value()->get_type());
                std::string ptr_key = ptr_type->get_name() + ptr_type->get_type_suffix();
                all_expr_with_ptr_type[ptr_key].push_back(memb_ref_expr);

            }
        }
    }
}

void SymbolTable::add_array (std::shared_ptr<Array> _array) {
    std::shared_ptr<ArrayType> new_array_type = std::static_pointer_cast<ArrayType>(_array->get_type());
    array.push_back(_array);
    std::shared_ptr<Type> base_type = new_array_type->get_base_type();
    if (new_array_type->get_base_type()->is_struct_type())
        for (int i = 0; i < _array->get_elements_count(); ++i)
            form_struct_member_expr(members_in_arrays, nullptr, std::static_pointer_cast<Struct>(_array->get_element(i)));
}


std::shared_ptr<DereferenceExpr> SymbolTable::deep_deref_expr_from_nest_ptr(std::shared_ptr<DereferenceExpr> expr) {
    if (!expr->get_value()->get_type()->is_ptr_type())
        return expr;
    // We store DereferenceExpr at each level
    std::shared_ptr<PointerType> ptr_type = std::static_pointer_cast<PointerType>(expr->get_value()->get_type());
    std::string ptr_key = ptr_type->get_name() + ptr_type->get_type_suffix();
    add_ptr_map_key(ptr_key);
    lval_expr_with_ptr_type[ptr_key].push_back(expr);
    all_expr_with_ptr_type [ptr_key].push_back(expr);

    return deep_deref_expr_from_nest_ptr(std::make_shared<DereferenceExpr>(expr));
}

void SymbolTable::add_ptr_map_key (std::string& key) {
    if (std::find(lval_ptr_map_keys.begin(), lval_ptr_map_keys.end(), key) == lval_ptr_map_keys.end())
        lval_ptr_map_keys.push_back(key);
}

void SymbolTable::add_pointer(std::shared_ptr<Pointer> ptr, std::shared_ptr<Expr> init_expr) {
    if (init_expr->get_id() != Node::NodeID::VAR_USE && init_expr->get_id() != Node::NodeID::MEMBER)
        ERROR("can add only VarUseExpr or MemberExpr");
    pointers.ptr.push_back(ptr);
    pointers.init_expr.push_back(std::make_shared<ReferenceExpr>(init_expr));

    // For every pointer we need to store pointer itself
    std::shared_ptr<VarUseExpr> ptr_use_expr = std::make_shared<VarUseExpr>(ptr);
    std::shared_ptr<PointerType> ptr_type = std::static_pointer_cast<PointerType>(ptr->get_type());
    std::string ptr_key = ptr_type->get_name() + ptr_type->get_type_suffix();
    add_ptr_map_key(ptr_key);
    lval_expr_with_ptr_type[ptr_key].push_back(ptr_use_expr);
    all_expr_with_ptr_type [ptr_key].push_back(ptr_use_expr);

    // Also we need to store ReferenceExpr to it
    std::shared_ptr<ReferenceExpr> ptr_ref_expr = std::make_shared<ReferenceExpr>(ptr_use_expr);
    ptr_type = std::static_pointer_cast<PointerType>(ptr_ref_expr->get_value()->get_type());
    ptr_key = ptr_type->get_name() + ptr_type->get_type_suffix();
    all_expr_with_ptr_type[ptr_key].push_back(ptr_ref_expr);

    // And also all DereferenceExpr
    std::shared_ptr<DereferenceExpr> deref_expr = std::make_shared<DereferenceExpr>(ptr_use_expr);
    pointers.deref_expr.push_back(deep_deref_expr_from_nest_ptr(deref_expr));
}

void SymbolTable::del_member_in_structs(int idx) {
    auto& member_exprs = std::get<ALL>(members_in_structs);
    member_exprs.erase(member_exprs.begin() + idx);
}

void SymbolTable::del_member_in_arrays(int idx) {
    auto& member_exprs = std::get<ALL>(members_in_arrays);
    member_exprs.erase(member_exprs.begin() + idx);
}

void SymbolTable::var_use_exprs_from_vars_in_arrays (std::vector<std::shared_ptr<Expr>>& ret, bool ignore_tmp_objs) {
    for (auto const& array_iter : array) {
        std::shared_ptr<ArrayType> array_iter_type = std::static_pointer_cast<ArrayType>(array_iter->get_type());
        if (array_iter_type->get_base_type()->is_int_type()) {
            if (ignore_tmp_objs && array_iter_type->get_kind() == ArrayType::Kind::STD_VEC &&
                array_iter_type->get_base_type()->get_int_type_id() == IntegerType::IntegerTypeID::BOOL)
                continue;

            for (int i = 0; i < array_iter->get_elements_count(); ++i)
                ret.emplace_back(std::make_shared<VarUseExpr>(array_iter->get_element(i)));
        }
    }
}

SymbolTable::ExprVector SymbolTable::get_var_use_exprs_from_vars () {
    std::vector<std::shared_ptr<Expr>> ret;
    for (auto const& i : variable)
        ret.emplace_back(std::make_shared<VarUseExpr>(i));
    return ret;
}

SymbolTable::ExprVector SymbolTable::get_var_use_exprs_in_arrays () {
    std::vector<std::shared_ptr<Expr>> ret;
    var_use_exprs_from_vars_in_arrays(ret);
    return ret;
}

SymbolTable::ExprVector SymbolTable::get_all_var_use_exprs(bool ignore_tmp_objs) {
    std::vector<std::shared_ptr<Expr>> ret = get_var_use_exprs_from_vars();
    var_use_exprs_from_vars_in_arrays(ret, ignore_tmp_objs);
    return ret;
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

void SymbolTable::emit_variable_check (std::ostream& stream, std::string offset) {
    for (const auto &i : variable) {
        stream << offset + "hash(&seed, " + i->get_name() + ");\n";
    }
}

void SymbolTable::emit_struct_type_static_memb_def (std::ostream& stream, std::string offset) {
    for (const auto &i : struct_type) {
        stream << i->get_static_memb_def() + "\n";
    }
}

void SymbolTable::emit_struct_type_static_memb_init (std::ostream& stream, std::string offset) {
    for (const auto &i : struct_type)
        stream << i->get_static_memb_init(offset) + "\n";
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

void SymbolTable::emit_array_extern_decl (std::ostream& stream, std::string offset) {
    for (const auto &i : array) {
        DeclStmt decl (i, nullptr, true);
        stream << offset;
        decl.emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_array_def (std::ostream& stream, std::string offset) {
    for (const auto &i : array) {
        std::shared_ptr<StubExpr> stub_init = nullptr;
        std::shared_ptr<ArrayType> array_type = std::static_pointer_cast<ArrayType>(i->get_type());
        if (options->is_cxx() && options->standard_id <= Options::CXX03 &&
           (array_type->get_kind() == ArrayType::STD_VEC || array_type->get_kind() == ArrayType::VAL_ARR)) {
            std::shared_ptr<ArrayType> c_array_type = std::make_shared<ArrayType>(array_type->get_base_type(),
                                                                                  array_type->get_size(),
                                                                                  ArrayType::C_ARR);
            std::string name = "tmp_" + i->get_name();
            std::shared_ptr<Array> tmp_array = std::make_shared<Array>(name, c_array_type);
            tmp_array->set_elements(i->get_elements());

            std::shared_ptr<DeclStmt> tmp_decl = std::make_shared<DeclStmt>(tmp_array, nullptr);
            stream << offset;
            tmp_decl->emit(stream);
            stream << "\n";

            std::stringstream stub_str_stream;
            if (array_type->get_kind() == ArrayType::STD_VEC) {
                stub_str_stream << name << ", " << name << " + sizeof(" << name << ") / ";
                stub_str_stream << "sizeof(" + c_array_type->get_base_type()->get_simple_name() << ")";
            }
            else if (array_type->get_kind() == ArrayType::VAL_ARR) {
                stub_str_stream << name << ", " << c_array_type->get_size();
            }
            else
                ERROR("bad array kind");
            stub_init = std::make_shared<StubExpr>(stub_str_stream.str());
        }
        std::shared_ptr<DeclStmt> decl = std::make_shared<DeclStmt>(i, stub_init);
        stream << offset;
        decl->emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_array_check (std::ostream& stream, std::string offset) {
    for (const auto &i : array)
        for (int j = 0; j < i->get_elements_count(); ++j) {
            std::shared_ptr<Data> array_elem = i->get_element(j);
            switch (array_elem->get_class_id()) {
                case Data::VAR:
                    stream << offset + "hash(&seed, " + array_elem->get_name() + ");\n";
                    break;
                case Data::STRUCT:
                    emit_single_struct_check(nullptr, std::static_pointer_cast<Struct>(array_elem), stream, offset);
                    break;
                case Data::POINTER:
                case Data::ARRAY:
                case Data::MAX_CLASS_ID:
                    ERROR("inappropriate Data class for array");
            }
        }
}

void SymbolTable::emit_ptr_extern_decl (std::ostream& stream, std::string offset) {
    for (int i = 0; i < pointers.ptr.size(); ++i) {
        DeclStmt decl (pointers.ptr.at(i), nullptr, true);
        stream << offset;
        decl.emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_ptr_def (std::ostream& stream, std::string offset) {
    for (int i = 0; i < pointers.ptr.size(); ++i) {
        DeclStmt decl (pointers.ptr.at(i), pointers.init_expr.at(i));
        stream << offset;
        decl.emit(stream);
        stream << "\n";
    }
}

void SymbolTable::emit_ptr_check (std::ostream& stream, std::string offset) {
    for (int i = 0; i < pointers.ptr.size(); ++i) {
        stream << offset + "hash(&seed, ";
        pointers.deref_expr.at(i)->emit(stream);
        stream << ");\n";
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
