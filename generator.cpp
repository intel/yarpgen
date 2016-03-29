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

#include "generator.h"

using namespace rl;

void ScalarTypeGen::generate () {
    IntegerType::IntegerTypeID int_type_id = (IntegerType::IntegerTypeID) rand_val_gen->get_rand_id(gen_policy->get_allowed_int_types());
    type = std::static_pointer_cast<IntegerType> (IntegerType::init(int_type_id));

    ModifierGen modifier_gen (gen_policy);
    modifier_gen.generate ();
    type->set_modifier(modifier_gen.get_modifier ());

    StaticSpecifierGen static_spec_gen (gen_policy);
    static_spec_gen.generate ();
    type->set_is_static(static_spec_gen.get_specifier ());
}

void ModifierGen::generate () {
    modifier = gen_policy->get_allowed_modifiers().at(
               rand_val_gen->get_rand_value<int>(0, gen_policy->get_allowed_modifiers().size() - 1));
}

void StaticSpecifierGen::generate () {
    if (gen_policy->get_allow_static_var())
        specifier = rand_val_gen->get_rand_value<int>(0, 1);
    else
        specifier = false;
}
int StructTypeGen::struct_num = 0;

void StructTypeGen::generate () {
    ModifierGen modifier_gen (gen_policy);
    modifier_gen.generate ();
    Type::Mod primary_mod = modifier_gen.get_modifier ();

    // TODO: allow static members in struct
//    StaticSpecifierGen static_spec_gen (gen_policy);
//    static_spec_gen.generate ();
//    bool primary_static_spec = static_spec_gen.get_specifier ();
    bool primary_static_spec = false;

    ScalarTypeGen scalar_type_gen (gen_policy);
    scalar_type_gen.generate();
    std::shared_ptr<Type> primary_type = scalar_type_gen.get_type();

    uint64_t struct_deptj = rand_val_gen->get_rand_value<int>(0, gen_policy->get_max_struct_depth());

    struct_type = std::static_pointer_cast<StructType> (StructType::init("struct_" + std::to_string(struct_num)));
    int struct_member_num = rand_val_gen->get_rand_value<int>(gen_policy->get_min_struct_members_num(), gen_policy->get_max_struct_members_num());
    for (int i = 0; i < struct_member_num; ++i) {
        if (gen_policy->get_allow_mix_types_in_struct()) {
            Data::VarClassID member_class = rand_val_gen->get_rand_id(gen_policy->get_member_class_prob());
            bool add_substruct = false;
            int substruct_type_idx = 0;
            std::shared_ptr<StructType> substruct_type = NULL;
            if (member_class == Data::VarClassID::STRUCT && gen_policy->get_max_struct_depth() > 0 && nested_struct_types.size() > 0) {
                substruct_type_idx = rand_val_gen->get_rand_value<int>(0, nested_struct_types.size() - 1);
                substruct_type = nested_struct_types.at(substruct_type_idx);
                if (substruct_type->get_nest_depth() + 1 == gen_policy->get_max_struct_depth()) {
                    add_substruct = false;
                }
                else {
                    add_substruct = true;
                }
            }
            if (add_substruct) {
               primary_type = substruct_type;
            }
            else {
                scalar_type_gen.generate();
                primary_type = scalar_type_gen.get_type();
            }
        }
        if (gen_policy->get_allow_mix_mod_in_struct()) {
            modifier_gen.generate ();
            primary_mod = modifier_gen.get_modifier ();
//            static_spec_gen.generate ();
//            primary_static_spec = static_spec_gen.get_specifier ();
        }
        primary_type->set_modifier(primary_mod);
        primary_type->set_is_static(primary_static_spec);
        struct_type->add_member(primary_type, "member_" + std::to_string(struct_num) + "_" + std::to_string(member_num++));
    }
    struct_num++;
}

void VariableValueGen::generate () {
    if (rand_init) {
        std::shared_ptr<Type> tmp_type = IntegerType::init (type_id);
        min_value = std::static_pointer_cast<IntegerType>(tmp_type)->get_min ();
        max_value = std::static_pointer_cast<IntegerType>(tmp_type)->get_max ();
    }
    switch (type_id) {
        case IntegerType::IntegerTypeID::BOOL:
            {
                value = (bool) rand_val_gen->get_rand_value<int>((bool) min_value, (bool) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::CHAR:
            {
                value = (signed char) rand_val_gen->get_rand_value<signed char>((signed char) min_value,
                                                                                (signed char) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            {
                value = (unsigned char) rand_val_gen->get_rand_value<unsigned char>((unsigned char) min_value,
                                                                                    (unsigned char) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::SHRT:
            {
                value = (short) rand_val_gen->get_rand_value<short>((short) min_value,
                                                                  (short) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::USHRT:
            {
                value = (unsigned short) rand_val_gen->get_rand_value<unsigned short>((unsigned short) min_value,
                                                                                      (unsigned short) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::INT:
            {
                value = (int) rand_val_gen->get_rand_value<int>((int) min_value,
                                                              (int) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::UINT:
            {
                value = (unsigned int) rand_val_gen->get_rand_value<unsigned int>((unsigned int) min_value,
                                                                                  (unsigned int) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::LINT:
            {
                value = (long int) rand_val_gen->get_rand_value<long int>((long int) min_value,
                                                                          (long int) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::ULINT:
            {
                value = (unsigned long int) rand_val_gen->get_rand_value<unsigned long int>((unsigned long int) min_value,
                                                                                            (unsigned long int) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::LLINT:
            {
                value = (long long int) rand_val_gen->get_rand_value<long long int>((long long int) min_value,
                                                                                    (long long int) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            {
                value = (unsigned long long int) rand_val_gen->get_rand_value<unsigned long long int>((unsigned long long int) min_value,
                                                                                                      (unsigned long long int) max_value);
            }
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad type in VariableValueGen::generate." << std::endl;
            exit (-1);
            break;
    }
}

void DataGen::rand_init_param () {
    if (!rand_init)
        return;

    ScalarTypeGen scalar_type_gen (gen_policy);
    scalar_type_gen.generate ();
    type_id = scalar_type_gen.get_int_type_id ();

    ModifierGen modifier_gen (gen_policy);
    modifier_gen.generate ();
    modifier = modifier_gen.get_modifier ();

    StaticSpecifierGen static_spec_gen (gen_policy);
    static_spec_gen.generate ();
    static_spec = static_spec_gen.get_specifier ();
}

void DataGen::rand_init_value () {
    VariableValueGen var_val_gen (gen_policy, type_id);
    var_val_gen.generate();
    assert (data != NULL && "DataGen::rand_init_value with empty data.");
    data->set_value(var_val_gen.get_value());
}

int ScalarVariableGen::variable_num = 0;

void ScalarVariableGen::generate () {
    rand_init_param ();
    Variable tmp_var ("var_" + std::to_string(variable_num), type_id, modifier, static_spec);
    data = std::make_shared<Variable> (tmp_var);
    rand_init_value();
    variable_num++;
}

int ArrayVariableGen::array_num = 0;

void ArrayVariableGen::generate () {
    rand_init_param ();
    if (rand_init) {
        size = rand_val_gen->get_rand_value<int>(gen_policy->get_min_array_size(), gen_policy->get_max_array_size());
        essence = (Array::Ess) (gen_policy->get_essence_differ() ? rand_val_gen->get_rand_value<int>(0, Array::Ess::MAX_ESS - 1) :
                                                                   gen_policy->get_primary_essence());
    }
    Array tmp_arr ("arr_" + std::to_string(array_num), type_id, modifier, static_spec, size, essence);
    data = std::make_shared<Array> (tmp_arr);
    rand_init_value();
    array_num++;
}

void StructValueGen::generate () {
    for (int i = 0; i < struct_var->get_num_of_members(); ++i) {
        if (struct_var->get_member(i)->get_type()->is_struct_type()) {
            StructValueGen struct_val_gen (gen_policy, std::static_pointer_cast<Struct>(struct_var->get_member(i)));
            struct_val_gen.generate();
        }
        else if (struct_var->get_member(i)->get_type()->is_int_type()) {
            VariableValueGen var_val_gen (gen_policy, std::static_pointer_cast<IntegerType>(struct_var->get_member(i)->get_type())->get_int_type_id());
            var_val_gen.generate();
            struct_var->get_member(i)->set_value(var_val_gen.get_value());
        }
        else {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported type of struct member in StructVariableGen::generate" << std::endl;
            exit(-1);
        }
    }
}

int StructVariableGen::struct_num = 0;

void StructVariableGen::generate () {
    if (rand_init) {
        StructTypeGen struct_type_gen (gen_policy);
        struct_type_gen.generate();
        struct_type = struct_type_gen.get_type();
    }
    Struct struct_var ("struct_obj_" + std::to_string(struct_num++), struct_type);
    data = std::make_shared<Struct> (struct_var);
    StructValueGen struct_val_gen (gen_policy, std::static_pointer_cast<Struct>(data));
    struct_val_gen.generate();
}

void DeclStmtGen::generate () {
    // TODO: Add non-scalar variables declaration
    if (rand_init) {
        // TODO: Enable gen_policy limits
        if (var_class_id == Data::VarClassID::VAR) {
            ScalarVariableGen scalar_var_gen (gen_policy);
            scalar_var_gen.generate ();
            data = scalar_var_gen.get_data();
        }
        else if (var_class_id == Data::VarClassID::ARR) {
            ArrayVariableGen array_var_gen (gen_policy);
            array_var_gen.generate ();
            data = array_var_gen.get_data ();
        }
        else {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": only scalar vasriables and arrays are allowed." << std::endl;
            exit (-1);
        }

        Context arith_ctx (*(gen_policy), NULL, Node::NodeID::MAX_STMT_ID, std::make_shared<SymbolTable>(local_sym_table), ctx);
        ArithStmtGen arith_stmt_gen (std::make_shared<Context>(arith_ctx), inp, NULL);
        arith_stmt_gen.generate();
        init = arith_stmt_gen.get_expr();
    }
    DeclStmt decl_stmt;
    decl_stmt.set_data (data);
    decl_stmt.set_init (init);
    data->set_value(init->get_value());
    decl_stmt.set_is_extern (is_extern);
    stmt = std::make_shared<DeclStmt> (decl_stmt);
}

std::shared_ptr<GenPolicy> ArithStmtGen::choose_and_apply_ssp_const_use (std::shared_ptr<GenPolicy> old_gen_policy) {
    if (old_gen_policy->get_chosen_arith_ssp_const_use () != ArithSSP::ConstUse::MAX_CONST_USE)
        return old_gen_policy;
    ArithSSP::ConstUse arith_ssp_const_use_id = rand_val_gen->get_rand_id (old_gen_policy->get_allowed_arith_ssp_const_use());
//    std::cerr << "arith_single_pattern_id: " << arith_single_pattern_id << std::endl;
    return old_gen_policy->apply_arith_ssp_const_use ((ArithSSP::ConstUse) arith_ssp_const_use_id);
}

std::shared_ptr<GenPolicy> ArithStmtGen::choose_and_apply_ssp_similar_op (std::shared_ptr<GenPolicy> old_gen_policy) {
    if (old_gen_policy->get_chosen_arith_ssp_similar_op () != ArithSSP::SimilarOp::MAX_SIMILAR_OP)
        return old_gen_policy;
    ArithSSP::SimilarOp arith_ssp_similar_op_id = rand_val_gen->get_rand_id (old_gen_policy->get_allowed_arith_ssp_similar_op());
//    std::cerr << "arith_single_pattern_id: " << arith_single_pattern_id << std::endl;
    return old_gen_policy->apply_arith_ssp_similar_op ((ArithSSP::SimilarOp) arith_ssp_similar_op_id);
}

std::shared_ptr<GenPolicy> ArithStmtGen::choose_and_apply_ssp () {
    std::shared_ptr<GenPolicy> new_policy = choose_and_apply_ssp_const_use(gen_policy);
    new_policy = choose_and_apply_ssp_similar_op(new_policy);
    return new_policy;
}

void ArithStmtGen::generate () {
    res_expr = gen_level(0);
    if (out != NULL) {
        AssignExpr assign;
        assign.set_to(out);
        assign.set_from(res_expr);
        res_expr = std::make_shared<AssignExpr> (assign);
    }
    ExprStmt res_stmt;
    res_stmt.set_expr(res_expr);
    stmt = std::make_shared<ExprStmt> (res_stmt);
}

std::shared_ptr<Expr> ArithStmtGen::gen_level (int depth) {
    std::shared_ptr<GenPolicy> old_gen_policy = gen_policy;
    gen_policy = choose_and_apply_ssp ();
    GenPolicy::ArithLeafID node_type = rand_val_gen->get_rand_id (gen_policy->get_arith_leaves());
    std::shared_ptr<Expr> ret;
    GenPolicy::ArithCSEGenID add_cse_prob = rand_val_gen->get_rand_id(gen_policy->get_arith_cse_gen());

    if (node_type == GenPolicy::ArithLeafID::Data || depth == gen_policy->get_max_arith_depth() || 
        (node_type == GenPolicy::ArithLeafID::CSE && gen_policy->get_cse().size() == 0)) {
        GenPolicy::ArithDataID data_type = rand_val_gen->get_rand_id (gen_policy->get_arith_data_distr());
        if (data_type == GenPolicy::ArithDataID::Const || inp.size() == 0) {
            IntegerType::IntegerTypeID type_id = rand_val_gen->get_rand_id(gen_policy->get_allowed_int_types());
            VariableValueGen var_val_gen (gen_policy, type_id);
            var_val_gen.generate ();
            ConstExpr const_expr;
            const_expr.set_type (type_id);;
            const_expr.set_data (var_val_gen.get_value());
            const_expr.propagate_type();
            const_expr.propagate_value();
            ret = std::make_shared<ConstExpr> (const_expr);
        }
        else if (data_type == GenPolicy::ArithDataID::Inp || 
            (data_type == GenPolicy::ArithDataID::Reuse && gen_policy->get_used_data_expr().size() == 0)) {
            int inp_use = rand_val_gen->get_rand_value<int>(0, inp.size() - 1);
            gen_policy->add_used_data_expr(inp.at(inp_use));
            inp.at(inp_use)->propagate_type();
            inp.at(inp_use)->propagate_value();
            ret = inp.at(inp_use);
        }
        else if (data_type == GenPolicy::ArithDataID::Reuse) {
            int reuse_data_num = rand_val_gen->get_rand_value<int>(0, gen_policy->get_used_data_expr().size() - 1);
            ret = gen_policy->get_used_data_expr().at(reuse_data_num);
        }
        else {
            exit (-1);
        }
    }
    else if (node_type == GenPolicy::ArithLeafID::Unary) { // Unary expr
        UnaryExpr unary;
        UnaryExpr::Op op_type = rand_val_gen->get_rand_id(gen_policy->get_allowed_unary_op());
        unary.set_op(op_type);
        unary.set_arg(gen_level(depth + 1));
        unary.propagate_type();
        Expr::UB ub = unary.propagate_value();
        ret = std::make_shared<UnaryExpr> (unary);
        if (ub)
            ret = rebuild_unary(ub, ret);
    }
    else if (node_type == GenPolicy::ArithLeafID::Binary) { // Binary expr
        BinaryExpr binary;
        BinaryExpr::Op op_type = rand_val_gen->get_rand_id(gen_policy->get_allowed_binary_op());
        binary.set_op(op_type);
        binary.set_lhs(gen_level(depth + 1));
        binary.set_rhs(gen_level(depth + 1));
        binary.propagate_type();
        Expr::UB ub = binary.propagate_value();
        ret = std::make_shared<BinaryExpr> (binary);
        if (ub)
            ret = rebuild_binary(ub, ret);
    }
    else if (node_type == GenPolicy::ArithLeafID::TypeCast) { // TypeCast expr
        TypeCastExpr type_cast;
        IntegerType::IntegerTypeID type_id = rand_val_gen->get_rand_id(gen_policy->get_allowed_int_types());
        type_cast.set_type(IntegerType::init(type_id));
        type_cast.set_expr(gen_level(depth + 1));
        type_cast.propagate_type();
        type_cast.propagate_value();
        ret = std::make_shared<TypeCastExpr> (type_cast);
    }
    else if (node_type == GenPolicy::ArithLeafID::CSE) {
        int cse_num = rand_val_gen->get_rand_value<int>(0, gen_policy->get_cse().size() - 1);
        ret = gen_policy->get_cse().at(cse_num);
        add_cse_prob = GenPolicy::ArithCSEGenID::MAX_CSE_GEN_ID;
    }
    else {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": inappropriate node type." << std::endl;
        exit (-1);
    }

    if (add_cse_prob == GenPolicy::ArithCSEGenID::Add &&
       ((gen_policy->get_cse().size() - 1 < gen_policy->get_max_cse_num()) || 
        (gen_policy->get_cse().size() == 0))) {
        gen_policy->add_cse(ret);
    }

    old_gen_policy->copy_data (gen_policy);
    gen_policy = old_gen_policy;
    return ret;
}

std::shared_ptr<Expr> ArithStmtGen::rebuild_unary (Expr::UB ub, std::shared_ptr<Expr> expr) {
    if (expr->get_id() != Node::NodeID::UNARY) {
        std::cerr << "ERROR: ArithExprGen::rebuild_unary : not UnaryExpr" << std::endl;
        return NULL;
    }
    std::shared_ptr<UnaryExpr> ret = std::static_pointer_cast<UnaryExpr>(expr);
    switch (ret->get_op()) {
        case UnaryExpr::PreInc:
            ret->set_op(UnaryExpr::Op::PreDec);
            break;
        case UnaryExpr::PostInc:
            ret->set_op(UnaryExpr::Op::PostDec);
            break;
        case UnaryExpr::PreDec:
            ret->set_op(UnaryExpr::Op::PreInc);
            break;
        case UnaryExpr::PostDec:
            ret->set_op(UnaryExpr::Op::PostInc);
            break;
        case UnaryExpr::Negate:
            ret->set_op(UnaryExpr::Op::Plus);
            break;
        case UnaryExpr::Plus:
        case UnaryExpr::LogNot:
        case UnaryExpr::BitNot:
            break;
        case UnaryExpr::MaxOp:
            std::cerr << "ERROR: ArithExprGen::rebuild_unary : MaxOp" << std::endl;
            break;
    }
    ret->propagate_type();
    Expr::UB ret_ub = ret->propagate_value();
    if (ret_ub) {
        std::cerr << "ERROR: ArithExprGen::rebuild_unary : illegal strategy" << std::endl;
        return NULL;
    }
    return ret;
}

static uint64_t msb(uint64_t x) {
    uint64_t ret = 0;
    while (x != 0) {
        ret++;
        x = x >> 1;
    }
    return ret;
}

std::shared_ptr<Expr> ArithStmtGen::rebuild_binary (Expr::UB ub, std::shared_ptr<Expr> expr) {
    if (expr->get_id() != Node::NodeID::BINARY) {
        std::cerr << "ERROR: ArithExprGen::rebuild_binary : not BinaryExpr" << std::endl;
        return NULL;
    }
    std::shared_ptr<BinaryExpr> ret = std::static_pointer_cast<BinaryExpr>(expr);
    switch (ret->get_op()) {
        case BinaryExpr::Add:
            ret->set_op(BinaryExpr::Op::Sub);
            break;
        case BinaryExpr::Sub:
            ret->set_op(BinaryExpr::Op::Add);
            break;
        case BinaryExpr::Mul:
            if (ub == Expr::SignOvfMin)
                ret->set_op(BinaryExpr::Op::Sub);
            else
                ret->set_op(BinaryExpr::Op::Div);
            break;
        case BinaryExpr::Div:
        case BinaryExpr::Mod:
            if (ub == Expr::ZeroDiv)
                ret->set_op(BinaryExpr::Op::Mul);
            else
               ret->set_op(BinaryExpr::Op::Sub);
            break;
        case BinaryExpr::Shr:
        case BinaryExpr::Shl:
            if ((ub == Expr::ShiftRhsNeg) || (ub == Expr::ShiftRhsLarge)) {
                std::shared_ptr<Expr> rhs = ret->get_rhs();
                ConstExpr const_expr;
                const_expr.set_type(rhs->get_int_type_id());

                std::shared_ptr<Expr> lhs = ret->get_lhs();
                uint64_t max_sht_val = lhs->get_type_bit_size();
                if ((ret->get_op() == BinaryExpr::Shl) && (lhs->get_type_is_signed()) && (ub == Expr::ShiftRhsLarge))
                    max_sht_val = max_sht_val - msb((uint64_t)lhs->get_value());
                uint64_t const_val = rand_val_gen->get_rand_value<int>(0, max_sht_val);
                if (ub == Expr::ShiftRhsNeg) {
                    std::shared_ptr<Type> tmp_type = IntegerType::init (rhs->get_int_type_id());
                    const_val += std::abs((long long int) rhs->get_value());
                    const_val = std::min(const_val, std::static_pointer_cast<IntegerType>(tmp_type)->get_max()); // TODO: it won't work with INT_MIN
                }
                else
                    const_val = rhs->get_value() - const_val;
                const_expr.set_data(const_val);
                const_expr.propagate_type();
                const_expr.propagate_value();

                BinaryExpr ins;
                if (ub == Expr::ShiftRhsNeg)
                    ins.set_op(BinaryExpr::Op::Add);
                else
                    ins.set_op(BinaryExpr::Op::Sub);
                ins.set_lhs(rhs);
                ins.set_rhs(std::make_shared<ConstExpr> (const_expr));
                ins.propagate_type();
                Expr::UB ins_ub = ins.propagate_value();
                ret->set_rhs(std::make_shared<BinaryExpr> (ins));
                if (ins_ub) {
                    std::cerr << "ArithExprGen::rebuild_binary : invalid shift rebuild (type 1)" << std::endl;
                    ret = std::static_pointer_cast<BinaryExpr> (rebuild_binary(ins_ub, ret));
                }
            }
            else {
                std::shared_ptr<Expr> lhs = ret->get_lhs();
                ConstExpr const_expr;
                const_expr.set_type(lhs->get_int_type_id());
                uint64_t const_val = std::static_pointer_cast<IntegerType>(IntegerType::init(lhs->get_int_type_id()))->get_max();
                const_expr.set_data(const_val);
                const_expr.propagate_type();
                const_expr.propagate_value();

                BinaryExpr ins;
                ins.set_op(BinaryExpr::Op::Add);
                ins.set_lhs(lhs);
                ins.set_rhs(std::make_shared<ConstExpr> (const_expr));
                ins.propagate_type();
                Expr::UB ins_ub = ins.propagate_value();
                ret->set_lhs(std::make_shared<BinaryExpr> (ins));
                if (ins_ub) {
                    std::cerr << "ArithExprGen::rebuild_binary : invalid shift rebuild (type 2)" << std::endl;
                    ret = std::static_pointer_cast<BinaryExpr> (rebuild_binary(ins_ub, ret));
                }
            }
            break;
        case BinaryExpr::Lt:
        case BinaryExpr::Gt:
        case BinaryExpr::Le:
        case BinaryExpr::Ge:
        case BinaryExpr::Eq:
        case BinaryExpr::Ne:
        case BinaryExpr::BitAnd:
        case BinaryExpr::BitOr:
        case BinaryExpr::BitXor:
        case BinaryExpr::LogAnd:
        case BinaryExpr::LogOr:
            break;
        case BinaryExpr::MaxOp:
            std::cerr << "ArithExprGen::rebuild_binary : invalid Op" << std::endl;
            break;
    }
    ret->propagate_type();
    Expr::UB ret_ub = ret->propagate_value();
    if (ret_ub) {
        return rebuild_binary(ret_ub, ret);
    }
    return ret;
}

void IfStmtGen::generate () {
    if (rand_init) {
        // TODO: What about stmt and stmtID?
        Context arith_ctx (*(gen_policy), NULL, Node::NodeID::MAX_STMT_ID, std::make_shared<SymbolTable>(local_sym_table), ctx);
        ArithStmtGen arith_stmt_gen (std::make_shared<Context>(arith_ctx), inp, NULL);
        arith_stmt_gen.generate();
        cond = arith_stmt_gen.get_expr();
        else_exist = rand_val_gen->get_rand_id(gen_policy->get_else_prob());
    }

    IfStmt if_stmt;
    if_stmt.set_cond(cond);

    Context if_scope_ctx (*(gen_policy), stmt,  Node::NodeID::IF, std::make_shared<SymbolTable>(local_sym_table), ctx);
    ScopeGen if_scope_gen (std::make_shared<Context> (if_scope_ctx));
    if_scope_gen.generate();
    if_br_scope = if_scope_gen.get_scope();

    for (auto i = if_br_scope.begin(); i != if_br_scope.end(); ++i)
        if_stmt.add_if_stmt(*i);

    if (else_exist) {
        Context else_scope_ctx (*(gen_policy), stmt,  Node::NodeID::IF, std::make_shared<SymbolTable>(local_sym_table), ctx);
        ScopeGen else_scope_gen (std::make_shared<Context> (if_scope_ctx));
        else_scope_gen.generate();
        else_br_scope = else_scope_gen.get_scope();

        if_stmt.set_else_exist (true);
        for (auto i = else_br_scope.begin(); i != else_br_scope.end(); ++i)
            if_stmt.add_else_stmt(*i);
    }

    stmt = std::make_shared<IfStmt> (if_stmt);
}

void ScopeGen::form_struct_member_expr (std::shared_ptr<MemberExpr> parent_memb_expr, std::shared_ptr<Struct> struct_var, std::vector<std::shared_ptr<Expr>>& inp) {
    for (int j = 0; j < struct_var->get_num_of_members(); ++j) {
        if (rand_val_gen->get_rand_id(gen_policy->get_member_use_prob())) {
            MemberExpr member_expr;
            member_expr.set_struct(struct_var);
            member_expr.set_identifier(j);
            member_expr.set_member_expr(parent_memb_expr);

            if (struct_var->get_member(j)->get_type()->is_struct_type())
                form_struct_member_expr(std::make_shared<MemberExpr> (member_expr), std::static_pointer_cast<Struct>(struct_var->get_member(j)), inp);
            else
                inp.push_back(std::make_shared<MemberExpr> (member_expr));
        }
    }
}

void ScopeGen::generate () {
    if (ctx->get_parent_ctx() == NULL) {
        int inp_var_num = rand_val_gen->get_rand_value<int>(gen_policy->get_min_inp_var_num(), gen_policy->get_max_inp_var_num());
        for (int i = 0; i < inp_var_num; ++i) {
            ScalarVariableGen inp_var_gen (gen_policy);
            inp_var_gen.generate ();
            ctx->get_extern_inp_sym_table()->add_variable (std::static_pointer_cast<Variable>(inp_var_gen.get_data()));
        }

        int struct_types_num = rand_val_gen->get_rand_value<int>(gen_policy->get_min_struct_types_num(), gen_policy->get_max_struct_types_num());
        for (int i = 0; i < struct_types_num; ++i) {
            StructTypeGen struct_type_gen (gen_policy, ctx->get_extern_inp_sym_table()->get_struct_types());
            struct_type_gen.generate();
            ctx->get_extern_inp_sym_table()->add_struct_type(struct_type_gen.get_type());
        }
        //TODO: add it to output variables
        int struct_num = rand_val_gen->get_rand_value<int>(gen_policy->get_min_struct_num(), gen_policy->get_max_struct_num());
        for (int i = 0; i < struct_num; ++i) {
            int struct_type_indx = rand_val_gen->get_rand_value<int>(0, struct_types_num - 1);
            StructVariableGen struct_var_gen (gen_policy, ctx->get_extern_inp_sym_table()->get_struct_types().at(struct_type_indx));
            struct_var_gen.generate();
            ctx->get_extern_inp_sym_table()->add_struct(std::static_pointer_cast<Struct>(struct_var_gen.get_data()));
        }
    }

    std::vector<std::shared_ptr<Expr>> inp;
    for (auto i : ctx->get_extern_inp_sym_table()->get_variables()) {
        VarUseExpr var_use;
        var_use.set_variable (i);
        inp.push_back(std::make_shared<VarUseExpr> (var_use));
    }

    for (auto i : ctx->get_extern_inp_sym_table()->get_structs()) {
        form_struct_member_expr(NULL, i, inp);
    }

    //TODO: add to gen_policy stmt number
    int arith_stmt_num = rand_val_gen->get_rand_value<int>(gen_policy->get_min_arith_stmt_num(), gen_policy->get_max_arith_stmt_num());
    for (int i = 0; i < arith_stmt_num; ++i) {
        Node::NodeID gen_id = rand_val_gen->get_rand_id(gen_policy->get_stmt_gen_prob());
        if (gen_id == Node::NodeID::DECL && (gen_policy->get_used_tmp_var_num() < gen_policy->get_max_tmp_var_num())) {
            Context decl_ctx (*(gen_policy), NULL, Node::NodeID::MAX_STMT_ID, std::make_shared<SymbolTable>(local_sym_table), ctx);
            DeclStmtGen tmp_var_decl_gen (std::make_shared<Context>(decl_ctx), Data::VarClassID::VAR, inp);
            tmp_var_decl_gen.generate();
            std::shared_ptr<Variable> tmp_var = std::static_pointer_cast<Variable>(tmp_var_decl_gen.get_data());
            local_sym_table.add_variable(tmp_var);
            VarUseExpr var_use;
            var_use.set_variable (tmp_var);
            inp.push_back(std::make_shared<VarUseExpr> (var_use));
            scope.push_back(tmp_var_decl_gen.get_stmt());
            gen_policy->add_used_tmp_var_num();
        }
        else if (gen_id == Node::NodeID::EXPR || (ctx->get_if_depth() == gen_policy->get_max_if_depth())) {
            ScalarVariableGen out_var_gen (gen_policy);
            out_var_gen.generate ();
            ctx->get_extern_out_sym_table()->add_variable (std::static_pointer_cast<Variable>(out_var_gen.get_data()));
            VarUseExpr var_use;
            var_use.set_variable (std::static_pointer_cast<Variable>(out_var_gen.get_data()));
            Context arith_ctx (*(gen_policy), NULL, Node::NodeID::MAX_STMT_ID, std::make_shared<SymbolTable>(local_sym_table), ctx);
            ArithStmtGen arith_stmt_gen (std::make_shared<Context>(arith_ctx), inp, std::make_shared<VarUseExpr> (var_use));
            arith_stmt_gen.generate();
            gen_policy->copy_data(arith_stmt_gen.get_gen_policy());
            scope.push_back(arith_stmt_gen.get_stmt());
        }
        else if (gen_id == Node::NodeID::IF) {
            Context if_ctx (*(gen_policy), NULL, Node::NodeID::MAX_STMT_ID, std::make_shared<SymbolTable>(local_sym_table), ctx);
            IfStmtGen if_stmt_gen (std::make_shared<Context>(if_ctx), inp);
            if_stmt_gen.generate();
            scope.push_back(if_stmt_gen.get_stmt());
        }
    }
}
