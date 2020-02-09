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

#include "stmt.h"

using namespace yarpgen;

void ExprStmt::emit(std::ostream &stream, std::string offset) {
    stream << offset;
    expr->emit(stream);
    stream << ";";
}

void DeclStmt::emit(std::ostream &stream, std::string offset) {
    stream << offset;
    //TODO: we need to do the right thing here
    stream << data->getType()->getName() << " ";
    stream << data->getName();
    if (init_expr.use_count() != 0) {
        stream << " = ";
        init_expr->emit(stream);
    }
    stream << ";";
}

void StmtBlock::emit(std::ostream &stream, std::string offset) {
    for (const auto &stmt : stmts) {
        stmt->emit(stream, offset + "    ");
        stream << "\n;";
    }
}

void ScopeStmt::emit(std::ostream &stream, std::string offset) {
    stream << "{" << offset;
    StmtBlock::emit(stream, offset + "    ");
    stream << offset << "}";
}

void LoopSeqStmt::LoopHead::emit(std::ostream &stream, std::string offset) {
    //TODO: it will handle simple cases. We need to improve it later.
    prefix->emit(stream, offset);
    stream << offset << "for (";

    auto place_sep = [this] (auto iter, std::string sep)->std::string {
        return iter != iters.end() - 1 ? std::move(sep) : "";
    };

    for (auto iter = iters.begin(); iter != iters.end(); ++iter) {
        stream << (*iter)->getType()->getName() << " ";
        stream << (*iter)->getName() << " = ";
        (*iter)->getStart()->emit(stream);
        place_sep(iter, ",");
    }
    stream << "; ";

    for (auto iter = iters.begin(); iter != iters.end(); ++iter) {
        stream << (*iter)->getName() << " < ";
        (*iter)->getEnd()->emit(stream);
        place_sep(iter, ",");
    }
    stream << "; ";

    for (auto iter = iters.begin(); iter != iters.end(); ++iter) {
        stream << (*iter)->getName() << " += ";
        (*iter)->getStep()->emit(stream);
        place_sep(iter, ",");
    }
    stream << ") ";

    body->emit(stream, offset + "    ");

    suffix->emit(stream, offset);
}

void LoopSeqStmt::emit(std::ostream &stream, std::string offset) {
    for (const auto &loop : loops) {
        loop->emit(stream, offset);
    }
}
