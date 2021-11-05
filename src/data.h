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
#include "type.h"
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
    // Value, end, step
    using array_val_t =
        std::tuple<IRValue, std::vector<size_t>, std::vector<size_t>>;

    Array(std::string _name, const std::shared_ptr<ArrayType> &_type,
          IRValue _val);
    IRValue getInitValues() { return init_vals; }
    array_val_t getCurrentValues() { return cur_vals; }
    void setValue(IRValue _val, std::deque<size_t> &span,
                  std::deque<size_t> &steps);

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
    IRValue init_vals;
    array_val_t cur_vals;
};

class Expr;

class Iterator : public Data {
  public:
    Iterator(std::string _name, std::shared_ptr<Type> _type,
             std::shared_ptr<Expr> _start, size_t _max_left_offset,
             std::shared_ptr<Expr> _end, size_t _max_right_offset,
             std::shared_ptr<Expr> _step, bool _degenerate)
        : Data(std::move(_name), std::move(_type)), start(std::move(_start)),
          max_left_offset(_max_left_offset), end(std::move(_end)),
          max_right_offset(_max_right_offset), step(std::move(_step)),
          degenerate(_degenerate) {}

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
    // For know we assume the latter, but it limits expressiveness
    std::shared_ptr<Expr> start;
    size_t max_left_offset;
    std::shared_ptr<Expr> end;
    size_t max_right_offset;
    std::shared_ptr<Expr> step;
    bool degenerate;
};

} // namespace yarpgen
