/*
Copyright (c) 2015-2016, Intel Corporation

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

#include <fstream>

#include "gen_policy.h"
#include "sym_table.h"
#include "stmt.h"

///////////////////////////////////////////////////////////////////////////////

namespace rl {

class Master {
    public:
        Master (std::string _out_folder);
        void generate ();
        std::string emit_func ();
        std::string emit_init ();
        std::string emit_decl ();
        std::string emit_hash ();
        std::string emit_check ();
        std::string emit_main ();

    private:
        void write_file (std::string of_name, std::string data);

        GenPolicy gen_policy;
        std::shared_ptr<ScopeStmt> program;
        std::shared_ptr<SymbolTable> extern_inp_sym_table;
        std::shared_ptr<SymbolTable> extern_mix_sym_table;
        std::shared_ptr<SymbolTable> extern_out_sym_table;
        std::string out_folder;
};
}

