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

namespace rl {

class Context;
class GenPolicy;

// Abstract class, serves as common ancestor for all expressions.
class Expr : public Node {
    public:
        Expr (Node::NodeID _id, std::shared_ptr<Data> _value) : Node(_id), value(_value) {}
        Type::TypeID get_type_id () { return value->get_type()->get_type_id (); }
        std::shared_ptr<Data> get_value ();

    protected:
        // This function does type conversions required by standard (implicit cast, integral promotion or
        // usual arithmetic conversions) to existing child nodes.
        // As a result, it inserts required TypeCastExpr between existing child nodes and current node.
        virtual bool propagate_type () = 0;
        // This function calculates value of current node, based on its child nodes.
        // Also it detects UB and eliminates it (for more information, see rebuild() method in inherited classes).
        // It requires propagate_type() to be called first.
        virtual UB propagate_value () = 0;
        std::shared_ptr<Data> value;
};

// Variable Use expression provides access to variable.
// Any interaction with a variable (access to its value) in generated test is represented with this class.
// For example, assignment to the variable may use VarUseExpr as lhs.
class VarUseExpr : public Expr {
    public:
        VarUseExpr (std::shared_ptr<Data> _var);
        std::shared_ptr<Expr> set_value (std::shared_ptr<Expr> _expr);
        std::string emit (std::string offset = "") { return value->get_name (); }

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }
};

// Assignment expression represents assignment of one expression to another.
// Its constructor replaces implicit cast (cast rhs to the type of lhs) with TypeCastExpr node and
// updates value of lhs (only if this assignment is evaluated in test).
// E.g.: lhs_expr = rhs_expr
class AssignExpr : public Expr {
    public:
        AssignExpr (std::shared_ptr<Expr> _to, std::shared_ptr<Expr> _from, bool _taken = true);
        std::string emit (std::string offset = "");

    private:
        bool propagate_type ();
        UB propagate_value ();

        // Destination (can be only VarUseExpr or MemberExpr).
        std::shared_ptr<Expr> to;
        // Rhs part of assignment expression.
        std::shared_ptr<Expr> from;
        // Taken indicates whether expression is evaluated and lhs value should be updated.
        bool taken;
};

// Type Cast expression represents implicit and explicit type casts.
// The creator of TypeCastExpr should make the decision about its kind (implicit or explicit)
// and pass it to constructor. All of implicit casts should be represented with this class.
// E.g.: (to_type) expr;
class TypeCastExpr : public Expr {
    public:
        TypeCastExpr (std::shared_ptr<Expr> _expr, std::shared_ptr<Type> _type, bool _is_implicit = false);
        std::string emit (std::string offset = "");
        static std::shared_ptr<TypeCastExpr> generate (std::shared_ptr<Context> ctx, std::shared_ptr<Expr> from);

    private:
        bool propagate_type ();
        UB propagate_value ();

        std::shared_ptr<Expr> expr;
        std::shared_ptr<Type> to_type;
        // This flag indicates if this conversion is implicit or not.
        // I.e. if it's implicit and omitted - the program behavior won't change.
        bool is_implicit;
};

// Constant expression represents constant values
// E.g.: 123ULL
class ConstExpr : public Expr {
    public:
        ConstExpr (AtomicType::ScalarTypedVal _val);
        std::string emit (std::string offset = "");
        static std::shared_ptr<ConstExpr> generate (std::shared_ptr<Context> ctx);

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }
};

// Arithmetic expression abstract class, serves as common ancestor for unary/binary expressions.
// We construct expression tree, using top-down approach.
// After that in bottom-up direction we propagate types and values (if we detect UB, we eliminate it).
class ArithExpr : public Expr {
    public:
        ArithExpr(Node::NodeID _node_id, std::shared_ptr<Data> _val) : Expr(_node_id, _val) {}
        static std::shared_ptr<Expr> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp);

    protected:
        static GenPolicy choose_and_apply_ssp_const_use (GenPolicy old_gen_policy);
        static GenPolicy choose_and_apply_ssp_similar_op (GenPolicy old_gen_policy);
        static GenPolicy choose_and_apply_ssp (GenPolicy old_gen_policy);
        static std::shared_ptr<Expr> gen_level (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, int par_depth);

        std::shared_ptr<Expr> integral_prom (std::shared_ptr<Expr> arg);
        std::shared_ptr<Expr> conv_to_bool (std::shared_ptr<Expr> arg);
};

// Unary expression represents all unary operators
// E.g.: +lhs_expr;
class UnaryExpr : public ArithExpr {
    public:
        enum Op {
            PreInc,  ///< Pre-increment //no
            PreDec,  ///< Pre-decrement //no
            PostInc, ///< Post-increment //no
            PostDec, ///< Post-decrement //no
            Plus,    ///< Plus //ip
            Negate,  ///< Negation //ip
            LogNot,  ///< Logical not //bool
            BitNot,  ///< Bit not //ip
            MaxOp
        };
        UnaryExpr (Op _op, std::shared_ptr<Expr> _arg);
        Op get_op () { return op; }
        std::string emit (std::string offset = "");
        static std::shared_ptr<UnaryExpr> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, int par_depth);

    private:
        bool propagate_type ();
        UB propagate_value ();
        // This function eliminates UB. It changes operator to complimentary.
        void rebuild (UB ub);

        Op op;
        std::shared_ptr<Expr> arg;
};

// Binary expression - represents all binary operators
// E.g.: lhs_expr + rhs_expr;
class BinaryExpr : public ArithExpr {
    public:
        enum Op {
            Add   , ///< Addition
            Sub   , ///< Subtraction
            Mul   , ///< Multiplication
            Div   , ///< Division
            Mod   , ///< Modulus
            Shl   , ///< Shift left
            Shr   , ///< Shift right

            Lt    ,  ///< Less than
            Gt    ,  ///< Greater than
            Le    ,  ///< Less than or equal
            Ge    , ///< Greater than or equal
            Eq    , ///< Equal
            Ne    , ///< Not equal

            BitAnd, ///< Bitwise AND
            BitXor, ///< Bitwise XOR
            BitOr , ///< Bitwise OR

            LogAnd, ///< Logical AND
            LogOr , ///< Logical OR

            MaxOp
        };

        BinaryExpr (Op _op, std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs);
        Op get_op () { return op; }
        std::string emit (std::string offset = "");
        static std::shared_ptr<BinaryExpr> generate (std::shared_ptr<Context> ctx, std::vector<std::shared_ptr<Expr>> inp, int par_depth);

    private:
        bool propagate_type ();
        UB propagate_value ();
        void perform_arith_conv ();
        // This function eliminates UB. It changes operator to complimentary or inserts new nodes.
        void rebuild (UB ub);

        Op op;
        std::shared_ptr<Expr> arg0;
        std::shared_ptr<Expr> arg1;
};

// Member expression - provides access to members of struct variable
// E.g.: struct_obj.member_1
class MemberExpr : public Expr {
    public:
        MemberExpr (std::shared_ptr<Struct> _struct, uint64_t _identifier);
        MemberExpr (std::shared_ptr<MemberExpr> _member_expr, uint64_t _identifier);
        std::shared_ptr<Expr> set_value (std::shared_ptr<Expr> _expr);
        std::string emit (std::string offset = "");

    private:
        bool propagate_type ();
        UB propagate_value ();
        std::shared_ptr<Expr> check_and_set_bit_field (std::shared_ptr<Expr> _expr);

        std::shared_ptr<MemberExpr> member_expr;
        std::shared_ptr<Struct> struct_var;
        uint64_t identifier;
};

}
