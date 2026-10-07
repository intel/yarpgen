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
#include "options.h"
#include "stmt.h"

#include <iostream>

using namespace yarpgen;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__            \
                      << ", function " << __func__ << "():\n    " << (msg)     \
                      << std::endl;                                            \
            abort();                                                           \
        }                                                                      \
    } while (false)

// The simple-loop profiles override parts of the policy that other options
// have already set up, and must not contradict them.
static void simpleLoopPolicyTest() {
    Options &options = Options::getInstance();
    VectorizerTarget orig_target = options.getVectorizerTarget();

    // "--max-array-dims=1": the GCC/Clang profile used to raise the limit to
    // 2, beyond what stencil_in_dim_prob was set up for.
    options.setMaxArrayDims(1);
    options.setVectorizerTarget(VectorizerTarget::GCC_CLANG);
    GenPolicy capped_gen_pol;
    capped_gen_pol.makeVectorizable(/*simple*/ true);
    CHECK(capped_gen_pol.array_dims_num_limit == 1, "array dims cap");
    options.setMaxArrayDims(0);

    // "--emit-pragmas=all": the MSVC profile allows no pragmas at all, which
    // used to make Pragma::create sample an empty distribution.
    options.setVectorizerTarget(VectorizerTarget::MSVC);
    auto msvc_gen_pol = std::make_shared<GenPolicy>();
    msvc_gen_pol->makeVectorizable(/*simple*/ true);
    auto pop_ctx = std::make_shared<PopulateCtx>();
    pop_ctx->setGenPolicy(msvc_gen_pol);
    auto pragmas = Pragma::create(
        static_cast<size_t>(PragmaKind::MAX_PRAGMA_KIND) - 1, pop_ctx);
    CHECK(pragmas.empty(), "no pragmas allowed");

    options.setVectorizerTarget(orig_target);
}

int main() {
    rand_val_gen = std::make_shared<RandValGen>(0);
    simpleLoopPolicyTest();

    auto gen_ctx = std::make_shared<GenCtx>();
    auto scope_stmt = ScopeStmt::generateStructure(gen_ctx);
    auto emit_ctx = std::make_shared<EmitCtx>();
    scope_stmt->emit(emit_ctx, std::cout);
    std::cout << std::endl;
    return 0;
}
