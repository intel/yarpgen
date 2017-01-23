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

#include "stmt.h"
#include "sym_table.h"

using namespace rl;

int Stmt::total_stmt_num = 0;

DeclStmt::DeclStmt (std::shared_ptr<Data> _data, std::shared_ptr<Expr> _init, bool _is_extern) :
                  Stmt(Node::NodeID::DECL), data(_data), init(_init), is_extern(_is_extern) {
    if (init == NULL)
        return;
    if (data->get_class_id() != Data::VarClassID::VAR || init->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can init only ScalarVariable in DeclStmt::DeclStmt" << std::endl;
        exit(-1);
    }
    if (is_extern) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": init of extern var in DeclStmt::DeclStmt" << std::endl;
        exit(-1);
    }
    std::shared_ptr<ScalarVariable> data_var = std::static_pointer_cast<ScalarVariable>(data);
    std::shared_ptr<TypeCastExpr> cast_type = std::make_shared<TypeCastExpr>(init, data_var->get_type());
    data_var->set_init_value(std::static_pointer_cast<ScalarVariable>(cast_type->get_value())->get_cur_value());
}

std::shared_ptr<DeclStmt> DeclStmt::generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp) {
    total_stmt_num++;
    std::shared_ptr<ScalarVariable> new_var = ScalarVariable::generate(ctx);
    std::shared_ptr<Expr> new_init = ArithExpr::generate(ctx, inp);
    std::shared_ptr<DeclStmt> ret =  std::make_shared<DeclStmt>(new_var, new_init);
    if (ctx->get_parent_ctx() == NULL || ctx->get_parent_ctx()->get_local_sym_table() == NULL) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": no par_ctx or local_sym_table in DeclStmt::generate" << std::endl;
        exit(-1);
    }
    ctx->get_parent_ctx()->get_local_sym_table()->add_variable(new_var);
    return ret;
}

std::string DeclStmt::emit (std::string offset) {
    std::string ret = offset;
    ret += data->get_type()->get_is_static() && !is_extern ? "static " : "";
    ret += is_extern ? "extern " : "";
    switch (data->get_type()->get_modifier()) {
        case Type::Mod::VOLAT:
            ret += "volatile ";
            break;
        case Type::Mod::CONST:
            ret += "const ";
            break;
        case Type::Mod::CONST_VOLAT:
            ret += "const volatile ";
            break;
        case Type::Mod::NTHG:
            break;
        case Type::Mod::MAX_MOD:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad modifier in DeclStmt::emit" << std::endl;
            exit(-1);
            break;
    }
    ret += data->get_type()->get_simple_name() + " " + data->get_name();
    if (data->get_type()->get_align() != 0 && is_extern) // TODO: Should we set __attribute__ to non-extern variable?
        ret += " __attribute__((aligned(" + std::to_string(data->get_type()->get_align()) + ")))";
    if (init != NULL) {
        if (data->get_class_id() == Data::VarClassID::STRUCT) {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": emit init of struct in DeclStmt::emit" << std::endl;
            exit(-1);
        }
        if (is_extern) {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": init of extern var in DeclStmt::emit" << std::endl;
            exit(-1);
        }
        ret += " = " + init->emit();
    }
    ret += ";";
    return ret;
}

std::shared_ptr<ScopeStmt> ScopeStmt::generate (std::shared_ptr<Context> ctx) {
    if (ctx->get_parent_ctx() == NULL)
        form_extern_sym_table(ctx);

    std::shared_ptr<ScopeStmt> ret = std::make_shared<ScopeStmt>();

    std::vector<std::shared_ptr<Expr>> inp = form_inp_from_ctx(ctx);
    std::vector<std::shared_ptr<Expr>> cse_inp = form_const_inp_from_ctx(ctx);

    //TODO: add to gen_policy stmt number
    int scope_stmt_num = rand_val_gen->get_rand_value<int>(ctx->get_gen_policy()->get_min_scope_stmt_num(), ctx->get_gen_policy()->get_max_scope_stmt_num());
    for (int i = 0; i < scope_stmt_num; ++i) {
        if (total_stmt_num >= ctx->get_gen_policy()->get_max_total_stmt_num())
            break;

        GenPolicy::ArithCSEGenID add_cse = rand_val_gen->get_rand_id(ctx->get_gen_policy()->get_arith_cse_gen());
        if (add_cse == GenPolicy::ArithCSEGenID::Add &&
           ((ctx->get_gen_policy()->get_cse().size() - 1 < ctx->get_gen_policy()->get_max_cse_num()) ||
            (ctx->get_gen_policy()->get_cse().size() == 0))) {
            ctx->get_gen_policy()->add_cse(ArithExpr::generate(ctx, cse_inp));
        }

        Node::NodeID gen_id = rand_val_gen->get_rand_id(ctx->get_gen_policy()->get_stmt_gen_prob());
        if (gen_id == Node::NodeID::EXPR) {
            //TODO: add to gen_policy
            bool use_mix = rand_val_gen->get_rand_value<int>(0, 1);
            GenPolicy::OutDataTypeID out_rata_type = rand_val_gen->get_rand_id(ctx->get_gen_policy()->get_out_data_type_prob());
            std::shared_ptr<Expr> assign_lhs = NULL;
            if (use_mix) {
                if (out_rata_type == GenPolicy::OutDataTypeID::VAR || ctx->get_extern_mix_sym_table()->get_avail_members().size() == 0) {
                    int mix_num = rand_val_gen->get_rand_value<int>(0, ctx->get_extern_mix_sym_table()->get_variables().size() - 1);
                    assign_lhs = std::make_shared<VarUseExpr>(ctx->get_extern_mix_sym_table()->get_variables().at(mix_num));
                }
                else {
                    int mix_num = rand_val_gen->get_rand_value<int>(0, ctx->get_extern_mix_sym_table()->get_avail_members().size() - 1);
                    assign_lhs = ctx->get_extern_mix_sym_table()->get_avail_members().at(mix_num);
                }
            }
            else {
                if (out_rata_type == GenPolicy::OutDataTypeID::VAR || ctx->get_extern_out_sym_table()->get_avail_members().size() == 0) {
                    std::shared_ptr<ScalarVariable> out_var = ScalarVariable::generate(ctx);
                    ctx->get_extern_out_sym_table()->add_variable (out_var);
                    assign_lhs = std::make_shared<VarUseExpr>(out_var);
                }
                else {
                    int out_num = rand_val_gen->get_rand_value<int>(0, ctx->get_extern_out_sym_table()->get_avail_members().size() - 1);
                    assign_lhs = ctx->get_extern_out_sym_table()->get_avail_members().at(out_num);
                    ctx->get_extern_out_sym_table()->del_avail_member(out_num);
                }
            }
            ret->add_stmt(ExprStmt::generate(ctx, inp, assign_lhs));
        }
        else if (gen_id == Node::NodeID::DECL || (ctx->get_if_depth() == ctx->get_gen_policy()->get_max_if_depth())) {
            std::shared_ptr<DeclStmt> tmp_decl = DeclStmt::generate(std::make_shared<Context>(*(ctx->get_gen_policy()), ctx, Node::NodeID::DECL, true), inp);
            std::shared_ptr<ScalarVariable> tmp_var = std::static_pointer_cast<ScalarVariable>(tmp_decl->get_data());
            inp.push_back(std::make_shared<VarUseExpr>(tmp_var));
            ret->add_stmt(tmp_decl);
        }
        else if (gen_id == Node::NodeID::IF) {
            ret->add_stmt(IfStmt::generate(std::make_shared<Context>(*(ctx->get_gen_policy()), ctx, Node::NodeID::IF, true), inp));
        }

    }
    return ret;
}

std::vector<std::shared_ptr<Expr>> ScopeStmt::form_const_inp_from_ctx (std::shared_ptr<Context> ctx) {
    std::vector<std::shared_ptr<Expr>> ret;
    for (auto i : ctx->get_extern_inp_sym_table()->get_variables()) {
        ret.push_back(std::make_shared<VarUseExpr> (i));
    }

    for (auto i : ctx->get_extern_inp_sym_table()->get_avail_const_members()) {
        ret.push_back(i);
    }
    return ret;
}

std::vector<std::shared_ptr<Expr>> ScopeStmt::form_inp_from_ctx (std::shared_ptr<Context> ctx) {
    std::vector<std::shared_ptr<Expr>> ret = form_const_inp_from_ctx (ctx);
    for (auto i : ctx->get_extern_mix_sym_table()->get_avail_members()) {
        ret.push_back(i);
    }
    for (auto i : ctx->get_extern_mix_sym_table()->get_variables()) {
        ret.push_back(std::make_shared<VarUseExpr> (i));
    }
    if (ctx->get_parent_ctx() != NULL)
        ret = form_inp_from_ctx(ctx->get_parent_ctx());
    //TODO: add struct members
    for (auto i : ctx->get_local_sym_table()->get_variables())
        ret.push_back(std::make_shared<VarUseExpr> (i));
    return ret;
}

void ScopeStmt::form_extern_sym_table(std::shared_ptr<Context> ctx) {
    int inp_var_num = rand_val_gen->get_rand_value<int>(ctx->get_gen_policy()->get_min_inp_var_num(), ctx->get_gen_policy()->get_max_inp_var_num());
    std::shared_ptr<Context> const_ctx = std::make_shared<Context>(*(ctx));
    GenPolicy const_gen_policy = *(const_ctx->get_gen_policy());
    const_gen_policy.set_allow_const(true);
    const_ctx->set_gen_policy(const_gen_policy);
    for (int i = 0; i < inp_var_num; ++i) {
        ctx->get_extern_inp_sym_table()->add_variable(ScalarVariable::generate(const_ctx));
    }
    //TODO: add to gen_policy
    int mix_var_num = rand_val_gen->get_rand_value<int>(ctx->get_gen_policy()->get_min_mix_var_num(), ctx->get_gen_policy()->get_max_mix_var_num());
    for (int i = 0; i < mix_var_num; ++i) {
        ctx->get_extern_mix_sym_table()->add_variable(ScalarVariable::generate(ctx));
    }

    int struct_types_num = rand_val_gen->get_rand_value<int>(ctx->get_gen_policy()->get_min_struct_types_num(), ctx->get_gen_policy()->get_max_struct_types_num());
    if (struct_types_num == 0)
        return;

    for (int i = 0; i < struct_types_num; ++i) {
        //TODO: Maybe we should create one container for all struct types? And should they all be equal?
        std::shared_ptr<StructType> struct_type = StructType::generate(ctx, ctx->get_extern_inp_sym_table()->get_struct_types());
        ctx->get_extern_inp_sym_table()->add_struct_type(struct_type);
        ctx->get_extern_out_sym_table()->add_struct_type(struct_type);
        ctx->get_extern_mix_sym_table()->add_struct_type(struct_type);
    }

    int inp_struct_num = rand_val_gen->get_rand_value<int>(ctx->get_gen_policy()->get_min_inp_struct_num(), ctx->get_gen_policy()->get_max_inp_struct_num());
    for (int i = 0; i < inp_struct_num; ++i) {
        int struct_type_indx = rand_val_gen->get_rand_value<int>(0, struct_types_num - 1);
        ctx->get_extern_inp_sym_table()->add_struct(Struct::generate(const_ctx, ctx->get_extern_inp_sym_table()->get_struct_types().at(struct_type_indx)));
    }
    int mix_struct_num = rand_val_gen->get_rand_value<int>(ctx->get_gen_policy()->get_min_mix_struct_num(), ctx->get_gen_policy()->get_max_mix_struct_num());
    for (int i = 0; i < mix_struct_num; ++i) {
        int struct_type_indx = rand_val_gen->get_rand_value<int>(0, struct_types_num - 1);
        ctx->get_extern_mix_sym_table()->add_struct(Struct::generate(ctx, ctx->get_extern_mix_sym_table()->get_struct_types().at(struct_type_indx)));
    }
    int out_struct_num = rand_val_gen->get_rand_value<int>(ctx->get_gen_policy()->get_min_out_struct_num(), ctx->get_gen_policy()->get_max_out_struct_num());
    for (int i = 0; i < out_struct_num; ++i) {
        int struct_type_indx = rand_val_gen->get_rand_value<int>(0, struct_types_num - 1);
        ctx->get_extern_out_sym_table()->add_struct(Struct::generate(ctx, ctx->get_extern_out_sym_table()->get_struct_types().at(struct_type_indx)));
    }
}

std::string ScopeStmt::emit (std::string offset) {
    std::string ret = offset + "{\n";
    for (auto i : scope)
        ret += i->emit(offset + "    ") + "\n";
    ret += offset + "}\n";
    return ret;
}

std::shared_ptr<ExprStmt> ExprStmt::generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, std::shared_ptr<Expr> out) {
    total_stmt_num++;
    //TODO: now it can be only assign. Do we want something more?
    std::shared_ptr<Expr> from = ArithExpr::generate(ctx, inp);
    std::shared_ptr<AssignExpr> assign_exp = std::make_shared<AssignExpr>(out, from, ctx->get_taken());
    return std::make_shared<ExprStmt>(assign_exp);
}

bool IfStmt::count_if_taken (std::shared_ptr<Expr> cond) {
    std::shared_ptr<TypeCastExpr> cond_to_bool = std::make_shared<TypeCastExpr> (cond, IntegerType::init(Type::IntegerTypeID::BOOL), true);
    if (cond_to_bool->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad class id in IfStmt::count_if_taken" << std::endl;
        exit(-1);
    }
    std::shared_ptr<ScalarVariable> cond_var = std::static_pointer_cast<ScalarVariable> (cond_to_bool->get_value());
    return cond_var->get_cur_value().val.bool_val;
}

IfStmt::IfStmt (std::shared_ptr<Expr> _cond, std::shared_ptr<ScopeStmt> _if_br, std::shared_ptr<ScopeStmt> _else_br) :
                Stmt(Node::NodeID::IF), cond(_cond), if_branch(_if_br), else_branch(_else_br) {
    if (cond == NULL || if_branch == NULL) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": if branchescan't be empty in IfStmt::IfStmt" << std::endl;
        exit(-1);
    }
    taken = count_if_taken(cond);
}

std::shared_ptr<IfStmt> IfStmt::generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp) {
    total_stmt_num++;
    std::shared_ptr<Expr> cond = ArithExpr::generate(ctx, inp);
    bool else_exist = rand_val_gen->get_rand_id(ctx->get_gen_policy()->get_else_prob());
    bool cond_taken = IfStmt::count_if_taken(cond);
    std::shared_ptr<ScopeStmt> then_br = ScopeStmt::generate(std::make_shared<Context>(*(ctx->get_gen_policy()), ctx, Node::NodeID::SCOPE, cond_taken));
    std::shared_ptr<ScopeStmt> else_br = NULL;
    if (else_exist)
        else_br = ScopeStmt::generate(std::make_shared<Context>(*(ctx->get_gen_policy()), ctx, Node::NodeID::SCOPE, !cond_taken));
    return std::make_shared<IfStmt>(cond, then_br, else_br);
}

std::string IfStmt::emit (std::string offset) {
    std::string ret = offset;
    ret += "if (" + cond->emit() + ")\n";
    ret += if_branch->emit(offset);
    if (else_branch != NULL) {
        ret += offset + "else\n";
        ret += else_branch->emit(offset);
    }
    return ret;
}
