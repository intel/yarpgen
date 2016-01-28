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

class Node {
    public:
        enum NodeID {
            // Expr types
            MIN_EXPR_ID,
            ASSIGN,
            BINARY,
            CONST,
            EXPR_LIST,
            FUNC_CALL,
            INDEX,
            TYPE_CAST,
            UNARY,
            VAR_USE,
            MAX_EXPR_ID,
            // Stmt type
            MIN_STMT_ID,
            DECL,
            EXPR,
            CNT_LOOP,
            IF,
            MAX_STMT_ID
        };
        Node () : id (MAX_EXPR_ID), is_expr(false) {}
        bool get_is_expr () { return is_expr; }
        NodeID get_id () { return id; }
        virtual std::string emit () = 0;

    protected:
        NodeID id;
        bool is_expr;
};

class Expr : public Node {
    public:
        enum UB {
            NoUB,
            NullPtr, // NULL ptr dereferencing
            SignOvf, // Signed overflow
            SignOvfMin, // Special case of signed overflow: INT_MIN * (-1)
            ZeroDiv, // FPE
            ShiftRhsNeg, // Shift by negative value
            ShiftRhsLarge, // // Shift by large value
            NegShift, // Shift of negative value
        };
        Expr () : value("", Type::TypeID::ULLINT, Variable::Mod::NTHNG, false) {is_expr = true;}
        Type::TypeID get_type_id () { return value.get_type()->get_id (); }
        bool get_type_is_signed () { return value.get_type()->get_is_signed(); }
        uint64_t get_type_bit_size () { return value.get_type()->get_bit_size(); }
        uint64_t get_value () { return value.get_value(); }
        bool get_type_sign () { return value.get_type()->get_is_signed(); }
        virtual void propagate_type () = 0;
        virtual UB propagate_value () = 0;

    protected:
        Variable value;
};

class VarUseExpr : public Expr {
    public:
        VarUseExpr () : var (NULL) { id = Node::NodeID::VAR_USE; }
        void set_variable (std::shared_ptr<Variable> _var);
        void propagate_type () { value.set_type(var->get_type()); }
        UB propagate_value () { value.set_value(var->get_value()); return NoUB; }
        std::string emit () { return var->get_name (); }

    private:
        std::shared_ptr<Variable> var;
};

class AssignExpr : public Expr {
    public:
        AssignExpr () : to (NULL), from (NULL) { id = Node::NodeID::ASSIGN; }
        void set_to (std::shared_ptr<Expr> _to);
        void set_from (std::shared_ptr<Expr> _from);
        void propagate_type ();
        UB propagate_value () { value.set_value(from->get_value()); return NoUB; }
        std::string emit ();

    private:
        std::shared_ptr<Expr> to;
        std::shared_ptr<Expr> from;
};

class IndexExpr : public Expr {
    public:
        IndexExpr () : base (NULL), index (NULL), is_subscr (true) { id = Node::NodeID::INDEX; }
        void set_base (std::shared_ptr<Array> _base);
        void set_index (std::shared_ptr<Expr> _index) { index = _index; }
        void set_is_subscr (bool _is_subscr) { is_subscr = _is_subscr || base->get_essence() == Array::Ess::C_ARR || base->get_essence() == Array::Ess::VAL_ARR; }
        void propagate_type () { value.set_type(base->get_base_type()); }
        UB propagate_value () { value.set_value(base->get_value()); return NoUB; }
        std::string emit ();

    private:
        std::shared_ptr<Array> base;
        std::shared_ptr<Expr> index;
        bool is_subscr; // We can access std::array and std::vector elements through [i] and at(i)
};

class ArithExpr :  public Expr {
    protected:
        std::shared_ptr<Expr> integral_prom (std::shared_ptr<Expr> arg);
        std::shared_ptr<Expr> conv_to_bool (std::shared_ptr<Expr> arg);
//TODO: What is it?        std::shared_ptr<Expr> integral_conv (std::shared_ptr<Expr> arg);
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

        BinaryExpr () : op (MaxOp), arg0 (NULL), arg1 (NULL) { id = Node::NodeID::BINARY; }
        void set_op (Op _op) { op = _op; }
        Op get_op () { return op; }
        void set_lhs (std::shared_ptr<Expr> _arg0) { arg0 = _arg0; }
        void set_rhs (std::shared_ptr<Expr> _arg1) { arg1 = _arg1; }
        std::shared_ptr<Expr> get_lhs() { return arg0; }
        std::shared_ptr<Expr> get_rhs() { return arg1; }
        void propagate_type ();
        UB propagate_value ();
        std::string emit ();

    private:
        void perform_arith_conv ();

        Op op;
        std::shared_ptr<Expr> arg0;
        std::shared_ptr<Expr> arg1;
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
        UnaryExpr () : op (MaxOp), arg (NULL) { id = Node::NodeID::UNARY; }
        void set_op (Op _op) { op = _op; }
        Op get_op () { return op; }
        void set_arg (std::shared_ptr<Expr> _arg) { arg = _arg; }
        void propagate_type ();
        UB propagate_value ();
        std::string emit ();

    private:
        Op op;
        std::shared_ptr<Expr> arg;
};

class ConstExpr : public Expr {
    public:
        ConstExpr () : data (0) { id = Node::NodeID::CONST; }
        void set_type (Type::TypeID type_id) { value.set_type(Type::init (type_id)); }
        void set_data (uint64_t _data) { data = _data; }
        void propagate_type () {};
        UB propagate_value () { value.set_value(data); return NoUB; }
        std::string emit ();

    private:
        uint64_t data;
};

class TypeCastExpr : public Expr {
    public:
        TypeCastExpr () : expr (NULL) { id = Node::NodeID::TYPE_CAST; }
        void set_type (std::shared_ptr<Type> _type) { value.set_type(_type); }
        void set_expr (std::shared_ptr<Expr> _expr) { expr = _expr; }
        void propagate_type () {};
        UB propagate_value () { value.set_value(expr->get_value()); return NoUB; }
        std::string emit ();

    private:
        std::shared_ptr<Expr> expr;
};

class ExprListExpr : public Expr {
    public:
        ExprListExpr () { id = Node::NodeID::EXPR_LIST; }
        void add_to_list(std::shared_ptr<Expr> expr) { expr_list.push_back(expr); }
        void propagate_type () {};
        UB propagate_value () { return NoUB; }
        std::string emit ();

    private:
        std::vector<std::shared_ptr<Expr>> expr_list;
};

class FuncCallExpr : public Expr {
    public:
        FuncCallExpr () : name (""), args (NULL) { id = Node::NodeID::FUNC_CALL; }
        void set_name (std::string _name) { name = _name; }
        void set_args (std::shared_ptr<Expr> _args) { args = _args; }
        void add_to_args (std::shared_ptr<Expr> _arg) { std::static_pointer_cast<ExprListExpr>(args)->add_to_list(_arg); }
        void propagate_type () {};
        UB propagate_value () { return NoUB; }
        std::string emit ();

    private:
        std::string name; // TODO: rewrite with IR representation
        std::shared_ptr<Expr> args;
};

class Stmt : public Node {
    public:
        Stmt () {};
};

class DeclStmt : public Stmt {
    public:
        DeclStmt () : data (NULL), init (NULL), is_extern(false) { id = Node::NodeID::DECL; }
        void set_is_extern (bool _is_extern) { is_extern = _is_extern; }
        void set_data (std::shared_ptr<Data> _data) { data = _data; }
        void set_init (std::shared_ptr<Expr> _init) { init = _init; }
        std::string emit ();

    private:
        bool is_extern;
        std::shared_ptr<Data> data;
        std::shared_ptr<Expr> init;
};

class ExprStmt : public Stmt {
    public:
        ExprStmt () : expr (NULL) { id = Node::NodeID::EXPR; }
        void set_expr (std::shared_ptr<Expr> _expr) { expr = _expr; }
        std::string emit ();

    private:
        std::shared_ptr<Expr> expr;
};

class LoopStmt : public Stmt {
    public:
        enum LoopID {
            FOR, WHILE, DO_WHILE, MAX_LOOP_ID
        };

        LoopStmt () : loop_id(MAX_LOOP_ID), cond(NULL) { id = Node::NodeID::MAX_STMT_ID; }
        void add_to_body (std::shared_ptr<Stmt> stmt) { body.push_back(stmt); }
        void set_cond (std::shared_ptr<Expr> _cond) { cond = _cond; }
        void set_loop_type (LoopID _loop_id) { loop_id = _loop_id; }
        virtual std::string emit () = 0;

    protected:
        LoopID loop_id;
        std::vector<std::shared_ptr<Stmt>> body;
        std::shared_ptr<Expr> cond;
};

class CntLoopStmt : public LoopStmt {
    public:
        CntLoopStmt () : iter(NULL), iter_decl(NULL), step_expr(NULL) { id = Node::NodeID::CNT_LOOP; }
        void set_iter (std::shared_ptr<Variable> _iter) { iter = _iter; }
        void set_iter_decl (std::shared_ptr<DeclStmt> _iter_decl) { iter_decl = _iter_decl; }
        void set_step_expr (std::shared_ptr<Expr> _step_expr) { step_expr = _step_expr; }
        std::string emit ();

    private:
        std::shared_ptr<Variable> iter;
        std::shared_ptr<DeclStmt> iter_decl;
        std::shared_ptr<Expr> step_expr;
};

class IfStmt : public Stmt {
    public:
        IfStmt () : else_exist(false), cond(NULL) { id = Node::NodeID::IF; }
        void set_cond (std::shared_ptr<Expr> _cond) { cond = _cond; }
        void add_if_stmt (std::shared_ptr<Stmt> if_stmt) { if_branch.push_back(if_stmt); }
        void add_else_stmt (std::shared_ptr<Stmt> else_stmt) { else_branch.push_back(else_stmt); }
        void set_else_exist (bool _else_exist) { else_exist = _else_exist; }
        bool get_else_exist () { return else_exist; }
        std::string emit ();

    private:
        bool else_exist;
        std::shared_ptr<Expr> cond;
        std::vector<std::shared_ptr<Stmt>> if_branch;
        std::vector<std::shared_ptr<Stmt>> else_branch;
};
