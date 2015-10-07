#pragma once

#include <vector>

#include "type.h"
#include "variable.h"

class Node {
    public:
        enum NodeID {
            // Expr types
            ASSIGN,
            BINARY,
            CONST,
            INDEX,
            TYPE_CAST,
            UNARY,
            VAR_USE,
            MAX_EXPR_ID,
            // Stmt type
            DECL,
            EXPR,
            CNT_LOOP,
            MAX_STMNT_ID
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
        Expr () : value("", Type::TypeID::ULLINT, Variable::Mod::NTHNG, false) {is_expr = false;}
        static std::shared_ptr<Expr> init (Node::NodeID _id);
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
        void set_is_subscr (bool _is_subscr) { is_subscr = _is_subscr || base->get_essence() == Array::Ess::C_ARR; }
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

class Stmnt : public Node {
    public:
        Stmnt () {};
        static std::shared_ptr<Stmnt> init (Node::NodeID _id);
};

class DeclStmnt : public Stmnt {
    public:
        DeclStmnt () : data (NULL), init (NULL), is_extern(false) { id = Node::NodeID::DECL; }
        void set_is_extern (bool _is_extern) { is_extern = _is_extern; }
        void set_data (std::shared_ptr<Data> _data) { data = _data; }
        void set_init (std::shared_ptr<Expr> _init) { init = _init; }
        std::string emit ();

    private:
        bool is_extern;
        std::shared_ptr<Data> data;
        std::shared_ptr<Expr> init;
};

class ExprStmnt : public Stmnt {
    public:
        ExprStmnt () : expr (NULL) { id = Node::NodeID::EXPR; }
        void set_expr (std::shared_ptr<Expr> _expr) { expr = _expr; }
        std::string emit ();

    private:
        std::shared_ptr<Expr> expr;
};

class LoopStmnt : public Stmnt {
    public:
        enum LoopID {
            FOR, WHILE, DO_WHILE, MAX_LOOP_ID
        };

        LoopStmnt () : loop_id(MAX_LOOP_ID), cond(NULL) { id = Node::NodeID::MAX_STMNT_ID; }
        void add_to_body (std::shared_ptr<Stmnt> stmnt) { body.push_back(stmnt); }
        void set_cond (std::shared_ptr<Expr> _cond) { cond = _cond; }
        void set_loop_type (LoopID _loop_id) { loop_id = _loop_id; }
        virtual std::string emit () = 0;

    protected:
        LoopID loop_id;
        std::vector<std::shared_ptr<Stmnt>> body;
        std::shared_ptr<Expr> cond;
};

class CntLoopStmnt : public LoopStmnt {
    public:
        CntLoopStmnt () : iter(NULL), iter_decl(NULL), step_expr(NULL) { id = Node::NodeID::CNT_LOOP; }
        void set_iter (std::shared_ptr<Variable> _iter) { iter = _iter; }
        void set_iter_decl (std::shared_ptr<DeclStmnt> _iter_decl) { iter_decl = _iter_decl; }
        void set_step_expr (std::shared_ptr<Expr> _step_expr) { step_expr = _step_expr; }
        std::string emit ();

    private:
        std::shared_ptr<Variable> iter;
        std::shared_ptr<DeclStmnt> iter_decl;
        std::shared_ptr<Expr> step_expr;
};
