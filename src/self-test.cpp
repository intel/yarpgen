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

#include <iostream>

#include "expr.h"
#include "ir_node.h"
#include "gen_policy.h"
#include "stmt.h"
#include "sym_table.h"
#include "type.h"
#include "variable.h"

using namespace yarpgen;

void self_test () {
    std::cout << "SELFTEST" << std::endl;
/*
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
    Context ctx_var (gen_policy, nullptr, Node::NodeID::MAX_STMT_ID, true);
    ctx_var.set_local_sym_table(std::make_shared<SymbolTable>());
    std::shared_ptr<Context> ctx = std::make_shared<Context>(ctx_var);

    std::shared_ptr<IntegerType> rand_int_type = IntegerType::generate(ctx);
    rand_int_type->dbg_dump();
    std::cout << "\n====================="<< std::endl;


    std::shared_ptr<BitField> rand_bit_field = BitField::generate(ctx);
    rand_bit_field->dbg_dump();
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


   std::vector<std::shared_ptr<Expr>> inp2;
    for (int i = 0; i < 10; ++i) {
        std::shared_ptr<ScalarVariable> rand_scalar_var = ScalarVariable::generate(ctx);
        std::shared_ptr<VarUseExpr> var_use = std::make_shared<VarUseExpr>(rand_scalar_var);
        inp2.push_back(var_use);
    }


    std::shared_ptr<Expr> unary_rand2 = UnaryExpr::generate(ctx, inp, 0);
    std::cout << "unary_rand: " << unary_rand2->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;


   std::shared_ptr<Expr> arith_rand = ArithExpr::generate(ctx, inp);
    std::cout << "arith_rand: " << arith_rand->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;


    for (int i = UnaryExpr::Op::PreInc; i < UnaryExpr::Op::MaxOp; ++i) {
        for (int j = Type::IntegerTypeID::BOOL; j < Type::IntegerTypeID::MAX_INT_ID; ++j) {
            if (j == Type::IntegerTypeID::BOOL &&
               (i == UnaryExpr::Op::PreInc ||i == UnaryExpr::Op::PostInc ||
                i == UnaryExpr::Op::PreDec ||i == UnaryExpr::Op::PostDec))
                continue;
            std::shared_ptr<IntegerType> type = IntegerType::init((Type::IntegerTypeID) j);
            std::shared_ptr<ScalarVariable> var = std::make_shared<ScalarVariable>("a", type);
            std::shared_ptr<VarUseExpr> var_use = std::make_shared<VarUseExpr>(var);
            std::shared_ptr<UnaryExpr> unary_expr = std::make_shared<UnaryExpr>((UnaryExpr::Op) i, var_use);
            std::cout << "unary_expr: " << unary_expr->emit() << std::endl;
            std::cout << "expr_type: " << std::static_pointer_cast<ScalarVariable>(unary_expr->get_value())->get_type()->get_int_type_id() << std::endl;
            std::cout << "arg_type: " << std::static_pointer_cast<ScalarVariable>(unary_expr->get_value())->get_cur_value().get_int_type_id() << std::endl;
            std::cout << "arg_val: " << std::static_pointer_cast<ScalarVariable>(unary_expr->get_value())->get_cur_value() << std::endl;
            if (std::static_pointer_cast<ScalarVariable>(unary_expr->get_value())->get_type()->get_int_type_id() !=
                std::static_pointer_cast<ScalarVariable>(unary_expr->get_value())->get_cur_value().get_int_type_id()) {
                std::cerr << "expr_type != arg_type" << std::endl;
                exit(-1);
            }
            std::cout << "\n====================="<< std::endl;
        }
    }


    for (int i = BinaryExpr::Op::Add; i < BinaryExpr::Op::MaxOp; ++i) {
        for (int j = Type::IntegerTypeID::BOOL; j < Type::IntegerTypeID::MAX_INT_ID; ++j) {
            for (int k = Type::IntegerTypeID::BOOL; k < Type::IntegerTypeID::MAX_INT_ID; ++k) {
                std::shared_ptr<IntegerType> lhs_type = IntegerType::init((Type::IntegerTypeID) j);
                std::shared_ptr<ScalarVariable> lhs_var = std::make_shared<ScalarVariable>("a", lhs_type);
                std::shared_ptr<VarUseExpr> lhs_use = std::make_shared<VarUseExpr>(lhs_var);


                std::shared_ptr<IntegerType> rhs_type = IntegerType::init((Type::IntegerTypeID) k);
                std::shared_ptr<ScalarVariable> rhs_var = std::make_shared<ScalarVariable>("b", rhs_type);
                std::shared_ptr<VarUseExpr> rhs_use = std::make_shared<VarUseExpr>(rhs_var);

                std::shared_ptr<BinaryExpr> binary_expr = std::make_shared<BinaryExpr>((BinaryExpr::Op) i, lhs_use, rhs_use);
                std::cout << "binary_expr: " << binary_expr->emit() << std::endl;
                std::cout << "expr_type: " << std::static_pointer_cast<ScalarVariable>(binary_expr->get_value())->get_type()->get_int_type_id() << std::endl;
                std::cout << "arg_type: " << std::static_pointer_cast<ScalarVariable>(binary_expr->get_value())->get_cur_value().get_int_type_id() << std::endl;
                std::cout << "arg_val: " << std::static_pointer_cast<ScalarVariable>(binary_expr->get_value())->get_cur_value() << std::endl;
                if (std::static_pointer_cast<ScalarVariable>(binary_expr->get_value())->get_type()->get_int_type_id() !=
                    std::static_pointer_cast<ScalarVariable>(binary_expr->get_value())->get_cur_value().get_int_type_id()) {
                    std::cerr << "expr_type != arg_type" << std::endl;
                    exit(-1);
                }
                std::cout << "\n====================="<< std::endl;

            }
        }
    }

    std::shared_ptr<ConstExpr> const_expr = ConstExpr::generate(ctx);
    std::cout << "const_expr: " << const_expr->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<TypeCastExpr> type_cast_expr = TypeCastExpr::generate(ctx, const_expr);
    std::cout << "type_cast_expr: " << type_cast_expr->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<DeclStmt> decl_stmt = DeclStmt::generate(std::make_shared<Context>(gen_policy, ctx, Node::NodeID::DECL, true), inp);
    std::cout << "decl_stmt: " << decl_stmt->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;

    std::shared_ptr<ExprStmt> expr_stmt = ExprStmt::generate(ctx, inp, inp.at(0));
    std::cout << "expr_stmt: " << expr_stmt->emit() << std::endl;
    std::cout << "\n====================="<< std::endl;
*/

    std::shared_ptr<StructType> struct_type0 = std::make_shared<StructType> ("struct A");
    std::shared_ptr<IntegerType> int_type = IntegerType::init(Type::IntegerTypeID::INT);
    struct_type0->add_member(int_type, "memb0");
    struct_type0->set_is_static(true);
    struct_type0->dbg_dump();

    std::shared_ptr<StructType> struct_type1 = std::make_shared<StructType> ("struct B");
    struct_type1->add_member(struct_type0, "memb1");
    struct_type1->dbg_dump();

    std::shared_ptr<Struct> struct0 = std::make_shared<Struct>("struct0", struct_type1);
    std::shared_ptr<Struct> struct1 = std::make_shared<Struct>("struct1", struct_type1);
    std::cout << std::static_pointer_cast<ScalarVariable>(std::static_pointer_cast<Struct>(struct0->get_member(0))->get_member(0))->get_cur_value() << std::endl;
    std::cout << std::static_pointer_cast<ScalarVariable>(std::static_pointer_cast<Struct>(struct1->get_member(0))->get_member(0))->get_cur_value() << std::endl;

    AtomicType::ScalarTypedVal char_const_val (Type::IntegerTypeID::INT);
    char_const_val.val.int_val = 100;
    std::shared_ptr<ConstExpr> char_const = std::make_shared<ConstExpr>(char_const_val);
    std::shared_ptr<MemberExpr> mem_expr0 = std::make_shared<MemberExpr>(struct0, 0);
    std::shared_ptr<MemberExpr> mem_expr1 = std::make_shared<MemberExpr>(mem_expr0, 0);
    std::shared_ptr<AssignExpr> struct0_assign = std::make_shared<AssignExpr>(mem_expr1, char_const);

    std::cout << struct0_assign->emit() << std::endl;

    std::cout << std::static_pointer_cast<ScalarVariable>(std::static_pointer_cast<Struct>(struct0->get_member(0))->get_member(0))->get_cur_value() << std::endl;
    std::cout << std::static_pointer_cast<ScalarVariable>(std::static_pointer_cast<Struct>(struct1->get_member(0))->get_member(0))->get_cur_value() << std::endl;
}
