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

namespace yarpgen {

class Data {
  public:
    Data(std::string _name, std::shared_ptr<Type> _type)
        : name(_name), type(_type) {}
    virtual ~Data() = default;

    std::string getName() { return name; }
    std::shared_ptr<Type> getType() { return type; }

    virtual bool isScalarVar() { return false; }
    virtual bool isArray() { return false; }

    virtual void dbgDump() = 0;

  protected:
    std::string name;
    std::shared_ptr<Type> type;
};

class ScalarVar : public Data {
  public:
    ScalarVar(std::string _name, const std::shared_ptr<IntegralType> &_type,
              IRValue _init_value)
        : Data(std::move(_name), _type), init_val(_init_value),
          cur_val(_init_value), changed(false) {}
    bool isScalarVar() final { return true; }
    IRValue getInitValue() { return init_val; }
    IRValue getCurrentValue() { return cur_val; }
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
          std::vector<std::shared_ptr<Data>> _vals)
        : Data(std::move(_name), _type), vals(std::move(_vals)) {}
    std::vector<std::shared_ptr<Data>> &getValues() { return vals; }

    void dbgDump() final;

  private:
    // We allow elements of the array to have different values.
    // It is the only way to properly test masked instructions and optimizations
    // designed to work with them.
    // Each vector represents a "cluster" with different values that is "copied"
    // over the whole array. The size of the cluster should be smaller than the
    // typical size of the vector in target architecture. This way we can cover
    // all of the interesting cases while preserving the simplicity of
    // the analysis.
    std::vector<std::shared_ptr<Data>> vals;
};

} // namespace yarpgen
