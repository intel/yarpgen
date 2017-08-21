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
#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>

#include "type.h"
#include "variable.h"
#include "expr.h"
#include "stmt.h"

///////////////////////////////////////////////////////////////////////////////

namespace yarpgen {

// This class links together id (for example, type of unary operator) and its probability.
// Usually it is used in the form of std::vector<Probability<id>> and defines all possible variants
// for random decision (probability itself measured in parts, similarly to std::discrete_distribution).
// According to agreement, sum of all probabilities in vector should be 100 (so we can treat 1 part as 1 percent).
template<typename T>
class Probability {
    public:
        Probability (T _id, uint64_t _prob) : id(_id), prob (_prob) {}
        T get_id () { return id; }
        uint64_t get_prob () { return prob; }
        void increase_prob(uint64_t add_prob) { prob += add_prob; }

    private:
        T id;
        uint64_t prob;
};

// According to agreement, Random Value Generator is the only way to get any random value in YARP Generator.
// It is used for different random decisions all over the source code.
// Also it tracks name numbering of all generated variables, struct and etc.
// Moreover, it performs shuffling of initial parameters of default generation policy.
class RandValGen {
    public:
        // Specific seed can be passed to constructor to reproduce the test.
        // Zero value is reserved (it notifies RandValGen that it can choose any)
        RandValGen (uint64_t _seed);

        template<typename T>
        T get_rand_value (T from, T to) {
            std::uniform_int_distribution<T> dis(from, to);
            return dis(rand_gen);
        }

        // Randomly chooses one of IDs, basing on std::vector<Probability<id>>.
        template<typename T>
        T get_rand_id (std::vector<Probability<T>> vec) {
            std::vector<double> discrete_dis_init;
            for (auto i : vec)
                discrete_dis_init.push_back(i.get_prob());
            std::discrete_distribution<int> discrete_dis(discrete_dis_init.begin(), discrete_dis_init.end());
            return vec.at(discrete_dis(rand_gen)).get_id();
        }

        // Randomly chooses one of vec elements
        template<typename T>
        T& get_rand_elem (std::vector<T>& vec) {
            uint64_t idx = get_rand_value(0UL, vec.size() - 1);
            return vec.at(idx);
        }

        // To improve variety of generated tests, we implement shuffling of
        // input probabilities (they are stored in GenPolicy).
        // TODO: sometimes this action increases test complexity, and tests becomes non-generatable.
        template <typename T>
        void shuffle_prob(std::vector<Probability<T>> &prob_vec) {
            int total_prob = 0;
            std::vector<double> discrete_dis_init;
            std::vector<Probability<T>> new_prob;
            for (auto i : prob_vec) {
                total_prob += i.get_prob();
                discrete_dis_init.push_back(i.get_prob());
                new_prob.push_back(Probability<T>(i.get_id(), 0));
            }

            std::uniform_int_distribution<int> dis (1, total_prob);
            int delta = round(((double) total_prob) / dis(rand_gen));

            std::discrete_distribution<int> discrete_dis(discrete_dis_init.begin(), discrete_dis_init.end());
            for (int i = 0; i < total_prob; i += delta)
                new_prob.at(discrete_dis(rand_gen)).increase_prob(delta);

            prob_vec = new_prob;
        }

    private:
        uint64_t seed;
        std::mt19937_64 rand_gen;
};

template <>
inline bool RandValGen::get_rand_value<bool> (bool from, bool to) {
    std::uniform_int_distribution<int> dis((int)from, (int)to);
    return (bool)dis(rand_gen);
}

extern std::shared_ptr<RandValGen> rand_val_gen;

// Singleton class which handles name's creation of all variables, structures, etc.
class NameHandler {
    public:
        static const std::string common_test_func_prefix;

        static NameHandler& get_instance() {
            static NameHandler instance;
            return instance;
        }

        NameHandler(const NameHandler& root) = delete;
        NameHandler& operator=(const NameHandler&) = delete;

        void set_test_func_prefix (uint32_t prefix) { test_func_prefix = common_test_func_prefix +
                                                                         std::to_string(prefix) + "_"; }
        std::string get_struct_type_name() { return test_func_prefix + "struct_" + std::to_string(++struct_type_count); }
        uint32_t    get_struct_type_count() { return struct_type_count; }
        std::string get_scalar_var_name() { return test_func_prefix + "var_" + std::to_string(++scalar_var_count); }
        std::string get_struct_var_name() { return test_func_prefix + "struct_obj_" + std::to_string(++struct_var_count); }
        std::string get_array_var_name() { return test_func_prefix + "array_" + std::to_string(++array_var_count); }
        void zero_out_counters () { struct_type_count = scalar_var_count = struct_var_count = array_var_count = 0; }

    private:
        NameHandler() : struct_type_count(0), scalar_var_count(0), struct_var_count(0) {};

        // Test function prefix is required for multiple functions in one test
        std::string test_func_prefix;
        uint32_t struct_type_count;
        uint32_t scalar_var_count;
        uint32_t struct_var_count;
        uint32_t array_var_count;
};

///////////////////////////////////////////////////////////////////////////////

// Nowadays we don't have patterns for anything other than a single statement,
// but new approach to arrays generation (and other improvements) are going to introduce it.
struct Pattern {

};

// Patterns ID for single arithmetic statement.
struct ArithSSP : public Pattern {

    // It changes constants usage probability
    enum ConstUse {
        CONST_BRANCH, // All leaves of arithmetic expression tree should be constants
        HALF_CONST,   // Half of all leaves of arithmetic expression tree should be constants
        MAX_CONST_USE
    };

    // It changes distribution of operators
    enum SimilarOp {
        ADDITIVE, // +, - (unary and binary)
        BITWISE,  // ~, &, |, ^
        LOGIC,    // !, &&, ||
        MUL,      // *, TODO: add div
        BIT_SH,   // ~, &, |, ^, >>, <<
        ADD_MUL,  // +, - (unary and binary) and *
        MAX_SIMILAR_OP
    };
};

struct ConstPattern : public Pattern {
    enum SpecialConst {
        Zero = 0,
        One = 1,
        Two = 2,
        Three = 3,
        Four = 4,
        Eight = 8,
        Sixteen = 16,
        MAX_SPECIAL_CONST // Maximal value for type
    };

    enum NewConstKind {
        EndBits, // Continuous bit block at the beginning or end
        BitBlock, // Continuous bit block in the middle
        MAX_NEW_CONST_KIND // New non-special constant
    };
};
///////////////////////////////////////////////////////////////////////////////
// GenPolicy stores actual distribution of all available parameters, used during
// random decision process (distributions, applied patterns and etc).
// This parameters are responsible for properties of output test.
// At start all parameters are loaded from config, then some of them are shuffled.
// GenPolicy could be modified during generation process in order to shape output test and gave it desired properties.
// Every randomly generated entity requires its Context, and Context contains its unique GenPolicy object.
class GenPolicy {
    public:
        // Utility enums. They are used as IDs in Probability<ID>
        enum ArithLeafID {
            Data, Unary, Binary, Conditional, TypeCast, CSE, MAX_LEAF_ID
        };

        enum ArithDataID {
            Inp, Const, MAX_DATA_ID
        };

        // TODO: this can be replaced with true/false
        enum ArithCSEGenID {
            Add, MAX_CSE_GEN_ID
        };

        //TODO: This can be replaced with enum from Data
        enum OutDataTypeID {
            VAR, STRUCT, VAR_IN_ARRAY, STRUCT_IN_ARRAY, MAX_OUT_DATA_TYPE_ID
        };

        enum OutDataCategoryID {
            MIX, OUT, MAX_OUT_DATA_CATEGORY_ID
        };

        enum BitFieldID {
            UNNAMED, NAMED, MAX_BIT_FIELD_ID
        };
        ///////////////////////////////////////////////////////////////////////

        // General functions
        GenPolicy ();
        void copy_data (std::shared_ptr<GenPolicy> old);
        void init_from_config();

        uint32_t get_test_func_count () { return test_func_count; }

        // Complexity section
        static void add_to_complexity(Node::NodeID node_id);
        void set_max_test_complexity (uint64_t _compl) { max_test_complexity = _compl; }
        uint64_t get_max_test_complexity () { return max_test_complexity; }
        static uint64_t  get_test_complexity () { return test_complexity; }

        // Integer types section - defines number and type (bool, char ...) of available integer types
        void rand_init_allowed_int_types ();
        void set_num_of_allowed_int_types (uint32_t _num_of_allowed_int_types) { num_of_allowed_int_types = _num_of_allowed_int_types; }
        uint32_t get_num_of_allowed_int_types () { return num_of_allowed_int_types; }
        std::vector<Probability<IntegerType::IntegerTypeID>>& get_allowed_int_types () { return allowed_int_types; }
        void add_allowed_int_type (Probability<IntegerType::IntegerTypeID> allowed_int_type) { allowed_int_types.push_back(allowed_int_type); }

        // cv-qualifiers section - defines available cv-qualifiers (nothing, const, volatile, const volatile)
        // TODO: Add check for options compability? Should allow_volatile + allow_const be equal to allow_const_volatile?
        void set_allow_volatile (bool _allow_volatile) { set_cv_qual(_allow_volatile, Type::CV_Qual::VOLAT); }
        bool get_allow_volatile () { return get_cv_qual(Type::CV_Qual::VOLAT); }
        void set_allow_const (bool _allow_const) { set_cv_qual(_allow_const, Type::CV_Qual::CONST); }
        bool get_allow_const () { return get_cv_qual(Type::CV_Qual::CONST); }
        void set_allow_const_volatile (bool _allow_const_volatile) {
            set_cv_qual(_allow_const_volatile, Type::CV_Qual::CONST_VOLAT); }
        bool get_allow_const_volatile () { return get_cv_qual(Type::CV_Qual::CONST_VOLAT); }
        std::vector<Type::CV_Qual>& get_allowed_cv_qual() { return allowed_cv_qual; }

        // Static specifier section
        void set_allow_static_var (bool _allow_static_var) { allow_static_var = _allow_static_var; }
        bool get_allow_static_var () { return allow_static_var; }

        // Struct section - defines everything, related to struct types: their total number, range for number of members,
        // trigger for cv-qualifiers and specifiers of members, range for depth of nested struct types,
        // distribution of bit fields properties ...
        void set_allow_struct (bool _allow_struct) { allow_struct = _allow_struct; }
        bool get_allow_struct () { return allow_struct; }
        void set_min_struct_type_count (uint32_t _min_struct_type_count) { min_struct_type_count = _min_struct_type_count; }
        uint32_t get_min_struct_type_count () { return min_struct_type_count; }
        void set_max_struct_type_count (uint32_t _max_struct_type_count) { max_struct_type_count = _max_struct_type_count; }
        uint32_t get_max_struct_type_count () { return max_struct_type_count; }
        void set_min_struct_member_count (uint32_t _min_struct_member_count) { min_struct_member_count = _min_struct_member_count; }
        uint32_t get_min_struct_member_count () { return min_struct_member_count; }
        void set_max_struct_member_count (uint32_t _max_struct_member_count) { max_struct_member_count = _max_struct_member_count; }
        uint32_t get_max_struct_member_count () { return max_struct_member_count; }
        void set_allow_mix_cv_qual_in_struct(bool mix) { allow_mix_cv_qual_in_struct = mix; }
        bool get_allow_mix_cv_qual_in_struct() { return allow_mix_cv_qual_in_struct; }
        void set_allow_static_members (bool _allow_static_members) { allow_static_members = _allow_static_members; }
        bool get_allow_static_members () { return allow_static_members; }
        void set_allow_mix_static_in_struct (bool mix) { allow_mix_static_in_struct = mix; }
        bool get_allow_mix_static_in_struct () { return allow_mix_static_in_struct; }
        void set_allow_mix_types_in_struct (bool mix) { allow_mix_types_in_struct = mix; }
        bool get_allow_mix_types_in_struct () { return allow_mix_types_in_struct; }
        std::vector<Probability<bool>> get_member_use_prob () { return member_use_prob; }
        void set_max_struct_depth (uint32_t _max_struct_depth) { max_struct_depth = _max_struct_depth; }
        uint32_t get_max_struct_depth () { return max_struct_depth; }
        std::vector<Probability<Data::VarClassID>>& get_member_class_prob () { return member_class_prob; }
        void set_min_bit_field_size (uint32_t _min_bit_field_size) { min_bit_field_size = _min_bit_field_size; }
        uint32_t get_min_bit_field_size () { return min_bit_field_size; }
        void set_max_bit_field_size (uint32_t _max_bit_field_size) { max_bit_field_size = _max_bit_field_size; }
        uint32_t get_max_bit_field_size () { return max_bit_field_size; }
        std::vector<Probability<BitFieldID>>& get_bit_field_prob () { return bit_field_prob; }
        void add_bit_field_prob(Probability<BitFieldID> prob) { bit_field_prob.push_back(prob); }

        // Variables section - defines total number of variables of each kind (input and mix),
        // distribution of type of output variables.
        void add_out_data_type_prob(Probability<OutDataTypeID> prob) { out_data_type_prob.push_back(prob); }
        std::vector<Probability<OutDataTypeID>> get_out_data_type_prob() { return out_data_type_prob; }
        void add_out_data_category_prob(Probability<OutDataCategoryID > prob) { out_data_category_prob.push_back(prob); }
        std::vector<Probability<OutDataCategoryID>> get_out_data_category_prob() { return out_data_category_prob; }
        void set_min_inp_var_count (uint32_t _min_inp_var_count) { min_inp_var_count = _min_inp_var_count; }
        uint32_t get_min_inp_var_count () { return min_inp_var_count; }
        void set_max_inp_var_count (uint32_t _max_inp_var_count) { max_inp_var_count = _max_inp_var_count; }
        uint32_t get_max_inp_var_count () { return max_inp_var_count; }
        void set_min_mix_var_count (uint32_t _min_mix_var_count) { min_mix_var_count = _min_mix_var_count; }
        uint32_t get_min_mix_var_count () { return min_mix_var_count; }
        void set_max_mix_var_count (uint32_t _max_mix_var_count) { max_mix_var_count = _max_mix_var_count; }
        uint32_t get_max_mix_var_count () { return max_mix_var_count; }
        void set_min_inp_struct_count (uint32_t _min_inp_struct_count) { min_inp_struct_count = _min_inp_struct_count; }
        uint32_t get_min_inp_struct_count () { return min_inp_struct_count; }
        void set_max_inp_struct_count (uint32_t _max_inp_struct_count) { max_inp_struct_count = _max_inp_struct_count; }
        uint32_t get_max_inp_struct_count () { return max_inp_struct_count; }
        void set_min_mix_struct_count (uint32_t _min_mix_struct_count) { min_mix_struct_count = _min_mix_struct_count; }
        uint32_t get_min_mix_struct_count () { return min_mix_struct_count; }
        void set_max_mix_struct_count (uint32_t _max_mix_struct_count) { max_mix_struct_count = _max_mix_struct_count; }
        uint32_t get_max_mix_struct_count () { return max_mix_struct_count; }
        void set_min_out_struct_count (uint32_t _min_out_struct_count) { min_out_struct_count = _min_out_struct_count; }
        uint32_t get_min_out_struct_count () { return min_out_struct_count; }
        void set_max_out_struct_count (uint32_t _max_out_struct_count) { max_out_struct_count = _max_out_struct_count; }
        uint32_t get_max_out_struct_count () { return max_out_struct_count; }
        uint32_t get_min_inp_array_count () { return min_inp_array_count; }
        void set_max_inp_array_count (uint32_t _max_inp_array_count) { max_inp_array_count = _max_inp_array_count; }
        uint32_t get_max_inp_array_count () { return max_inp_array_count; }
        void set_min_mix_array_count (uint32_t _min_mix_array_count) { min_mix_array_count = _min_mix_array_count; }
        uint32_t get_min_mix_array_count () { return min_mix_array_count; }
        void set_max_mix_array_count (uint32_t _max_mix_array_count) { max_mix_array_count = _max_mix_array_count; }
        uint32_t get_max_mix_array_count () { return max_mix_array_count; }
        void set_min_out_array_count (uint32_t _min_out_array_count) { min_out_array_count = _min_out_array_count; }
        uint32_t get_min_out_array_count () { return min_out_array_count; }
        void set_max_out_array_count (uint32_t _max_out_array_count) { max_out_array_count = _max_out_array_count; }
        uint32_t get_max_out_array_count () { return max_out_array_count; }

        // Arrays section - defines arrays' sizes, their kind, base type probability
        uint32_t get_min_array_size () { return min_array_size; }
        void set_min_array_size (uint32_t _min_array_size) { min_array_size = _min_array_size; }
        uint32_t get_max_array_size () { return max_array_size; }
        void set_max_array_size (uint32_t _max_array_size) { max_array_size = _max_array_size; }
        std::vector<Probability<ArrayType::Kind>>& get_array_kind_prob () { return array_kind_prob; }
        std::vector<Probability<Type::TypeID>>& get_array_base_type_prob () { return array_base_type_prob; }
        void set_min_array_type_count (uint32_t _min_array_type_count) { min_array_type_count = _min_array_type_count; }
        uint32_t get_min_array_type_count () { return min_array_type_count; }
        void set_max_array_type_count (uint32_t _max_array_type_count) { max_array_type_count = _max_array_type_count; }
        uint32_t get_max_array_type_count () { return max_array_type_count; }
        std::vector<Probability<ArrayType::ElementSubscript>>& get_array_elem_subs_prob () { return array_elem_subs_prob; }

        // Arithmetic expression tree section - defines depth, operators distribution, kind of leaves
        void set_max_arith_depth (uint32_t _max_arith_depth) { max_arith_depth = _max_arith_depth; }
        uint32_t get_max_arith_depth () { return max_arith_depth; }
        void add_unary_op (Probability<UnaryExpr::Op> prob) { allowed_unary_op.push_back(prob); }
        std::vector<Probability<UnaryExpr::Op>>& get_allowed_unary_op () { return allowed_unary_op; }
        void add_binary_op (Probability<BinaryExpr::Op> prob) { allowed_binary_op.push_back(prob); }
        std::vector<Probability<BinaryExpr::Op>>& get_allowed_binary_op () { return allowed_binary_op; }
        std::vector<Probability<ArithLeafID>>& get_arith_leaves () { return arith_leaves; }
        std::vector<Probability<ArithDataID>>& get_arith_data_distr () { return arith_data_distr; }
        void set_max_total_expr_count(uint32_t max_count) { max_total_expr_count = max_count; }
        uint32_t get_max_total_expr_count() { return max_total_expr_count; }
        void set_max_func_expr_count(uint32_t max_count) { max_func_expr_count = max_count; }
        uint32_t get_max_func_expr_count() { return max_func_expr_count; }

        // CSE section
        void set_max_cse_count (uint32_t _max_cse_count) { max_cse_count = _max_cse_count; }
        uint32_t get_max_cse_count () { return max_cse_count; }
        // TODO: add depth control
        std::vector<std::shared_ptr<Expr>>& get_cse () { return cse; };
        void add_cse (std::shared_ptr<Expr> expr) { cse.push_back(expr); }
        std::vector<Probability<ArithCSEGenID>>& get_arith_cse_gen () { return arith_cse_gen; }

        // Single statement pattern
        std::vector<Probability<ArithSSP::ConstUse>>& get_allowed_arith_ssp_const_use () { return allowed_arith_ssp_const_use; }
        ArithSSP::ConstUse get_chosen_arith_ssp_const_use () { return chosen_arith_ssp_const_use; }
        GenPolicy apply_arith_ssp_const_use (ArithSSP::ConstUse pattern_id);
        std::vector<Probability<ArithSSP::SimilarOp>>& get_allowed_arith_ssp_similar_op () { return allowed_arith_ssp_similar_op; }
        ArithSSP::SimilarOp get_chosen_arith_ssp_similar_op () { return chosen_arith_ssp_similar_op; }
        GenPolicy apply_arith_ssp_similar_op (ArithSSP::SimilarOp pattern_id);

        // Constant generation
        uint32_t get_const_buffer_size () { return const_buffer_size; }
        std::vector<Probability<bool>>& get_new_const_prob () { return new_const_prob; }
        std::vector<Probability<bool>>& get_new_const_type_prob () { return new_const_type_prob; }
        std::vector<Probability<ConstPattern::SpecialConst>>& get_special_const_prob () { return special_const_prob; }
        std::vector<Probability<ConstPattern::NewConstKind>>& get_new_const_kind_prob () { return new_const_kind_prob; }
        std::vector<Probability<UnaryExpr::Op>>& get_const_transform_prob () { return const_transform_prob; }

        // Statement section - defines their number (per scope and total), distribution and properties
        std::vector<Probability<Node::NodeID>>& get_stmt_gen_prob () { return stmt_gen_prob; }
        void set_min_scope_stmt_count (uint32_t _min_scope_stmt_count) { min_scope_stmt_count = _min_scope_stmt_count; }
        uint32_t get_min_scope_stmt_count () { return min_scope_stmt_count; }
        void set_max_scope_stmt_count (uint32_t _max_scope_stmt_count) { max_scope_stmt_count = _max_scope_stmt_count; }
        uint32_t get_max_scope_stmt_count () { return max_scope_stmt_count; }
        void set_max_total_stmt_count (uint32_t _max_total_stmt_count) { max_total_stmt_count = _max_total_stmt_count; }
        uint32_t get_max_total_stmt_count () { return max_total_stmt_count; }
        void set_max_func_stmt_count (uint32_t _max_func_stmt_count) { max_func_stmt_count = _max_func_stmt_count; }
        uint32_t get_max_func_stmt_count () { return max_func_stmt_count; }
        std::vector<Probability<bool>>& get_else_prob () { return else_prob; }
        void set_max_if_depth (uint32_t _max_if_depth) { max_if_depth = _max_if_depth; }
        uint32_t get_max_if_depth () { return max_if_depth; }

        ///////////////////////////////////////////////////////////////////////

    private:
        static bool default_was_loaded;

        // Number of independent test functions in one test
        uint32_t test_func_count;

        // Complexity
        static uint64_t test_complexity;
        uint64_t max_test_complexity;

        // Types
        uint32_t num_of_allowed_int_types;
        std::vector<Probability<IntegerType::IntegerTypeID>> allowed_int_types;

        // cv-qualifiers
        void set_cv_qual(bool value, Type::CV_Qual cv_qual);
        bool get_cv_qual(Type::CV_Qual cv_qual);
        std::vector<Type::CV_Qual> allowed_cv_qual;

        // Static specifier
        bool allow_static_var;

        // Struct
        bool allow_struct;
        uint32_t min_struct_type_count;
        uint32_t max_struct_type_count;
        uint32_t min_struct_member_count;
        uint32_t max_struct_member_count;
        bool allow_mix_cv_qual_in_struct;
        bool allow_mix_static_in_struct;
        bool allow_mix_types_in_struct;
        bool allow_static_members;
        std::vector<Probability<bool>> member_use_prob;
        std::vector<Probability<Data::VarClassID>> member_class_prob;
        uint32_t max_struct_depth;
        uint32_t min_bit_field_size;
        uint32_t max_bit_field_size;
        std::vector<Probability<BitFieldID>> bit_field_prob;

        // Variable
        std::vector<Probability<OutDataTypeID>> out_data_type_prob;
        std::vector<Probability<OutDataCategoryID>> out_data_category_prob;
        uint32_t min_inp_var_count;
        uint32_t max_inp_var_count;
        uint32_t min_mix_var_count;
        uint32_t max_mix_var_count;
        uint32_t min_inp_struct_count;
        uint32_t max_inp_struct_count;
        uint32_t min_mix_struct_count;
        uint32_t max_mix_struct_count;
        uint32_t min_out_struct_count;
        uint32_t max_out_struct_count;
        uint32_t min_inp_array_count;
        uint32_t max_inp_array_count;
        uint32_t min_mix_array_count;
        uint32_t max_mix_array_count;
        uint32_t min_out_array_count;
        uint32_t max_out_array_count;

        // Array
        uint32_t min_array_size;
        uint32_t max_array_size;
        std::vector<Probability<ArrayType::Kind>> array_kind_prob;
        std::vector<Probability<Type::TypeID>> array_base_type_prob;
        uint32_t min_array_type_count;
        uint32_t max_array_type_count;
        std::vector<Probability<ArrayType::ElementSubscript>> array_elem_subs_prob;

        // Arithmetic expression tree
        uint32_t max_arith_depth;
        std::vector<Probability<UnaryExpr::Op>> allowed_unary_op;
        std::vector<Probability<BinaryExpr::Op>> allowed_binary_op;
        std::vector<Probability<ArithLeafID>> arith_leaves;
        std::vector<Probability<ArithDataID>> arith_data_distr;
        uint32_t max_total_expr_count;
        uint32_t max_func_expr_count;

        // CSE
        uint32_t max_cse_count;
        std::vector<Probability<ArithCSEGenID>> arith_cse_gen;
        std::vector<std::shared_ptr<Expr>> cse;

        // Single statement pattern
        std::vector<Probability<ArithSSP::ConstUse>> allowed_arith_ssp_const_use;
        ArithSSP::ConstUse chosen_arith_ssp_const_use;
        std::vector<Probability<ArithSSP::SimilarOp>> allowed_arith_ssp_similar_op;
        ArithSSP::SimilarOp chosen_arith_ssp_similar_op;

        // Constant generation
        uint32_t const_buffer_size;
        std::vector<Probability<bool>> new_const_prob;
        std::vector<Probability<bool>> new_const_type_prob;
        std::vector<Probability<ConstPattern::SpecialConst>> special_const_prob;
        std::vector<Probability<ConstPattern::NewConstKind>> new_const_kind_prob;
        std::vector<Probability<UnaryExpr::Op>> const_transform_prob;

        // Statements
        uint32_t min_scope_stmt_count;
        uint32_t max_scope_stmt_count;
        uint32_t max_total_stmt_count;
        uint32_t max_func_stmt_count;
        std::vector<Probability<Node::NodeID>> stmt_gen_prob;
        std::vector<Probability<bool>> else_prob;
        uint32_t max_if_depth;
};

extern GenPolicy default_gen_policy;
}
