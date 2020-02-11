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

#include "data.h"

#include <utility>

void yarpgen::ScalarVar::dbgDump() {
    std::cout << "Scalar var: " << name << std::endl;
    std::cout << "Type info:" << std::endl;
    type->dbgDump();
    std::cout << "Init val: " << init_val << std::endl;
    std::cout << "Current val: " << cur_val << std::endl;
    std::cout << "Was changed: " << changed << std::endl;
}

void yarpgen::Array::dbgDump() {
    std::cout << "Array: " << name << std::endl;
    std::cout << "Type info:" << std::endl;
    type->dbgDump();
    init_vals->dbgDump();
    cur_vals->dbgDump();
}

yarpgen::Array::Array(std::string _name,
                      const std::shared_ptr<ArrayType> &_type,
                      std::shared_ptr<Data> _val)
    : Data(std::move(_name), _type), init_vals(_val), cur_vals(_val),
      was_changed(false) {
    if (!type->isArrayType())
        ERROR("Array variable should have an ArrayType");

    auto array_type = std::static_pointer_cast<ArrayType>(type);
    if (array_type->getBaseType() != init_vals->getType())
        ERROR("Can't initialize array with variable of wrong type");

    ub_code = init_vals->getUBCode();
}
void yarpgen::Array::setValue(std::shared_ptr<Data> _val) {
    auto array_type = std::static_pointer_cast<ArrayType>(type);
    if (array_type->getBaseType() != _val->getType())
        ERROR("Can't initialize array with variable of wrong type");
    cur_vals = std::move(_val);
    ub_code = cur_vals->getUBCode();
    was_changed = true;
}

void yarpgen::Iterator::setParameters(std::shared_ptr<yarpgen::Expr> _start,
                                      std::shared_ptr<yarpgen::Expr> _end,
                                      std::shared_ptr<yarpgen::Expr> _step) {
    start = std::move(_start);
    end = std::move(_end);
    step = std::move(_step);
}
