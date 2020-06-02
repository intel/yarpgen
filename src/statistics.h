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
#pragma once

#include "enums.h"
#include <array>
#include <cstdlib>

namespace yarpgen {
class Statistics {
  public:
    static Statistics &getInstance() {
        static Statistics instance;
        return instance;
    }
    Statistics(const Statistics &options) = delete;
    Statistics &operator=(const Statistics &) = delete;

    void addStmt(size_t val = 1) { stmt_num += val; }
    size_t getStmtNum() { return stmt_num; }

    void addUB(UBKind kind) { ub_num.at(static_cast<size_t>(kind))++; }

  private:
    Statistics() : stmt_num(0), ub_num({}) {}

    size_t stmt_num;
    // TODO: count undefined behavior stats
    std::array<size_t, static_cast<size_t>(UBKind::MaxUB)> ub_num;
};

} // namespace yarpgen
