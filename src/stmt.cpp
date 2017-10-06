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

#include "stmt.h"
#include "sym_table.h"
#include "util.h"

using namespace yarpgen;

uint32_t Stmt::total_stmt_count = 0;
uint32_t Stmt::func_stmt_count = 0;

// C++03 and previous versions doesn't allow to use list-initialization for vector and valarray,
// so we need to use StubExpr as init expression
static bool is_cxx03_and_special_arr_kind(std::shared_ptr<Data> data) {
    return options->is_cxx() && options->standard_id <= Options::CXX03 &&
           data->get_class_id() == Data::ARRAY &&
           (std::static_pointer_cast<ArrayType>(data->get_type())->get_kind() == ArrayType::STD_VEC ||
            std::static_pointer_cast<ArrayType>(data->get_type())->get_kind() == ArrayType::VAL_ARR);
}

DeclStmt::DeclStmt (std::shared_ptr<Data> _data, std::shared_ptr<Expr> _init, bool _is_extern) :
                  Stmt(Node::NodeID::DECL), data(_data), init(_init), is_extern(_is_extern) {
    if (init == nullptr || is_cxx03_and_special_arr_kind(data))
        return;
    if (is_extern)
        ERROR("init of extern var DeclStmt");
    // Declaration of new variable
    if (data->get_class_id() == Data::VarClassID::VAR) {
        if (init->get_value()->get_class_id() != Data::VarClassID::VAR)
            ERROR("can init only ScalarVariable or Pointer in DeclStmt");
        std::shared_ptr<ScalarVariable> data_var = std::static_pointer_cast<ScalarVariable>(data);
        std::shared_ptr<TypeCastExpr> cast_type = std::make_shared<TypeCastExpr>(init, data_var->get_type());
        data_var->set_init_value(std::static_pointer_cast<ScalarVariable>(cast_type->get_value())->get_cur_value());
    }
    // Declaration of new pointer
    else if (data->get_class_id() == Data::VarClassID::POINTER) {
        if (init->get_value()->get_class_id() != Data::VarClassID::POINTER)
            ERROR("can init only ScalarVariable or Pointer in DeclStmt");
        std::shared_ptr<Pointer> data_ptr = std::static_pointer_cast<Pointer>(data);
        if (!init->get_value()->get_type()->is_ptr_type())
            ERROR("init of pointer with expression which has non-pointer type");
        data_ptr->set_pointee(std::static_pointer_cast<Pointer>(init->get_value())->get_pointee());
    }
    else
        ERROR("can init only ScalarVariable or Pointer in DeclStmt");
}

// This function randomly creates new ScalarVariable, its initializing arithmetic expression and
// adds new variable to local_sym_table of parent Context
std::shared_ptr<DeclStmt> DeclStmt::generate (std::shared_ptr<Context> ctx,
                                              std::vector<std::shared_ptr<Expr>> inp,
                                              bool count_up_total) {
    Stmt::increase_stmt_count();
    GenPolicy::add_to_complexity(Node::NodeID::DECL);
    if (ctx->get_parent_ctx() == nullptr || ctx->get_parent_ctx()->get_local_sym_table() == nullptr)
        ERROR("no parent context or local_sym_table (DeclStmt)");

    std::shared_ptr<DeclStmt> ret;
    std::shared_ptr<Expr> new_init;

    // Generate declaration of new variable
    if (!inp.front()->get_value()->get_type()->is_ptr_type()) {
        std::shared_ptr<ScalarVariable> new_var = ScalarVariable::generate(ctx);
        new_init = ArithExpr::generate(ctx, inp);
        ret = std::make_shared<DeclStmt>(new_var, new_init);
        ctx->get_parent_ctx()->get_local_sym_table()->add_variable(new_var);
    }
    else {
        // Generate declaration of new pointer
        NameHandler& name_handler = NameHandler::get_instance();
        new_init = rand_val_gen->get_rand_elem(inp);

        // Pointer requires shared_ptr to original variable, so we should extract it
        std::shared_ptr<Data> raw_expr_data;
        if (new_init->get_id() == Node::NodeID::VAR_USE)
            raw_expr_data = std::static_pointer_cast<VarUseExpr>(new_init)->get_raw_value();
        else if (new_init->get_id() == Node::NodeID::MEMBER)
            raw_expr_data = std::static_pointer_cast<MemberExpr>(new_init)->get_raw_value();
        else
            raw_expr_data = new_init->get_value();

        // Nowadays we don't allow casting between pointer of different types, so they should have the same
        std::shared_ptr<PointerType> new_ptr_type = std::static_pointer_cast<PointerType>(raw_expr_data->get_type());
        std::shared_ptr<Pointer> new_ptr = std::make_shared<Pointer>(name_handler.get_ptr_var_name(), new_ptr_type);

        ret = std::make_shared<DeclStmt>(new_ptr, new_init);
        ctx->get_parent_ctx()->get_local_sym_table()->add_pointer(new_ptr, new_init);
    }

    if (count_up_total)
        Expr::increase_expr_count(new_init->get_complexity());
    return ret;
}

// This function creates list-initialization for structs in arrays
static void emit_list_init_for_struct(std::ostream &stream, std::shared_ptr<Struct> struct_elem) {
    stream << "{";
    uint64_t member_count = struct_elem->get_member_count();
    for (int i = 0; i < member_count; ++i) {
        std::shared_ptr<Data> member = struct_elem->get_member(i);
        // Skip static members
        if (member->get_type()->get_is_static())
            continue;

        switch (member->get_class_id()) {
            case Data::VAR: {
                std::shared_ptr<ScalarVariable> var_member = std::static_pointer_cast<ScalarVariable>(member);
                ConstExpr init_const(var_member->get_init_value());
                init_const.emit(stream);
            }
                break;
            case Data::STRUCT: {
                std::shared_ptr<Struct> struct_member = std::static_pointer_cast<Struct>(member);
                emit_list_init_for_struct(stream, struct_member);
            }
                break;
            case Data::POINTER:
            case Data::ARRAY:
            case Data::MAX_CLASS_ID:
                ERROR("inappropriate type of Struct member");
                break;
        }
        if (i < member_count - 1)
            stream << ", ";
    }
    stream << "} ";
}

void DeclStmt::emit (std::ostream& stream, std::string offset) {
    stream << offset;
    stream << (data->get_type()->get_is_static() && !is_extern ? "static " : "");
    stream << (is_extern ? "extern " : "");
    switch (data->get_type()->get_cv_qual()) {
        case Type::CV_Qual::VOLAT:
            stream << "volatile ";
            break;
        case Type::CV_Qual::CONST:
            stream << "const ";
            break;
        case Type::CV_Qual::CONST_VOLAT:
            stream << "const volatile ";
            break;
        case Type::CV_Qual::NTHG:
            break;
        case Type::CV_Qual::MAX_CV_QUAL:
            ERROR("bad cv_qual (DeclStmt)");
            break;
    }
    stream << data->get_type()->get_simple_name() + " " + data->get_name() + data->get_type()->get_type_suffix();
    if (data->get_type()->get_align() != 0 && is_extern) // TODO: Should we set __attribute__ to non-extern variable?
        stream << " __attribute__((aligned(" + std::to_string(data->get_type()->get_align()) + ")))";
    if (init != nullptr &&
       // C++03 and previous versions doesn't allow to use list-initialization for vector and valarray,
       // so we need to use StubExpr as init expression
       !is_cxx03_and_special_arr_kind(data)) {
        if (data->get_class_id() == Data::VarClassID::STRUCT ||
           (data->get_class_id() == Data::VarClassID::ARRAY && !is_cxx03_and_special_arr_kind(data))) {
            ERROR("emit init of struct (DeclStmt)");
        }
        if (is_extern) {
            ERROR("init of extern var (DeclStmt)");
        }
        stream << " = ";
        init->emit(stream);
    }
    if (data->get_class_id() == Data::VarClassID::ARRAY && !is_extern) {
        //TODO: it is a stub. We should use something to represent list-initialization.
        if (!is_cxx03_and_special_arr_kind(data)) {
            stream << " = {";
            std::shared_ptr<Array> array = std::static_pointer_cast<Array>(data);
            std::shared_ptr<ArrayType> array_type = std::static_pointer_cast<ArrayType>(array->get_type());
            uint64_t array_elements_count = array->get_elements_count();
            // std::array requires additional curly brackets in list-initialization
            if (array_type->get_kind() == ArrayType::STD_ARR)
                stream << "{";

            for (int i = 0; i < array_elements_count; ++i) {
                if (array_type->get_base_type()->is_int_type()) {
                    std::shared_ptr<ScalarVariable> elem = std::static_pointer_cast<ScalarVariable>(
                            array->get_element(i));
                    ConstExpr init_const(elem->get_init_value());
                    init_const.emit(stream);
                } else if (array_type->get_base_type()->is_struct_type()) {
                    std::shared_ptr<Struct> elem = std::static_pointer_cast<Struct>(array->get_element(i));
                    emit_list_init_for_struct(stream, elem);
                } else
                    ERROR("bad base type of array");
                if (i < array_elements_count - 1)
                    stream << ", ";
            }

            // std::array requires additional curly brackets in list-initialization
            if (array_type->get_kind() == ArrayType::STD_ARR)
                stream << "}";
            stream << "}";
        }
        else {
            // Same note about C++03 and previous versions
            stream << " (";
            std::static_pointer_cast<StubExpr>(init)->emit(stream);
            stream << ")";
        }
    }
    stream << ";";
}

// This function returns DereferenceExpr for nested pointers up to base variable
static std::shared_ptr<DereferenceExpr> deep_deref_expr_from_nest_ptr(std::shared_ptr<DereferenceExpr> expr) {
    if (!expr->get_value()->get_type()->is_ptr_type())
        return expr;
    return deep_deref_expr_from_nest_ptr(std::make_shared<DereferenceExpr>(expr));
}

// One of the most important generation methods (top-level generator for everything between curve brackets).
// It acts as a top-level dispatcher for other statement generation functions.
// Also it initially fills extern symbol table.
std::shared_ptr<ScopeStmt> ScopeStmt::generate (std::shared_ptr<Context> ctx) {
    GenPolicy::add_to_complexity(Node::NodeID::SCOPE);

    std::shared_ptr<ScopeStmt> ret = std::make_shared<ScopeStmt>();

    // Before the main generation loop starts, we need to extract from all contexts every input / mixed variable and structure.
    std::vector<std::shared_ptr<Expr>> inp = extract_inp_and_mix_from_ctx(ctx);

    //TODO: add to gen_policy stmt number
    auto p = ctx->get_gen_policy();
    uint32_t scope_stmt_count = rand_val_gen->get_rand_value(p->get_min_scope_stmt_count(),
                                                             p->get_max_scope_stmt_count());

    for (uint32_t i = 0; i < scope_stmt_count; ++i) {
        if (Stmt::total_stmt_count >= p->get_max_total_stmt_count() ||
            Stmt::func_stmt_count  >= p->get_max_func_stmt_count())
            //TODO: Can we somehow eliminate compiler timeout with the help of this?
            //GenPolicy::get_test_complexity() >= p->get_max_test_complexity())
            break;

        // Randomly decide if we want to create a new CSE
        GenPolicy::ArithCSEGenID add_cse = rand_val_gen->get_rand_id(p->get_arith_cse_gen());
        if (add_cse == GenPolicy::ArithCSEGenID::Add &&
           ((p->get_cse().size() - 1 < p->get_max_cse_count()) ||
            (p->get_cse().size() == 0))) {
            std::vector<std::shared_ptr<Expr>> cse_inp = extract_inp_from_ctx(ctx);
            p->add_cse(ArithExpr::generate(ctx, cse_inp));
        }

        // Randomly pick next Stmt ID
        Node::NodeID gen_id = rand_val_gen->get_rand_id(p->get_stmt_gen_prob());
        // ExprStmt
        if (gen_id == Node::NodeID::EXPR) {
            // Are we going to use mixed variable or create new output variable?
            bool use_mix = rand_val_gen->get_rand_id(p->get_out_data_category_prob()) ==
                           GenPolicy::OutDataCategoryID::MIX;
            GenPolicy::OutDataTypeID out_data_type = rand_val_gen->get_rand_id(p->get_out_data_type_prob());
            // Create assignment to pointer
            if (out_data_type == GenPolicy::OutDataTypeID::POINTER) {
                auto ptr_assign_generation = [&ret, &out_data_type, &ctx] (std::shared_ptr<SymbolTable> sym_table) {
                    // Extract pointer maps from symbol table
                    std::map<std::string, SymbolTable::ExprVector> &lval_map = sym_table->get_lval_expr_with_ptr_type();
                    std::map<std::string, SymbolTable::ExprVector> &all_map = sym_table->get_all_expr_with_ptr_type();
                    std::vector<std::string> ptr_type_keys = sym_table->get_lval_ptr_map_keys();
                    if (!ptr_type_keys.empty()) {
                        // Pass chosen data to ExprStmt::generate method
                        std::string chosen_key = rand_val_gen->get_rand_elem(ptr_type_keys);
                        std::shared_ptr<Expr> lhs = rand_val_gen->get_rand_elem(lval_map.at(chosen_key));
                        ret->add_stmt(ExprStmt::generate(ctx, all_map.at(chosen_key), lhs, true));
                    }
                    else
                        // We can't create assignment to pointer, so fall back to variable
                        out_data_type = GenPolicy::OutDataTypeID::VAR;
                };

                if (!use_mix)
                    ptr_assign_generation(ctx->get_extern_mix_sym_table());
                else
                    ptr_assign_generation(ctx->get_extern_out_sym_table());
            }
            // Create assignment to variable or member
            if (out_data_type != GenPolicy::OutDataTypeID::POINTER) {
                std::shared_ptr<Expr> assign_lhs = nullptr;

                // This function checks if we have any suitable members / variables / arrays
                auto check_ctx_for_zero_size = [&out_data_type](std::shared_ptr<SymbolTable> sym_table) -> bool {
                    return (out_data_type == GenPolicy::OutDataTypeID::VAR_IN_ARRAY &&
                            sym_table->get_var_use_exprs_in_arrays().empty()) ||
                           (out_data_type == GenPolicy::OutDataTypeID::MEMBER &&
                            sym_table->get_members_in_structs().empty()) ||
                           (out_data_type == GenPolicy::OutDataTypeID::MEMBER_IN_ARRAY &&
                            sym_table->get_members_in_arrays().empty()) ||
                           (out_data_type == GenPolicy::OutDataTypeID::DEREFERENCE &&
                            sym_table->get_deref_exprs().empty());
                };

                // This function randomly picks element from vector.
                // Also it optionally returns picked element's id in ret_rand_num
                auto pick_elem = [&assign_lhs](auto vector_of_exprs, uint32_t *ret_rand_num = nullptr) {
                    uint64_t rand_num = rand_val_gen->get_rand_value(0UL, vector_of_exprs.size() - 1);
                    assign_lhs = vector_of_exprs.at(rand_num);
                    if (ret_rand_num != nullptr)
                        *ret_rand_num = rand_num;
                };

                if (!use_mix || ctx->get_extern_mix_sym_table()->get_var_use_exprs_from_vars().empty()) {
                    bool zero_size = check_ctx_for_zero_size(ctx->get_extern_out_sym_table());

                    // Create new output variable or we don't have any suitable members / variables / arrays
                    if (out_data_type == GenPolicy::OutDataTypeID::VAR || zero_size) {
                        std::shared_ptr<ScalarVariable> out_var = ScalarVariable::generate(ctx);
                        ctx->get_extern_out_sym_table()->add_variable(out_var);
                        assign_lhs = std::make_shared<VarUseExpr>(out_var);
                    }
                        // Use variable in output array
                    else if (out_data_type == GenPolicy::OutDataTypeID::VAR_IN_ARRAY)
                        //TODO: we should also delete it (make it nonreusable)
                        pick_elem(ctx->get_extern_out_sym_table()->get_var_use_exprs_in_arrays());
                        // Use member of output struct
                    else if (out_data_type == GenPolicy::OutDataTypeID::MEMBER) {
                        uint32_t elem_num;
                        pick_elem(ctx->get_extern_out_sym_table()->get_members_in_structs(), &elem_num);
                        ctx->get_extern_out_sym_table()->del_member_in_structs(elem_num);
                    }
                        // Use member of struct in output array
                    else if (out_data_type == GenPolicy::OutDataTypeID::MEMBER_IN_ARRAY) {
                        uint32_t elem_num;
                        pick_elem(ctx->get_extern_out_sym_table()->get_members_in_arrays(), &elem_num);
                        ctx->get_extern_out_sym_table()->del_member_in_arrays(elem_num);
                    }
                        // Use dereference expression to output data
                    else if (out_data_type == GenPolicy::OutDataTypeID::DEREFERENCE)
                        //TODO: we should also delete it (make it nonreusable)
                        pick_elem(ctx->get_extern_out_sym_table()->get_deref_exprs());
                    else
                        ERROR("bad OutDataTypeID");

                } else {
                    bool zero_size = check_ctx_for_zero_size(ctx->get_extern_mix_sym_table());

                    // Use mixed variable or we don't have any suitable members / variables / arrays
                    if (out_data_type == GenPolicy::OutDataTypeID::VAR || zero_size)
                        pick_elem(ctx->get_extern_mix_sym_table()->get_var_use_exprs_from_vars());
                        // Use variable in mixed array
                    else if (out_data_type == GenPolicy::OutDataTypeID::VAR_IN_ARRAY)
                        pick_elem(ctx->get_extern_mix_sym_table()->get_var_use_exprs_in_arrays());
                        // Use member of mixed struct
                    else if (out_data_type == GenPolicy::OutDataTypeID::MEMBER)
                        pick_elem(ctx->get_extern_mix_sym_table()->get_members_in_structs());
                        // Use member of struct in mixed array
                    else if (out_data_type == GenPolicy::OutDataTypeID::MEMBER_IN_ARRAY)
                        pick_elem(ctx->get_extern_mix_sym_table()->get_members_in_arrays());
                        // Use dereference expression to mix data
                    else if (out_data_type == GenPolicy::OutDataTypeID::DEREFERENCE)
                        pick_elem(ctx->get_extern_mix_sym_table()->get_deref_exprs());
                    else
                        ERROR("bad OutDataTypeID");

                }
                ret->add_stmt(ExprStmt::generate(ctx, inp, assign_lhs, true));
            }
        }
        // DeclStmt or if we want IfStmt, but have reached its depth limit
        else if (gen_id == Node::NodeID::DECL || (ctx->get_if_depth() == p->get_max_if_depth())) {
            std::shared_ptr<Context> decl_ctx = std::make_shared<Context>(*(p), ctx, Node::NodeID::DECL, true);
            std::shared_ptr<DeclStmt> tmp_decl;

            GenPolicy::DeclStmtGenID decl_stmt_id = rand_val_gen->get_rand_id(p->get_decl_stmt_gen_id_prob());
            if (decl_stmt_id == GenPolicy::DeclStmtGenID::Pointer) {
                std::map<std::string, ExprVector> ptr_expr_map = extract_all_local_ptr_exprs(ctx);

                // Collect all keys from joined local maps
                std::vector<std::string> ptr_keys;
                for (auto const& i: ptr_expr_map)
                    ptr_keys.push_back(i.first);

                if (!ptr_keys.empty()) {
                    std::string chosen_ptr_key = rand_val_gen->get_rand_elem(ptr_keys);
                    std::vector<std::shared_ptr<Expr>> chosen_expr_vec = ptr_expr_map.at(chosen_ptr_key);
                    // Unite local ptr map with mixed
                    std::vector<std::shared_ptr<Expr>> mix_ptr_expr_vec = ctx->get_extern_mix_sym_table()->
                                                                               get_all_expr_with_ptr_type()[chosen_ptr_key];
                    chosen_expr_vec.insert(chosen_expr_vec.end(), mix_ptr_expr_vec.begin(), mix_ptr_expr_vec.end());
                    // Pass all chosen data to DeclStmt::generate
                    tmp_decl = DeclStmt::generate(decl_ctx, chosen_expr_vec, true);
                    std::shared_ptr<Pointer> tmp_ptr = std::static_pointer_cast<Pointer>(tmp_decl->get_data());
                    // Add new pointer to inp
                    std::shared_ptr<VarUseExpr> tmp_ptr_use = std::make_shared<VarUseExpr>(tmp_ptr);
                    inp.push_back(deep_deref_expr_from_nest_ptr(std::make_shared<DereferenceExpr>(tmp_ptr_use)));
                }
                else
                    // We can't create declaration for new pointer, so fall back to variable
                    decl_stmt_id = GenPolicy::DeclStmtGenID::Variable;
            }
            if (decl_stmt_id == GenPolicy::DeclStmtGenID::Variable) {
                tmp_decl = DeclStmt::generate(decl_ctx, inp, true);
                // Add created variable to inp
                std::shared_ptr<ScalarVariable> tmp_var = std::static_pointer_cast<ScalarVariable>(tmp_decl->get_data());
                inp.push_back(std::make_shared<VarUseExpr>(tmp_var));
            }
            ret->add_stmt(tmp_decl);
        }
        // IfStmt
        else if (gen_id == Node::NodeID::IF) {
            ret->add_stmt(IfStmt::generate(std::make_shared<Context>(*(p), ctx, Node::NodeID::IF, true), inp, true));
        }

    }
    return ret;
}

// This function extracts pointer map from local symbol table of current Context and all it's predecessors.
std::map<std::string, ScopeStmt::ExprVector> ScopeStmt::extract_all_local_ptr_exprs(std::shared_ptr<Context> ctx) {
    std::map<std::string, ExprVector> ret = ctx->get_local_sym_table()->get_all_expr_with_ptr_type();
    if (ctx->get_parent_ctx() != nullptr) {
        std::map<std::string, ExprVector> parent_map = extract_all_local_ptr_exprs(ctx->get_parent_ctx());
        for (auto& i : parent_map) {
            ScopeStmt::ExprVector& ret_vec = ret[i.first];
            ScopeStmt::ExprVector& parent_vec = i.second;
            ret_vec.insert(ret_vec.end(), parent_vec.begin(), parent_vec.end());
        }
    }
    return ret;
};

// CSE shouldn't change during the scope to make generation process easy. In order to achieve this,
// we use only "input" variables for them and this function extracts such variables from extern symbol table.
std::vector<std::shared_ptr<Expr>> ScopeStmt::extract_inp_from_ctx(std::shared_ptr<Context> ctx) {
    std::vector<std::shared_ptr<Expr>> ret = ctx->get_extern_inp_sym_table()->get_all_var_use_exprs();

    for (auto i : ctx->get_extern_inp_sym_table()->get_const_members_in_structs())
        ret.push_back(i);

    for (auto i : ctx->get_extern_inp_sym_table()->get_const_members_in_arrays())
        ret.push_back(i);

    for (auto i : ctx->get_extern_inp_sym_table()->get_deref_exprs())
        ret.push_back(i);

    return ret;
}

// This function extracts local symbol tables of current Context and all it's predecessors.
std::vector<std::shared_ptr<Expr>> ScopeStmt::extract_locals_from_ctx(std::shared_ptr<Context> ctx) {
    //TODO: add struct members
    std::vector<std::shared_ptr<Expr>> ret = ctx->get_local_sym_table()->get_all_var_use_exprs();
    std::vector<std::shared_ptr<DereferenceExpr>>& deref_expr = ctx->get_local_sym_table()->get_deref_exprs();
    ret.insert(ret.end(), deref_expr.begin(), deref_expr.end());

    if (ctx->get_parent_ctx() != nullptr) {
        std::vector<std::shared_ptr<Expr>> parent_locals = extract_locals_from_ctx(ctx->get_parent_ctx());
        ret.insert(ret.end(), parent_locals.begin(), parent_locals.end());
    }

    return ret;
}

// This function extracts all available input and mixed variables from extern symbol table and
// local symbol tables of current Context and all it's predecessors (by calling extract_locals_from_ctx).
// TODO: we create multiple entry for variables from extern_sym_tables
std::vector<std::shared_ptr<Expr>> ScopeStmt::extract_inp_and_mix_from_ctx(std::shared_ptr<Context> ctx) {
    //TODO: extract_inp_from_ctx extracts only invariant members of structs
    std::vector<std::shared_ptr<Expr>> ret = extract_inp_from_ctx(ctx);
    for (auto i : ctx->get_extern_mix_sym_table()->get_members_in_structs())
        ret.push_back(i);
    for (auto i : ctx->get_extern_mix_sym_table()->get_members_in_arrays())
        ret.push_back(i);
    for (auto i : ctx->get_extern_mix_sym_table()->get_all_var_use_exprs())
        ret.push_back(i);
    for (auto i : ctx->get_extern_mix_sym_table()->get_deref_exprs())
        ret.push_back(i);

    auto locals = extract_locals_from_ctx(ctx);
    ret.insert(ret.end(), locals.begin(), locals.end());

    return ret;
}

void ScopeStmt::emit (std::ostream& stream, std::string offset) {
    stream << offset + "{\n";
    for (const auto &i : scope) {
        i->emit(stream, offset + "    ");
        stream << "\n";
    }
    stream << offset + "}\n";
}

// This function randomly creates new AssignExpr and wraps it to ExprStmt.
std::shared_ptr<ExprStmt> ExprStmt::generate (std::shared_ptr<Context> ctx,
                                              std::vector<std::shared_ptr<Expr>> inp,
                                              std::shared_ptr<Expr> out,
                                              bool count_up_total) {
    Stmt::increase_stmt_count();
    GenPolicy::add_to_complexity(Node::NodeID::EXPR);

    std::shared_ptr<AssignExpr> assign_exp;
    //TODO: now it can be only assign. Do we want something more?
    if (!out->get_value()->get_type()->is_ptr_type()) {
        std::shared_ptr<Expr> from = ArithExpr::generate(ctx, inp);
        assign_exp = std::make_shared<AssignExpr>(out, from, ctx->get_taken());
    }
    else {
        assign_exp = std::make_shared<AssignExpr>(out, rand_val_gen->get_rand_elem(inp), ctx->get_taken());
    }
    if (count_up_total)
        Expr::increase_expr_count(assign_exp->get_complexity());
    GenPolicy::add_to_complexity(Node::NodeID::ASSIGN);
    return std::make_shared<ExprStmt>(assign_exp);
}

void ExprStmt::emit (std::ostream& stream, std::string offset) {
    stream << offset;
    expr->emit(stream);
    stream << ";";
}

bool IfStmt::count_if_taken (std::shared_ptr<Expr> cond) {
    std::shared_ptr<TypeCastExpr> cond_to_bool = std::make_shared<TypeCastExpr> (cond, IntegerType::init(Type::IntegerTypeID::BOOL), true);
    if (cond_to_bool->get_value()->get_class_id() != Data::VarClassID::VAR) {
        ERROR("bad class id (IfStmt)");
    }
    std::shared_ptr<ScalarVariable> cond_var = std::static_pointer_cast<ScalarVariable> (cond_to_bool->get_value());
    return cond_var->get_cur_value().val.bool_val;
}

IfStmt::IfStmt (std::shared_ptr<Expr> _cond, std::shared_ptr<ScopeStmt> _if_br, std::shared_ptr<ScopeStmt> _else_br) :
                Stmt(Node::NodeID::IF), cond(_cond), if_branch(_if_br), else_branch(_else_br) {
    if (cond == nullptr || if_branch == nullptr) {
        ERROR("if branchescan't be empty (IfStmt)");
    }
    taken = count_if_taken(cond);
}

// This function randomly creates new IfStmt (its condition, if branch body and and optional else branch).
std::shared_ptr<IfStmt> IfStmt::generate (std::shared_ptr<Context> ctx,
                                          std::vector<std::shared_ptr<Expr>> inp,
                                          bool count_up_total) {
    Stmt::increase_stmt_count();
    GenPolicy::add_to_complexity(Node::NodeID::IF);
    std::shared_ptr<Expr> cond = ArithExpr::generate(ctx, inp);
    if (count_up_total)
        Expr::increase_expr_count(cond->get_complexity());
    bool else_exist = rand_val_gen->get_rand_id(ctx->get_gen_policy()->get_else_prob());
    bool cond_taken = IfStmt::count_if_taken(cond);
    std::shared_ptr<ScopeStmt> then_br = ScopeStmt::generate(std::make_shared<Context>(*(ctx->get_gen_policy()), ctx, Node::NodeID::SCOPE, cond_taken));
    std::shared_ptr<ScopeStmt> else_br = nullptr;
    if (else_exist)
        else_br = ScopeStmt::generate(std::make_shared<Context>(*(ctx->get_gen_policy()), ctx, Node::NodeID::SCOPE, !cond_taken));
    return std::make_shared<IfStmt>(cond, then_br, else_br);
}

void IfStmt::emit (std::ostream& stream, std::string offset) {
    stream << offset << "if (";
    cond->emit(stream);
    stream << ")\n";
    if_branch->emit(stream, offset);
    if (else_branch != nullptr) {
        stream << offset + "else\n";
        else_branch->emit(stream, offset);
    }
}
