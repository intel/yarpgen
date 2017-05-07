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

#include <fstream>

#include "gen_policy.h"
#include "sym_table.h"
#include "stmt.h"

///////////////////////////////////////////////////////////////////////////////

namespace yarpgen {

// This class serves as a driver to generation process.
// First of all, you should initialize global variable rand_val_gen with seed (see RandValGen).
// After that you can call generate method (it will do all the work).
// To print-out result, just call consecutively all emit methods.
//
// Generation process starts with initialization of global Context and extern SymTable.
// After it, recursive Scope generation method starts
class Master {
    public:
        Master (std::string _out_folder);

        // It initializes global Context and launches generation process.
        void generate ();

        // Print-out methods
        // To get valid test, all of them should be called (the order doesn't matter)
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
        // There are three kind of global variables which exist in test.
        // All of them are declared as external variables for core test function
        // to prevent constant propagation optimization.
        // Also all they are initialized at startup of the test to prevent UB.
        // Main difference between them is what happens after:
        // 1) Input variables - they can't change their value (it is necessary for CSE)
        // 2) Mixed variables - they can change their value multiple times.
        //    They are used in checksum calculation.
        // 3) Output variables - they can change their value only once.
        //    They are also used in checksum calculation.
        std::shared_ptr<SymbolTable> extern_inp_sym_table;
        std::shared_ptr<SymbolTable> extern_mix_sym_table;
        std::shared_ptr<SymbolTable> extern_out_sym_table;
        std::string out_folder;
};
}

