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

namespace yarpgen {

class Node {
    public:
        enum NodeID {
            // Expr types
            MIN_EXPR_ID,
            ASSIGN,
            BINARY,
            CONST,
//            EXPR_LIST,
//            FUNC_CALL,
            TYPE_CAST,
            UNARY,
            VAR_USE,
            MEMBER,
            MAX_EXPR_ID,
            // Stmt type
            MIN_STMT_ID,
            DECL,
            EXPR,
            SCOPE,
//            CNT_LOOP,
            IF,
//            BREAK,
//            CONTINUE,
            MAX_STMT_ID
        };
        Node (NodeID _id) : id(_id) {}
        NodeID get_id () { return id; }
        virtual std::string emit (std::string offset = "") = 0;

    private:
        NodeID id;
};
}
