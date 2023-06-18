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

#pragma once

#include "data.h"
#include "emit_policy.h"
#include "expr.h"
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
    EvalCtx()
        : total_iter_num(-1), mul_vals_iter(nullptr), use_main_vals(true) {}
    // TODO: we use string as a unique identifier and it is not a right way to
    // do it
    std::map<std::string, DataType> input;
    // The total number of iterations that we have to do
    // -1 is used as a poison value that indicates that the information is
    // unknown
    int64_t total_iter_num;

    // Iterator that is used to iterate over multiple values
    std::shared_ptr<Iterator> mul_vals_iter;
    // If true, we use main values for evaluation
    bool use_main_vals;
};

class GenCtx {
  public:
    GenCtx() : loop_depth(0), if_else_depth(0), inside_foreach(false) {
        gen_policy = std::make_shared<GenPolicy>();
    }
    void setGenPolicy(std::shared_ptr<GenPolicy> gen_pol) {
        gen_policy = std::move(gen_pol);
    }
    std::shared_ptr<GenPolicy> getGenPolicy() { return gen_policy; };

    size_t getLoopDepth() { return loop_depth; }
    void incLoopDepth(size_t change) { loop_depth += change; }
    void decLoopDepth(size_t change) { loop_depth -= change; }
    size_t getIfElseDepth() { return if_else_depth; }
    void incIfElseDepth() { if_else_depth++; }
    void decIfElseDepth() { if_else_depth--; }

    void setInsideForeach(bool _inside) { inside_foreach = _inside; }
    bool isInsideForeach() { return inside_foreach; }

  protected:
    std::shared_ptr<GenPolicy> gen_policy;
    // Current loop depth
    size_t loop_depth;
    size_t if_else_depth;

    // ISPC
    bool inside_foreach;
};

// Auxiliary class for stencil generation
// It defines all parameters for each stencil element
class ArrayStencilParams {
  public:
    explicit ArrayStencilParams(std::shared_ptr<Array> _arr)
        : arr(std::move(_arr)), dims_defined(false), offsets_defined(false),
          dims_order(SubscriptOrderKind::RANDOM) {}

    // We use an array of structures instead of a structure of arrays to keep
    // parameters of each dimension together
    // If parameter is not defined, it is set to false/nullptr/zero accordingly
    struct ArrayStencilDimParams {
        bool dim_active;
        // This is an absolute index of the iterator in context
        // We need to save this info to create special subscript expressions
        size_t abs_idx;
        std::shared_ptr<Iterator> iter;
        int64_t offset;

        ArrayStencilDimParams()
            : dim_active(false), abs_idx(0), iter(nullptr), offset(0) {}
    };

    std::shared_ptr<Array> getArray() { return arr; }

    void setParams(std::vector<ArrayStencilDimParams> _params,
                   bool _dims_defined, bool _offsets_defined,
                   SubscriptOrderKind _dims_order) {
        params = std::move(_params);
        dims_defined = _dims_defined;
        offsets_defined = _offsets_defined;
        dims_order = _dims_order;
    }

    std::vector<ArrayStencilDimParams> &getParams() { return params; }

    bool areDimsDefined() const { return dims_defined; }
    bool areOffsetsDefined() const { return offsets_defined; }
    SubscriptOrderKind getDimsOrderKind() const { return dims_order; }

  private:
    std::shared_ptr<Array> arr;
    bool dims_defined;
    bool offsets_defined;
    SubscriptOrderKind dims_order;
    std::vector<ArrayStencilDimParams> params;
};

class SymbolTable {
  public:
    void addVar(std::shared_ptr<ScalarVar> var) { vars.push_back(var); }
    void addArray(std::shared_ptr<Array> array);
    void addIters(std::shared_ptr<Iterator> iter) { iters.push_back(iter); }
    void deleteLastIters() { iters.pop_back(); }

    std::vector<std::shared_ptr<ScalarVar>> getVars() { return vars; }
    std::vector<std::shared_ptr<Array>> getArrays() { return arrays; }
    std::vector<std::shared_ptr<Array>> getArraysWithDimNum(size_t dim);
    std::vector<std::shared_ptr<Iterator>> &getIters() { return iters; }

    void addVarExpr(std::shared_ptr<ScalarVarUseExpr> var) {
        avail_vars.push_back(var);
    }

    std::vector<std::shared_ptr<ScalarVarUseExpr>> getAvailVars() {
        return avail_vars;
    }

    void setStencilsParams(std::vector<ArrayStencilParams> _stencils) {
        stencils = _stencils;
    }
    std::vector<ArrayStencilParams> &getStencilsParams() { return stencils; }

  private:
    std::vector<std::shared_ptr<ScalarVar>> vars;
    std::vector<std::shared_ptr<Array>> arrays;
    std::map<size_t, std::vector<std::shared_ptr<Array>>> array_dim_map;
    std::vector<std::shared_ptr<Iterator>> iters;
    std::vector<std::shared_ptr<ScalarVarUseExpr>> avail_vars;
    std::vector<ArrayStencilParams> stencils;
};

// TODO: should we inherit it from Generation Context or should it be a separate
// thing?
class PopulateCtx : public GenCtx {
  public:
    PopulateCtx();
    explicit PopulateCtx(std::shared_ptr<PopulateCtx> ctx);

    std::shared_ptr<PopulateCtx> getParentCtx() { return par_ctx; }

    std::shared_ptr<SymbolTable> getExtInpSymTable() { return ext_inp_sym_tbl; }
    std::shared_ptr<SymbolTable> getExtOutSymTable() { return ext_out_sym_tbl; }
    std::shared_ptr<SymbolTable> getLocalSymTable() { return local_sym_tbl; }
    void setExtInpSymTable(std::shared_ptr<SymbolTable> _sym_table) {
        ext_inp_sym_tbl = std::move(_sym_table);
    }
    void setExtOutSymTable(std::shared_ptr<SymbolTable> _sym_table) {
        ext_out_sym_tbl = std::move(_sym_table);
    }

    size_t getArithDepth() { return arith_depth; }
    void incArithDepth() { arith_depth++; }
    void decArithDepth() { arith_depth--; }

    bool isTaken() { return taken; }
    void setTaken(bool _taken) { taken = _taken; }

    bool isInsideMutation() { return inside_mutation; }
    void setIsInsideMutation(bool _val) { inside_mutation = _val; }

    void setInsideOMPSimd(bool val) { inside_omp_simd = val; }
    bool isInsideOMPSimd() { return inside_omp_simd; }

    size_t generateNumberOfDims(ArrayDimsUseKind dims_use_kind) const;
    void addDimension(size_t dim) { dims.push_back(dim); }
    std::vector<size_t> getDimensions() { return dims; }
    void deleteLastDim() { dims.pop_back(); }

    void setInStencil(bool _val) { in_stencil = _val; }
    bool getInStencil() { return in_stencil; }

    void setMulValsIter(std::shared_ptr<Iterator> _iter) {
        mul_vals_iter = _iter;
    }
    std::shared_ptr<Iterator> getMulValsIter() { return mul_vals_iter; }

    void setAllowMulVals(bool _val) { allow_mul_vals = _val; }
    bool getAllowMulVals() { return allow_mul_vals; }

  private:
    std::shared_ptr<PopulateCtx> par_ctx;
    std::shared_ptr<SymbolTable> ext_inp_sym_tbl;
    std::shared_ptr<SymbolTable> ext_out_sym_tbl;
    std::shared_ptr<SymbolTable> local_sym_tbl;
    size_t arith_depth;
    // If the test will actually execute the code
    bool taken;

    // This flag indicates if we are inside a region that we try to mutate
    bool inside_mutation;

    // As of now, the pragma omp simd is attached to a loop and can't be nested.
    // TODO: we need to think about pragma omp ordered simd
    bool inside_omp_simd;

    // Each loop header has a limit that any iterator should respect
    // For the simplicity, we assume that the limit is the same for all
    // iterators in the same loop nest. Different loop nests can have different
    // limits
    // TODO: We need to eliminate this constraint in the future
    // For now, we use vector with the same number in all elements to keep track
    // of the loop depth. This way, it will be easier to change the code in the
    // future
    std::vector<size_t> dims;

    // This flag indicates if we are inside arithmetic tree generation for
    // stencil pattern
    bool in_stencil;

    // This iterator is used to operate with multiple values
    std::shared_ptr<Iterator> mul_vals_iter;
    // If we want to allow multiple values in this context
    bool allow_mul_vals;
};

// TODO: maybe we need to inherit from some class
class EmitCtx {
  public:
    // This is a hack. EmitPolicy is required to get a name of a variable.
    // Because we use a name of a variable as a unique ID, we end up creating
    // a lot of objects that are useless.
    // TODO: replace UID type of vars to something better
    static std::shared_ptr<EmitCtx> default_emit_ctx;

    EmitCtx() : ispc_types(false), sycl_access(false) {
        emit_policy = std::make_shared<EmitPolicy>();
    }
    std::shared_ptr<EmitPolicy> getEmitPolicy() { return emit_policy; }

    void setIspcTypes(bool _val) { ispc_types = _val; }
    bool useIspcTypes() { return ispc_types; }

    void setSYCLAccess(bool _val) { sycl_access = _val; }
    bool useSYCLAccess() { return sycl_access; }

    void setSYCLPrefix(std::string _val) { sycl_prefix = std::move(_val); }
    std::string getSYCLPrefix() { return sycl_prefix; }

  private:
    std::shared_ptr<EmitPolicy> emit_policy;
    bool ispc_types;
    bool sycl_access;
    std::string sycl_prefix;
};
} // namespace yarpgen
