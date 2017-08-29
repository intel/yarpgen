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
#include <map>

#include "gen_policy.h"

///////////////////////////////////////////////////////////////////////////////

using namespace yarpgen;

const uint32_t TEST_FUNC_COUNT = 5;

const uint32_t MAX_ALLOWED_INT_TYPES = 3;

const uint32_t MAX_ARITH_DEPTH = 5;
const uint32_t MAX_TOTAL_EXPR_COUNT = 50000000;
const uint32_t MAX_FUNC_EXPR_COUNT = 10000000;

const uint32_t MIN_SCOPE_STMT_COUNT = 5;
const uint32_t MAX_SCOPE_STMT_COUNT = 10;

const uint32_t MAX_TOTAL_STMT_COUNT = 5000;
const uint32_t MAX_FUNC_STMT_COUNT = 1000;

const uint32_t MIN_INP_VAR_COUNT = 20;
const uint32_t MAX_INP_VAR_COUNT = 60;
const uint32_t MIN_MIX_VAR_COUNT = 20;
const uint32_t MAX_MIX_VAR_COUNT = 60;

const uint32_t MAX_CSE_COUNT = 5;

const uint32_t MAX_IF_DEPTH = 3;

const uint64_t MAX_TEST_COMPLEXITY = UINT64_MAX;

const uint32_t MIN_STRUCT_TYPES_COUNT = 0;
const uint32_t MAX_STRUCT_TYPES_COUNT = 6;
const uint32_t MIN_INP_STRUCT_COUNT = 0;
const uint32_t MAX_INP_STRUCT_COUNT = 6;
const uint32_t MIN_MIX_STRUCT_COUNT = 0;
const uint32_t MAX_MIX_STRUCT_COUNT = 6;
const uint32_t MIN_OUT_STRUCT_COUNT = 0;
const uint32_t MAX_OUT_STRUCT_COUNT = 8;
const uint32_t MIN_STRUCT_MEMBER_COUNT = 1;
const uint32_t MAX_STRUCT_MEMBER_COUNT = 10;
const uint32_t MAX_STRUCT_DEPTH = 5;
const uint32_t MIN_BIT_FIELD_SIZE = 8;
const uint32_t MAX_BIT_FIELD_SIZE = 2; //TODO: unused, because it cause different result for LLVM and GCC. See pr70733

const uint32_t CONST_BUFFER_SIZE = 4;

// This switch totally disables array in generated tests
const bool DISABLE_ARRAYS = false;
const uint32_t MIN_ARRAY_SIZE = 2;
const uint32_t MAX_ARRAY_SIZE = 10;
const uint32_t MIN_ARRAY_TYPES_COUNT = 0;
const uint32_t MAX_ARRAY_TYPES_COUNT = 6;
const uint32_t MIN_INP_ARRAY_COUNT = 0;
const uint32_t MAX_INP_ARRAY_COUNT = 6;
const uint32_t MIN_MIX_ARRAY_COUNT = 0;
const uint32_t MAX_MIX_ARRAY_COUNT = 6;
const uint32_t MIN_OUT_ARRAY_COUNT = 0;
const uint32_t MAX_OUT_ARRAY_COUNT = 8;

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<RandValGen> yarpgen::rand_val_gen;

RandValGen::RandValGen (uint64_t _seed) {
    if (_seed != 0) {
        seed = _seed;
    }
    else {
        std::random_device rd;
        seed = rd ();
    }
    std::cout << "/*SEED " << options->plane_yarpgen_version << "_" << seed << "*/" << std::endl;
    rand_gen = std::mt19937_64(seed);
}

const std::string NameHandler::common_test_func_prefix = "tf_";

///////////////////////////////////////////////////////////////////////////////

bool GenPolicy::default_was_loaded = false;
GenPolicy yarpgen::default_gen_policy;

GenPolicy::GenPolicy () {
    if (default_was_loaded)
        *this = default_gen_policy;
}

void GenPolicy::init_from_config () {
    test_func_count = TEST_FUNC_COUNT;

    num_of_allowed_int_types = MAX_ALLOWED_INT_TYPES;
    rand_init_allowed_int_types();

    allowed_cv_qual.push_back (Type::CV_Qual::NTHG);

    allow_static_var = false;
    if (options->is_c())
        allow_static_members = false;
    else if (options->is_cxx())
        allow_static_members = true;
    else {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can't detect language subset" << std::endl;
        exit(-1);
    }

    allow_struct = true;
    min_struct_type_count = MIN_STRUCT_TYPES_COUNT;
    max_struct_type_count = MAX_STRUCT_TYPES_COUNT;
    min_inp_struct_count = MIN_INP_STRUCT_COUNT;
    max_inp_struct_count = MAX_INP_STRUCT_COUNT;
    min_mix_struct_count = MIN_MIX_STRUCT_COUNT;
    max_mix_struct_count = MAX_MIX_STRUCT_COUNT;
    min_out_struct_count = MIN_OUT_STRUCT_COUNT;
    max_out_struct_count = MAX_OUT_STRUCT_COUNT;
    min_struct_member_count = MIN_STRUCT_MEMBER_COUNT;
    max_struct_member_count = MAX_STRUCT_MEMBER_COUNT;
    allow_mix_cv_qual_in_struct = false;
    allow_mix_static_in_struct = true;
    allow_mix_types_in_struct = true;
    member_use_prob.push_back(Probability<bool>(true, 80));
    member_use_prob.push_back(Probability<bool>(false, 20));
    rand_val_gen->shuffle_prob(member_use_prob);
    max_struct_depth = MAX_STRUCT_DEPTH;
    member_class_prob.push_back(Probability<Data::VarClassID>(Data::VarClassID::VAR, 70));
    member_class_prob.push_back(Probability<Data::VarClassID>(Data::VarClassID::STRUCT, 30));
    rand_val_gen->shuffle_prob(member_class_prob);
    min_bit_field_size = MIN_BIT_FIELD_SIZE;
    max_bit_field_size = MAX_BIT_FIELD_SIZE;
    bit_field_prob.push_back(Probability<BitFieldID>(UNNAMED, 15));
    bit_field_prob.push_back(Probability<BitFieldID>(NAMED, 20));
    bit_field_prob.push_back(Probability<BitFieldID>(MAX_BIT_FIELD_ID, 65));
    rand_val_gen->shuffle_prob(bit_field_prob);

    out_data_type_prob.emplace_back(Probability<OutDataTypeID>(VAR, 50));
    out_data_type_prob.emplace_back(Probability<OutDataTypeID>(STRUCT, 25));
    out_data_type_prob.emplace_back(Probability<OutDataTypeID>(VAR_IN_ARRAY, 15));
    out_data_type_prob.emplace_back(Probability<OutDataTypeID>(STRUCT_IN_ARRAY, 10));
    rand_val_gen->shuffle_prob(out_data_type_prob);

    out_data_category_prob.emplace_back(Probability<OutDataCategoryID>(MIX, 50));
    out_data_category_prob.emplace_back(Probability<OutDataCategoryID>(OUT, 50));
    rand_val_gen->shuffle_prob(out_data_category_prob);

    min_array_size = MIN_ARRAY_SIZE;
    max_array_size = MAX_ARRAY_SIZE;
    array_base_type_prob.emplace_back(Probability<Type::TypeID>(Type::BUILTIN_TYPE, 60));
    array_base_type_prob.emplace_back(Probability<Type::TypeID>(Type::STRUCT_TYPE, 40));
    rand_val_gen->shuffle_prob(array_base_type_prob);
    array_kind_prob.emplace_back(Probability<ArrayType::Kind>(ArrayType::C_ARR, 40));
    if (options->is_cxx()) {
        array_kind_prob.emplace_back(Probability<ArrayType::Kind>(ArrayType::VAL_ARR, 15));
        if (options->standard_id >= Options::CXX11) {
            array_kind_prob.emplace_back(Probability<ArrayType::Kind>(ArrayType::STD_ARR, 15));
            array_kind_prob.emplace_back(Probability<ArrayType::Kind>(ArrayType::STD_VEC, 15));
        }
    }
    rand_val_gen->shuffle_prob(array_kind_prob);
    array_elem_subs_prob.emplace_back(Probability<ArrayType::ElementSubscript>(ArrayType::Brackets, 50));
    array_elem_subs_prob.emplace_back(Probability<ArrayType::ElementSubscript>(ArrayType::At, 50));
    rand_val_gen->shuffle_prob(array_elem_subs_prob);
    min_array_type_count = MIN_ARRAY_TYPES_COUNT;
    max_array_type_count = MAX_ARRAY_TYPES_COUNT;
    min_inp_array_count = MIN_INP_ARRAY_COUNT;
    max_inp_array_count = MAX_INP_ARRAY_COUNT;
    min_mix_array_count = MIN_MIX_ARRAY_COUNT;
    max_mix_array_count = MAX_MIX_ARRAY_COUNT;
    min_out_array_count = MIN_OUT_ARRAY_COUNT;
    max_out_array_count = MAX_OUT_ARRAY_COUNT;
    if (DISABLE_ARRAYS) {
        min_array_type_count = max_array_type_count = 0;
        min_inp_array_count = max_inp_array_count = 0;
        min_mix_array_count = max_mix_array_count = 0;
        min_out_array_count = max_out_array_count = 0;
    }

    max_arith_depth = MAX_ARITH_DEPTH;
    max_total_expr_count = MAX_TOTAL_EXPR_COUNT;
    max_func_expr_count = MAX_FUNC_EXPR_COUNT;

    min_scope_stmt_count = MIN_SCOPE_STMT_COUNT;
    max_scope_stmt_count = MAX_SCOPE_STMT_COUNT;

    max_total_stmt_count = MAX_TOTAL_STMT_COUNT;
    max_func_stmt_count = MAX_FUNC_STMT_COUNT;

    min_inp_var_count = MIN_INP_VAR_COUNT;
    max_inp_var_count = MAX_INP_VAR_COUNT;
    min_mix_var_count = MIN_MIX_VAR_COUNT;
    max_mix_var_count = MAX_MIX_VAR_COUNT;

    max_cse_count = MAX_CSE_COUNT;

    for (int i = UnaryExpr::Op::Plus; i < UnaryExpr::Op::MaxOp; ++i) {
        Probability<UnaryExpr::Op> prob ((UnaryExpr::Op) i, 10);
        allowed_unary_op.push_back (prob);
    }
    rand_val_gen->shuffle_prob(allowed_unary_op);

    for (int i = 0; i < BinaryExpr::Op::MaxOp; ++i) {
        Probability<BinaryExpr::Op> prob ((BinaryExpr::Op) i, 10);
        allowed_binary_op.push_back (prob);
    }
    rand_val_gen->shuffle_prob(allowed_binary_op);

    Probability<Node::NodeID> decl_gen (Node::NodeID::DECL, 10);
    stmt_gen_prob.push_back (decl_gen);
    Probability<Node::NodeID> assign_gen (Node::NodeID::EXPR, 10);
    stmt_gen_prob.push_back (assign_gen);
    Probability<Node::NodeID> if_gen (Node::NodeID::IF, 10);
    stmt_gen_prob.push_back (if_gen);
    rand_val_gen->shuffle_prob(stmt_gen_prob);

    Probability<ArithLeafID> data_leaf (ArithLeafID::Data, 11);
    arith_leaves.push_back (data_leaf);
    Probability<ArithLeafID> unary_leaf (ArithLeafID::Unary, 21);
    arith_leaves.push_back (unary_leaf);
    Probability<ArithLeafID> binary_leaf (ArithLeafID::Binary, 46);
    arith_leaves.push_back (binary_leaf);
    Probability<ArithLeafID> cond_leaf (ArithLeafID::Conditional, 3);
    arith_leaves.push_back (cond_leaf);
    Probability<ArithLeafID> type_cast_leaf (ArithLeafID::TypeCast, 11);
    arith_leaves.push_back (type_cast_leaf);
    Probability<ArithLeafID> cse_leaf (ArithLeafID::CSE, 8);
    arith_leaves.push_back (cse_leaf);
    rand_val_gen->shuffle_prob(arith_leaves);

    Probability<ArithDataID> inp_data (ArithDataID::Inp, 80);
    arith_data_distr.push_back (inp_data);
    Probability<ArithDataID> const_data (ArithDataID::Const, 20);
    arith_data_distr.push_back (const_data);
    rand_val_gen->shuffle_prob(arith_data_distr);

    Probability<ArithCSEGenID> add_cse (ArithCSEGenID::Add, 20);
    arith_cse_gen.push_back (add_cse);
    Probability<ArithCSEGenID> max_cse_gen (ArithCSEGenID::MAX_CSE_GEN_ID, 80);
    arith_cse_gen.push_back (max_cse_gen);
    rand_val_gen->shuffle_prob(arith_cse_gen);

    Probability<ArithSSP::ConstUse> const_branch (ArithSSP::ConstUse::CONST_BRANCH, 5);
    allowed_arith_ssp_const_use.push_back(const_branch);
    Probability<ArithSSP::ConstUse> half_const (ArithSSP::ConstUse::HALF_CONST, 5);
    allowed_arith_ssp_const_use.push_back(half_const);
    Probability<ArithSSP::ConstUse> no_ssp_const_use (ArithSSP::ConstUse::MAX_CONST_USE, 90);
    allowed_arith_ssp_const_use.push_back(no_ssp_const_use);
    rand_val_gen->shuffle_prob(allowed_arith_ssp_const_use);

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
    rand_val_gen->shuffle_prob(allowed_arith_ssp_similar_op);

    chosen_arith_ssp_similar_op = ArithSSP::SimilarOp::MAX_SIMILAR_OP;

    const_buffer_size = CONST_BUFFER_SIZE;
    new_const_prob.emplace_back(Probability<bool>(true, 50));
    new_const_prob.emplace_back(Probability<bool>(false, 50));
    rand_val_gen->shuffle_prob(new_const_prob);
    new_const_type_prob.emplace_back(Probability<bool>(true, 50));
    new_const_type_prob.emplace_back(Probability<bool>(false, 50));
    rand_val_gen->shuffle_prob(new_const_type_prob);
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::Zero, 10));
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::One, 10));
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::Two, 10));
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::Three, 10));
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::Four, 10));
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::Eight, 10));
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::Sixteen, 10));
    special_const_prob.emplace_back(Probability<ConstPattern::SpecialConst>(ConstPattern::MAX_SPECIAL_CONST, 10));
    rand_val_gen->shuffle_prob(special_const_prob);
    new_const_kind_prob.emplace_back(Probability<ConstPattern::NewConstKind>(ConstPattern::EndBits, 25));
    new_const_kind_prob.emplace_back(Probability<ConstPattern::NewConstKind>(ConstPattern::BitBlock, 25));
    new_const_kind_prob.emplace_back(Probability<ConstPattern::NewConstKind>(ConstPattern::MAX_NEW_CONST_KIND, 50));
    rand_val_gen->shuffle_prob(new_const_kind_prob);
    const_transform_prob.emplace_back(Probability<UnaryExpr::Op>(UnaryExpr::Op::Negate, 33));
    const_transform_prob.emplace_back(Probability<UnaryExpr::Op>(UnaryExpr::Op::BitNot, 33));
    const_transform_prob.emplace_back(Probability<UnaryExpr::Op>(UnaryExpr::Op::Plus, 33));
    rand_val_gen->shuffle_prob(const_transform_prob);

    Probability<bool> else_exist (true, 50);
    else_prob.push_back(else_exist);
    Probability<bool> no_else (false, 50);
    else_prob.push_back(no_else);
    rand_val_gen->shuffle_prob(else_prob);

    max_if_depth = MAX_IF_DEPTH;

    max_test_complexity = MAX_TEST_COMPLEXITY;

    default_was_loaded = true;
}

void GenPolicy::copy_data (std::shared_ptr<GenPolicy> old) {
    cse = old->get_cse();
}

GenPolicy GenPolicy::apply_arith_ssp_const_use (ArithSSP::ConstUse pattern_id) {
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
    return new_policy;
}

GenPolicy GenPolicy::apply_arith_ssp_similar_op (ArithSSP::SimilarOp pattern_id) {
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
    return new_policy;
}

void GenPolicy::rand_init_allowed_int_types () {
    allowed_int_types.clear ();
    std::vector<IntegerType::IntegerTypeID> tmp_allowed_int_types;
    uint32_t gen_types = 0;
    while (gen_types < num_of_allowed_int_types) {
        auto type = (IntegerType::IntegerTypeID) rand_val_gen->get_rand_value(0, IntegerType::IntegerTypeID::MAX_INT_ID - 1);
        if (type == IntegerType::IntegerTypeID::BOOL && options->is_c())
            continue;
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

void GenPolicy::set_cv_qual(bool value, Type::CV_Qual cv_qual) {
    if (value)
        allowed_cv_qual.push_back (cv_qual);
    else
        allowed_cv_qual.erase (std::remove (allowed_cv_qual.begin(), allowed_cv_qual.end(), cv_qual), allowed_cv_qual.end());
}

bool GenPolicy::get_cv_qual(Type::CV_Qual cv_qual) {
    return (std::find(allowed_cv_qual.begin(), allowed_cv_qual.end(), cv_qual) != allowed_cv_qual.end());
}

// Abstract measure of complexity of execution
static const std::map<Node::NodeID, uint64_t> NodeComplexity {
    {Node::NodeID::ASSIGN, 5},
    {Node::NodeID::BINARY, 10},
    {Node::NodeID::CONST, 5},
    {Node::NodeID::TYPE_CAST, 10},
    {Node::NodeID::UNARY, 5},
    {Node::NodeID::VAR_USE, 5},
    {Node::NodeID::MEMBER, 10},
    {Node::NodeID::MAX_EXPR_ID, UINT64_MAX},
    {Node::NodeID::MIN_STMT_ID, UINT64_MAX},
    {Node::NodeID::DECL, 30},
    {Node::NodeID::EXPR, 20},
    {Node::NodeID::SCOPE, 5},
    {Node::NodeID::IF, 50},
    {Node::NodeID::MAX_STMT_ID, UINT64_MAX}
};

uint64_t GenPolicy::test_complexity = 0;

void GenPolicy::add_to_complexity(Node::NodeID node_id) {
    test_complexity += NodeComplexity.at(node_id);
}
