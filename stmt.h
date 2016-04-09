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

#include <vector>

#include "type.h"
#include "variable.h"
#include "ir_node.h"
#include "expr.h"

namespace rl {

class Context;

class Stmt : public Node {
    public:
        Stmt (Node::NodeID _id) : Node(_id) {};
};

class DeclStmt : public Stmt {
    public:
        DeclStmt (std::shared_ptr<Data> _data, std::shared_ptr<Expr> _init, bool _is_extern = false);
        void set_is_extern (bool _is_extern) { is_extern = _is_extern; }
        std::string emit (std::string offset = "");
        static std::shared_ptr<DeclStmt> generate (Context ctx, std::vector<std::shared_ptr<Expr>> inp);

    private:
        std::shared_ptr<Data> data;
        std::shared_ptr<Expr> init;
        bool is_extern;
};

class ExprStmt : public Stmt {
    public:
        ExprStmt (std::shared_ptr<Expr> _expr) : Stmt(Node::NodeID::EXPR), expr(_expr) {}
        std::string emit (std::string offset = "") { return offset + expr->emit() + ";"; }

    private:
        std::shared_ptr<Expr> expr;
};

class ScopeStmt : public Stmt {
    public:
        ScopeStmt () : Stmt(Node::NodeID::SCOPE) {}
        void add_stmt (std::shared_ptr<Stmt> stmt) { scope.push_back(stmt); }
        std::string emit (std::string offset = "");

    private:
        std::vector<std::shared_ptr<Stmt>> scope;
};

class IfStmt : public Stmt {
    public:
        IfStmt (std::shared_ptr<Expr> cond, std::shared_ptr<ScopeStmt> if_branch, std::shared_ptr<ScopeStmt> else_branch);
        static bool count_if_taken (std::shared_ptr<Expr> cond);
        std::string emit (std::string offset = "");

    private:
        bool taken;
        std::shared_ptr<Expr> cond;
        std::shared_ptr<ScopeStmt> if_branch;
        std::shared_ptr<ScopeStmt> else_branch;
};
}
