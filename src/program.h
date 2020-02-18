/*
Copyright (c) 2015-2017, Intel Corporation
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
    static std::shared_ptr<ScopeStmt> generate();
    void emit(std::ostream& stream);

  private:
    void emitCheckFunc(std::ostream &stream);
    void emitDecl(std::ostream &stream);
    void emitCheck(std::ostream &stream);
    void emitTest(std::ostream &stream);
    void emitMain(std::ostream &stream);

    std::shared_ptr<SymbolTable> ext_inp_sym_tbl;
    std::shared_ptr<SymbolTable> ext_out_sym_tbl;
    std::shared_ptr<ScopeStmt> new_test;
};

}