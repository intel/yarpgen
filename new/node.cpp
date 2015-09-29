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
        case Node::NodeID::TYPE_CAST:
            ret = std::make_shared<TypeCastExpr> (TypeCastExpr());
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

std::shared_ptr<Expr> ArithExpr::integral_prom (std::shared_ptr<Expr> arg) {
    //[conv.prom]
    if (arg->get_type_id() >= Type::TypeID::INT) // can't perform integral promotiom
        return arg;
    TypeCastExpr ret;
    ret.set_type(Type::init(Type::TypeID::INT));
    ret.set_expr(arg);
    return std::make_shared<TypeCastExpr>(ret);
}

std::shared_ptr<Expr> ArithExpr::conv_to_bool (std::shared_ptr<Expr> arg) {
    if (arg->get_type_id() == Type::TypeID::BOOL)
        return arg;
    TypeCastExpr ret;
    ret.set_type(Type::init(Type::TypeID::BOOL));
    ret.set_expr(arg);
    return std::make_shared<TypeCastExpr>(ret);
}

void BinaryExpr::perform_arith_conv () {
    // integral promotion should be a part of it, but it was moved to base class
    // 10.5.1
    if (arg0->get_type_id() == arg1->get_type_id())
        return;
    // 10.5.2
    if (arg0->get_type_sign() == arg1->get_type_sign()) {
        TypeCastExpr ins;
        ins.set_type(Type::init(std::max(arg0->get_type_id(), arg1->get_type_id())));
        if (arg0->get_type_id() <  arg1->get_type_id()) {
            ins.set_expr(arg0);
            arg0 = std::make_shared<TypeCastExpr>(ins);
        }
        else {
            ins.set_expr(arg1);
            arg1 = std::make_shared<TypeCastExpr>(ins);
        }
        return;
    }
    if ((!arg0->get_type_sign() && (arg0->get_type_id() >= arg1->get_type_id())) || // 10.5.3
         (arg0->get_type_sign() && Type::can_repr_value (arg1->get_type_id(), arg0->get_type_id()))) { // 10.5.4
        TypeCastExpr ins;
        ins.set_type(Type::init(arg0->get_type_id()));
        ins.set_expr(arg1);
        arg1 = std::make_shared<TypeCastExpr>(ins);
        return;
    }
    if ((!arg1->get_type_sign() && (arg1->get_type_id() >= arg0->get_type_id())) || // 10.5.3
         (arg1->get_type_sign() && Type::can_repr_value (arg0->get_type_id(), arg1->get_type_id()))) { // 10.5.4
        TypeCastExpr ins;
        ins.set_type(Type::init(arg1->get_type_id()));
        ins.set_expr(arg0);
        arg0 = std::make_shared<TypeCastExpr>(ins);
        return;
    }
    // 10.5.5
    TypeCastExpr ins0;
    TypeCastExpr ins1;
    if (arg0->get_type_sign()) {
        ins0.set_type(Type::init(Type::get_corr_unsig(arg0->get_type_id())));
        ins1.set_type(Type::init(Type::get_corr_unsig(arg0->get_type_id())));
    }
    if (arg1->get_type_sign()) {
        ins0.set_type(Type::init(Type::get_corr_unsig(arg1->get_type_id())));
        ins1.set_type(Type::init(Type::get_corr_unsig(arg1->get_type_id())));
    }
    ins0.set_expr(arg0);
    arg0 = std::make_shared<TypeCastExpr>(ins0);
    ins1.set_expr(arg1);
    arg1 = std::make_shared<TypeCastExpr>(ins1);
}

void BinaryExpr::propagate_type () {
    if (op == MaxOp || arg0 == NULL || arg1 == NULL) {
        std::cerr << "ERROR: BinaryExpr::propagate_type specify args" << std::endl;
        type = Type::init (Type::TypeID::MAX_TYPE_ID);
        return;
    }
    switch (op) {
        case Add:
        case Sub:
        case Mul:
        case Div:
        case Mod:
        case Lt:
        case Gt:
        case Le:
        case Ge:
        case Eq:
        case Ne:
        case BitAnd:
        case BitXor:
        case BitOr:
            // arithmetic conversions
            arg0 = integral_prom (arg0);
            arg1 = integral_prom (arg1);
            perform_arith_conv();
            break;
        case Shl:
        case Shr:
            arg0 = integral_prom (arg0);
            arg1 = integral_prom (arg1);
            break;
        case LogAnd:
        case LogOr:
            arg0 = conv_to_bool (arg0);
            arg1 = conv_to_bool (arg1);
            break;
        case MaxOp:
            std::cerr << "ERROR: BinaryExpr::propagate_type bad operator" << std::endl;
            break;
    }
    type = Type::init (arg0->get_type_id());
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

void UnaryExpr::propagate_type () {
    if (op == MaxOp || arg == NULL) {
        std::cerr << "ERROR: UnaryExpr::propagate_type specify args" << std::endl;
        type = Type::init (Type::TypeID::MAX_TYPE_ID);
        return;
    }
    switch (op) {
        case PreInc:
        case PreDec:
        case PostInc:
        case PostDec:
            break;
        case Plus:
        case Negate:
        case BitNot:
            arg = integral_prom (arg);
            break;
        case LogNot:
            arg = conv_to_bool (arg);
            break;
        case MaxOp:
            std::cerr << "ERROR: UnaryExpr::propagate_type bad operator" << std::endl;
            break;
    }
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
        case Plus:
            op_str = "+";
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
        case Type::TypeID::BOOL:
            ret += std::to_string((bool) data);
            break;
        case Type::TypeID::CHAR:
            ret += std::to_string((signed char) data);
            break;
        case Type::TypeID::UCHAR:
            ret += std::to_string((unsigned char) data);
            break;
        case Type::TypeID::SHRT:
            ret += std::to_string((short) data);
            break;
        case Type::TypeID::USHRT:
            ret += std::to_string((unsigned short) data);
            break;
        case Type::TypeID::INT:
            ret += std::to_string((int) data);
            break;
        case Type::TypeID::UINT:
            ret += std::to_string((unsigned int) data);
            break;
        case Type::TypeID::LINT:
            ret += std::to_string((long int) data);
            break;
        case Type::TypeID::ULINT:
            ret += std::to_string((unsigned long int) data);
            break;
        case Type::TypeID::LLINT:
            ret += std::to_string((long long int) data);
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

std::string TypeCastExpr::emit () {
    std::string ret = "(" + type->get_name() + ")";
    ret += expr->emit();
    return ret;
}

std::shared_ptr<Stmnt> Stmnt::init (Node::NodeID _id) {
    std::shared_ptr<Stmnt> ret (NULL);
    switch (_id) {
        case Node::NodeID::DECL:
            ret = std::make_shared<DeclStmnt> (DeclStmnt());
            break;
        case Node::NodeID::EXPR:
            ret = std::make_shared<ExprStmnt> (ExprStmnt());
            break;
        case Node::NodeID::CNT_LOOP:
            ret = std::make_shared<CntLoopStmnt> (CntLoopStmnt());
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
    if (data->get_align() != 0)
        ret += " __attribute__((aligned(" + std::to_string(data->get_align()) + ")))";
    if (init != NULL) {
        if (data->get_class_id() == Variable::VarClassID::ARR) {
            std::cerr << "ERROR in DeclStmnt::emit init of array" << std::endl;
            return ret;
        }
        if (is_extern) {
            std::cerr << "ERROR in DeclStmnt::emit init of extern" << std::endl;
            return ret;
        }
        ret += " = " + init->emit();
    }
    return ret;
}

std::string ExprStmnt::emit() {
    return expr->emit();
}

std::string CntLoopStmnt::emit() {
    std::string ret;
    switch (loop_id) {
        case LoopStmnt::LoopID::FOR:
            ret += "for (";
            ret += iter_decl->emit() + "; ";
            ret += cond->emit() + "; ";
            ret += step_expr->emit() + ") {\n";
            break;
        case LoopStmnt::LoopID::WHILE:
            ret += iter_decl->emit() + ";\n";
            ret += "while (" + cond->emit() + ") {\n";
            break;
        case LoopStmnt::LoopID::DO_WHILE:
            ret += iter_decl->emit() + ";\n";
            ret += "do {\n";
            break;
        case LoopStmnt::LoopID::MAX_LOOP_ID:
            std::cerr << "ERROR in CntLoopStmnt::emit invalid loop id" << std::endl;
            break;
    }

    for (auto i = this->body.begin(); i != this->body.end(); ++i)
        ret += (*i)->emit() + ";\n";

    if (loop_id == LoopStmnt::LoopID::WHILE ||
        loop_id == LoopStmnt::LoopID::DO_WHILE) {
        ret += step_expr->emit() + ";\n";
    }

    ret += "}\n";

    if (loop_id == LoopStmnt::LoopID::DO_WHILE) {
        ret += "while (" + cond->emit() + ");";
    }

    return ret;
}
