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

    std::shared_ptr<IntegerType> lint_type = IntegerType::init(AtomicType::IntegerTypeID::LINT, AtomicType::Mod::VOLAT, false, 0);
    lint_type->dbg_dump();

    std::shared_ptr<ScalarVariable> lint_val = std::make_shared<ScalarVariable>("li", lint_type);
    lint_val->dbg_dump();

    std::shared_ptr<VarUseExpr> lint_use = std::make_shared<VarUseExpr>(lint_val);
    std::cout << "lint_use " << lint_use->emit() << std::endl;


    AtomicType::ScalarTypedVal char_const_val (Type::IntegerTypeID::CHAR);
    std::shared_ptr<ConstExpr> char_const = std::make_shared<ConstExpr>(char_const_val);
    std::shared_ptr<AssignExpr> lint_char = std::make_shared<AssignExpr>(lint_use, char_const);
    std::cout << "int_bool " << lint_char->emit() << std::endl;
    lint_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<UnaryExpr> unary_bit_not = std::make_shared<UnaryExpr>(UnaryExpr::BitNot, char_const);
    std::shared_ptr<AssignExpr> bit_not_assign = std::make_shared<AssignExpr>(lint_use, unary_bit_not);
    std::cout << "bit_not_assign " << bit_not_assign->emit() << std::endl;
    bit_not_assign->get_value()->dbg_dump();
    lint_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<BinaryExpr> binary_add = std::make_shared<BinaryExpr>(BinaryExpr::Op::Add, unary_bit_not, char_const);
    std::shared_ptr<AssignExpr> add_assign = std::make_shared<AssignExpr>(lint_use, binary_add);
    std::cout << "add_assign " << add_assign->emit() << std::endl;
    add_assign->get_value()->dbg_dump();
    lint_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;
}
