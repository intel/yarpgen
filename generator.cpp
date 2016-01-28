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

#include "generator.h"

void ScalarTypeGen::generate () {
    Type::TypeID type_id = gen_policy->get_allowed_types().at(
                           rand_val_gen->get_rand_value<int>(0, gen_policy->get_allowed_types().size() - 1));
    type = Type::init (type_id);
}

void ModifierGen::generate () {
    modifier = gen_policy->get_allowed_modifiers().at(
               rand_val_gen->get_rand_value<int>(0, gen_policy->get_allowed_modifiers().size() - 1));
}
