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
    num_of_allowed_types = MAX_ALLOWED_TYPES;
    rand_init_allowed_types();

    allowed_modifiers.push_back (Data::Mod::NTHNG);
/*
    allow_local_var = true;
    allow_arrays = true;
    allow_scalar_variables = true;
*/
}

void GenPolicy::rand_init_allowed_types () {
    allowed_types.clear ();
    int gen_types = 0;
    while (gen_types < num_of_allowed_types) {
        Type::TypeID type = (Type::TypeID) rand_val_gen->get_rand_value<int>(Type::TypeID::CHAR, Type::TypeID::MAX_INT_ID - 1);
        if (std::find(allowed_types.begin(), allowed_types.end(), type) == allowed_types.end()) {
            allowed_types.push_back (type);
            gen_types++;
        }
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
