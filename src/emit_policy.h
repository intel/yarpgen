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

#pragma once

#include "enums.h"
#include "utils.h"
#include <vector>

namespace yarpgen {

class EmitPolicy {
  public:
    EmitPolicy();

    std::vector<Probability<bool>> asserts_check_distr;
    std::vector<Probability<bool>> pass_as_param_distr;
    std::vector<Probability<bool>> emit_align_attr_distr;
    std::vector<Probability<AlignmentSize>> align_size_distr;
};

} // namespace yarpgen
