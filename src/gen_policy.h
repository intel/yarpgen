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

namespace rl {

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

class RandValGen {
    public:
        RandValGen (uint64_t _seed);
        template<typename T>
        T get_rand_value (T from, T to) {
            std::uniform_int_distribution<T> dis(from, to);
            return dis(rand_gen);
        }

        template<typename T>
        T get_rand_id (std::vector<Probability<T>> vec) {
            std::vector<double> discrete_dis_init;
            for (auto i : vec)
                discrete_dis_init.push_back(i.get_prob());
            std::discrete_distribution<int> discrete_dis(discrete_dis_init.begin(), discrete_dis_init.end());
            return vec.at(discrete_dis(rand_gen)).get_id();
        }

        std::string get_struct_type_name() { return "struct_" + std::to_string(++struct_type_num); }
        uint64_t get_struct_type_num() { return struct_type_num; }
        std::string get_scalar_var_name() { return "var_" + std::to_string(++scalar_var_num); }
        std::string get_struct_var_name() { return "struct_obj_" + std::to_string(++struct_var_num); }

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
        static uint64_t struct_type_num;
        static uint64_t scalar_var_num;
        static uint64_t struct_var_num;
};

extern std::shared_ptr<RandValGen> rand_val_gen;

///////////////////////////////////////////////////////////////////////////////

struct Pattern {

};

// Patterns ID for single arithmetic statement
struct ArithSSP : public Pattern {
    enum ConstUse {
        CONST_BRANCH, HALF_CONST, MAX_CONST_USE
    };
    enum SimilarOp {
        ADDITIVE, BITWISE, LOGIC, MUL, BIT_SH, ADD_MUL, MAX_SIMILAR_OP
    };
};

///////////////////////////////////////////////////////////////////////////////
class GenPolicy {
    public:
        GenPolicy ();

        enum ArithLeafID {
            Data, Unary, Binary, TypeCast, CSE, MAX_LEAF_ID
        };

        enum ArithDataID {
            Inp, Const, MAX_DATA_ID
        };

        enum ArithCSEGenID {
            Add, MAX_CSE_GEN_ID
        };

        enum OutDataTypeID {
            VAR, STRUCT, MAX_OUT_DATA_TYPE_ID
        };

        enum BitFieldID {
            UNNAMED, NAMED, MAX_BIT_FIELD_ID
        };

        void copy_data (std::shared_ptr<GenPolicy> old);

        void set_num_of_allowed_int_types (int _num_of_allowed_int_types) { num_of_allowed_int_types = _num_of_allowed_int_types; }
        int get_num_of_allowed_int_types () { return num_of_allowed_int_types; }
        void rand_init_allowed_int_types ();
        std::vector<Probability<IntegerType::IntegerTypeID>>& get_allowed_int_types () { return allowed_int_types; }
        void add_allowed_int_type (Probability<IntegerType::IntegerTypeID> allowed_int_type) { allowed_int_types.push_back(allowed_int_type); }

        // TODO: Add check for options compability? Should allow_volatile + allow_const be equal to allow_const_volatile?
        void set_allow_volatile (bool _allow_volatile) { set_modifier (_allow_volatile, Type::Mod::VOLAT); }
        bool get_allow_volatile () { return get_modifier (Type::Mod::VOLAT); }
        void set_allow_const (bool _allow_const) { set_modifier (_allow_const, Type::Mod::CONST); }
        bool get_allow_const () { return get_modifier (Type::Mod::CONST); }
        void set_allow_const_volatile (bool _allow_const_volatile) { set_modifier (_allow_const_volatile, Type::Mod::CONST_VOLAT); }
        bool get_allow_const_volatile () { return get_modifier (Type::Mod::CONST_VOLAT); }
        std::vector<Type::Mod>& get_allowed_modifiers () { return allowed_modifiers; }

        void set_allow_static_var (bool _allow_static_var) { allow_static_var = _allow_static_var; }
        bool get_allow_static_var () { return allow_static_var; }

        void set_allow_static_members (bool _allow_static_members) { allow_static_members = _allow_static_members; }
        bool get_allow_static_members () { return allow_static_members; }


        void set_max_arith_depth (int _max_arith_depth) { max_arith_depth = _max_arith_depth; }
        int get_max_arith_depth () { return max_arith_depth; }

        std::vector<Probability<Node::NodeID>>& get_stmt_gen_prob () { return stmt_gen_prob; }

        void set_min_scope_stmt_num (int _min_scope_stmt_num) { min_scope_stmt_num = _min_scope_stmt_num; }
        int get_min_scope_stmt_num () { return min_scope_stmt_num; }
        void set_max_scope_stmt_num (int _max_scope_stmt_num) { max_scope_stmt_num = _max_scope_stmt_num; }
        int get_max_scope_stmt_num () { return max_scope_stmt_num; }
        void set_max_total_stmt_num (int _max_total_stmt_num) { max_total_stmt_num = _max_total_stmt_num; }
        int get_max_total_stmt_num () { return max_total_stmt_num; }

        void set_min_inp_var_num (int _min_inp_var_num) { min_inp_var_num = _min_inp_var_num; }
        int get_min_inp_var_num () { return min_inp_var_num; }
        void set_max_inp_var_num (int _max_inp_var_num) { max_inp_var_num = _max_inp_var_num; }
        int get_max_inp_var_num () { return max_inp_var_num; }

        void set_min_mix_var_num (int _min_mix_var_num) { min_mix_var_num = _min_mix_var_num; }
        int get_min_mix_var_num () { return min_mix_var_num; }
        void set_max_mix_var_num (int _max_mix_var_num) { max_mix_var_num = _max_mix_var_num; }
        int get_max_mix_var_num () { return max_mix_var_num; }

        void add_unary_op (Probability<UnaryExpr::Op> prob) { allowed_unary_op.push_back(prob); }
        std::vector<Probability<UnaryExpr::Op>>& get_allowed_unary_op () { return allowed_unary_op; }
        void add_binary_op (Probability<BinaryExpr::Op> prob) { allowed_binary_op.push_back(prob); }
        std::vector<Probability<BinaryExpr::Op>>& get_allowed_binary_op () { return allowed_binary_op; }
        std::vector<Probability<ArithLeafID>>& get_arith_leaves () { return arith_leaves; }
        std::vector<Probability<ArithDataID>>& get_arith_data_distr () { return arith_data_distr; }

        // Pattern
        std::vector<Probability<ArithSSP::ConstUse>>& get_allowed_arith_ssp_const_use () { return allowed_arith_ssp_const_use; }
        ArithSSP::ConstUse get_chosen_arith_ssp_const_use () { return chosen_arith_ssp_const_use; }
        GenPolicy apply_arith_ssp_const_use (ArithSSP::ConstUse pattern_id);

        std::vector<Probability<ArithSSP::SimilarOp>>& get_allowed_arith_ssp_similar_op () { return allowed_arith_ssp_similar_op; }
        ArithSSP::SimilarOp get_chosen_arith_ssp_similar_op () { return chosen_arith_ssp_similar_op; }
        GenPolicy apply_arith_ssp_similar_op (ArithSSP::SimilarOp pattern_id);

        void set_max_cse_num (int _max_cse_num) { max_cse_num = _max_cse_num; }
        int get_max_cse_num () { return max_cse_num; }
        // TODO: add depth control
        std::vector<std::shared_ptr<Expr>>& get_cse () { return cse; };
        void add_cse (std::shared_ptr<Expr> expr) { cse.push_back(expr); }
        std::vector<Probability<ArithCSEGenID>>& get_arith_cse_gen () { return arith_cse_gen; }

        std::vector<Probability<bool>>& get_else_prob () { return else_prob; }
        void set_max_if_depth (int _max_if_depth) { max_if_depth = _max_if_depth; }
        int get_max_if_depth () { return max_if_depth; }

        void set_allow_struct (bool _allow_struct) { allow_struct = _allow_struct; }
        bool get_allow_struct () { return allow_struct; }
        void set_min_struct_types_num (uint64_t _min_struct_types_num) { min_struct_types_num = _min_struct_types_num; }
        uint64_t get_min_struct_types_num () { return min_struct_types_num; }
        void set_max_struct_types_num (uint64_t _max_struct_types_num) { max_struct_types_num = _max_struct_types_num; }
        uint64_t get_max_struct_types_num () { return max_struct_types_num; }
        void set_min_inp_struct_num (uint64_t _min_inp_struct_num) { min_inp_struct_num = _min_inp_struct_num; }
        uint64_t get_min_inp_struct_num () { return min_inp_struct_num; }
        void set_max_inp_struct_num (uint64_t _max_inp_struct_num) { max_inp_struct_num = _max_inp_struct_num; }
        uint64_t get_max_inp_struct_num () { return max_inp_struct_num; }
        void set_min_mix_struct_num (uint64_t _min_mix_struct_num) { min_mix_struct_num = _min_mix_struct_num; }
        uint64_t get_min_mix_struct_num () { return min_mix_struct_num; }
        void set_max_mix_struct_num (uint64_t _max_mix_struct_num) { max_mix_struct_num = _max_mix_struct_num; }
        uint64_t get_max_mix_struct_num () { return max_mix_struct_num; }
        void set_min_out_struct_num (uint64_t _min_out_struct_num) { min_out_struct_num = _min_out_struct_num; }
        uint64_t get_min_out_struct_num () { return min_out_struct_num; }
        void set_max_out_struct_num (uint64_t _max_out_struct_num) { max_out_struct_num = _max_out_struct_num; }
        uint64_t get_max_out_struct_num () { return max_out_struct_num; }
        void set_min_struct_members_num (uint64_t _min_struct_members_num) { min_struct_members_num = _min_struct_members_num; }
        uint64_t get_min_struct_members_num () { return min_struct_members_num; }
        void set_max_struct_members_num (uint64_t _max_struct_members_num) { max_struct_members_num = _max_struct_members_num; }
        uint64_t get_max_struct_members_num () { return max_struct_members_num; }
        void set_allow_mix_mod_in_struct (bool mix) { allow_mix_mod_in_struct = mix; }
        bool get_allow_mix_mod_in_struct () { return allow_mix_mod_in_struct; }
        void set_allow_mix_static_in_struct (bool mix) { allow_mix_static_in_struct = mix; }
        bool get_allow_mix_static_in_struct () { return allow_mix_static_in_struct; }
        void set_allow_mix_types_in_struct (bool mix) { allow_mix_types_in_struct = mix; }
        bool get_allow_mix_types_in_struct () { return allow_mix_types_in_struct; }
        std::vector<Probability<bool>> get_member_use_prob () { return member_use_prob; }
        void set_max_struct_depth (uint64_t _max_struct_depth) { max_struct_depth = _max_struct_depth; }
        uint64_t get_max_struct_depth () { return max_struct_depth; }
        std::vector<Probability<Data::VarClassID>>& get_member_class_prob () { return member_class_prob; }
        void add_out_data_type_prob(Probability<OutDataTypeID> prob) { out_data_type_prob.push_back(prob); }
        std::vector<Probability<OutDataTypeID>> get_out_data_type_prob() { return out_data_type_prob; }
        void set_min_bit_field_size (uint64_t _min_bit_field_size) { min_bit_field_size = _min_bit_field_size; }
        uint64_t get_min_bit_field_size () { return min_bit_field_size; }
        void set_max_bit_field_size (uint64_t _max_bit_field_size) { max_bit_field_size = _max_bit_field_size; }
        uint64_t get_max_bit_field_size () { return max_bit_field_size; }
        std::vector<Probability<BitFieldID>>& get_bit_field_prob () { return bit_field_prob; }
        void add_bit_field_prob(Probability<BitFieldID> prob) { bit_field_prob.push_back(prob); }
        void init_from_config();


    private:
        static bool default_was_loaded;
        // Number of allowed integer types
        int num_of_allowed_int_types;
        // Allowed types of variables and basic types of arrays
        std::vector<Probability<IntegerType::IntegerTypeID>> allowed_int_types;

        bool allow_struct;
        uint64_t min_struct_types_num;
        uint64_t max_struct_types_num;
        uint64_t min_inp_struct_num;
        uint64_t max_inp_struct_num;
        uint64_t min_mix_struct_num;
        uint64_t max_mix_struct_num;
        uint64_t min_out_struct_num;
        uint64_t max_out_struct_num;
        uint64_t min_struct_members_num;
        uint64_t max_struct_members_num;
        bool allow_mix_mod_in_struct;
        bool allow_mix_static_in_struct;
        bool allow_mix_types_in_struct;
        std::vector<Probability<bool>> member_use_prob;
        std::vector<Probability<Data::VarClassID>> member_class_prob;
        uint64_t max_struct_depth;
        std::vector<Probability<OutDataTypeID>> out_data_type_prob;
        uint64_t min_bit_field_size;
        uint64_t max_bit_field_size;
        std::vector<Probability<BitFieldID>> bit_field_prob;

        void set_modifier (bool value, Type::Mod modifier);
        bool get_modifier (Type::Mod modifier);
        std::vector<Type::Mod> allowed_modifiers;

        bool allow_static_var;
        bool allow_static_members;

        int max_arith_depth;
        int min_scope_stmt_num;
        int max_scope_stmt_num;
        int max_total_stmt_num;

        std::vector<Probability<Node::NodeID>> stmt_gen_prob;

        std::vector<Probability<UnaryExpr::Op>> allowed_unary_op;
        std::vector<Probability<BinaryExpr::Op>> allowed_binary_op;
        std::vector<Probability<ArithLeafID>> arith_leaves;
        std::vector<Probability<ArithDataID>> arith_data_distr;

        std::vector<Probability<ArithSSP::ConstUse>> allowed_arith_ssp_const_use;
        ArithSSP::ConstUse chosen_arith_ssp_const_use;

        std::vector<Probability<ArithSSP::SimilarOp>> allowed_arith_ssp_similar_op;
        ArithSSP::SimilarOp chosen_arith_ssp_similar_op;

        int max_cse_num;
        std::vector<Probability<ArithCSEGenID>> arith_cse_gen;
        std::vector<std::shared_ptr<Expr>> cse;

        int min_inp_var_num;
        int max_inp_var_num;
        int min_mix_var_num;
        int max_mix_var_num;

        std::vector<Probability<bool>> else_prob;
        int max_if_depth;
};

extern GenPolicy default_gen_policy;
}
