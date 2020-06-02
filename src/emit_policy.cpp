/*
Copyright (c) 2020, Intel Corporation
Copyright (c) 2020, University of Utah

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

#include "emit_policy.h"

using namespace yarpgen;

EmitPolicy::EmitPolicy() {
    asserts_check_distr.emplace_back(Probability<bool>(true, 50));
    asserts_check_distr.emplace_back(Probability<bool>(false, 50));

    pass_as_param_distr.emplace_back(Probability<bool>(true, 50));
    pass_as_param_distr.emplace_back(Probability<bool>(true, 50));

    emit_align_attr_distr.emplace_back(Probability<bool>(true, 50));
    emit_align_attr_distr.emplace_back(Probability<bool>(true, 50));

    align_size_distr.emplace_back(
        Probability<AlignmentSize>(AlignmentSize::A16, 33));
    align_size_distr.emplace_back(
        Probability<AlignmentSize>(AlignmentSize::A32, 33));
    align_size_distr.emplace_back(
        Probability<AlignmentSize>(AlignmentSize::A64, 33));
}
