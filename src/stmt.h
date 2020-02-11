/*
Copyright (c) 2015-2019, Intel Corporation

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
#include "expr.h"
#include "ir_node.h"

#include <iostream>
#include <memory>
#include <utility>

namespace yarpgen {

class Stmt : public IRNode {
  public:
    virtual IRNodeKind getKind() { return IRNodeKind::MAX_STMT_KIND; }
};

class ExprStmt : public Stmt {
  public:
    explicit ExprStmt(std::shared_ptr<Expr> _expr) : expr(std::move(_expr)) {}
    IRNodeKind getKind() final { return IRNodeKind::EXPR; }

    std::shared_ptr<Expr> getExpr() { return expr; }

    void emit(std::ostream &stream, std::string offset = "") final;

  private:
    std::shared_ptr<Expr> expr;
};

class DeclStmt : public Stmt {
  public:
    explicit DeclStmt(std::shared_ptr<Data> _data) : data(std::move(_data)) {}
    DeclStmt(std::shared_ptr<Data> _data, std::shared_ptr<Expr> _expr)
        : data(std::move(_data)), init_expr(std::move(_expr)) {}
    IRNodeKind getKind() final { return IRNodeKind::DECL; }
    void emit(std::ostream &stream, std::string offset = "") final;

  private:
    std::shared_ptr<Data> data;
    std::shared_ptr<Expr> init_expr;
};

class StmtBlock : public Stmt {
  public:
    StmtBlock() = default;
    explicit StmtBlock(std::vector<std::shared_ptr<Stmt>> _stmts)
        : stmts(std::move(_stmts)) {}
    IRNodeKind getKind() final { return IRNodeKind::DECL; }

    void addStmt(std::shared_ptr<Stmt> stmt) {
        stmts.push_back(std::move(stmt));
    }

    void emit(std::ostream &stream, std::string offset = "") override;

  private:
    std::vector<std::shared_ptr<Stmt>> stmts;
};

class ScopeStmt : public StmtBlock {
  public:
    void emit(std::ostream &stream, std::string offset = "") final;
};

class LoopStmt : public Stmt {};

class LoopSeqStmt : public LoopStmt {
  public:
    // TODO: move to private?
    // TODO: LoopNestStmt have almost the same header. Should we merge them?
    class LoopHead {
      public:
        void addPrefix(std::shared_ptr<StmtBlock> _prefix) {
            prefix = std::move(_prefix);
        }
        void addIterator(std::shared_ptr<Iterator> _iter) {
            iters.push_back(std::move(_iter));
        }
        void addBody(std::shared_ptr<ScopeStmt> _body) {
            body = std::move(_body);
        }
        void addSuffix(std::shared_ptr<StmtBlock> _suffix) {
            suffix = std::move(_suffix);
        }

        void emit(std::ostream &stream, std::string offset = "");

      private:
        std::shared_ptr<StmtBlock> prefix;
        // Loop iterations space is defined by the iterators that we can use
        std::vector<std::shared_ptr<Iterator>> iters;
        std::shared_ptr<ScopeStmt> body;
        std::shared_ptr<StmtBlock> suffix;
    };

    void addLoop(std::shared_ptr<LoopHead> _loop) {
        loops.push_back(std::move(_loop));
    }
    void emit(std::ostream &stream, std::string offset = "") final;

  private:
    std::vector<std::shared_ptr<LoopHead>> loops;
};

class LoopNestStmt : public LoopStmt {};
} // namespace yarpgen
