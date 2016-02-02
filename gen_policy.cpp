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

int MAX_ALLOWED_TYPES = 3;

int MIN_ARRAY_SIZE = 1000;
int MAX_ARRAY_SIZE = 10000;

int MAX_ARITH_DEPTH = 5;

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

int RandValGen::get_rand_id (std::vector<Probability> vec) {
    uint64_t max_prob = 0;
    for (auto i = vec.begin(); i != vec.end(); ++i)
        max_prob += (*i).get_prob();
    uint64_t rand_num = get_rand_value<uint64_t> (0, max_prob);
    int k = 0;
    for (auto i = vec.begin(); i != vec.end(); ++i) {
        max_prob -= (*i).get_prob();
        if (rand_num >= max_prob)
            return (*i).get_id();
    }
    std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unable to select any id." << std::endl;
    exit (-1);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

GenPolicy::GenPolicy () {
    num_of_allowed_types = MAX_ALLOWED_TYPES;
    rand_init_allowed_types();

    allowed_modifiers.push_back (Data::Mod::NTHNG);

    allow_static_var = false;

    min_array_size = MIN_ARRAY_SIZE;
    max_array_size = MAX_ARRAY_SIZE;

    essence_differ = false;
    primary_essence = (Array::Ess) rand_val_gen->get_rand_value<int>(Array::Ess::C_ARR, Array::Ess::MAX_ESS - 1);

    max_arith_depth = MAX_ARITH_DEPTH;

    for (int i = UnaryExpr::Op::Plus; i < UnaryExpr::Op::MaxOp; ++i) {
        Probability prob (i, 1);
        allowed_unary_op.push_back (prob);
    }

    for (int i = 0; i < BinaryExpr::Op::MaxOp; ++i) {
        Probability prob (i, 1);
        allowed_binary_op.push_back (prob);
    }

    Probability data_leaf (ArithLeafID::Data, 10);
    arith_leaves.push_back (data_leaf);
    Probability unary_leaf (ArithLeafID::Unary, 20);
    arith_leaves.push_back (unary_leaf);
    Probability binary_leaf (ArithLeafID::Binary, 50);
    arith_leaves.push_back (binary_leaf);
    Probability type_cast_leaf (ArithLeafID::TypeCast, 10);
    arith_leaves.push_back (type_cast_leaf);

    Probability inp_data (ArithDataID::Inp, 70);
    arith_data_distr.push_back (inp_data);
    Probability const_data (ArithDataID::Const, 30);
    arith_data_distr.push_back (const_data);

    Probability const_branch (ArithSinglePattern::ConstBranch, 10);
    allowed_arith_single_patterns.push_back(const_branch);
    Probability half_const (ArithSinglePattern::HalfConst, 10);
    allowed_arith_single_patterns.push_back(half_const);
    Probability no_pattern (ArithSinglePattern::MAX_ARITH_SINGLE_PATTERN, 80);
    allowed_arith_single_patterns.push_back(no_pattern);

    chosen_arith_single_pattern = ArithSinglePattern::MAX_ARITH_SINGLE_PATTERN;

    allow_local_var = true;
/*
    allow_arrays = true;
    allow_scalar_variables = true;
*/
}

std::shared_ptr<GenPolicy> GenPolicy::apply_arith_single_pattern (ArithSinglePattern pattern_id) {
    chosen_arith_single_pattern = pattern_id;
    GenPolicy new_policy = *this;
    if (pattern_id == ArithSinglePattern::ConstBranch) {
        new_policy.arith_data_distr.clear();
        Probability const_data (ArithDataID::Const, 100);
        new_policy.arith_data_distr.push_back (const_data);
    }
    else if (pattern_id == ArithSinglePattern::HalfConst) {
        new_policy.arith_data_distr.clear();
        Probability inp_data (ArithDataID::Inp, 50);
        new_policy.arith_data_distr.push_back (inp_data);
        Probability const_data (ArithDataID::Const, 50);
        new_policy.arith_data_distr.push_back (const_data);
    }
    return std::make_shared<GenPolicy> (new_policy);
}

void GenPolicy::rand_init_allowed_types () {
    allowed_types.clear ();
    std::vector<Type::TypeID> tmp_allowed_types;
    int gen_types = 0;
    while (gen_types < num_of_allowed_types) {
        Type::TypeID type = (Type::TypeID) rand_val_gen->get_rand_value<int>(0, Type::TypeID::MAX_INT_ID - 1);
        if (std::find(tmp_allowed_types.begin(), tmp_allowed_types.end(), type) == tmp_allowed_types.end()) {
            tmp_allowed_types.push_back (type);
            gen_types++;
        }
    }
    for (auto i : tmp_allowed_types) {
        Probability prob (i, 1);
        allowed_types.push_back (prob);
    }
}

void GenPolicy::set_modifier (bool value, Data::Mod modifier) {
    if (value)
        allowed_modifiers.push_back (modifier);
    else
        allowed_modifiers.erase (std::remove (allowed_modifiers.begin(), allowed_modifiers.end(), modifier), allowed_modifiers.end());
}

bool GenPolicy::get_modifier (Data::Mod modifier) {
    return (std::find(allowed_modifiers.begin(), allowed_modifiers.end(), modifier) != allowed_modifiers.end());
}
