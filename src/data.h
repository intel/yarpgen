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

#include "enums.h"
#include "ir_value.h"
#include "options.h"
#include "type.h"
#include <array>
#include <deque>
#include <string>
#include <utility>

namespace yarpgen {

class GenCtx;
class PopulateCtx;
class EmitCtx;

class Data {
  public:
    Data(std::string _name, std::shared_ptr<Type> _type)
        : name(std::move(_name)), type(std::move(_type)),
          ub_code(UBKind::Uninit), is_dead(true), alignment(0) {}
    virtual ~Data() = default;

    virtual std::string getName(std::shared_ptr<EmitCtx> ctx) { return name; }
    void setName(std::string _name) { name = std::move(_name); }
    std::shared_ptr<Type> getType() { return type; }

    UBKind getUBCode() { return ub_code; }
    void setUBCode(UBKind _ub) { ub_code = _ub; }
    bool hasUB() { return ub_code != UBKind::NoUB; }

    virtual bool isScalarVar() { return false; }
    virtual bool isArray() { return false; }
    virtual bool isIterator() { return false; }
    virtual bool isTypedData() { return false; }

    virtual DataKind getKind() { return DataKind::MAX_DATA_KIND; }

    virtual void dbgDump() = 0;

    virtual std::shared_ptr<Data> makeVarying() = 0;

    void setIsDead(bool val) { is_dead = val; }
    bool getIsDead() { return is_dead; }

    void setAlignment(size_t _alignment) { alignment = _alignment; }
    size_t getAlignment() { return alignment; }

  protected:
    template <typename T> static std::shared_ptr<Data> makeVaryingImpl(T val) {
        auto ret = std::make_shared<T>(val);
        ret->type = ret->getType()->makeVarying();
        return ret;
    }

    std::string name;
    std::shared_ptr<Type> type;
    // It is not enough to have UB code just inside the IRValue.
    // E.g. if we go out of the array bounds of a multidimensional array,
    // we can't return IRValue and we need to indicate an error.
    UBKind ub_code;

    // Sometimes we create more variables than we use.
    // They create a lot of dead code in the test, so we need to prune them.
    bool is_dead;
    size_t alignment;
};

// Shorthand to make it simpler
using DataType = std::shared_ptr<Data>;

// This class serves as a placeholder for one of the real data classes.
// We propagate the type information before we propagate values.
// One option is to separate type from the value and store both. This idea has
// several downsides:
// 1. The value already stores the type
// 2. Type and data will be disjoint from each other, which can lead to
//    divergence and conflicts between them.
// Current solution is to create a placeholder class that stores just the type.
// It should be replaced with real data class after we start to propagate values
class TypedData : public Data {
  public:
    explicit TypedData(std::shared_ptr<Type> _type)
        : Data("", std::move(_type)) {}
    bool isTypedData() final { return true; }
    DataType replaceWith(DataType _new_data);
    void dbgDump() final;
    std::shared_ptr<Data> makeVarying() override {
        return makeVaryingImpl(*this);
    };
};

class ScalarVar : public Data {
  public:
    ScalarVar(std::string _name, const std::shared_ptr<IntegralType> &_type,
              IRValue _init_value)
        : Data(std::move(_name), _type), init_val(_init_value),
          cur_val(_init_value) {
        ub_code = init_val.getUBCode();
    }
    bool isScalarVar() final { return true; }
    DataKind getKind() final { return DataKind::VAR; }

    std::string getName(std::shared_ptr<EmitCtx> ctx) override;

    IRValue getInitValue() { return init_val; }
    IRValue getCurrentValue() { return cur_val; }
    void setCurrentValue(IRValue _val) {
        cur_val = _val;
        ub_code = cur_val.getUBCode();
    }

    void dbgDump() final;

    static std::shared_ptr<ScalarVar> create(std::shared_ptr<PopulateCtx> ctx);

    std::shared_ptr<Data> makeVarying() override {
        return makeVaryingImpl(*this);
    };

  private:
    IRValue init_val;
    IRValue cur_val;
};

class Array : public Data {
  public:
    Array(std::string _name, const std::shared_ptr<ArrayType> &_type,
          IRValue _val);
    IRValue getInitValues(bool use_main_vals) {
        return mul_vals_axis_idx == -1 || use_main_vals
                   ? init_vals[Options::main_val_idx]
                   : init_vals[Options::alt_val_idx];
    }
    IRValue getCurrentValues(bool use_main_vals) {
        return mul_vals_axis_idx == -1 || use_main_vals
                   ? cur_vals[Options::main_val_idx]
                   : cur_vals[Options::alt_val_idx];
    }
    void setInitValue(IRValue _val, bool use_main_vals,
                      int64_t mul_val_axis_idx);
    void setCurrentValue(IRValue _val, bool use_main_vals);
    int64_t getMulValsAxisIdx() { return mul_vals_axis_idx; }

    bool isArray() final { return true; }
    DataKind getKind() final { return DataKind::ARR; }

    void dbgDump() final;
    static std::shared_ptr<Array> create(std::shared_ptr<PopulateCtx> ctx,
                                         bool inp);

    std::shared_ptr<Data> makeVarying() override {
        return makeVaryingImpl(*this);
    };

  private:
    // TODO:
    // We want elements of the array to have different values.
    // It is the only way to properly test masked instructions and optimizations
    // designed to work with them.
    // Each vector represents a "cluster" with different values that is "copied"
    // over the whole array. The size of the cluster should be smaller than the
    // typical target architecture vector size. This way we can cover
    // all of the interesting cases while preserving the simplicity of
    // the analysis.

    // Span of initialization value can always be determined from array
    // dimensions and current value span
    std::array<IRValue, Options::vals_number> init_vals;
    std::array<IRValue, Options::vals_number> cur_vals;
    // We use int64_t to use negative values as poison values that
    // indicate that the values are uniform
    int64_t mul_vals_axis_idx;
};

class Expr;

class Iterator : public Data {
  public:
    Iterator(std::string _name, std::shared_ptr<Type> _type,
             std::shared_ptr<Expr> _start, size_t _max_left_offset,
             std::shared_ptr<Expr> _end, size_t _max_right_offset,
             std::shared_ptr<Expr> _step, bool _degenerate,
             size_t _total_iters_num)
        : Data(std::move(_name), std::move(_type)), start(std::move(_start)),
          max_left_offset(_max_left_offset), end(std::move(_end)),
          max_right_offset(_max_right_offset), step(std::move(_step)),
          degenerate(_degenerate), total_iters_num(_total_iters_num),
          supports_mul_values(false), main_vals_on_last_iter(true) {}

    bool isIterator() final { return true; }
    DataKind getKind() final { return DataKind::ITER; }

    std::shared_ptr<Expr> getStart() { return start; }
    size_t getMaxLeftOffset() { return max_left_offset; }
    size_t getMaxRightOffset() { return max_right_offset; }
    std::shared_ptr<Expr> getEnd() { return end; }
    std::shared_ptr<Expr> getStep() { return step; }
    void setParameters(std::shared_ptr<Expr> _start, std::shared_ptr<Expr> _end,
                       std::shared_ptr<Expr> _step);
    bool isDegenerate() { return degenerate; }
    size_t getTotalItersNum() { return total_iters_num; }

    void setSupportsMulValues(bool _supports_mul_values) {
        supports_mul_values = _supports_mul_values;
    }
    bool getSupportsMulValues() { return supports_mul_values; }
    void setMainValsOnLastIter(bool _main_vals_on_last_iter) {
        main_vals_on_last_iter = _main_vals_on_last_iter;
    }
    bool getMainValsOnLastIter() { return main_vals_on_last_iter; }

    void dbgDump() final;

    static std::shared_ptr<Iterator> create(std::shared_ptr<PopulateCtx> ctx,
                                            size_t _end_val,
                                            bool is_uniform = true);
    void populate(std::shared_ptr<PopulateCtx> ctx);

    std::shared_ptr<Data> makeVarying() override {
        return makeVaryingImpl(*this);
    };

  private:
    // TODO: should the expression contain full update on the iterator or only
    // the "meaningful" part?
    // For now we assume the latter, but it limits expressiveness
    std::shared_ptr<Expr> start;
    size_t max_left_offset;
    std::shared_ptr<Expr> end;
    size_t max_right_offset;
    std::shared_ptr<Expr> step;
    bool degenerate;
    // Total number of iterations
    size_t total_iters_num;
    // A flag that indicates whether the iterator supports multiple values
    bool supports_mul_values;
    // A flag that indicates whether the iterator has main values on the last
    // iteration
    bool main_vals_on_last_iter;
};

} // namespace yarpgen
