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
#include "gen_policy.h"

///////////////////////////////////////////////////////////////////////////////

int MAX_ALLOWED_INT_TYPES = 3;

int MIN_ARRAY_SIZE = 1000;
int MAX_ARRAY_SIZE = 10000;

int MAX_ARITH_DEPTH = 5;

int MIN_ARITH_STMT_NUM = 5;
int MAX_ARITH_STMT_NUM = 10;

int MAX_TMP_VAR_NUM = 5;

int MIN_INP_VAR_NUM = 20;
int MAX_INP_VAR_NUM = 60;

int MAX_CSE_NUM = 5;

int MAX_IF_DEPTH = 3;


int MIN_STRUCT_TYPES_NUM = 3;
int MAX_STRUCT_TYPES_NUM = 6;
int MIN_STRUCT_NUM = 4;
int MAX_STRUCT_NUM = 8;
int MIN_STRUCT_MEMBERS_NUM = 5;
int MAX_STRUCT_MEMBERS_NUM = 10;


///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<RandValGen> rand_val_gen;

RandValGen::RandValGen (uint64_t _seed) {
    if (_seed != 0) {
        seed = _seed;
    }
    else {
        std::random_device rd;
        seed = rd ();
    }
    std::cout << "/*SEED " << seed << "*/\n";
    rand_gen = std::mt19937_64(seed);
}

///////////////////////////////////////////////////////////////////////////////

GenPolicy::GenPolicy () {
    num_of_allowed_int_types = MAX_ALLOWED_INT_TYPES;
    rand_init_allowed_int_types();

    allowed_modifiers.push_back (Type::Mod::NTHG);

    allow_static_var = false;

    allow_struct = true;
    min_struct_types_num = MIN_STRUCT_TYPES_NUM;
    max_struct_types_num = MAX_STRUCT_TYPES_NUM;
    min_struct_num = MIN_STRUCT_NUM;
    max_struct_num = MAX_STRUCT_NUM;
    min_struct_members_num = MIN_STRUCT_MEMBERS_NUM;
    max_struct_members_num = MAX_STRUCT_MEMBERS_NUM;
    allow_mix_mod_in_struct = false;
    allow_mix_types_in_struct = true;
    member_use_prob.push_back(Probability<bool>(true, 70));
    member_use_prob.push_back(Probability<bool>(false, 30));

    min_array_size = MIN_ARRAY_SIZE;
    max_array_size = MAX_ARRAY_SIZE;

    essence_differ = false;
    primary_essence = (Array::Ess) rand_val_gen->get_rand_value<int>(Array::Ess::C_ARR, Array::Ess::MAX_ESS - 1);

    max_arith_depth = MAX_ARITH_DEPTH;

    min_arith_stmt_num = MIN_ARITH_STMT_NUM;
    max_arith_stmt_num = MAX_ARITH_STMT_NUM;

    min_inp_var_num = MIN_INP_VAR_NUM;
    max_inp_var_num = MAX_INP_VAR_NUM;

    max_tmp_var_num = MAX_TMP_VAR_NUM;
    used_tmp_var_num = 0;

    max_cse_num = MAX_CSE_NUM;

    for (int i = UnaryExpr::Op::Plus; i < UnaryExpr::Op::MaxOp; ++i) {
        Probability<UnaryExpr::Op> prob ((UnaryExpr::Op) i, 1);
        allowed_unary_op.push_back (prob);
    }

    for (int i = 0; i < BinaryExpr::Op::MaxOp; ++i) {
        Probability<BinaryExpr::Op> prob ((BinaryExpr::Op) i, 1);
        allowed_binary_op.push_back (prob);
    }

    Probability<Node::NodeID> decl_gen (Node::NodeID::DECL, 10);
    stmt_gen_prob.push_back (decl_gen);
    Probability<Node::NodeID> assign_gen (Node::NodeID::EXPR, 10);
    stmt_gen_prob.push_back (assign_gen);
    Probability<Node::NodeID> if_gen (Node::NodeID::IF, 10);
    stmt_gen_prob.push_back (if_gen);

    Probability<ArithLeafID> data_leaf (ArithLeafID::Data, 10);
    arith_leaves.push_back (data_leaf);
    Probability<ArithLeafID> unary_leaf (ArithLeafID::Unary, 20);
    arith_leaves.push_back (unary_leaf);
    Probability<ArithLeafID> binary_leaf (ArithLeafID::Binary, 45);
    arith_leaves.push_back (binary_leaf);
    Probability<ArithLeafID> type_cast_leaf (ArithLeafID::TypeCast, 10);
    arith_leaves.push_back (type_cast_leaf);
    Probability<ArithLeafID> cse_leaf (ArithLeafID::CSE, 5);
    arith_leaves.push_back (cse_leaf);

    Probability<ArithDataID> inp_data (ArithDataID::Inp, 80);
    arith_data_distr.push_back (inp_data);
    Probability<ArithDataID> const_data (ArithDataID::Const, 10);
    arith_data_distr.push_back (const_data);
    Probability<ArithDataID> reuse_data (ArithDataID::Reuse, 10);
    arith_data_distr.push_back (reuse_data);

    Probability<ArithCSEGenID> add_cse (ArithCSEGenID::Add, 10);
    arith_cse_gen.push_back (add_cse);
    Probability<ArithCSEGenID> max_cse_gen (ArithCSEGenID::MAX_CSE_GEN_ID, 10);
    arith_cse_gen.push_back (max_cse_gen);

    Probability<ArithSSP::ConstUse> const_branch (ArithSSP::ConstUse::CONST_BRANCH, 5);
    allowed_arith_ssp_const_use.push_back(const_branch);
    Probability<ArithSSP::ConstUse> half_const (ArithSSP::ConstUse::HALF_CONST, 5);
    allowed_arith_ssp_const_use.push_back(half_const);
    Probability<ArithSSP::ConstUse> no_ssp_const_use (ArithSSP::ConstUse::MAX_CONST_USE, 90);
    allowed_arith_ssp_const_use.push_back(no_ssp_const_use);

    chosen_arith_ssp_const_use = ArithSSP::ConstUse::MAX_CONST_USE;

    Probability<ArithSSP::SimilarOp> additive (ArithSSP::SimilarOp::ADDITIVE, 5);
    allowed_arith_ssp_similar_op.push_back(additive);
    Probability<ArithSSP::SimilarOp> bitwise (ArithSSP::SimilarOp::BITWISE, 5);
    allowed_arith_ssp_similar_op.push_back(bitwise);
    Probability<ArithSSP::SimilarOp> logic (ArithSSP::SimilarOp::LOGIC, 5);
    allowed_arith_ssp_similar_op.push_back(logic);
    Probability<ArithSSP::SimilarOp> mul (ArithSSP::SimilarOp::MUL, 5);
    allowed_arith_ssp_similar_op.push_back(mul);
    Probability<ArithSSP::SimilarOp> bit_sh (ArithSSP::SimilarOp::BIT_SH, 5);
    allowed_arith_ssp_similar_op.push_back(bit_sh);
    Probability<ArithSSP::SimilarOp> add_mul (ArithSSP::SimilarOp::ADD_MUL, 5);
    allowed_arith_ssp_similar_op.push_back(add_mul);
    Probability<ArithSSP::SimilarOp> no_ssp_similar_op (ArithSSP::SimilarOp::MAX_SIMILAR_OP, 70);
    allowed_arith_ssp_similar_op.push_back(no_ssp_similar_op);

    chosen_arith_ssp_similar_op = ArithSSP::SimilarOp::MAX_SIMILAR_OP;

    allow_local_var = true;

    Probability<bool> else_exist (true, 50);
    else_prob.push_back(else_exist);
    Probability<bool> no_else (false, 50);
    else_prob.push_back(no_else);

    max_if_depth = MAX_IF_DEPTH;
/*
    allow_arrays = true;
    allow_scalar_variables = true;
*/
}

void GenPolicy::copy_data (std::shared_ptr<GenPolicy> old) {
    used_data_expr = old->get_used_data_expr();
    cse = old->get_cse();
}

void GenPolicy::add_used_data_expr (std::shared_ptr<Expr> expr) {
    if (std::find(used_data_expr.begin(), used_data_expr.end(), expr) == used_data_expr.end())
        used_data_expr.push_back (expr);
}

std::shared_ptr<GenPolicy> GenPolicy::apply_arith_ssp_const_use (ArithSSP::ConstUse pattern_id) {
    chosen_arith_ssp_const_use = pattern_id;
    GenPolicy new_policy = *this;
    if (pattern_id == ArithSSP::ConstUse::CONST_BRANCH) {
        new_policy.arith_data_distr.clear();
        Probability<ArithDataID> const_data (ArithDataID::Const, 100);
        new_policy.arith_data_distr.push_back (const_data);
    }
    else if (pattern_id == ArithSSP::ConstUse::HALF_CONST) {
        new_policy.arith_data_distr.clear();
        Probability<ArithDataID> inp_data (ArithDataID::Inp, 50);
        new_policy.arith_data_distr.push_back (inp_data);
        Probability<ArithDataID> const_data (ArithDataID::Const, 50);
        new_policy.arith_data_distr.push_back (const_data);
    }
    return std::make_shared<GenPolicy> (new_policy);
}

std::shared_ptr<GenPolicy> GenPolicy::apply_arith_ssp_similar_op (ArithSSP::SimilarOp pattern_id) {
    chosen_arith_ssp_similar_op = pattern_id;
    GenPolicy new_policy = *this;
    if (pattern_id == ArithSSP::SimilarOp::ADDITIVE || pattern_id == ArithSSP::SimilarOp::ADD_MUL) {
        new_policy.allowed_unary_op.clear();
        // TODO: add default probability to gen_policy;
        Probability<UnaryExpr::Op> plus (UnaryExpr::Op::Plus, 50);
        new_policy.allowed_unary_op.push_back (plus);
        Probability<UnaryExpr::Op> negate (UnaryExpr::Op::Negate, 50);
        new_policy.allowed_unary_op.push_back (negate);

        new_policy.allowed_binary_op.clear();
        // TODO: add default probability to gen_policy;
        Probability<BinaryExpr::Op> add (BinaryExpr::Op::Add, 33);
        new_policy.allowed_binary_op.push_back (add);
        Probability<BinaryExpr::Op> sub (BinaryExpr::Op::Sub, 33);
        new_policy.allowed_binary_op.push_back (sub);

        if (pattern_id == ArithSSP::SimilarOp::ADD_MUL) {
            Probability<BinaryExpr::Op> mul (BinaryExpr::Op::Mul, 33);
            new_policy.allowed_binary_op.push_back (mul);
        }
    }
    else if (pattern_id == ArithSSP::SimilarOp::BITWISE || pattern_id == ArithSSP::SimilarOp::BIT_SH) {
        new_policy.allowed_unary_op.clear();
        Probability<UnaryExpr::Op> bit_not (UnaryExpr::Op::BitNot, 100);
        new_policy.allowed_unary_op.push_back (bit_not);

        new_policy.allowed_binary_op.clear();
        Probability<BinaryExpr::Op> bit_and (BinaryExpr::Op::BitAnd, 20);
        new_policy.allowed_binary_op.push_back (bit_and);
        Probability<BinaryExpr::Op> bit_xor (BinaryExpr::Op::BitXor, 20);
        new_policy.allowed_binary_op.push_back (bit_xor);
        Probability<BinaryExpr::Op> bit_or (BinaryExpr::Op::BitOr, 20);
        new_policy.allowed_binary_op.push_back (bit_or);

        if (pattern_id == ArithSSP::SimilarOp::BIT_SH) {
            Probability<BinaryExpr::Op> shl (BinaryExpr::Op::Shl, 20);
            new_policy.allowed_binary_op.push_back (shl);
            Probability<BinaryExpr::Op> shr (BinaryExpr::Op::Shr, 20);
            new_policy.allowed_binary_op.push_back (shr);
        }
    }
    else if (pattern_id == ArithSSP::SimilarOp::LOGIC) {
        new_policy.allowed_unary_op.clear();
        Probability<UnaryExpr::Op> log_not (UnaryExpr::Op::LogNot, 100);
        new_policy.allowed_unary_op.push_back (log_not);

        new_policy.allowed_binary_op.clear();
        Probability<BinaryExpr::Op> log_and (BinaryExpr::Op::LogAnd, 50);
        new_policy.allowed_binary_op.push_back (log_and);
        Probability<BinaryExpr::Op> log_or (BinaryExpr::Op::LogOr, 50);
        new_policy.allowed_binary_op.push_back (log_or);
    }
    else if (pattern_id == ArithSSP::SimilarOp::MUL) {
        // TODO: what about unary expr?
        new_policy.allowed_binary_op.clear();
        Probability<BinaryExpr::Op> mul (BinaryExpr::Op::Mul, 100);
        new_policy.allowed_binary_op.push_back (mul);
    }
    return std::make_shared<GenPolicy> (new_policy);
}

void GenPolicy::rand_init_allowed_int_types () {
    allowed_int_types.clear ();
    std::vector<IntegerType::IntegerTypeID> tmp_allowed_int_types;
    int gen_types = 0;
    while (gen_types < num_of_allowed_int_types) {
        IntegerType::IntegerTypeID type = (IntegerType::IntegerTypeID) rand_val_gen->get_rand_value<int>(0, IntegerType::IntegerTypeID::MAX_INT_ID - 1);
        if (std::find(tmp_allowed_int_types.begin(), tmp_allowed_int_types.end(), type) == tmp_allowed_int_types.end()) {
            tmp_allowed_int_types.push_back (type);
            gen_types++;
        }
    }
    for (auto i : tmp_allowed_int_types) {
        Probability<IntegerType::IntegerTypeID> prob (i, 1);
        allowed_int_types.push_back (prob);
    }
}

void GenPolicy::set_modifier (bool value, Type::Mod modifier) {
    if (value)
        allowed_modifiers.push_back (modifier);
    else
        allowed_modifiers.erase (std::remove (allowed_modifiers.begin(), allowed_modifiers.end(), modifier), allowed_modifiers.end());
}

bool GenPolicy::get_modifier (Type::Mod modifier) {
    return (std::find(allowed_modifiers.begin(), allowed_modifiers.end(), modifier) != allowed_modifiers.end());
}
