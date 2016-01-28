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

///////////////////////////////////////////////////////////////////////////////

class RandValGen {
    public:
        RandValGen (uint64_t _seed);
        template<typename T>
        T get_rand_value (T from, T to) {
            std::uniform_int_distribution<T> dis(from, to);
            return dis(rand_gen);
        }

    private:
        uint64_t seed;
        std::mt19937_64 rand_gen;
};

extern std::shared_ptr<RandValGen> rand_val_gen;

///////////////////////////////////////////////////////////////////////////////

class GenPolicy {
    public:
        GenPolicy ();

        void set_num_of_allowed_types (int _num_of_allowed_types) { num_of_allowed_types = _num_of_allowed_types; }
        int get_num_of_allowed_types () { return num_of_allowed_types; }
        void rand_init_allowed_types ();
        std::vector<Type::TypeID>& get_allowed_types () { return allowed_types; }
        void set_allowed_types (std::vector<Type::TypeID> _allowed_types) { allowed_types = _allowed_types; }

        // TODO: Add check for options compability? Should allow_volatile + allow_const be equal to allow_const_volatile?
        void set_allow_volatile (bool _allow_volatile) { set_modifier (_allow_volatile, Data::Mod::VOLAT); }
        bool get_allow_volatile () { return get_modifier (Data::Mod::VOLAT); }
        void set_allow_const (bool _allow_const) { set_modifier (_allow_const, Data::Mod::CONST); }
        bool get_allow_const () { return get_modifier (Data::Mod::CONST); }
        void set_allow_const_volatile (bool _allow_const_volatile) { set_modifier (_allow_const_volatile, 
                                                                                   Data::Mod::CONST_VOLAT); }
        bool get_allow_const_volatile () { return get_modifier (Data::Mod::CONST_VOLAT); }
        std::vector<Data::Mod>& get_allowed_modifiers () { return allowed_modifiers; }

/*
        void set_allow_ (bool _allow_) { allow_ = _allow_; }
        bool get_allow_ () { return allow_; }

        // TODO: Add check for options compability
        void set_allow_local_var (bool _allow_local_var) { allow_local_var = _allow_local_var; }
        bool get_allow_local_var () { return allow_local_var; }
        void set_allow_arrays (bool _allow_arrays) { allow_arrays = _allow_arrays; }
        bool get_allow_arrays () { return allow_arrays; }
        void set_allow_scalar_variables (bool _allow_scalar_variables) { allow_scalar_variables = _allow_scalar_variables; }
        bool get_allow_scalar_variables () { return allow_scalar_variables; }
*/

    private:
        // Number of allowed types
        int num_of_allowed_types;
        // Allowed types of variables and basic types of arrays
        std::vector<Type::TypeID> allowed_types;

        void set_modifier (bool value, Data::Mod modifier);
        bool get_modifier (Data::Mod modifier);
        std::vector<Data::Mod> allowed_modifiers;
/*
        // Indicates whether the local variables are allowed
        bool allow_local_var;
        // Indicates whether the arrays  are allowed
        bool allow_arrays;
        // Indicates whether the scalar (non-array) variables are allowed
        bool allow_scalar_variables;
*/
};
