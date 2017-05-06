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

#include <vector>

#include "type.h"
#include "variable.h"
#include "ir_node.h"
#include "expr.h"

namespace yarpgen {

class Context;

// Abstract class, serves as common ancestor for all statements.
class Stmt : public Node {
    public:
        Stmt (Node::NodeID _id) : Node(_id) {};

    protected:
        static int total_stmt_count;
};

// Declaration statement creates new variable (declares variable in current context) and adds it to local symbol table:
// E.g.: variable_declaration = init_statement;
// Also it provides emission of extern declarations (this ability used only in print-out process):
// E.g.: extern variable_declaration;
class DeclStmt : public Stmt {
    public:
        DeclStmt (std::shared_ptr<Data> _data, std::shared_ptr<Expr> _init, bool _is_extern = false);
        void set_is_extern (bool _is_extern) { is_extern = _is_extern; }
        std::shared_ptr<Data> get_data () { return data; }
        std::string emit (std::string offset = "");
        static std::shared_ptr<DeclStmt> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp);

    private:
        std::shared_ptr<Data> data;
        std::shared_ptr<Expr> init;
        bool is_extern;
};

// Expression statement 'converts' any expression to statement.
// For example, it allows to use AssignExpr as statement:
// var_16 = 123ULL * 10;
class ExprStmt : public Stmt {
    public:
        ExprStmt (std::shared_ptr<Expr> _expr) : Stmt(Node::NodeID::EXPR), expr(_expr) {}
        std::string emit (std::string offset = "") { return offset + expr->emit() + ";"; }
        static std::shared_ptr<ExprStmt> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, std::shared_ptr<Expr> out);

    private:
        std::shared_ptr<Expr> expr;
};

// Scope statement represents scope and its content:
// E.g.:
// {
//     ...
// }
//TODO: it also fills global SymTable at startup. Master class should do it.
class ScopeStmt : public Stmt {
    public:
        ScopeStmt () : Stmt(Node::NodeID::SCOPE) {}
        void add_stmt (std::shared_ptr<Stmt> stmt) { scope.push_back(stmt); }
        std::string emit (std::string offset = "");
        static std::shared_ptr<ScopeStmt> generate (std::shared_ptr<Context> ctx);

    private:
        static std::vector<std::shared_ptr<Expr>> extract_inp_from_ctx(std::shared_ptr<Context> ctx);
        static std::vector<std::shared_ptr<Expr>> extract_inp_and_mix_from_ctx(std::shared_ptr<Context> ctx);
        static void form_extern_sym_table(std::shared_ptr<Context> ctx);
        std::vector<std::shared_ptr<Stmt>> scope;
};

// If statement - represents if...else statement. Else branch is optional.
// E.g.:
// if (cond) {
// ...
// }
// <else {
// ...
// }>
class IfStmt : public Stmt {
    public:
        IfStmt (std::shared_ptr<Expr> cond, std::shared_ptr<ScopeStmt> if_branch, std::shared_ptr<ScopeStmt> else_branch);
        static bool count_if_taken (std::shared_ptr<Expr> cond);
        std::string emit (std::string offset = "");
        static std::shared_ptr<IfStmt> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp);

    private:
        // TODO: do we need it? It should indicate whether the scope is evaluated.
        bool taken;
        std::shared_ptr<Expr> cond;
        std::shared_ptr<ScopeStmt> if_branch;
        std::shared_ptr<ScopeStmt> else_branch;
};
}
