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

#include "stmt.h"

#include <memory>

namespace yarpgen {

class ProgramGenerator {
  public:
    ProgramGenerator();
    void emit();

  private:
    void emitCheckFunc(std::ostream &stream);
    void emitDecl(std::shared_ptr<EmitCtx> ctx, std::ostream &stream);
    void emitInit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream);
    void emitCheck(std::shared_ptr<EmitCtx> ctx, std::ostream &stream);
    void emitExtDecl(std::shared_ptr<EmitCtx> ctx, std::ostream &stream);
    void emitTest(std::shared_ptr<EmitCtx> ctx, std::ostream &stream);
    void emitMain(std::shared_ptr<EmitCtx> ctx, std::ostream &stream);

    std::shared_ptr<SymbolTable> ext_inp_sym_tbl;
    std::shared_ptr<SymbolTable> ext_out_sym_tbl;
    std::shared_ptr<ScopeStmt> new_test;

    unsigned long long int hash_seed;
    void hash(unsigned long long int const v);
    void hashArray(std::shared_ptr<Array> const &arr);
    void hashArrayStep(std::shared_ptr<Array> const &arr,
                       std::vector<size_t> &dims, std::vector<size_t> &idx_vec,
                       size_t cur_idx, bool has_to_use_init_val,
                       uint64_t &init_val, uint64_t &cur_val,
                       std::vector<size_t> &steps);
};

} // namespace yarpgen
