/*
Copyright (c) 2015-2020, Intel Corporation
Copyright (c) 2019-2020, University of Utah

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

#include "context.h"
#include "enums.h"
#include "expr.h"
#include "ir_node.h"

#include <iostream>
#include <memory>
#include <numeric>
#include <utility>

namespace yarpgen {

class Stmt : public IRNode {
  public:
    virtual IRNodeKind getKind() { return IRNodeKind::MAX_STMT_KIND; }
    // We need to know if we have any nested foreach to make a correct decision
    // about the number of iterations. Those two decisions are made in
    // different places, so we need to have a way to communicate this
    virtual bool detectNestedForeach() { return false; }
};

class ExprStmt : public Stmt {
  public:
    explicit ExprStmt(std::shared_ptr<Expr> _expr) : expr(std::move(_expr)) {}
    IRNodeKind getKind() final { return IRNodeKind::EXPR; }

    std::shared_ptr<Expr> getExpr() { return expr; }

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<ExprStmt> create(std::shared_ptr<PopulateCtx> ctx);

  private:
    std::shared_ptr<Expr> expr;
};

class DeclStmt : public Stmt {
  public:
    explicit DeclStmt(std::shared_ptr<Data> _data) : data(std::move(_data)) {}
    DeclStmt(std::shared_ptr<Data> _data, std::shared_ptr<Expr> _expr)
        : data(std::move(_data)), init_expr(std::move(_expr)) {}
    IRNodeKind getKind() final { return IRNodeKind::DECL; }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;

  private:
    std::shared_ptr<Data> data;
    std::shared_ptr<Expr> init_expr;
};

class StmtBlock : public Stmt {
  public:
    StmtBlock() = default;
    explicit StmtBlock(std::vector<std::shared_ptr<Stmt>> _stmts)
        : stmts(std::move(_stmts)) {}
    IRNodeKind getKind() override { return IRNodeKind::BLOCK; }

    void addStmt(std::shared_ptr<Stmt> stmt) {
        stmts.push_back(std::move(stmt));
    }

    std::vector<std::shared_ptr<Stmt>> getStmts() { return stmts; }

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") override;
    static std::shared_ptr<StmtBlock>
    generateStructure(std::shared_ptr<GenCtx> ctx);
    void populate(std::shared_ptr<PopulateCtx> ctx) override;

    bool detectNestedForeach() override {
        return std::accumulate(stmts.begin(), stmts.end(), false,
                               [](bool acc, const std::shared_ptr<Stmt> &stmt) {
                                   return acc || stmt->detectNestedForeach();
                               });
    }

  protected:
    std::vector<std::shared_ptr<Stmt>> stmts;
};

class ScopeStmt : public StmtBlock {
  public:
    IRNodeKind getKind() final { return IRNodeKind::SCOPE; }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<ScopeStmt>
    generateStructure(std::shared_ptr<GenCtx> ctx);
};

class LoopStmt : public Stmt {};

class Pragma {
  public:
    explicit Pragma(PragmaKind _kind) : kind(_kind) {}
    PragmaKind getKind() { return kind; }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "");
    static std::shared_ptr<Pragma> create(std::shared_ptr<PopulateCtx> ctx);
    static std::vector<std::shared_ptr<Pragma>>
    create(size_t num, std::shared_ptr<PopulateCtx> ctx);

  private:
    PragmaKind kind;
};

class LoopHead {
  public:
    LoopHead()
        : prefix(nullptr), suffix(nullptr), is_foreach(false),
          same_iter_space(false), vectorizable(false) {}

    std::shared_ptr<StmtBlock> getPrefix() { return prefix; }
    void addPrefix(std::shared_ptr<StmtBlock> _prefix) {
        prefix = std::move(_prefix);
    }
    void addIterator(std::shared_ptr<Iterator> _iter) {
        iters.push_back(std::move(_iter));
    }
    std::vector<std::shared_ptr<Iterator>> getIterators() { return iters; }
    std::shared_ptr<StmtBlock> getSuffix() { return suffix; }
    void addSuffix(std::shared_ptr<StmtBlock> _suffix) {
        suffix = std::move(_suffix);
    }
    void emitPrefix(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                    std::string offset = "");
    void emitHeader(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                    std::string offset = "");
    void emitSuffix(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                    std::string offset = "");

    void setIsForeach(bool _val) { is_foreach = _val; }
    bool isForeach() { return is_foreach; }

    std::shared_ptr<Iterator>
    populateIterators(std::shared_ptr<PopulateCtx> ctx, size_t _end_val);
    void createPragmas(std::shared_ptr<PopulateCtx> ctx);
    bool hasSIMDPragma();

    static void populateArrays(std::shared_ptr<PopulateCtx> ctx);

    void setSameIterSpace() { same_iter_space = true; }

    void setVectorizable() { vectorizable = true; }

  private:
    std::shared_ptr<StmtBlock> prefix;
    // Loop iterations space is defined by the iterators that we can use
    std::vector<std::shared_ptr<Iterator>> iters;
    std::shared_ptr<StmtBlock> suffix;

    std::vector<std::shared_ptr<Pragma>> pragmas;

    // ISPC
    bool is_foreach;

    // We need to save info about of the loop belongs to the loop sequence with
    // the same iteration space (mostly for debug purposes)
    bool same_iter_space;

    bool vectorizable;
};

// According to the agreement, a single standalone loop should be represented as
// a LoopSeqStmt of size one
class LoopSeqStmt : public LoopStmt {
  public:
    IRNodeKind getKind() final { return IRNodeKind::LOOP_SEQ; }
    void
    addLoop(std::pair<std::shared_ptr<LoopHead>, std::shared_ptr<ScopeStmt>>
                _loop) {
        loops.push_back(std::move(_loop));
    }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<LoopSeqStmt>
    generateStructure(std::shared_ptr<GenCtx> ctx);
    void populate(std::shared_ptr<PopulateCtx> ctx) override;

    bool detectNestedForeach() override {
        return std::accumulate(
            loops.begin(), loops.end(), false,
            [](bool acc, const std::pair<std::shared_ptr<LoopHead>,
                                         std::shared_ptr<ScopeStmt>> &loop) {
                return acc || loop.first->isForeach() ||
                       loop.second->detectNestedForeach();
            });
    }

  private:
    std::vector<
        std::pair<std::shared_ptr<LoopHead>, std::shared_ptr<ScopeStmt>>>
        loops;
};

class LoopNestStmt : public LoopStmt {
  public:
    IRNodeKind getKind() final { return IRNodeKind::LOOP_NEST; }
    void addLoop(std::shared_ptr<LoopHead> _loop) {
        loops.push_back(std::move(_loop));
    }
    void addBody(std::shared_ptr<ScopeStmt> _body) { body = std::move(_body); }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<LoopNestStmt>
    generateStructure(std::shared_ptr<GenCtx> ctx);
    void populate(std::shared_ptr<PopulateCtx> ctx) override;

    bool detectNestedForeach() override {
        return std::accumulate(
                   loops.begin(), loops.end(), false,
                   [](bool acc, const std::shared_ptr<LoopHead> &loop) {
                       return acc || loop->isForeach();
                   }) ||
               body->detectNestedForeach();
    }

  private:
    std::vector<std::shared_ptr<LoopHead>> loops;
    std::shared_ptr<StmtBlock> body;
};

class IfElseStmt : public Stmt {
  public:
    IfElseStmt(std::shared_ptr<Expr> _cond, std::shared_ptr<ScopeStmt> _then_br,
               std::shared_ptr<ScopeStmt> _else_br)
        : cond(std::move(_cond)), then_br(std::move(_then_br)),
          else_br(std::move(_else_br)) {}
    IRNodeKind getKind() final { return IRNodeKind::IF_ELSE; }
    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<IfElseStmt>
    generateStructure(std::shared_ptr<GenCtx> ctx);
    void populate(std::shared_ptr<PopulateCtx> ctx) final;

    bool detectNestedForeach() override {
        return then_br->detectNestedForeach() ||
               (else_br.use_count() != 0 && else_br->detectNestedForeach());
    }

  private:
    std::shared_ptr<Expr> cond;
    std::shared_ptr<ScopeStmt> then_br;
    std::shared_ptr<ScopeStmt> else_br;
};

class StubStmt : public Stmt {
  public:
    explicit StubStmt(std::string _text) : text(std::move(_text)) {}
    IRNodeKind getKind() final { return IRNodeKind::STUB; }

    void emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
              std::string offset = "") final;
    static std::shared_ptr<StubStmt>
    generateStructure(std::shared_ptr<GenCtx> ctx);

  private:
    std::string text;
};
} // namespace yarpgen
