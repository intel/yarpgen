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

DeclStmt::DeclStmt (std::shared_ptr<Data> _data, std::shared_ptr<Expr> _init, bool _is_extern) :
                  Stmt(Node::NodeID::DECL), data(_data), init(_init), is_extern(_is_extern) {
    if (init == nullptr)
        return;
    if (data->get_class_id() != Data::VarClassID::VAR || init->get_value()->get_class_id() != Data::VarClassID::VAR) {
        ERROR("can init only ScalarVariable in (DeclStmt)");
    }
    if (is_extern) {
        ERROR("init of extern var (DeclStmt)");
    }
    std::shared_ptr<ScalarVariable> data_var = std::static_pointer_cast<ScalarVariable>(data);
    std::shared_ptr<TypeCastExpr> cast_type = std::make_shared<TypeCastExpr>(init, data_var->get_type());
    data_var->set_init_value(std::static_pointer_cast<ScalarVariable>(cast_type->get_value())->get_cur_value());
}

// This function randomly creates new ScalarVariable, its initializing arithmetic expression and
// adds new variable to local_sym_table of parent Context
std::shared_ptr<DeclStmt> DeclStmt::generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp) {
    total_stmt_count++;
    GenPolicy::add_to_complexity(Node::NodeID::DECL);

    std::shared_ptr<ScalarVariable> new_var = ScalarVariable::generate(ctx);
    std::shared_ptr<Expr> new_init = ArithExpr::generate(ctx, inp);
    std::shared_ptr<DeclStmt> ret =  std::make_shared<DeclStmt>(new_var, new_init);
    if (ctx->get_parent_ctx() == nullptr || ctx->get_parent_ctx()->get_local_sym_table() == nullptr) {
        ERROR("no par_ctx or local_sym_table (DeclStmt)");
    }
    ctx->get_parent_ctx()->get_local_sym_table()->add_variable(new_var);
    return ret;
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
    stream << data->get_type()->get_simple_name() + " " + data->get_name();
    if (data->get_type()->get_align() != 0 && is_extern) // TODO: Should we set __attribute__ to non-extern variable?
        stream << " __attribute__((aligned(" + std::to_string(data->get_type()->get_align()) + ")))";
    if (init != nullptr) {
        if (data->get_class_id() == Data::VarClassID::STRUCT) {
            ERROR("emit init of struct (DeclStmt)");
        }
        if (is_extern) {
            ERROR("init of extern var (DeclStmt)");
        }
        stream << " = ";
        init->emit(stream);
    }
    stream << ";";
}

// One of the most important generation methods (top-level generator for everything between curve brackets).
// It acts as a top-level dispatcher for other statement generation functions.
// Also it initially fills extern symbol table.
std::shared_ptr<ScopeStmt> ScopeStmt::generate (std::shared_ptr<Context> ctx) {
    GenPolicy::add_to_complexity(Node::NodeID::SCOPE);

    //TODO: this should be inside Master
    if (ctx->get_parent_ctx() == nullptr)
        form_extern_sym_table(ctx);

    std::shared_ptr<ScopeStmt> ret = std::make_shared<ScopeStmt>();

    // Before the main generation loop starts, we need to extract from all contexts every input / mixed variable and structure.
    std::vector<std::shared_ptr<Expr>> inp = extract_inp_and_mix_from_ctx(ctx);

    //TODO: add to gen_policy stmt number
    auto p = ctx->get_gen_policy();
    uint32_t scope_stmt_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_scope_stmt_count(), p->get_max_scope_stmt_count());
    for (uint32_t i = 0; i < scope_stmt_count; ++i) {
        if (total_stmt_count >= p->get_max_total_stmt_count())
            //TODO: Can we somehow eliminate compiler timeout with the help of this?
            //GenPolicy::get_test_complexity() >= p->get_max_test_complexity())
            break;

        // Randomly decide if we want to create a new CSE
        GenPolicy::ArithCSEGenID add_cse = rand_val_gen->get_rand_id(p->get_arith_cse_gen());
        if (add_cse == GenPolicy::ArithCSEGenID::Add &&
           ((p->get_cse().size() - 1 < p->get_max_cse_count()) ||
            (p->get_cse().size() == 0))) {
            std::vector<std::shared_ptr<Expr>> cse_inp = extract_inp_from_ctx(ctx);
            p->add_cse(ArithExpr::generate(ctx, cse_inp, false));
        }

        // Randomly pick next Stmt ID
        Node::NodeID gen_id = rand_val_gen->get_rand_id(p->get_stmt_gen_prob());
        // ExprStmt
        if (gen_id == Node::NodeID::EXPR) {
            //TODO: add to gen_policy
            // Are we going to use mixed variable or create new outpu variable?
            bool use_mix = rand_val_gen->get_rand_value<uint32_t>(0, 1);
            GenPolicy::OutDataTypeID out_data_type = rand_val_gen->get_rand_id(p->get_out_data_type_prob());
            std::shared_ptr<Expr> assign_lhs = nullptr;
            if (use_mix) {
                // Use mixed variable or we don't have any suitable members
                if (out_data_type == GenPolicy::OutDataTypeID::VAR || ctx->get_extern_mix_sym_table()->get_avail_members().size() == 0) {
                    uint32_t mix_num = rand_val_gen->get_rand_value<uint32_t>(0, ctx->get_extern_mix_sym_table()->get_variables().size() - 1);
                    assign_lhs = std::make_shared<VarUseExpr>(ctx->get_extern_mix_sym_table()->get_variables().at(mix_num));
                }
                // Use member of mixed struct
                else {
                    uint32_t mix_num = rand_val_gen->get_rand_value<uint32_t>(0, ctx->get_extern_mix_sym_table()->get_avail_members().size() - 1);
                    assign_lhs = ctx->get_extern_mix_sym_table()->get_avail_members().at(mix_num);
                }
            }
            else {
                // Create new output variable or we don't have any suitable members
                if (out_data_type == GenPolicy::OutDataTypeID::VAR || ctx->get_extern_out_sym_table()->get_avail_members().size() == 0) {
                    std::shared_ptr<ScalarVariable> out_var = ScalarVariable::generate(ctx);
                    ctx->get_extern_out_sym_table()->add_variable (out_var);
                    assign_lhs = std::make_shared<VarUseExpr>(out_var);
                }
                // Use member of output struct
                else {
                    uint32_t out_num = rand_val_gen->get_rand_value<uint32_t>(0, ctx->get_extern_out_sym_table()->get_avail_members().size() - 1);
                    assign_lhs = ctx->get_extern_out_sym_table()->get_avail_members().at(out_num);
                    ctx->get_extern_out_sym_table()->del_avail_member(out_num);
                }
            }
            ret->add_stmt(ExprStmt::generate(ctx, inp, assign_lhs));
        }
        // DeclStmt or if we want IfStmt, but have reached its depth limit
        else if (gen_id == Node::NodeID::DECL || (ctx->get_if_depth() == p->get_max_if_depth())) {
            std::shared_ptr<DeclStmt> tmp_decl = DeclStmt::generate(std::make_shared<Context>(*(p), ctx, Node::NodeID::DECL, true), inp);
            // Add created variable to inp
            std::shared_ptr<ScalarVariable> tmp_var = std::static_pointer_cast<ScalarVariable>(tmp_decl->get_data());
            inp.push_back(std::make_shared<VarUseExpr>(tmp_var));
            ret->add_stmt(tmp_decl);
        }
        // IfStmt
        else if (gen_id == Node::NodeID::IF) {
            ret->add_stmt(IfStmt::generate(std::make_shared<Context>(*(p), ctx, Node::NodeID::IF, true), inp));
        }

    }
    return ret;
}

// CSE shouldn't change during the scope to make generation process easy. In order to achieve this,
// we use only "input" variables for them and this function extracts such variables from extern symbol table.
std::vector<std::shared_ptr<Expr>> ScopeStmt::extract_inp_from_ctx(std::shared_ptr<Context> ctx) {
    std::vector<std::shared_ptr<Expr>> ret;
    for (auto i : ctx->get_extern_inp_sym_table()->get_variables()) {
        ret.push_back(std::make_shared<VarUseExpr> (i));
    }

    for (auto i : ctx->get_extern_inp_sym_table()->get_avail_const_members()) {
        ret.push_back(i);
    }
    return ret;
}

// This function extracts all available input and mixed variables from extern sybmol table and
// local symbol tables of current Context and all it's predecessors.
// TODO: we create multiple entry for variables from extern_sym_tables
std::vector<std::shared_ptr<Expr>> ScopeStmt::extract_inp_and_mix_from_ctx(std::shared_ptr<Context> ctx) {
    std::vector<std::shared_ptr<Expr>> ret = extract_inp_from_ctx(ctx);
    for (auto i : ctx->get_extern_mix_sym_table()->get_avail_members()) {
        ret.push_back(i);
    }
    for (auto i : ctx->get_extern_mix_sym_table()->get_variables()) {
        ret.push_back(std::make_shared<VarUseExpr> (i));
    }
    if (ctx->get_parent_ctx() != nullptr)
        ret = extract_inp_and_mix_from_ctx(ctx->get_parent_ctx());
    //TODO: add struct members
    for (auto i : ctx->get_local_sym_table()->get_variables())
        ret.push_back(std::make_shared<VarUseExpr> (i));
    return ret;
}

// This function initially fills extern symbol table with inp and mix variables. It also creates type structs definitions.
void ScopeStmt::form_extern_sym_table(std::shared_ptr<Context> ctx) {
    auto p = ctx->get_gen_policy();
    // Allow const cv-qualifier in gen_policy, pass it to new Context
    std::shared_ptr<Context> const_ctx = std::make_shared<Context>(*(ctx));
    GenPolicy const_gen_policy = *(const_ctx->get_gen_policy());
    const_gen_policy.set_allow_const(true);
    const_ctx->set_gen_policy(const_gen_policy);
    // Generate random number of random input variables
    uint32_t inp_var_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_inp_var_count(), p->get_max_inp_var_count());
    for (uint32_t i = 0; i < inp_var_count; ++i) {
        ctx->get_extern_inp_sym_table()->add_variable(ScalarVariable::generate(const_ctx));
    }
    //TODO: add to gen_policy
    // Same for mixed variables
    uint32_t mix_var_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_mix_var_count(), p->get_max_mix_var_count());
    for (uint32_t i = 0; i < mix_var_count; ++i) {
        ctx->get_extern_mix_sym_table()->add_variable(ScalarVariable::generate(ctx));
    }

    uint32_t struct_type_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_struct_type_count(), p->get_max_struct_type_count());
    if (struct_type_count == 0)
        return;

    // Create random number of struct definition
    for (uint32_t i = 0; i < struct_type_count; ++i) {
        //TODO: Maybe we should create one container for all struct types? And should they all be equal?
        std::shared_ptr<StructType> struct_type = StructType::generate(ctx, ctx->get_extern_inp_sym_table()->get_struct_types());
        ctx->get_extern_inp_sym_table()->add_struct_type(struct_type);
        ctx->get_extern_out_sym_table()->add_struct_type(struct_type);
        ctx->get_extern_mix_sym_table()->add_struct_type(struct_type);
    }

    // Create random number of input structures
    uint32_t inp_struct_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_inp_struct_count(), p->get_max_inp_struct_count());
    for (uint32_t i = 0; i < inp_struct_count; ++i) {
        uint32_t struct_type_indx = rand_val_gen->get_rand_value<uint32_t>(0, struct_type_count - 1);
        ctx->get_extern_inp_sym_table()->add_struct(Struct::generate(const_ctx, ctx->get_extern_inp_sym_table()->get_struct_types().at(struct_type_indx)));
    }
    // Same for mixed structures
    uint32_t mix_struct_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_mix_struct_count(), p->get_max_mix_struct_count());
    for (uint32_t i = 0; i < mix_struct_count; ++i) {
        uint32_t struct_type_indx = rand_val_gen->get_rand_value<uint32_t>(0, struct_type_count - 1);
        ctx->get_extern_mix_sym_table()->add_struct(Struct::generate(ctx, ctx->get_extern_mix_sym_table()->get_struct_types().at(struct_type_indx)));
    }
    // Same for output structures
    uint32_t out_struct_count = rand_val_gen->get_rand_value<uint32_t>(p->get_min_out_struct_count(), p->get_max_out_struct_count());
    for (uint32_t i = 0; i < out_struct_count; ++i) {
        uint32_t struct_type_indx = rand_val_gen->get_rand_value<uint32_t>(0, struct_type_count - 1);
        ctx->get_extern_out_sym_table()->add_struct(Struct::generate(ctx, ctx->get_extern_out_sym_table()->get_struct_types().at(struct_type_indx)));
    }
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
std::shared_ptr<ExprStmt> ExprStmt::generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, std::shared_ptr<Expr> out) {
    total_stmt_count++;
    GenPolicy::add_to_complexity(Node::NodeID::EXPR);

    //TODO: now it can be only assign. Do we want something more?
    std::shared_ptr<Expr> from = ArithExpr::generate(ctx, inp);
    std::shared_ptr<AssignExpr> assign_exp = std::make_shared<AssignExpr>(out, from, ctx->get_taken());
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
std::shared_ptr<IfStmt> IfStmt::generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp) {
    total_stmt_count++;
    GenPolicy::add_to_complexity(Node::NodeID::IF);
    std::shared_ptr<Expr> cond = ArithExpr::generate(ctx, inp);
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
