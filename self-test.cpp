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
#include "stmt.h"
#include "gen_policy.h"
#include "sym_table.h"

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

    std::shared_ptr<DeclStmt> int_decl = std::make_shared<DeclStmt>(int_val, char_const, false);
    std::cout << "int_decl: " << int_decl->emit() << std::endl;
    int_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<ExprStmt> assign_expr_stmt= std::make_shared<ExprStmt>(bit_not_assign);
    std::cout << "assign_expr_stmt: " << assign_expr_stmt->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<ScopeStmt> scope = std::make_shared<ScopeStmt>();
    scope->add_stmt(assign_expr_stmt);
    std::cout << "scope: " << scope->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<Type> uint_t = IntegerType::init(Type::IntegerTypeID::UINT);
    std::shared_ptr<StructType> sub_struct = std::make_shared<StructType>("sub_struct");
    sub_struct->add_member(uint_t, "uint_mem");
    std::shared_ptr<StructType> par_struct = std::make_shared<StructType>("par_struct");
    par_struct->add_member(sub_struct, "sub_struct_mem");
    std::shared_ptr<Struct> par_struct_val = std::make_shared<Struct>("AAA", par_struct);
    par_struct_val->dbg_dump();
    std::shared_ptr<MemberExpr> par_struct_mem = std::make_shared<MemberExpr>(par_struct_val, 0);
    std::cout << "par_struct_mem: " << par_struct_mem->emit() << std::endl;
    std::shared_ptr<MemberExpr> sub_struct_mem = std::make_shared<MemberExpr>(par_struct_mem, 0);
    std::cout << "sub_struct_mem: " << sub_struct_mem->emit() << std::endl;
    std::shared_ptr<AssignExpr> sub_struct_mem_assign = std::make_shared<AssignExpr>(sub_struct_mem, unary_bit_not);
    std::cout << "sub_struct_mem_assign: " << sub_struct_mem_assign->emit() << std::endl;
    par_struct_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<IfStmt> if_stmt = std::make_shared<IfStmt>(unary_bit_not, scope, scope);
    std::cout << "if_stmt: " << if_stmt->emit() << std::endl;
    std::cout << "taken: " << IfStmt::count_if_taken(unary_bit_not) << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<IntegerType> llint_type = IntegerType::init(AtomicType::IntegerTypeID::LLINT);

    std::shared_ptr<ScalarVariable> if_val = std::make_shared<ScalarVariable>("if_val", llint_type);
    std::shared_ptr<VarUseExpr> if_val_use = std::make_shared<VarUseExpr>(if_val);
    std::shared_ptr<AssignExpr> if_assign = std::make_shared<AssignExpr>(if_val_use, char_const);
    std::shared_ptr<ExprStmt> if_assign_stmt = std::make_shared<ExprStmt>(if_assign);
    std::shared_ptr<ScopeStmt> if_scope = std::make_shared<ScopeStmt>();
    if_scope->add_stmt(if_assign_stmt);

    std::shared_ptr<ScalarVariable> else_val = std::make_shared<ScalarVariable>("else_val", llint_type);
    std::shared_ptr<VarUseExpr> else_val_use = std::make_shared<VarUseExpr>(else_val);
    std::shared_ptr<AssignExpr> else_assign = std::make_shared<AssignExpr>(else_val_use, unary_bit_not, false);
    std::shared_ptr<ExprStmt> else_assign_stmt = std::make_shared<ExprStmt>(else_assign);
    std::shared_ptr<ScopeStmt> else_scope = std::make_shared<ScopeStmt>();
    else_scope->add_stmt(else_assign_stmt);

    std::shared_ptr<IfStmt> assign_if_stmt = std::make_shared<IfStmt>(unary_bit_not, if_scope, else_scope);
    std::cout << "assign_if_stmt: " << assign_if_stmt->emit() << std::endl;
    if_val->dbg_dump();
    else_val->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    GenPolicy gen_policy;
    Context ctx (gen_policy, NULL);

    std::shared_ptr<IntegerType> rand_int_type = IntegerType::generate(ctx);
    rand_int_type->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<StructType> rand_struct_type = StructType::generate(ctx);
    rand_struct_type->dbg_dump();
    std::vector<std::shared_ptr<StructType>> substr_types;
    substr_types.push_back(rand_struct_type);
    std::shared_ptr<StructType> rand_par_struct_type = StructType::generate(ctx, substr_types);
    rand_par_struct_type->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    AtomicType::ScalarTypedVal rand_val = AtomicType::ScalarTypedVal::generate(ctx, Type::IntegerTypeID::SHRT);
    std::cout << "rand_val: " << rand_val << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<ScalarVariable> rand_scalar_var = ScalarVariable::generate(ctx);
    rand_scalar_var->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<Struct> rand_struct = Struct::generate(ctx);
    rand_struct->dbg_dump();
    std::cout << "\n====================="<< std::endl;

    std::vector<std::shared_ptr<Expr>> inp;
    inp.push_back(bool_use);
    inp.push_back(int_use);
    inp.push_back(lint_use);
    inp.push_back(if_val_use);
    inp.push_back(else_val_use);
    std::shared_ptr<Expr> unary_rand = UnaryExpr::generate(ctx, inp, 0);
    std::cout << "unary_rand: " << unary_rand->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

}
