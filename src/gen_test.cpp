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

#include "context.h"
#include "stmt.h"

#include <iostream>

using namespace yarpgen;

int main() {
    rand_val_gen = std::make_shared<RandValGen>(0);

    auto gen_ctx = std::make_shared<GenCtx>();
    auto scope_stmt = ScopeStmt::generateStructure(gen_ctx);
    auto emit_ctx = std::make_shared<EmitCtx>();
    scope_stmt->emit(emit_ctx, std::cout);
    std::cout << std::endl;
    return 0;
}
