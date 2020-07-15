/*
Copyright (c) 2019-2020, Intel Corporation
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
#include "context.h"

#include <utility>

using namespace yarpgen;

PopulateCtx::PopulateCtx(std::shared_ptr<PopulateCtx> _par_ctx)
    : par_ctx(std::move(_par_ctx)), ext_inp_sym_tbl(par_ctx->ext_inp_sym_tbl),
      ext_out_sym_tbl(par_ctx->ext_out_sym_tbl), arith_depth(0), taken(true),
      inside_omp_simd(false) {
    local_sym_tbl = std::make_shared<SymbolTable>();
    if (par_ctx.use_count() != 0) {
        local_sym_tbl =
            std::make_shared<SymbolTable>(*(par_ctx->getLocalSymTable()));
        loop_depth = par_ctx->getLoopDepth();
        arith_depth = par_ctx->getArithDepth();
        taken = par_ctx->isTaken();
        inside_omp_simd = par_ctx->inside_omp_simd;
    }
}

PopulateCtx::PopulateCtx() {
    par_ctx = nullptr;
    local_sym_tbl = std::make_shared<SymbolTable>();
    ext_inp_sym_tbl = std::make_shared<SymbolTable>();
    ext_out_sym_tbl = std::make_shared<SymbolTable>();
    arith_depth = 0;
    taken = true;
    inside_omp_simd = false;
}
