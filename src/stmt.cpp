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

#include "stmt.h"
#include "options.h"
#include "statistics.h"

#include <algorithm>
#include <utility>

using namespace yarpgen;

void ExprStmt::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                    std::string offset) {
    stream << offset;
    expr->emit(ctx, stream);
    stream << ";";
}

std::shared_ptr<ExprStmt> ExprStmt::create(std::shared_ptr<PopulateCtx> ctx) {
    auto expr = AssignmentExpr::create(ctx);
    EvalCtx eval_ctx;
    expr->evaluate(eval_ctx);
    return std::make_shared<ExprStmt>(expr);
}

void DeclStmt::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                    std::string offset) {
    stream << offset;
    // TODO: we need to do the right thing here
    stream << data->getType()->getName(ctx) << " ";
    stream << data->getName(ctx);
    if (init_expr.use_count() != 0) {
        stream << " = ";
        init_expr->emit(ctx, stream);
    }
    stream << ";";
}

void StmtBlock::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                     std::string offset) {
    for (const auto &stmt : stmts) {
        stmt->emit(ctx, stream, offset);
        // TODO: will that work if we have suffix?
        if (stmt->getKind() != IRNodeKind::LOOP_SEQ &&
            stmt->getKind() != IRNodeKind::LOOP_NEST)
            stream << "\n";
    }
}

std::shared_ptr<StmtBlock>
StmtBlock::generateStructure(std::shared_ptr<GenCtx> ctx) {
    std::vector<std::shared_ptr<Stmt>> stmts;

    auto gen_policy = ctx->getGenPolicy();
    size_t stmt_num = rand_val_gen->getRandId(gen_policy->scope_stmt_num_distr);
    stmts.reserve(stmt_num);

    Statistics &stats = Statistics::getInstance();

    std::shared_ptr<Stmt> new_stmt;
    for (size_t i = 0; i < stmt_num; ++i) {
        IRNodeKind stmt_kind =
            rand_val_gen->getRandId(gen_policy->stmt_kind_struct_distr);

        bool fallback = false;
        // Last stmt that we can fit
        fallback |= stats.getStmtNum() + 1 >= gen_policy->stmt_num_lim;
        // LoopSeq and If-else create two new stmt
        fallback |= (stmt_kind == IRNodeKind::LOOP_SEQ ||
                     stmt_kind == IRNodeKind::IF_ELSE) &&
                    (stats.getStmtNum() + 2 >= gen_policy->stmt_num_lim);
        // Loop nest creates at least three new stmt (single loop is a loop
        // stmt)
        fallback |= stmt_kind == IRNodeKind::LOOP_NEST &&
                    (stats.getStmtNum() + 3 >= gen_policy->stmt_num_lim);
        if (fallback)
            break;

        if (stmt_kind == IRNodeKind::LOOP_SEQ &&
            ctx->getLoopDepth() < gen_policy->loop_depth_limit)
            new_stmt = LoopSeqStmt::generateStructure(ctx);
        else if (stmt_kind == IRNodeKind::LOOP_NEST &&
                 ctx->getLoopDepth() + 2 <= gen_policy->loop_depth_limit) {
            new_stmt = LoopNestStmt::generateStructure(ctx);
        }
        else if (stmt_kind == IRNodeKind::IF_ELSE &&
                 ctx->getIfElseDepth() + 1 <= gen_policy->if_else_depth_limit)
            new_stmt = IfElseStmt::generateStructure(ctx);
        else {
            new_stmt = StubStmt::generateStructure(ctx);
            stats.addStmt();
        }
        stmts.push_back(new_stmt);
    }

    return std::make_shared<StmtBlock>(stmts);
}

void StmtBlock::populate(std::shared_ptr<PopulateCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();

    if (ctx->getLoopDepth() != 0) {
        size_t new_arrays_num =
            rand_val_gen->getRandId(gen_pol->new_arr_num_distr);
        for (size_t i = 0; i < new_arrays_num; ++i) {
            ctx->getExtInpSymTable()->addArray(Array::create(ctx, true));
        }
    }

    for (auto &stmt : stmts) {
        if (stmt->getKind() != IRNodeKind::STUB)
            stmt->populate(ctx);
        else {
            std::shared_ptr<Stmt> new_stmt;
            IRNodeKind new_stmt_kind =
                rand_val_gen->getRandId(gen_pol->stmt_kind_pop_distr);
            if (new_stmt_kind == IRNodeKind::ASSIGN) {
                new_stmt = ExprStmt::create(ctx);
            }
            else
                ERROR("Bad IRNode kind drawing");
            stmt = new_stmt;
        }
    }
}

void ScopeStmt::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                     std::string offset) {
    stream << offset << "{\n";
    StmtBlock::emit(ctx, stream, offset + "    ");
    stream << offset << "}\n";
}

std::shared_ptr<ScopeStmt>
ScopeStmt::generateStructure(std::shared_ptr<GenCtx> ctx) {
    // TODO: will that work?
    auto new_scope = std::make_shared<ScopeStmt>();
    auto stmt_block = StmtBlock::generateStructure(std::move(ctx));
    new_scope->stmts = stmt_block->getStmts();
    return new_scope;
}

void LoopHead::emitPrefix(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::string offset) {
    if (prefix.use_count() != 0)
        prefix->emit(ctx, stream, std::move(offset));
}

void LoopHead::emitHeader(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::string offset) {
    stream << offset;

    auto place_sep = [this](auto iter, std::string sep) -> std::string {
        return iter != iters.end() - 1 ? std::move(sep) : "";
    };

    if (!isForeach()) {
        stream << "for (";

        for (auto iter = iters.begin(); iter != iters.end(); ++iter) {
            stream << (*iter)->getType()->getName(ctx) << " ";
            stream << (*iter)->getName(ctx) << " = ";
            (*iter)->getStart()->emit(ctx, stream);
            stream << place_sep(iter, ", ");
        }
        stream << "; ";

        for (auto iter = iters.begin(); iter != iters.end(); ++iter) {
            stream << (*iter)->getName(ctx) << " < ";
            (*iter)->getEnd()->emit(ctx, stream);
            stream << place_sep(iter, ", ");
        }
        stream << "; ";

        for (auto iter = iters.begin(); iter != iters.end(); ++iter) {
            stream << (*iter)->getName(ctx) << " += ";
            (*iter)->getStep()->emit(ctx, stream);
            stream << place_sep(iter, ", ");
        }
        stream << ") ";
    }
    else {
        stream << (iters.size() == 1 ? "foreach" : "foreach_tiled") << "(";

        for (auto iter = iters.begin(); iter != iters.end(); ++iter) {
            stream << (*iter)->getName(ctx) << " = ";
            (*iter)->getStart()->emit(ctx, stream);
            stream << "...";
            (*iter)->getEnd()->emit(ctx, stream);
            stream << place_sep(iter, ", ");
        }

        stream << ") ";
    }
}

void LoopHead::emitSuffix(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                          std::string offset) {
    if (suffix.use_count() != 0)
        suffix->emit(ctx, stream, std::move(offset));
}
void LoopHead::populateIterators(std::shared_ptr<PopulateCtx> ctx) {
    for (auto &iter : iters)
        iter->populate(ctx);
}

void LoopSeqStmt::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                       std::string offset) {
    stream << offset << "/* LoopSeq " << std::to_string(loops.size())
           << " */\n";

    for (const auto &loop : loops) {
        loop.first->emitPrefix(ctx, stream, offset);
        loop.first->emitHeader(ctx, stream, offset);
        stream << "\n";
        loop.second->emit(ctx, stream, offset);
        loop.first->emitSuffix(ctx, stream, offset);
    }
}

std::shared_ptr<LoopSeqStmt>
LoopSeqStmt::generateStructure(std::shared_ptr<GenCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    size_t loop_num = rand_val_gen->getRandId(gen_pol->loop_seq_num_distr);

    Options &options = Options::getInstance();

    auto new_loop_seq = std::make_shared<LoopSeqStmt>();
    auto new_ctx = std::make_shared<GenCtx>(*ctx);
    // TODO: is it the right place to do it?
    new_ctx->incLoopDepth(1);
    for (size_t i = 0; i < loop_num; ++i) {
        bool gen_foreach = false;
        auto new_loop_head = std::make_shared<LoopHead>();

        if (options.isISPC())
            gen_foreach = !ctx->isInsideForeach() &&
                          rand_val_gen->getRandId(gen_pol->foreach_distr);
        if (gen_foreach) {
            new_ctx->setInsideForeach(true);
            new_loop_head->setIsForeach();
        }

        size_t iter_num = rand_val_gen->getRandId(gen_pol->iters_num_distr);
        for (size_t iter_idx = 0; iter_idx < iter_num; ++iter_idx) {
            auto new_iter = Iterator::create(ctx, /*is_uniform*/ !gen_foreach);
            // TODO: at some point we might use only some iterators
            new_iter->setIsDead(false);
            new_loop_head->addIterator(new_iter);
        }
        auto new_loop_body = ScopeStmt::generateStructure(new_ctx);
        new_loop_seq->addLoop(std::make_pair(new_loop_head, new_loop_body));

        if (gen_foreach)
            new_ctx->setInsideForeach(false);
    }

    Statistics &stats = Statistics::getInstance();
    stats.addStmt(loop_num);

    return new_loop_seq;
}

void LoopSeqStmt::populate(std::shared_ptr<PopulateCtx> ctx) {
    for (auto &loop : loops) {
        auto loop_head = loop.first;
        if (loop_head->getPrefix().use_count() != 0)
            loop_head->getPrefix()->populate(ctx);
        auto new_ctx = std::make_shared<PopulateCtx>(ctx);
        new_ctx->incLoopDepth(1);
        loop_head->populateIterators(ctx);
        new_ctx->getLocalSymTable()->addIters(loop_head->getIterators());
        bool old_ctx_state = new_ctx->isTaken();
        // TODO: what if we have multiple iterators
        if (loop_head->getIterators().front()->isDegenerate())
            new_ctx->setTaken(false);
        new_ctx->setInsideForeach(loop.first->isForeach());
        loop.second->populate(new_ctx);
        new_ctx->decLoopDepth(1);
        new_ctx->getLocalSymTable()->deleteLastIters();
        new_ctx->setInsideForeach(false);
        new_ctx->setTaken(old_ctx_state);
        if (loop_head->getSuffix().use_count() != 0)
            loop_head->getSuffix()->populate(ctx);
    }
}

void LoopNestStmt::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                        std::string offset) {
    stream << offset << "/* LoopNest " << std::to_string(loops.size())
           << " */\n";

    std::string new_offset = offset;
    for (const auto &loop : loops) {
        loop->emitPrefix(ctx, stream, new_offset);
        loop->emitHeader(ctx, stream, new_offset);
        stream << "\n" << new_offset << "{\n";
        new_offset += "    ";
    }

    body->emit(ctx, stream, new_offset);
    new_offset.erase(new_offset.size() - 4, 4);

    for (const auto &loop : loops) {
        stream << new_offset << "} \n";
        loop->emitSuffix(ctx, stream, new_offset);
        new_offset.erase(new_offset.size() - 4, 4);
    }
}

std::shared_ptr<LoopNestStmt>
LoopNestStmt::generateStructure(std::shared_ptr<GenCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    size_t nest_depth = rand_val_gen->getRandId(gen_pol->loop_nest_depth_distr);

    // We need to limit a maximal loop depth that we can generate
    nest_depth =
        std::min(gen_pol->loop_depth_limit - ctx->getLoopDepth(), nest_depth);

    Options &options = Options::getInstance();

    auto new_loop_nest = std::make_shared<LoopNestStmt>();
    for (size_t i = 0; i < nest_depth; ++i) {
        auto new_loop = std::make_shared<LoopHead>();

        bool gen_foreach = false;
        if (options.isISPC())
            gen_foreach = !ctx->isInsideForeach() &&
                          rand_val_gen->getRandId(gen_pol->foreach_distr);
        if (gen_foreach) {
            new_loop->setIsForeach();
            ctx->setInsideForeach(true);
        }

        size_t iter_num = rand_val_gen->getRandId(gen_pol->iters_num_distr);
        for (size_t iter_idx = 0; iter_idx < iter_num; ++iter_idx) {
            auto new_iter = Iterator::create(ctx, !gen_foreach);
            // TODO: at some point we might create dead iterators
            new_iter->setIsDead(false);
            new_loop->addIterator(new_iter);
        }
        new_loop_nest->addLoop(new_loop);
    }

    Statistics &stats = Statistics::getInstance();
    stats.addStmt(nest_depth);

    auto new_ctx = std::make_shared<GenCtx>(*ctx);
    new_ctx->incLoopDepth(nest_depth);
    new_loop_nest->addBody(ScopeStmt::generateStructure(new_ctx));

    return new_loop_nest;
}

void LoopNestStmt::populate(std::shared_ptr<PopulateCtx> ctx) {
    auto new_ctx = std::make_shared<PopulateCtx>(ctx);
    bool old_ctx_state = new_ctx->isTaken();
    std::vector<std::shared_ptr<LoopHead>>::iterator taken_switch_id;
    for (auto i = loops.begin(); i != loops.end(); ++i) {
        if ((*i)->getPrefix().use_count() != 0) {
            (*i)->getPrefix()->populate(new_ctx);
            taken_switch_id = i;
        }
        new_ctx->incLoopDepth(1);
        (*i)->populateIterators(ctx);
        new_ctx->getLocalSymTable()->addIters((*i)->getIterators());
        if ((*i)->isForeach())
            new_ctx->setInsideForeach(true);
        if ((*i)->getIterators().front()->isDegenerate())
            new_ctx->setTaken(false);
    }
    body->populate(new_ctx);
    for (auto i = loops.begin(); i != loops.end(); ++i) {
        new_ctx->decLoopDepth(1);
        new_ctx->getLocalSymTable()->deleteLastIters();
        if (i == taken_switch_id)
            new_ctx->setTaken(old_ctx_state);
        if ((*i)->isForeach())
            new_ctx->setInsideForeach(false);
        if ((*i)->getSuffix().use_count() != 0)
            (*i)->getSuffix()->populate(new_ctx);
    }
}

void IfElseStmt::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                      std::string offset) {
    stream << offset << "if (";
    // We can dump test structure before populating it
    if (cond.use_count() != 0)
        cond->emit(ctx, stream);
    stream << ")\n";
    then_br->emit(ctx, stream, offset);
    if (else_br.use_count() != 0) {
        stream << "else ";
        else_br->emit(ctx, stream, offset);
    }
}

std::shared_ptr<IfElseStmt>
IfElseStmt::generateStructure(std::shared_ptr<GenCtx> ctx) {
    auto gen_pol = ctx->getGenPolicy();
    auto new_ctx = std::make_shared<GenCtx>(*ctx);
    new_ctx->incIfElseDepth();
    auto then_br = ScopeStmt::generateStructure(new_ctx);
    bool else_br_exist = rand_val_gen->getRandId(gen_pol->else_br_distr);
    std::shared_ptr<ScopeStmt> else_br;
    if (else_br_exist)
        else_br = ScopeStmt::generateStructure(new_ctx);

    Statistics &stats = Statistics::getInstance();
    stats.addStmt();

    return std::make_shared<IfElseStmt>(nullptr, then_br, else_br);
}

void IfElseStmt::populate(std::shared_ptr<PopulateCtx> ctx) {
    cond = ArithmeticExpr::create(ctx);

    if (!cond->getValue()->isScalarVar()) {
        ERROR("Can perform conversion to bool only on scalar variables");
    }

    std::shared_ptr<IntegralType> int_type =
        std::static_pointer_cast<IntegralType>(cond->getValue()->getType());
    if (int_type->getIntTypeId() != IntTypeID::BOOL) {
        cond = std::make_shared<TypeCastExpr>(
            cond,
            IntegralType::init(IntTypeID::BOOL, false, CVQualifier::NONE,
                               cond->getValue()->getType()->isUniform()),
            true);
    }

    EvalCtx eval_ctx;
    std::shared_ptr<Data> cond_eval_res = cond->evaluate(eval_ctx);
    IRValue cond_val =
        std::static_pointer_cast<ScalarVar>(cond_eval_res)->getCurrentValue();

    auto new_ctx = std::make_shared<PopulateCtx>(*ctx);
    new_ctx->incIfElseDepth();
    bool cond_taken = cond_val.getValueRef<bool>();
    new_ctx->setTaken(ctx->isTaken() && cond_taken);

    then_br->populate(new_ctx);
    if (else_br.use_count() != 0) {
        new_ctx->setTaken(ctx->isTaken() && !cond_taken);
        else_br->populate(new_ctx);
    }
}

void StubStmt::emit(std::shared_ptr<EmitCtx> ctx, std::ostream &stream,
                    std::string offset) {
    stream << offset << text;
}

std::shared_ptr<StubStmt>
StubStmt::generateStructure(std::shared_ptr<GenCtx> ctx) {
    NameHandler &nh = NameHandler::getInstance();
    return std::make_shared<StubStmt>("Stub stmt #" + nh.getStubStmtIdx());
}
