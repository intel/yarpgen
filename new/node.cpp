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
        case Node::NodeID::BINARY:
            ret = std::make_shared<BinaryExpr> (BinaryExpr());
            break;
        case Node::NodeID::INDEX:
            ret = std::make_shared<IndexExpr> (IndexExpr());
            break;
        case Node::NodeID::VAR_USE:
            ret = std::make_shared<VarUseExpr> (VarUseExpr());
            break;
        case Node::NodeID::UNARY:
            ret = std::make_shared<UnaryExpr> (UnaryExpr());
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

void AssignExpr::set_to (std::shared_ptr<Expr> _to) {
    to = _to;
    // TODO: choose strategy
    type = Type::init (to->get_type_id ());
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

void IndexExpr::set_base (std::shared_ptr<Array> _base) {
    base = _base;
    type = base->get_type();
}

std::string IndexExpr::emit () {
    std::string ret = base->get_name();
    ret += is_subscr ? " [" : ".at(";
    ret += index->emit();
    ret += is_subscr ? "]" : ")";
    return ret;
}

void BinaryExpr::determ_type () {
    if (op == MaxOp || arg0 == NULL || arg1 == NULL) {
        type = Type::init (Type::TypeID::MAX_TYPE_ID);
        return;
    }
    // TODO: add integer promotion and implicit type conversion
    type = Type::init (std::max(arg0->get_type_id(), arg1->get_type_id()));
}

std::string BinaryExpr::emit () {
    std::string ret = "((" + arg0->emit() + ")";
    switch (op) {
        case Add:
            ret += " + ";
            break;
        case Sub:
            ret += " - ";
            break;
        case Mul:
            ret += " * ";
            break;
        case Div:
            ret += " / ";
            break;
        case Mod:
            ret += " % ";
            break;
        case Shl:
            ret += " << ";
            break;
        case Shr:
            ret += " >> ";
            break;
        case Lt:
            ret += " < ";
            break;
        case Gt:
            ret += " > ";
            break;
        case Le:
            ret += " <= ";
            break;
        case Ge:
            ret += " >= ";
            break;
        case Eq:
            ret += " == ";
            break;
        case Ne:
            ret += " != ";
            break;
        case BitAnd:
            ret += " & ";
            break;
        case BitXor:
            ret += " ^ ";
            break;
        case BitOr:
            ret += " | ";
            break;
        case LogAnd:
            ret += " && ";
            break;
        case LogOr:
            ret += " || ";
            break;
        case MaxOp:
            std::cerr << "BAD OPERATOR" << std::endl;
            break;
        }
        ret += "(" + arg1->emit() + "))";
        return ret;
}

void UnaryExpr::determ_type () {
    if (op == MaxOp || arg == NULL) {
        type = Type::init (Type::TypeID::MAX_TYPE_ID);
        return;
    }
    // TODO: add integer promotion and implicit type conversion
    type = Type::init (arg->get_type_id());
}

std::string UnaryExpr::emit () {
    std::string op_str = "";
    switch (op) {
        case PreInc:
        case PostInc:
            op_str = "++";
            break;
        case PreDec:
        case PostDec:
            op_str = "--";
            break;
        case Negate:
            op_str = "-";
            break;
        case LogNot:
            op_str = "!";
            break;
        case BitNot:
            op_str = "~";
            break;
        case MaxOp:
            std::cerr << "BAD OPERATOR" << std::endl;
            break;
    }
    std::string ret = "";
    if (op == PostInc || op == PostDec)
        ret = "((" + arg->emit() + ")" + op_str + ")";
    else
        ret = "(" + op_str + "(" +  arg->emit() + "))";
    return ret;
}
