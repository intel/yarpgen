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

#include <random>
#include <algorithm>
#include <iostream>
#include <memory>

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
        Probability (T _id, int _prob) : id(_id), prob (_prob) {}
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

        std::string get_struct_type_name() { return "struct_" + std::to_string(++struct_type_count); }
        uint64_t    get_struct_type_count() { return struct_type_count; }
        std::string get_scalar_var_name() { return "var_" + std::to_string(++scalar_var_count); }
        std::string get_struct_var_name() { return "struct_obj_" + std::to_string(++struct_var_count); }

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
        static uint64_t struct_type_count;
        static uint64_t scalar_var_count;
        static uint64_t struct_var_count;
};

extern std::shared_ptr<RandValGen> rand_val_gen;

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
            Data, Unary, Binary, TypeCast, CSE, MAX_LEAF_ID
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
            VAR, STRUCT, MAX_OUT_DATA_TYPE_ID
        };

        enum BitFieldID {
            UNNAMED, NAMED, MAX_BIT_FIELD_ID
        };
        ///////////////////////////////////////////////////////////////////////

        // General functions
        GenPolicy ();
        void copy_data (std::shared_ptr<GenPolicy> old);
        void init_from_config();

        // Complexity section
        static void add_to_complexity(Node::NodeID node_id);
        void set_max_test_complexity (uint64_t _compl) { max_test_complexity = _compl; }
        uint64_t get_max_test_complexity () { return max_test_complexity; }
        static uint64_t  get_test_complexity () { return test_complexity; }

        // Integer types section - defines number and type (bool, char ...) of available integer types
        void rand_init_allowed_int_types ();
        void set_num_of_allowed_int_types (int _num_of_allowed_int_types) { num_of_allowed_int_types = _num_of_allowed_int_types; }
        int get_num_of_allowed_int_types () { return num_of_allowed_int_types; }
        std::vector<Probability<IntegerType::IntegerTypeID>>& get_allowed_int_types () { return allowed_int_types; }
        void add_allowed_int_type (Probability<IntegerType::IntegerTypeID> allowed_int_type) { allowed_int_types.push_back(allowed_int_type); }

        // Modifiers section - defines available modifiers (nothing, const, volatile, const volatile)
        // TODO: Add check for options compability? Should allow_volatile + allow_const be equal to allow_const_volatile?
        void set_allow_volatile (bool _allow_volatile) { set_modifier (_allow_volatile, Type::Mod::VOLAT); }
        bool get_allow_volatile () { return get_modifier (Type::Mod::VOLAT); }
        void set_allow_const (bool _allow_const) { set_modifier (_allow_const, Type::Mod::CONST); }
        bool get_allow_const () { return get_modifier (Type::Mod::CONST); }
        void set_allow_const_volatile (bool _allow_const_volatile) { set_modifier (_allow_const_volatile, Type::Mod::CONST_VOLAT); }
        bool get_allow_const_volatile () { return get_modifier (Type::Mod::CONST_VOLAT); }
        std::vector<Type::Mod>& get_allowed_modifiers () { return allowed_modifiers; }

        // Static specifier section
        void set_allow_static_var (bool _allow_static_var) { allow_static_var = _allow_static_var; }
        bool get_allow_static_var () { return allow_static_var; }

        // Struct section - defines everything, related to struct types: their total number, range for number of members,
        // trigger for modifiers and specifiers of members, range for depth of nested struct types,
        // distribution of bit fields properties ...
        void set_allow_struct (bool _allow_struct) { allow_struct = _allow_struct; }
        bool get_allow_struct () { return allow_struct; }
        void set_min_struct_type_count (uint64_t _min_struct_type_count) { min_struct_type_count = _min_struct_type_count; }
        uint64_t get_min_struct_type_count () { return min_struct_type_count; }
        void set_max_struct_type_count (uint64_t _max_struct_type_count) { max_struct_type_count = _max_struct_type_count; }
        uint64_t get_max_struct_type_count () { return max_struct_type_count; }
        void set_min_inp_struct_count (uint64_t _min_inp_struct_count) { min_inp_struct_count = _min_inp_struct_count; }
        void set_min_struct_member_count (uint64_t _min_struct_member_count) { min_struct_member_count = _min_struct_member_count; }
        uint64_t get_min_struct_member_count () { return min_struct_member_count; }
        void set_max_struct_member_count (uint64_t _max_struct_member_count) { max_struct_member_count = _max_struct_member_count; }
        uint64_t get_max_struct_member_count () { return max_struct_member_count; }
        void set_allow_mix_mod_in_struct (bool mix) { allow_mix_mod_in_struct = mix; }
        bool get_allow_mix_mod_in_struct () { return allow_mix_mod_in_struct; }
        void set_allow_static_members (bool _allow_static_members) { allow_static_members = _allow_static_members; }
        bool get_allow_static_members () { return allow_static_members; }
        void set_allow_mix_static_in_struct (bool mix) { allow_mix_static_in_struct = mix; }
        bool get_allow_mix_static_in_struct () { return allow_mix_static_in_struct; }
        void set_allow_mix_types_in_struct (bool mix) { allow_mix_types_in_struct = mix; }
        bool get_allow_mix_types_in_struct () { return allow_mix_types_in_struct; }
        std::vector<Probability<bool>> get_member_use_prob () { return member_use_prob; }
        void set_max_struct_depth (uint64_t _max_struct_depth) { max_struct_depth = _max_struct_depth; }
        uint64_t get_max_struct_depth () { return max_struct_depth; }
        std::vector<Probability<Data::VarClassID>>& get_member_class_prob () { return member_class_prob; }
        void add_out_data_type_prob(Probability<OutDataTypeID> prob) { out_data_type_prob.push_back(prob); }
        void set_min_bit_field_size (uint64_t _min_bit_field_size) { min_bit_field_size = _min_bit_field_size; }
        uint64_t get_min_bit_field_size () { return min_bit_field_size; }
        void set_max_bit_field_size (uint64_t _max_bit_field_size) { max_bit_field_size = _max_bit_field_size; }
        uint64_t get_max_bit_field_size () { return max_bit_field_size; }
        std::vector<Probability<BitFieldID>>& get_bit_field_prob () { return bit_field_prob; }
        void add_bit_field_prob(Probability<BitFieldID> prob) { bit_field_prob.push_back(prob); }

        // Variables section - defines total number of variables of each kind (input and mix),
        // distribution of type of output variables.
        void set_min_inp_var_count (int _min_inp_var_count) { min_inp_var_count = _min_inp_var_count; }
        int get_min_inp_var_count () { return min_inp_var_count; }
        void set_max_inp_var_count (int _max_inp_var_count) { max_inp_var_count = _max_inp_var_count; }
        int get_max_inp_var_count () { return max_inp_var_count; }
        void set_min_mix_var_count (int _min_mix_var_count) { min_mix_var_count = _min_mix_var_count; }
        int get_min_mix_var_count () { return min_mix_var_count; }
        void set_max_mix_var_count (int _max_mix_var_count) { max_mix_var_count = _max_mix_var_count; }
        int get_max_mix_var_count () { return max_mix_var_count; }
        std::vector<Probability<OutDataTypeID>> get_out_data_type_prob() { return out_data_type_prob; }
        uint64_t get_min_inp_struct_count () { return min_inp_struct_count; }
        void set_max_inp_struct_count (uint64_t _max_inp_struct_count) { max_inp_struct_count = _max_inp_struct_count; }
        uint64_t get_max_inp_struct_count () { return max_inp_struct_count; }
        void set_min_mix_struct_count (uint64_t _min_mix_struct_count) { min_mix_struct_count = _min_mix_struct_count; }
        uint64_t get_min_mix_struct_count () { return min_mix_struct_count; }
        void set_max_mix_struct_count (uint64_t _max_mix_struct_count) { max_mix_struct_count = _max_mix_struct_count; }
        uint64_t get_max_mix_struct_count () { return max_mix_struct_count; }
        void set_min_out_struct_count (uint64_t _min_out_struct_count) { min_out_struct_count = _min_out_struct_count; }
        uint64_t get_min_out_struct_count () { return min_out_struct_count; }
        void set_max_out_struct_count (uint64_t _max_out_struct_count) { max_out_struct_count = _max_out_struct_count; }
        uint64_t get_max_out_struct_count () { return max_out_struct_count; }

        // Arithmetic expression tree section - defines depth, operators distribution, kind of leaves
        void set_max_arith_depth (int _max_arith_depth) { max_arith_depth = _max_arith_depth; }
        int get_max_arith_depth () { return max_arith_depth; }
        void add_unary_op (Probability<UnaryExpr::Op> prob) { allowed_unary_op.push_back(prob); }
        std::vector<Probability<UnaryExpr::Op>>& get_allowed_unary_op () { return allowed_unary_op; }
        void add_binary_op (Probability<BinaryExpr::Op> prob) { allowed_binary_op.push_back(prob); }
        std::vector<Probability<BinaryExpr::Op>>& get_allowed_binary_op () { return allowed_binary_op; }
        std::vector<Probability<ArithLeafID>>& get_arith_leaves () { return arith_leaves; }
        std::vector<Probability<ArithDataID>>& get_arith_data_distr () { return arith_data_distr; }

        // CSE section
        void set_max_cse_count (int _max_cse_count) { max_cse_count = _max_cse_count; }
        int get_max_cse_count () { return max_cse_count; }
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

        // Statement section - defines their number (per scope and total), distribution and properties
        std::vector<Probability<Node::NodeID>>& get_stmt_gen_prob () { return stmt_gen_prob; }
        void set_min_scope_stmt_count (int _min_scope_stmt_count) { min_scope_stmt_count = _min_scope_stmt_count; }
        int get_min_scope_stmt_count () { return min_scope_stmt_count; }
        void set_max_scope_stmt_count (int _max_scope_stmt_count) { max_scope_stmt_count = _max_scope_stmt_count; }
        int get_max_scope_stmt_count () { return max_scope_stmt_count; }
        void set_max_total_stmt_count (int _max_total_stmt_count) { max_total_stmt_count = _max_total_stmt_count; }
        int get_max_total_stmt_count () { return max_total_stmt_count; }
        std::vector<Probability<bool>>& get_else_prob () { return else_prob; }
        void set_max_if_depth (int _max_if_depth) { max_if_depth = _max_if_depth; }
        int get_max_if_depth () { return max_if_depth; }

        ///////////////////////////////////////////////////////////////////////

    private:
        static bool default_was_loaded;

        // Complexity
        static uint64_t test_complexity;
        uint64_t max_test_complexity;

        // Types
        int num_of_allowed_int_types;
        std::vector<Probability<IntegerType::IntegerTypeID>> allowed_int_types;

        // Modifiers
        void set_modifier (bool value, Type::Mod modifier);
        bool get_modifier (Type::Mod modifier);
        std::vector<Type::Mod> allowed_modifiers;

        // Static specifier
        bool allow_static_var;

        // Struct
        bool allow_struct;
        uint64_t min_struct_type_count;
        uint64_t max_struct_type_count;
        uint64_t min_struct_member_count;
        uint64_t max_struct_member_count;
        bool allow_mix_mod_in_struct;
        bool allow_mix_static_in_struct;
        bool allow_mix_types_in_struct;
        bool allow_static_members;
        std::vector<Probability<bool>> member_use_prob;
        std::vector<Probability<Data::VarClassID>> member_class_prob;
        uint64_t max_struct_depth;
        std::vector<Probability<OutDataTypeID>> out_data_type_prob;
        uint64_t min_bit_field_size;
        uint64_t max_bit_field_size;
        std::vector<Probability<BitFieldID>> bit_field_prob;

        // Variable
        int min_inp_var_count;
        int max_inp_var_count;
        int min_mix_var_count;
        int max_mix_var_count;
        uint64_t min_inp_struct_count;
        uint64_t max_inp_struct_count;
        uint64_t min_mix_struct_count;
        uint64_t max_mix_struct_count;
        uint64_t min_out_struct_count;
        uint64_t max_out_struct_count;

        // Arithmetic expression tree
        int max_arith_depth;
        std::vector<Probability<UnaryExpr::Op>> allowed_unary_op;
        std::vector<Probability<BinaryExpr::Op>> allowed_binary_op;
        std::vector<Probability<ArithLeafID>> arith_leaves;
        std::vector<Probability<ArithDataID>> arith_data_distr;

        // CSE
        int max_cse_count;
        std::vector<Probability<ArithCSEGenID>> arith_cse_gen;
        std::vector<std::shared_ptr<Expr>> cse;

        // Single statement pattern
        std::vector<Probability<ArithSSP::ConstUse>> allowed_arith_ssp_const_use;
        ArithSSP::ConstUse chosen_arith_ssp_const_use;
        std::vector<Probability<ArithSSP::SimilarOp>> allowed_arith_ssp_similar_op;
        ArithSSP::SimilarOp chosen_arith_ssp_similar_op;

        // Statements
        int min_scope_stmt_count;
        int max_scope_stmt_count;
        int max_total_stmt_count;
        std::vector<Probability<Node::NodeID>> stmt_gen_prob;
        std::vector<Probability<bool>> else_prob;
        int max_if_depth;
};

extern GenPolicy default_gen_policy;
}
