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
        case Node::NodeID::CONST:
            ret = std::make_shared<ConstExpr> (ConstExpr());
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

std::string ConstExpr::emit () {
    std::string ret = "";
    switch (get_type_id ()) {
        case Type::TypeID::UINT:
            ret += std::to_string((unsigned int) data);
            break;
        case Type::TypeID::ULINT:
            ret += std::to_string((unsigned long int) data);
            break;
        case Type::TypeID::ULLINT:
            ret += std::to_string((unsigned long long int) data);
            break;
        case Type::TypeID::PTR:
        case Type::TypeID::MAX_INT_ID:
        case Type::TypeID::MAX_TYPE_ID:
            std::cerr << "BAD TYPE in ConstExpr::emit ()" << std::endl;
            break;
    }
    ret += type->get_suffix ();
    return ret;
}

std::shared_ptr<Stmnt> Stmnt::init (Node::NodeID _id) {
    std::shared_ptr<Stmnt> ret (NULL);
    switch (_id) {
        case Node::NodeID::DECL:
            ret = std::make_shared<DeclStmnt> (DeclStmnt());
            break;
        case Node::NodeID::MAX_STMNT_ID:
            break;
    }
    return ret;
}

std::string DeclStmnt::emit () {
    std::string ret = "";
    ret += data->get_is_static() && !is_extern ? "static " : "";
    ret += is_extern ? "extern " : "";
    switch (data->get_modifier()) {
        case Variable::Mod::VOLAT:
            ret += "volatile ";
            break;
        case Variable::Mod::CONST:
            ret += "const ";
            break;
        case Variable::Mod::CONST_VOLAT:
            ret += "const volatile ";
            break;
        case Variable::Mod::NTHNG:
            break;
         case Variable::Mod::MAX_MOD:
                std::cerr << "ERROR in DeclStmnt::emit bad modifier" << std::endl;
                    break;
    }
    if (data->get_class_id() == Variable::VarClassID::ARR) {
        std::shared_ptr<Array> arr = std::static_pointer_cast<Array>(data);
        switch (arr->get_essence()) {
            case Array::Ess::STD_ARR:
                ret += "std::array<" + arr->get_base_type()->get_name() + ", " + std::to_string(arr->get_size()) + ">";
                ret += " " + arr->get_name();
                break;
            case Array::Ess::STD_VEC:
                ret += "std::vector<" + arr->get_base_type()->get_name() + ">";
                ret += " " + arr->get_name();
                ret += is_extern ? "" : " (" + std::to_string(arr->get_size()) + ", 0)";
                break;
            case Array::Ess::C_ARR:
                ret +=  arr->get_base_type()->get_name();
                ret += " " + arr->get_name();
                ret += " [" + std::to_string(arr->get_size()) + "]";
                break;
            case Array::Ess::MAX_ESS:
                std::cerr << "ERROR in DeclStmnt::emit bad array essence" << std::endl;
                break;
        }
    }
    else {
        ret += data->get_type()->get_name() + " " + data->get_name();
    }
    if (init != NULL) {
        if (data->get_class_id() == Variable::VarClassID::ARR) {
            std::cerr << "ERROR in DeclStmnt::emit init of array" << std::endl;
            return ret;
        }
        ret += " = " + init->emit();
    }
    return ret;
}
