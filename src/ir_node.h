/*
Copyright (c) 2015-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

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

#include <iostream>

namespace yarpgen {

class EmitCtx;

class IRNode {
  public:
    virtual ~IRNode() = default;
    // This method emits internal representation of the test to a contextual
    // form using recursive calls. The callee is responsible for all of the
    // wrapping (e.g., parentheses). The caller is obligated to handle the
    // offset properly.
    // TODO: in the future we might output the same test using different
    // language constructions
    virtual void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                      std::string offset = "") = 0;
    // TODO: make it pure virtual later
    virtual void populate(std::shared_ptr<PopulateCtx> ctx){};
};

} // namespace yarpgen
