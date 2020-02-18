/*
Copyright (c) 2019-2020, Intel Corporation

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

#include "data.h"
#include "gen_policy.h"

#include <map>
#include <string>
#include <utility>

namespace yarpgen {

// Class that is used to determine the evaluation context.
// It allows us to evaluate the same arithmetic tree with different input
// values.
class EvalCtx {
  public:
    // TODO: we use string as a unique identifier and it is not a right way to
    // do it
    std::map<std::string, DataType> input;
};

class GenCtx {
  public:
    GenCtx() : loop_depth(0) { gen_policy = std::make_shared<GenPolicy>(); }
    void setGenPolicy(std::shared_ptr<GenPolicy> gen_pol) { gen_policy = std::move(gen_pol); }
    std::shared_ptr<GenPolicy> getGenPolicy() { return gen_policy; };

    size_t getLoopDepth() { return loop_depth; }
    void incLoopDepth(size_t change) { loop_depth += change; }
    void decLoopDepth(size_t change) { loop_depth -= change; }


  private:
    std::shared_ptr<GenPolicy> gen_policy;
    // Current loop depth
    size_t loop_depth;
};

class SymbolTable {
  public:
    void addVar(std::shared_ptr<ScalarVar> var) { vars.push_back(var); }
    void addArray(std::shared_ptr<Array> array) { arrays.push_back(array); }
    void addIter(std::shared_ptr<Iterator> iter) {iters.push_back(iter); }

    std::vector<std::shared_ptr<ScalarVar>> getVars() { return vars; }
    std::vector<std::shared_ptr<Array>> getArrays() { return arrays; }
    std::vector<std::shared_ptr<Iterator>> getIters() { return iters; }

  private:
    std::vector<std::shared_ptr<ScalarVar>> vars;
    std::vector<std::shared_ptr<Array>> arrays;
    std::vector<std::shared_ptr<Iterator>> iters;
};

//TODO: should we inherit it from Generation Context or should it be a separate thing?
class PopulateCtx : public GenCtx {
  public:
    PopulateCtx() = default;
    PopulateCtx(std::shared_ptr<PopulateCtx> ctx);

    std::shared_ptr<SymbolTable> getExtInpSymTablet() { return ext_inp_sym_tbl; }
    std::shared_ptr<SymbolTable> getExtOutSymTablet() { return ext_out_sym_tbl; }
    void setExtInpSymTable(std::shared_ptr<SymbolTable> _sym_table) { ext_inp_sym_tbl = std::move(_sym_table); }
    void setExtOutSymTable(std::shared_ptr<SymbolTable> _sym_table) { ext_out_sym_tbl = std::move(_sym_table); }

  private:
    std::shared_ptr<PopulateCtx> par_ctx;
    std::shared_ptr<SymbolTable> ext_inp_sym_tbl;
    std::shared_ptr<SymbolTable> ext_out_sym_tbl;
    std::shared_ptr<SymbolTable> local_sym_tbl;
};
} // namespace yarpgen
