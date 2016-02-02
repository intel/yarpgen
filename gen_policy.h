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

class Probability {
    public:
        Probability (int _id, int _prob) : id(_id), prob (_prob) {}
        int get_id () { return id; }
        uint64_t get_prob () { return prob; }

    private:
        int id;
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
        int get_rand_id (std::vector<Probability> vec);

    private:
        uint64_t seed;
        std::mt19937_64 rand_gen;
};

extern std::shared_ptr<RandValGen> rand_val_gen;

///////////////////////////////////////////////////////////////////////////////

class GenPolicy {
    public:
        GenPolicy ();

        enum ArithLeafID {
            Data, Unary, Binary, TypeCast, MAX_LEAF_ID
        };

        void set_num_of_allowed_types (int _num_of_allowed_types) { num_of_allowed_types = _num_of_allowed_types; }
        int get_num_of_allowed_types () { return num_of_allowed_types; }
        void rand_init_allowed_types ();
        std::vector<Probability>& get_allowed_types () { return allowed_types; }
        void add_allowed_type (Probability allowed_type) { allowed_types.push_back(allowed_type); }

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

        void add_unary_op (Probability prob) { allowed_unary_op.push_back(prob); }
        std::vector<Probability>& get_allowed_unary_op () { return allowed_unary_op; }
        std::vector<Probability>& get_allowed_binary_op () { return allowed_binary_op; }
        std::vector<Probability>& get_arith_leaves () { return arith_leaves; }

        // TODO: Add check for options compability
        void set_allow_local_var (bool _allow_local_var) { allow_local_var = _allow_local_var; }
        bool get_allow_local_var () { return allow_local_var; }
/*
        void set_allow_arrays (bool _allow_arrays) { allow_arrays = _allow_arrays; }
        bool get_allow_arrays () { return allow_arrays; }
        void set_allow_scalar_variables (bool _allow_scalar_variables) { allow_scalar_variables = _allow_scalar_variables; }
        bool get_allow_scalar_variables () { return allow_scalar_variables; }
*/

    private:
        // Number of allowed types
        int num_of_allowed_types;
        // Allowed types of variables and basic types of arrays
        std::vector<Probability> allowed_types;

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

        std::vector<Probability> allowed_unary_op;
        std::vector<Probability> allowed_binary_op;
        std::vector<Probability> arith_leaves;

        // Indicates whether the local variables are allowed
        bool allow_local_var;
/*
        // Indicates whether the arrays  are allowed
        bool allow_arrays;
        // Indicates whether the scalar (non-array) variables are allowed
        bool allow_scalar_variables;
*/
};
