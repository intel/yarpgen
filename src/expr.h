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

class Expr : public Node {
    public:
        Expr (Node::NodeID _id, std::shared_ptr<Data> _value) : Node(_id), value(_value) {}
        Type::TypeID get_type_id () { return value->get_type()->get_type_id (); }
        std::shared_ptr<Data> get_value ();

    protected:
        virtual bool propagate_type () = 0;
        virtual UB propagate_value () = 0;
        std::shared_ptr<Data> value;
};

class VarUseExpr : public Expr {
    public:
        VarUseExpr (std::shared_ptr<Data> _var);
        std::shared_ptr<Expr> set_value (std::shared_ptr<Expr> _expr);
        std::string emit (std::string offset = "") { return value->get_name (); }

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }
};

class AssignExpr : public Expr {
    public:
        AssignExpr (std::shared_ptr<Expr> _to, std::shared_ptr<Expr> _from, bool _taken = true);
        std::string emit (std::string offset = "");

    private:
        bool propagate_type ();
        UB propagate_value ();

        std::shared_ptr<Expr> to;
        std::shared_ptr<Expr> from;
        bool taken;
};

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
        bool is_implicit;
};

class ConstExpr : public Expr {
    public:
        ConstExpr (AtomicType::ScalarTypedVal _val);
        std::string emit (std::string offset = "");
        static std::shared_ptr<ConstExpr> generate (std::shared_ptr<Context> ctx);

    private:
        bool propagate_type () { return true; }
        UB propagate_value () { return NoUB; }
};

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
        void rebuild (UB ub);

        Op op;
        std::shared_ptr<Expr> arg;
};

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
        void rebuild (UB ub);

        Op op;
        std::shared_ptr<Expr> arg0;
        std::shared_ptr<Expr> arg1;
};

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
