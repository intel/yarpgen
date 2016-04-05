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

#include "type.h"
#include "variable.h"
#include "ir_node.h"
#include "expr.h"

using namespace rl;

std::shared_ptr<Data> Expr::get_value () {
    switch (value->get_class_id()) {
        case Data::VarClassID::VAR: {
            std::shared_ptr<ScalarVariable> scalar_var = std::make_shared<ScalarVariable>(*(std::static_pointer_cast<ScalarVariable>(value)));
            scalar_var->set_name("");
            return scalar_var;
        }
        case Data::VarClassID::STRUCT: {
            std::shared_ptr<Struct> struct_var = std::make_shared<Struct>(*(std::static_pointer_cast<Struct>(value)));
            struct_var->set_name("");
            return struct_var;
        }
        case Data::VarClassID::MAX_CLASS_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported Data::VarClassID in Expr::get_value" << std::endl;
            exit(-1);
    }
}

void VarUseExpr::set_value (std::shared_ptr<Data> _new_value) {
    if (_new_value->get_class_id() != value->get_class_id()) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": different Data::VarClassID in VarUseExpr::set_value" << std::endl;
        exit(-1);
    }
    switch (value->get_class_id()) {
        case Data::VarClassID::VAR:
            //TODO: Add integer type id check. We can't assign different types
            std::static_pointer_cast<ScalarVariable>(value)->set_cur_value(std::static_pointer_cast<ScalarVariable>(_new_value)->get_cur_value());
            break;
        case Data::VarClassID::STRUCT:
            //TODO: implement for Struct
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": Struct is unsupported in in VarUseExpr::set_value" << std::endl;
            exit(-1);
            break;
        case Data::VarClassID::MAX_CLASS_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": unsupported Data::VarClassID in VarUseExpr::set_value" << std::endl;
            exit(-1);
    }
}

AssignExpr::AssignExpr (std::shared_ptr<Expr> _to, std::shared_ptr<Expr> _from) :
                        Expr(Node::NodeID::ASSIGN, _to->get_value()), to(_to), from(_from) {
    if (to->get_id() != Node::NodeID::VAR_USE) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can assign only to variable in AssignExpr::AssignExpr" << std::endl;
        exit(-1);
    }
    propagate_type();
    propagate_value();
}

bool AssignExpr::propagate_type () {
    //TODO:StructType check for struct assignment
        if (value->get_class_id() == Data::VarClassID::VAR &&
            from->get_value()->get_class_id() == Data::VarClassID::VAR) {
            from = std::make_shared<TypeCastExpr>(from, value->get_type(), true);
        }
        else {
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": struct are unsupported in AssignExpr::propagate_value" << std::endl;
            exit(-1);
        }
    return true;
}

UB AssignExpr::propagate_value () {
    if (to->get_id() == Node::NodeID::VAR_USE) {
            std::static_pointer_cast<VarUseExpr>(to)->set_value(from->get_value());
    }
    else {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can assign only to variable in AssignExpr::propagate_value" << std::endl;
        exit(-1);
    }
    value = from->get_value();
    return NoUB;
}

std::string AssignExpr::emit (std::string offset) {
    std::string ret = offset;
    ret += to->emit();
    ret += " = ";
    ret += from->emit();
    return ret;
}

TypeCastExpr::TypeCastExpr (std::shared_ptr<Expr> _expr, std::shared_ptr<Type> _type, bool _is_implicit) :
              Expr(Node::NodeID::TYPE_CAST, NULL), expr(_expr), to_type(_type), is_implicit(_is_implicit) {
    propagate_type();
    propagate_value();
}

bool TypeCastExpr::propagate_type () {
    if (to_type->get_int_type_id() == Type::IntegerTypeID::MAX_INT_ID ||
        expr->get_value()->get_type()->get_int_type_id() == Type::IntegerTypeID::MAX_INT_ID) {
        //TODO: what about overloaded struct types cast?
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can cast only integer types in TypeCastExpr::propagate_type" << std::endl;
        exit(-1);
    }
    return true;
}

UB TypeCastExpr::propagate_value () {
    if (expr->get_value()->get_class_id() != Data::VarClassID::VAR) {
        //TODO: what about overloaded struct types cast?
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can cast only integer types in TypeCastExpr::propagate_value" << std::endl;
        exit(-1);
    }
    //TODO: Is it always safe to cast value to ScalarVariable?
    value = std::make_shared<ScalarVariable>("", std::static_pointer_cast<IntegerType>(to_type));
    std::static_pointer_cast<ScalarVariable>(value)->set_cur_value(std::static_pointer_cast<ScalarVariable>(expr->get_value())->get_cur_value().cast_type(to_type->get_int_type_id()));
    return NoUB;
}

std::string TypeCastExpr::emit (std::string offset) {
    std::string ret = offset;
    //TODO: add parameter to gen_policy
    if (!is_implicit)
        ret += "(" + value->get_type()->get_simple_name() + ")";
    else
        ret += "(" + value->get_type()->get_simple_name() + ")";
    ret += "(" + expr->emit() + ")";
    return ret;
}

std::string ConstExpr::emit (std::string offset) {
    std::string ret = offset;
    std::shared_ptr<ScalarVariable> scalar_val = std::static_pointer_cast<ScalarVariable>(value);
    switch (scalar_val->get_type()->get_int_type_id()) {
        case IntegerType::IntegerTypeID::BOOL:
            ret += std::to_string(scalar_val->get_cur_value().val.bool_val);
            break;
        case IntegerType::IntegerTypeID::CHAR:
            ret += std::to_string(scalar_val->get_cur_value().val.char_val);
            break;
        case IntegerType::IntegerTypeID::UCHAR:
            ret += std::to_string(scalar_val->get_cur_value().val.uchar_val);
            break;
        case IntegerType::IntegerTypeID::SHRT:
            ret += std::to_string(scalar_val->get_cur_value().val.shrt_val);
            break;
        case IntegerType::IntegerTypeID::USHRT:
            ret += std::to_string(scalar_val->get_cur_value().val.ushrt_val);
            break;
        case IntegerType::IntegerTypeID::INT:
            ret += std::to_string(scalar_val->get_cur_value().val.int_val);
            break;
        case IntegerType::IntegerTypeID::UINT:
            ret += std::to_string(scalar_val->get_cur_value().val.uint_val);
            break;
        case IntegerType::IntegerTypeID::LINT:
            ret += std::to_string(scalar_val->get_cur_value().val.lint_val);
            break;
        case IntegerType::IntegerTypeID::ULINT:
            ret += std::to_string(scalar_val->get_cur_value().val.ulint_val);
            break;
        case IntegerType::IntegerTypeID::LLINT:
            ret += std::to_string(scalar_val->get_cur_value().val.llint_val);
            break;
        case IntegerType::IntegerTypeID::ULLINT:
            ret += std::to_string(scalar_val->get_cur_value().val.ullint_val);
            break;
        case IntegerType::IntegerTypeID::MAX_INT_ID:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad int type id in Constexpr::emit" << std::endl;
            exit(-1);
    }
    ret += std::static_pointer_cast<AtomicType>(scalar_val->get_type())->get_suffix ();
    return ret;
}

std::shared_ptr<Expr> ArithExpr::integral_prom (std::shared_ptr<Expr> arg) {
    if (arg->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can perform integral_prom only on ScalarVariable in ArithExpr::integral_prom" << std::endl;
        exit(-1);
    }

    //[conv.prom]
    if (arg->get_value()->get_type()->get_int_type_id() >= IntegerType::IntegerTypeID::INT) // can't perform integral promotiom
        return arg;
    return std::make_shared<TypeCastExpr>(arg, IntegerType::init(Type::IntegerTypeID::INT), true);
}

std::shared_ptr<Expr> ArithExpr::conv_to_bool (std::shared_ptr<Expr> arg) {
    if (arg->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can perform conv_to_bool only on ScalarVariable in ArithExpr::conv_to_bool" << std::endl;
        exit(-1);
    }

    if (arg->get_value()->get_type()->get_int_type_id() == IntegerType::IntegerTypeID::BOOL) // can't perform integral promotiom
        return arg;
    return std::make_shared<TypeCastExpr>(arg, IntegerType::init(Type::IntegerTypeID::BOOL), true);
}

UnaryExpr::UnaryExpr (Op _op, std::shared_ptr<Expr> _arg) :
                       ArithExpr(Node::NodeID::UNARY, _arg->get_value()), op (_op), arg (_arg) {
    //TODO: add UB elimination strategy
    propagate_type();
    propagate_value();
}

bool UnaryExpr::propagate_type () {
    if (op == MaxOp || arg == NULL) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad args in UnaryExpr::propagate_type" << std::endl;
        exit(-1);
        return false;
    }

    //TODO: what about overloadedstruct operators?
    if (arg->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can perform propagate_type only on ScalarVariable in UnaryExpr::propagate_type" << std::endl;
        exit(-1);
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
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad op in UnaryExpr::propagate_type" << std::endl;
            exit(-1);
            break;
    }
    value = arg->get_value();
    return true;
}

UB UnaryExpr::propagate_value () {
    if (op == MaxOp || arg == NULL) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad op in UnaryExpr::propagate_value" << std::endl;
        exit(-1);
        return NullPtr;
    }

    //TODO: what about overloadedstruct operators?
    if (arg->get_value()->get_class_id() != Data::VarClassID::VAR) {
        std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": can perform propagate_value only on ScalarVariable in UnaryExpr::propagate_value" << std::endl;
        exit(-1);
    }

    std::shared_ptr<ScalarVariable> scalar_val = std::static_pointer_cast<ScalarVariable>(arg->get_value());

    AtomicType::ScalarTypedVal new_val (scalar_val->get_type()->get_int_type_id());

    switch (op) {
        case PreInc:
        case PostInc:
            new_val = scalar_val->get_cur_value()++;
            break;
        case PreDec:
        case PostDec:
            new_val = scalar_val->get_cur_value()--;
            break;
        case Plus:
            new_val = scalar_val->get_cur_value();
            break;
        case Negate:
            new_val = -scalar_val->get_cur_value();
            break;
        case BitNot:
            new_val = ~scalar_val->get_cur_value();
            break;
        case LogNot:
            new_val = !scalar_val->get_cur_value();
            break;
        case MaxOp:
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad op in UnaryExpr::propagate_value" << std::endl;
            exit(-1);
            break;
    }

    if (!new_val.has_ub())
        std::static_pointer_cast<ScalarVariable>(value)->set_cur_value(new_val);
    return new_val.get_ub();
}

std::string UnaryExpr::emit (std::string offset) {
    std::string op_str = offset;
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
            std::cerr << "ERROR at " << __FILE__ << ":" << __LINE__ << ": bad op in UnaryExpr::emit" << std::endl;
            exit(-1);
            break;
    }
    std::string ret = "";
    if (op == PostInc || op == PostDec)
        ret = "((" + arg->emit() + ")" + op_str + ")";
    else
        ret = "(" + op_str + "(" + arg->emit() + "))";
    return ret;
}
