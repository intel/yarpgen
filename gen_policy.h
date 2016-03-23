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

#include "type.h"
#include "variable.h"
#include "node.h"

///////////////////////////////////////////////////////////////////////////////

template<typename T>
class Probability {
    public:
        Probability (T _id, int _prob) : id(_id), prob (_prob) {}
        T get_id () { return id; }
        uint64_t get_prob () { return prob; }

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
        }

    private:
        uint64_t seed;
        std::mt19937_64 rand_gen;
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
            Inp, Const, Reuse, MAX_DATA_ID
        };

        enum ArithCSEGenID {
            Add, MAX_CSE_GEN_ID
        };

        void copy_data (std::shared_ptr<GenPolicy> old);

        void set_num_of_allowed_int_types (int _num_of_allowed_int_types) { num_of_allowed_int_types = _num_of_allowed_int_types; }
        int get_num_of_allowed_int_types () { return num_of_allowed_int_types; }
        void rand_init_allowed_int_types ();
        std::vector<Probability<IntegerType::IntegerTypeID>>& get_allowed_int_types () { return allowed_int_types; }
        void add_allowed_int_type (Probability<IntegerType::IntegerTypeID> allowed_int_type) { allowed_int_types.push_back(allowed_int_type); }

        // TODO: Add check for options compability? Should allow_volatile + allow_const be equal to allow_const_volatile?
        void set_allow_volatile (bool _allow_volatile) { set_modifier (_allow_volatile, Data::Mod::VOLAT); }
        bool get_allow_volatile () { return get_modifier (Data::Mod::VOLAT); }
        void set_allow_const (bool _allow_const) { set_modifier (_allow_const, Data::Mod::CONST); }
        bool get_allow_const () { return get_modifier (Data::Mod::CONST); }
        void set_allow_const_volatile (bool _allow_const_volatile) { set_modifier (_allow_const_volatile, Data::Mod::CONST_VOLAT); }
        bool get_allow_const_volatile () { return get_modifier (Data::Mod::CONST_VOLAT); }
        std::vector<Data::Mod>& get_allowed_modifiers () { return allowed_modifiers; }

        void set_allow_static_var (bool _allow_static_var) { allow_static_var = _allow_static_var; }
        bool get_allow_static_var () { return allow_static_var; }

        void set_min_array_size (int _min_array_size) { min_array_size = _min_array_size; }
        int get_min_array_size () { return min_array_size; }
        void set_max_array_size (int _max_array_size) { max_array_size = _max_array_size; }
        int get_max_array_size () { return max_array_size; }

        void set_essence_differ (bool _essence_differ) { essence_differ = _essence_differ; }
        bool get_essence_differ () { return essence_differ; }
        void set_primary_essence (Array::Ess _primary_essence) { primary_essence = _primary_essence; }
        Array::Ess get_primary_essence () { return primary_essence; }

        void set_max_arith_depth (int _max_arith_depth) { max_arith_depth = _max_arith_depth; }
        int get_max_arith_depth () { return max_arith_depth; }

        std::vector<Probability<Node::NodeID>>& get_stmt_gen_prob () { return stmt_gen_prob; }

        void set_min_arith_stmt_num (int _min_arith_stmt_num) { min_arith_stmt_num = _min_arith_stmt_num; }
        int get_min_arith_stmt_num () { return min_arith_stmt_num; }
        void set_max_arith_stmt_num (int _max_arith_stmt_num) { max_arith_stmt_num = _max_arith_stmt_num; }
        int get_max_arith_stmt_num () { return max_arith_stmt_num; }

        void add_unary_op (Probability<UnaryExpr::Op> prob) { allowed_unary_op.push_back(prob); }
        std::vector<Probability<UnaryExpr::Op>>& get_allowed_unary_op () { return allowed_unary_op; }
        void add_binary_op (Probability<BinaryExpr::Op> prob) { allowed_binary_op.push_back(prob); }
        std::vector<Probability<BinaryExpr::Op>>& get_allowed_binary_op () { return allowed_binary_op; }
        std::vector<Probability<ArithLeafID>>& get_arith_leaves () { return arith_leaves; }
        std::vector<Probability<ArithDataID>>& get_arith_data_distr () { return arith_data_distr; }

        // TODO: Add check for options compability
        void set_allow_local_var (bool _allow_local_var) { allow_local_var = _allow_local_var; }
        bool get_allow_local_var () { return allow_local_var; }

        // Pattern
        std::vector<Probability<ArithSSP::ConstUse>>& get_allowed_arith_ssp_const_use () { return allowed_arith_ssp_const_use; }
        ArithSSP::ConstUse get_chosen_arith_ssp_const_use () { return chosen_arith_ssp_const_use; }
        std::shared_ptr<GenPolicy> apply_arith_ssp_const_use (ArithSSP::ConstUse pattern_id);

        std::vector<Probability<ArithSSP::SimilarOp>>& get_allowed_arith_ssp_similar_op () { return allowed_arith_ssp_similar_op; }
        ArithSSP::SimilarOp get_chosen_arith_ssp_similar_op () { return chosen_arith_ssp_similar_op; }
        std::shared_ptr<GenPolicy> apply_arith_ssp_similar_op (ArithSSP::SimilarOp pattern_id);

        std::vector<std::shared_ptr<Expr>>& get_used_data_expr () { return used_data_expr; }
        void add_used_data_expr (std::shared_ptr<Expr> expr);

        void set_max_cse_num (int _max_cse_num) { max_cse_num = _max_cse_num; }
        int get_max_cse_num () { return max_cse_num; }
        // TODO: add depth control
        std::vector<std::shared_ptr<Expr>>& get_cse () { return cse; };
        void add_cse (std::shared_ptr<Expr> expr) { cse.push_back(expr); }
        std::vector<Probability<ArithCSEGenID>>& get_arith_cse_gen () { return arith_cse_gen; }

        void set_max_tmp_var_num (int _max_tmp_var_num) { max_tmp_var_num = _max_tmp_var_num; }
        int get_max_tmp_var_num () { return max_tmp_var_num; }
        int get_used_tmp_var_num () { return used_tmp_var_num; }
        void add_used_tmp_var_num () { used_tmp_var_num++; }


        std::vector<Probability<bool>>& get_else_prob () { return else_prob; }
        void set_max_if_depth (int _max_if_depth) { max_if_depth = _max_if_depth; }
        int get_max_if_depth () { return max_if_depth; }

/*
        void set_allow_arrays (bool _allow_arrays) { allow_arrays = _allow_arrays; }
        bool get_allow_arrays () { return allow_arrays; }
        void set_allow_scalar_variables (bool _allow_scalar_variables) { allow_scalar_variables = _allow_scalar_variables; }
        bool get_allow_scalar_variables () { return allow_scalar_variables; }
*/

    private:
        // Number of allowed integer types
        int num_of_allowed_int_types;
        // Allowed types of variables and basic types of arrays
        std::vector<Probability<IntegerType::IntegerTypeID>> allowed_int_types;

        void set_modifier (bool value, Data::Mod modifier);
        bool get_modifier (Data::Mod modifier);
        std::vector<Data::Mod> allowed_modifiers;

        bool allow_static_var;

        int min_array_size;
        int max_array_size;

        // TODO: Add vector of allowed essence
        bool essence_differ;
        Array::Ess primary_essence;

        int max_arith_depth;
        int min_arith_stmt_num;
        int max_arith_stmt_num;

        std::vector<Probability<Node::NodeID>> stmt_gen_prob;

        std::vector<Probability<UnaryExpr::Op>> allowed_unary_op;
        std::vector<Probability<BinaryExpr::Op>> allowed_binary_op;
        std::vector<Probability<ArithLeafID>> arith_leaves;
        std::vector<Probability<ArithDataID>> arith_data_distr;

        std::vector<Probability<ArithSSP::ConstUse>> allowed_arith_ssp_const_use;
        ArithSSP::ConstUse chosen_arith_ssp_const_use;

        std::vector<Probability<ArithSSP::SimilarOp>> allowed_arith_ssp_similar_op;
        ArithSSP::SimilarOp chosen_arith_ssp_similar_op;

        std::vector<std::shared_ptr<Expr>> used_data_expr;

        int max_cse_num;
        std::vector<Probability<ArithCSEGenID>> arith_cse_gen;
        std::vector<std::shared_ptr<Expr>> cse;

        int max_tmp_var_num;
        int used_tmp_var_num;

        // Indicates whether the local variables are allowed
        bool allow_local_var;

        std::vector<Probability<bool>> else_prob;
        int max_if_depth;
/*
        // Indicates whether the arrays  are allowed
        bool allow_arrays;
        // Indicates whether the scalar (non-array) variables are allowed
        bool allow_scalar_variables;
*/
};
