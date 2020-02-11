/*
Copyright (c) 2019, Intel Corporation

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
#include <string>
#include <utility>

namespace yarpgen {

class Data {
  public:
    Data(std::string _name, std::shared_ptr<Type> _type)
        : name(std::move(_name)), type(std::move(_type)),
          ub_code(UBKind::Uninit) {}
    virtual ~Data() = default;

    std::string getName() { return name; }
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

  protected:
    std::string name;
    std::shared_ptr<Type> type;
    // It is not enough to have UB code just inside the IRValue.
    // E.g. if we go out of the array bounds of a multidimensional array,
    // we can't return IRValue and we need to indicate an error.
    UBKind ub_code;
};

// Shorthand to make it simpler
using DataType = std::shared_ptr<Data>;

class ScalarVar : public Data {
  public:
    ScalarVar(std::string _name, const std::shared_ptr<IntegralType> &_type,
              IRValue _init_value)
        : Data(std::move(_name), _type), init_val(_init_value),
          cur_val(_init_value), changed(false) {
        ub_code = init_val.getUBCode();
    }
    bool isScalarVar() final { return true; }
    DataKind getKind() final { return DataKind::VAR; }

    IRValue getInitValue() { return init_val; }
    IRValue getCurrentValue() { return cur_val; }
    void setCurrentValue(IRValue _val) {
        cur_val = _val;
        ub_code = cur_val.getUBCode();
        changed = true;
    }
    bool wasChanged() { return changed; }

    void dbgDump() final;

  private:
    IRValue init_val;
    IRValue cur_val;
    bool changed;
};

class Array : public Data {
  public:
    Array(std::string _name, const std::shared_ptr<ArrayType> &_type,
          std::shared_ptr<Data> _val);
    std::shared_ptr<Data> getInitValues() { return init_vals; }
    std::shared_ptr<Data> getCurrentValues() { return cur_vals; }
    void setValue(std::shared_ptr<Data> _val);
    bool wasChanged() { return was_changed; }

    bool isArray() final { return true; }
    DataKind getKind() final { return DataKind::ARR; }

    void dbgDump() final;

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
    std::shared_ptr<Data> init_vals;
    std::shared_ptr<Data> cur_vals;
    bool was_changed;
};

class Expr;

class Iterator : public Data {
  public:
    Iterator(std::string _name, std::shared_ptr<Type> _type,
             std::shared_ptr<Expr> _start, std::shared_ptr<Expr> _end,
             std::shared_ptr<Expr> _step)
        : Data(std::move(_name), std::move(_type)), start(std::move(_start)),
          end(std::move(_end)), step(std::move(_step)) {}

    bool isIterator() final { return true; }
    DataKind getKind() final { return DataKind::ITER; }

    std::shared_ptr<Expr> getStart() { return start; }
    std::shared_ptr<Expr> getEnd() { return end; }
    std::shared_ptr<Expr> getStep() { return step; }
    void setParameters(std::shared_ptr<Expr> _start, std::shared_ptr<Expr> _end,
                       std::shared_ptr<Expr> _step);

  private:
    // TODO: should the expression contain full update on the iterator or only
    // the "meaningful" part?
    // For know we assume the latter, but it limits expressiveness
    std::shared_ptr<Expr> start;
    std::shared_ptr<Expr> end;
    std::shared_ptr<Expr> step;
};

} // namespace yarpgen
