#pragma once

#include "type.h"
#include "variable.h"

class Node {
    public:
        enum NodeID {
            // Expr types
            ASSIGN,
            VAR_USE,
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
        VarUseExpr () { id = Node::NodeID::VAR_USE; }
        void set_variable (std::shared_ptr<Variable> _var);
        std::string emit () { return var->get_name (); }

    private:
        std::shared_ptr<Variable> var;
};

class AssignExpr : public Expr {
    public:
        AssignExpr () { id = Node::NodeID::ASSIGN; }
        void set_to (std::shared_ptr<Expr> _to) { to = _to; }
        void set_from (std::shared_ptr<Expr> _from);
        std::string emit ();

    private:
        std::shared_ptr<Expr> to, from;
};
