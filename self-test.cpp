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

#include <iostream>

#include "type.h"
#include "variable.h"
#include "ir_node.h"
#include "expr.h"

using namespace rl;

void self_test () {
    std::cout << "SELFTEST" << std::endl;

    std::shared_ptr<IntegerType> bool_type = IntegerType::init(AtomicType::IntegerTypeID::BOOL);
    bool_type->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<IntegerType> int_type = IntegerType::init(AtomicType::IntegerTypeID::INT, AtomicType::Mod::CONST, true, 32);
    int_type->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<StructType> struct_type = std::make_shared<StructType>("AA");
    struct_type->add_member(bool_type, "memb_1");
    struct_type->add_member(int_type, "memb_2");
    struct_type->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<ScalarVariable> bool_val = std::make_shared<ScalarVariable>("b", bool_type);
    bool_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<ScalarVariable> int_val = std::make_shared<ScalarVariable>("i", int_type);
    int_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<Struct> struct_val = std::make_shared<Struct>("srtuct_0", struct_type);
    struct_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<VarUseExpr> bool_use = std::make_shared<VarUseExpr>(bool_val);
    std::cout << "bool_use " << bool_use->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<VarUseExpr> int_use = std::make_shared<VarUseExpr>(int_val);
    std::cout << "int_use " << int_use->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<VarUseExpr> struct_use = std::make_shared<VarUseExpr>(struct_val);
    std::cout << "struct_use " << struct_use->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<AssignExpr> int_bool = std::make_shared<AssignExpr>(int_use, bool_use);
    std::cout << "int_bool " << int_bool->emit() << std::endl;
    int_bool->get_value()->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    int_val->dbg_dump();
    bool_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;
}
