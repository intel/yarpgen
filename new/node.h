#pragma once

#include "type.h"
#include "variable.h"

class Node {
    public:
        enum NodeID {
            // Expr types
            ASSIGN,
            BINARY,
            INDEX,
            VAR_USE,
            UNARY,
            MAX_EXPR_ID
            // Stmt type
        };

        bool get_is_expr () { return is_expr; }
        NodeID get_id () { return id; }
        virtual std::string emit () = 0;

    protected:
        NodeID id;
        bool is_expr;
};

class Expr : public Node {
    public:
        Expr ();
        static std::shared_ptr<Expr> init (Node::NodeID _id);
        Type::TypeID get_type_id () { return type->get_id (); }

    protected:
        std::shared_ptr<Type> type;
};

class VarUseExpr : public Expr {
    public:
        VarUseExpr () : var (NULL) { id = Node::NodeID::VAR_USE; }
        void set_variable (std::shared_ptr<Variable> _var);
        std::string emit () { return var->get_name (); }

    private:
        std::shared_ptr<Variable> var;
};

class AssignExpr : public Expr {
    public:
        AssignExpr () : to (NULL), from (NULL) { id = Node::NodeID::ASSIGN; }
        void set_to (std::shared_ptr<Expr> _to);
        void set_from (std::shared_ptr<Expr> _from);
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
        void set_is_subscr (bool _is_subscr) { is_subscr = _is_subscr || base->get_essence() == Array::Ess::C_ARR; }
        std::string emit ();

    private:
        std::shared_ptr<Array> base;
        std::shared_ptr<Expr> index;
        bool is_subscr; // We can access std::array and std::vector elements through [i] and at(i)
};

class BinaryExpr : public Expr {
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
        void set_lhs (std::shared_ptr<Expr> _arg0) { arg0 = _arg0; }
        void set_rhs (std::shared_ptr<Expr> _arg1) { arg1 = _arg1; }
        void determ_type ();
        std::string emit ();

    private:
        Op op;
        std::shared_ptr<Expr> arg0;
        std::shared_ptr<Expr> arg1;
};

class UnaryExpr : public Expr {
    public:
        enum Op {
            PreInc,  ///< Pre-increment
            PreDec,  ///< Pre-decrement
            PostInc, ///< Post-increment
            PostDec, ///< Post-decrement
            Negate,  ///< Negation
            LogNot,  ///< Logical not
            BitNot,  ///< Bit not
            MaxOp
        };
        UnaryExpr () : op (MaxOp), arg (NULL) { id = Node::NodeID::UNARY; }
        void set_op (Op _op) { op = _op; }
        void set_arg (std::shared_ptr<Expr> _arg) { arg = _arg; }
        void determ_type ();
        std::string emit ();

    private:
        Op op;
        std::shared_ptr<Expr> arg;
};
