#include "node.h"

Expr::Expr () {
    is_expr = true;
    type = Type::init(Type::TypeID::MAX_TYPE_ID);
}

std::shared_ptr<Expr> Expr::init (Node::NodeID _id) {
    std::shared_ptr<Expr> ret (NULL);
    switch (_id) {
        case Node::NodeID::ASSIGN:
            ret = std::make_shared<AssignExpr> (AssignExpr());
            break;
        case Node::NodeID::VAR_USE:
            ret = std::make_shared<VarUseExpr> (VarUseExpr());
            break;
        case Node::NodeID::MAX_EXPR_ID:
            break;
    }
    return ret;
}

void VarUseExpr::set_variable (std::shared_ptr<Variable> _var) {
    var = _var;
    type = var->get_type ();
}

void AssignExpr::set_from (std::shared_ptr<Expr> _from) {
    // TODO: add type conversion expr
    from = _from;
}

std::string AssignExpr::emit () {
    std::string ret = to->emit();
    ret += " = ";
    ret += from->emit();
    return ret;
}
